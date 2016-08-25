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
#include "parallel.h"
//#include "assert.h"
#include "sequence.h"
#include "gettime.h"
#include "loadLassoData.h"

// Optimization problem
//      \arg \min_x ||Ax-y|| + \lambda |x|_1
//
// We call values of vector x "features" and "weights"
//

#define STATS 0

timer loop1Time;
timer loop2Time;

void reportDetailedTime(int iterations) {
  cout << "Iterations = " << iterations << endl;
  cout << "  time per loop 1 = " << loop1Time.total()/iterations << endl;
  cout << "  time per loop 2 = " << loop2Time.total()/iterations << endl;;
}

struct feature {
  valuetype_t covar;  // Covariane of a a column
  valuetype_t Ay_i;   // (Ay)_i
};

int P;

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
std::vector<valuetype_t> xo; 
std::vector<valuetype_t> dx; 
std::vector<valuetype_t> Ax; 

// Major optimization: always keep updated vector 'Ax'

valuetype_t delta_threshold;

valuetype_t sqr(valuetype_t x) {return x*x;}

double multf = 2.0;

void initialize_feature(int feat_idx) {
  sparse_array& col = A_cols[feat_idx];
  feature& feat = feature_consts[feat_idx];
  x[feat_idx] = 0.0;
  
  // Precompute covariance of a feature
  feat.covar = 0;
  for(int i=0; i<col.length(); i++) {
    feat.covar += sqr(col.values[i]);
  }
  feat.covar *= multf;
  
  // Precompute (Ay)_i
  feat.Ay_i = 0.0;
  for(int i=0; i<col.length(); i++) {
    feat.Ay_i += col.values[i]*y[col.idxs[i]];
  }
  feat.Ay_i *= multf;
}

void initialize() {
  feature_consts.reserve(nx);
  x.resize(nx);
  xo.resize(nx);
  dx.resize(nx);
  Ax.resize(ny);
  parallel_for(int i=0; i<nx; i++) {
    initialize_feature(i);
    xo[i] = 1e10;
  }
  parallel_for(int i=0; i<ny; i++) {
    Ax[i] = 0.0;
  }
}

valuetype_t soft_threshold(valuetype_t _lambda, valuetype_t shootDiff) {
  if (shootDiff > _lambda) return _lambda - shootDiff;
  if (shootDiff < -_lambda) return -_lambda - shootDiff ;
  else return 0;
}

double shoot1(int x_i, valuetype_t lambda) {
  feature& feat = feature_consts[x_i];
  valuetype_t oldvalue = x[x_i];
  
  // Compute dotproduct A'_i*(Ax)
  valuetype_t AtAxj = 0.0;
  sparse_array& col = A_cols[x_i];
  int len=col.length();
  for(int i=0; i<len; i++) {
    AtAxj += col.values[i] * Ax[col.idxs[i]];
  }
  
  valuetype_t S_j = multf * AtAxj - feat.covar * oldvalue - feat.Ay_i;
  xo[x_i] = abs(S_j);
  valuetype_t newvalue = soft_threshold(lambda,S_j)/feat.covar;
  x[x_i] = newvalue;
  return (newvalue - oldvalue);
}

