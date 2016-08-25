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

//
// Integrated timer and checker for Nearest Neighbor. Because of the complexity
// of naive nearest-neighbor search, checking is done by querying a random number
// nearest neighbors. Program is not allowed to do any work when the checking is done,
// only lookups are allowed.
// by Aapo Kyrola

#include <iostream>
#include <algorithm>
#include "sequence.h"
#include "gettime.h" 
#include "utils.h"
#include "cilk.h" 
#include "IO.h"
#include "parseCommandLine.h"
#include "PointGenerator.h"
#include <vector>

using namespace std;
using namespace benchIO;

#define SQR(x) ((x)*(x))

double distsqr(double3 a, double3 b) {
    return (SQR(a[0]-b[0])+SQR(a[1]-b[1])+SQR(a[2]-b[2]));
}



int nearest_neighbor_naive(std::vector<double3> & points, int orig) {
    double minsqr=1e30; 
    int minidx=-1;
    for(int i=0; i<points.size(); i++) {
        if (i != orig) {
            double r2 = distsqr(points[i], points[orig]);
            if (r2<minsqr) {
                minidx = i;
                minsqr = r2;
            }
        }
    }
    assert(minidx>=0 && minidx != orig);
    return minidx;
}

bool checkNN(vector<double3> & pointset) {
    // Use max 0.5 secs for checking or max 1000 points
    int maxpoints = 1000;
    double maxtime = 0.5;
    timer ctimer;
    ctimer.start();
    srand(time(NULL));
    for(int i=0; i<maxpoints; i++) {
        if (ctimer.total() < maxtime || i < 50) {
            int pointcheck = random()%pointset.size();
            double3 nearestpoint = pointset[nearest_neighbor_naive(pointset, pointcheck)];
            double3 alg_nn = getNearestPoint(pointcheck);
            // Two points may be same distance, so it is not enough to check correct point index
            if (distsqr(pointset[pointcheck], alg_nn) == distsqr(pointset[pointcheck], nearestpoint)) {
                // ok
            } else {
                std::cout.precision(5);
                std::cout << "Fail, algorithm returned incorrect nn for point " << pointcheck << std::endl;
                std::cout << "Distance to alg's nn: " << distsqr(pointset[pointcheck], alg_nn) << " correct: " <<  
                        distsqr(pointset[pointcheck], nearestpoint) << std::endl;
                std::cout << "Failed query point was: " << pointset[pointcheck][0] << " " << pointset[pointcheck][1] << " "   
                        << pointset[pointcheck][2] << std::endl;
                std::cout << "Algorithm returned: " << alg_nn[0] << " " << alg_nn[1] << " " << alg_nn[2] << std::endl;
                return false;
            }
        } else {
            std::cout << ":: Exceeded 0.5 sec checking limit." << std::endl;
            break;
        }
    }
    return true;
}
 
void timeNN(std::string testType, size_t numPoints, char * outFile, int rounds) {
    vector<double3> pointset;
  
    srand(1);
    if (testType == "grid") {
        createGridPoints((int) sqrt(numPoints), (int) sqrt(numPoints), pointset);
    }
    else if (testType == "random") {
        createRandomPoints(10.0, 10.0, numPoints, pointset);
    }
    else if (testType == "shell") {
        createShellPoints(10.0,  numPoints, pointset);
    } else assert(false);

    for (int i=0; i < rounds; i++) {
      startTime();
      prepareForPoints(pointset.size());
      parallel_for(int i=0; i<pointset.size(); i++) {
         addPoint(i, pointset[i]);
      }
      computeNN(1);
      nextTimeN();
      
      bool success = checkNN(pointset);
      if (outFile != NULL) {
         FILE * outf = fopen(outFile, "w");
         fprintf(outf, "%d\n", (success ? 1 : 0));
         fclose(outf);
      }
        
     }
     cout << endl;    
}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv," [-o <outFile>] [-r <rounds>] [-n <numPoints>] [-t <testType=random|shell|grid>]");
  char* testType = P.getOptionValue("-t");
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);
  size_t numPoints = (size_t) P.getOptionIntValue("-n", 1000);
  timeNN(std::string(testType), numPoints, oFile, rounds);
}
