// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2010 Guy Blelloch and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include <vector>
#include <cmath>
#include "cas_array.h" // Concurrent array
#include "loadLassoData.h"
#include "cilk.h"
#include "sequence.h"
#include "gettime.h"

// Optimization problem
//      \arg \min_x ||Ax-y|| + \lambda |x|_1
//
// We call values of vector x "features" and "weights"
//

int P;

struct feature {
    valuetype_t covar;  // Covariane of a a column
    valuetype_t Ay_i;   // (Ay)_i
};

// Column-wise sparse matrix representation. 
std::vector<sparse_array> A_cols, A_rows;

// Vector of observations
std::vector<valuetype_t> y;

// Problem size and configuration
int nx;  // Number of features/weights
int ny;  // Number of datapoints

// Feature values and precomputed constants
std::vector<feature> feature_consts;
std::vector<valuetype_t> x; 

// Major optimization: always keep updated vector 'Ax'
cas_array<valuetype_t> Ax;

valuetype_t delta_threshold = 1e-4;
valuetype_t sqr(valuetype_t x) {return x*x;}

void initialize_feature(int feat_idx) {
    sparse_array& col = A_cols[feat_idx];
    feature& feat = feature_consts[feat_idx];
    x[feat_idx] = 0.0;

    // Precompute covariance of a feature
    feat.covar = 0;
    for(int i=0; i<col.length(); i++) {
        feat.covar += sqr(col.values[i]);
    }
    feat.covar *= 2;
    
    // Precompute (Ay)_i
    feat.Ay_i = 0.0;
    for(int i=0; i<col.length(); i++) {
        feat.Ay_i += col.values[i]*y[col.idxs[i]];
    }
    feat.Ay_i *= 2;

}

void initialize() {
    feature_consts.reserve(nx);
    x.resize(nx);
    Ax.resize(ny);
    cilk_for(int i=0; i<nx; i++) {
        initialize_feature(i);
    }
}

valuetype_t soft_thresholdO(valuetype_t _lambda, valuetype_t shootDiff) {
    return (shootDiff > _lambda)* (_lambda - shootDiff) + 
	               (shootDiff < -_lambda) * (-_lambda- shootDiff) ;
}

valuetype_t soft_threshold(valuetype_t _lambda, valuetype_t shootDiff) {
  if (shootDiff > _lambda) return _lambda - shootDiff;
  if (shootDiff < -_lambda) return -_lambda - shootDiff ;
  else return 0;
}

double shoot(int x_i, valuetype_t lambda) {
    feature& feat = feature_consts[x_i];
    valuetype_t oldvalue = x[x_i];
    
    // Compute dotproduct A'_i*(Ax)
    valuetype_t AtAxj = 0.0;
    sparse_array& col = A_cols[x_i];
    int len=col.length();
    for(int i=0; i<len; i++) {
        AtAxj += col.values[i] * Ax[col.idxs[i]];
    }
    
    valuetype_t S_j = 2 * AtAxj - feat.covar * oldvalue - feat.Ay_i;
    valuetype_t newvalue = soft_threshold(lambda,S_j)/feat.covar;
    valuetype_t delta = (newvalue - oldvalue);
    
    // Update ax
    if (delta != 0.0) {
        if (P>1) {
            for(int i=0; i<len; i++) {
	            Ax.add(col.idxs[i], col.values[i] * delta);
            }
        } else {
            for(int i=0; i<len; i++) {
	            Ax[col.idxs[i]] += col.values[i] * delta;
            }
        }
        x[x_i] = newvalue;
    }
    return std::abs(delta);
}

// Find such lambda that if used for regularization,
// optimum would have all weights zero.
valuetype_t compute_max_lambda() {
    valuetype_t maxlambda = 0.0;
    for(int i=0; i<nx; i++) {
        maxlambda = std::max(maxlambda, std::abs(feature_consts[i].Ay_i));
    }
    return maxlambda;
}

valuetype_t get_term_threshold(int k, int K) {
  // Stricter termination threshold for last step in the optimization.
  return (k == 0 ? delta_threshold  : (delta_threshold + k*(delta_threshold*50)/K));
}

// TODO: remove
 valuetype_t compute_objective(valuetype_t _lambda, std::vector<valuetype_t>& x, valuetype_t * l1x = NULL, valuetype_t * l2err = NULL) {
    double least_sqr = 0;
     
    for(int i=0; i<ny; i++) {
        least_sqr += (Ax[i]-y[i])*(Ax[i]-y[i]);					
    }
    
     // Penalty check for feature 0
    double penalty = 0.0;
    for(int i=0; i<nx; i++) {
        penalty += std::abs(x[i]);
    }
    if (l1x != NULL) *l1x = penalty;
    if (l2err != NULL) *l2err = least_sqr;
    return penalty * _lambda + least_sqr;
}



void main_optimization_loop(double lambda, double target_objective) {
    // Empirically found heuristic for deciding how many steps
    // to take on the regularizatio path
    P = cilk::current_worker_count();  
    int regularization_path_length = 1+(int)(nx/2000);
    valuetype_t lambda_max = compute_max_lambda();
    valuetype_t lambda_min = lambda;
    valuetype_t alpha = pow(lambda_max/lambda_min, 1.0/(1.0*regularization_path_length));
    int regularization_path_step = regularization_path_length;
    
    // Adoped from Guy's code
    if (P == 1) 
        delta_threshold = 10e-4;
    else
        delta_threshold = 25e-4;
    
    long long int num_of_shoots = 0;
    int counter = 0;
    double *delta = new double[nx];
    startTime();
    do {
        double maxChange=0;
        lambda = lambda_min * pow(alpha, regularization_path_step);
        
        // Make a full sweep
        if (P>1) {
            // Parallel loop
            cilk_for(int i=0; i<nx; i++) 
	            delta[i] = shoot(i, lambda);
            maxChange = sequence::reduce(delta,nx,utils::maxF<double>());
        } else {
            // Sequential loop  
            for(int i=0; i<nx; i++) {
                maxChange = std::max(maxChange, shoot(i,lambda));
            }
        }
        num_of_shoots += nx;
        counter++;

        // Convergence check.
        // We use a simple trick to converge faster for the intermediate sub-optimization problems
        // on the regularization path. This is important because we do not care about accuracy for
        // the intermediate problems, just want a good warm start for next round.
        if (regularization_path_step>0) {
            bool converged = (maxChange <= get_term_threshold(regularization_path_step,regularization_path_length));
            if (converged || counter>std::min(100, (100-regularization_path_step)*2)) {
                counter = 0;
                regularization_path_step--; 
            }
        } else {
            valuetype_t obj = compute_objective(lambda, x, NULL, NULL);
            if (obj<target_objective || counter>5000) return;
        }
    } while (regularization_path_step >= 0);
}

void lassoRead(char * inputfile) {
   load_lasso_data(inputfile, &A_cols, &A_rows, &y, nx, ny);
}

void lassoWriteResults(char * outputfile) {
   // Output results
    if (outputfile != NULL) {
        FILE * outf = fopen(outputfile, "w");
        for(int i=0; i<nx; i++) {
            if (x[i] != 0.0) fprintf(outf, "%d,%lf\n", i, x[i]); 
        }
        fclose(outf);
    }
}


double solveLasso(double lambda, double accuracy) {
    initialize();
    main_optimization_loop(lambda, accuracy); 
}