void shoot2(int x_i, valuetype_t delta) {
  sparse_array& col = A_cols[x_i];
  int len=col.length();
  // Update ax
  if (P>1) {
    for(int i=0; i<len; i++) {
      //utils::writeAdd(&Ax[col.idxs[i]], col.values[i] * delta);
      Ax[col.idxs[i]] += col.values[i]*delta;
    }
  } else {
    if (delta != 0.0) {
      for(int i=0; i<len; i++) {
	Ax[col.idxs[i]] += col.values[i]*delta;
      }
    }
  }
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


valuetype_t compute_objective(valuetype_t _lambda, std::vector<valuetype_t>& x) {
  double *least_sqrA = new double[ny];
  parallel_for(int i=0; i<ny; i++) {
    least_sqrA[i] = (Ax[i]-y[i])*(Ax[i]-y[i]);
  }
  double least_sqr = sequence::reduce(least_sqrA,ny,utils::addF<double>());
  
  double penalty = 0.0;
  for(int i=0; i<nx; i++) {
    penalty += std::abs(x[i]);
  }
  return penalty * _lambda + least_sqr;
}

struct equalZeroF { int operator() (valuetype_t a) {return a == 0.0;}};

void main_optimization_loop(double lambda, double accuracy_goal) {
  // Empirically found heuristic for deciding how many steps
  // to take on the regularization path
  P = cilk::current_worker_count();  
  int regularization_path_length = 1+(int)(nx/2000);
  valuetype_t lambda_max = compute_max_lambda();
  valuetype_t lambda_min = lambda;
  valuetype_t alpha = pow(lambda_max/lambda_min, 1.0/(1.0*regularization_path_length));
  int regularization_path_step = regularization_path_length;
  
  long long int num_of_shoots = 0;
  int counter = 0;
  int heavy_shoots = 0;
  double *delta = new double[nx];
  int *cnt = new int[nx];
  double obj = 1e10;

  if (P == 1) 
    delta_threshold = 10e-4;
  else
    delta_threshold = 25e-4;

  do {
    double maxChange = 0;
    lambda = lambda_min * pow(alpha, regularization_path_step);
    
    if (P>1) {
        // Parallel loop
        // Make a full sweep of shoots. Commit only larger shoots to Ax.

      loop1Time.start();
      
        parallel_for(int i=0; i<nx; i++) {
	  if (xo[i] < lambda) {
	    xo[i] = xo[i] * 1.1 + lambda * .01;
	    //cnt[i] = 0;
	  }
	  else {
	    int ix = i;
	    double dx = shoot1(ix, lambda);
	    double adx = abs(dx);
	    delta[ix] = adx;
	    if (adx > 2e-4) {
	      shoot2(ix, dx);
	    } 
	    //cnt[i] = 1;
	  }
        }
        //cout << "nonzeros = " << sequence::reduce(cnt,nx,utils::addF<int>()) << endl;

      loop1Time.stop();

      loop2Time.start();
      
      int mval = (regularization_path_step > 6) ? 8 : 
	((regularization_path_step > 0) ? 4 : 2);

        // Recompute Ax to reflect changes in x-vector
      if (counter % mval == 0) {
	parallel_for (int i=0; i<ny; i++) {
	  sparse_array row = A_rows[i];
	  double s = 0;
	  for(int j=0; j<row.length(); j++) {
	    s += row.values[j] * x[row.idxs[j]];
	  }
	  //cnt[i] = (Ax[i] - s)/Ax[i] > 1e-4;
	  Ax[i] = s;
	}
        //cout << "big = " << sequence::reduce(cnt,ny,utils::addF<int>()) << endl;
	obj = compute_objective(lambda_min, x);
	loop2Time.stop();
      }
        maxChange = sequence::reduce(delta,nx,utils::maxF<double>());
    } else {
        // Sequential loop
      loop1Time.start();
        for(int i=0; i<nx; i++) {
	  if (xo[i] < lambda) {
	    xo[i] = xo[i] * 1.1 + lambda * .01;
	    //cnt[i] = 0;
	  } else {
	    int ix = i;
	    double dx = shoot1(ix, lambda);
	    double adx = abs(dx);
	    maxChange =  std::max(adx, maxChange);
	    shoot2(ix, dx);  // Sequential version always commits changes, no need to defer. Also nowe
	    // sequential version does not need to make the second loop.
	  }
	}
	obj = compute_objective(lambda_min, x);
      loop1Time.stop();
    }
    

    //cout << "obj = " << obj << " counter = " << counter << endl;
    num_of_shoots += nx;
    counter++;
       
    // Convergence check.
    // We use a simple trick to converge faster for the intermediate sub-optimization problems
    // on the regularization path. This is important because we do not care about accuracy for
    // the intermediate problems, just want a good warm start for next round.
    bool converged;
    int maxCount;
    if (regularization_path_step == 0) {
      maxCount = 10000;
      converged = obj < accuracy_goal;
    } else {
      maxCount = max(20,(20-regularization_path_step)*8);
      converged = (maxChange <= get_term_threshold(regularization_path_step,
						   regularization_path_length));
    }
    if (converged || counter > maxCount) {
      if (0)  {
        //valuetype_t obj = compute_objective(lambda_min, x);
        cout << "step = " << regularization_path_step << " counter = " << counter 
        << " obj = " << obj << " shoots = " << num_of_shoots 
            << " thr=" << get_term_threshold(regularization_path_step,
                                                      regularization_path_length)
                                                        << " lambda=" << lambda<< endl;
      }
      counter = 0;
      regularization_path_step--;
    }
    //nextTime("tail");
  } while (regularization_path_step >= 0);
  if (STATS) {
    cout << "Objective = " << compute_objective(lambda, x) << endl;
    reportDetailedTime(num_of_shoots / nx);
  }
}

double solveLasso(double lambda, double accuracy) {
  initialize();
  main_optimization_loop(lambda, accuracy); 
}

