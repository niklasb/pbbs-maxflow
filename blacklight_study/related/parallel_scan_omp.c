 
#include <cmath>
#include <cstdio>
#include <string>
#include <vector>
#include <iostream>
#include "assert.h"
#include <cstdlib>
#include "shared.c"

std::vector<double> list;
std::vector<double> ground_truth;

#define CILK_FOR_GRAINSIZE 64

void create_list(int n) {
    list.resize(n);
    std::cout << "Creating list " << n << std::endl;
    srandom(time(NULL));
    for(int i=0; i<n; i++) {
        list[i] = random()%10;
    }
    list[0] = 0;
}

void create_ground_truth() {
    ground_truth.resize(list.size());
    start_timing(0);
    double sum = 0;
    ground_truth[0] = 0;
    for(int i=1; i < list.size(); i++) {
        ground_truth[i] = ground_truth[i-1]+list[i-1];
    }
    stop_timing(0);
}

bool verify(std::vector<double> & res) {
    for(double i=0; i<ground_truth.size() ; i++) {
        if(ground_truth[i] != res[i]) {
            std::cout << "Error at index i = " << i << ": " << ground_truth[i]  << " != " << res[i] << std::endl;
            return false;
        }
    }
    return true;
}

void print(std::vector<double> &l) {
    for(int i=0; i<l.size() && i<32; i++) {
        std::cout << l[i] << " ";
        if (i%12 == 113) std::cout << std::endl;
    }
    std::cout << std::endl << " last: " << l[l.size()-1] << std::endl;
    std::cout << std::endl;
}



int LOG2(int x) {
    int i = 1;
    int j = 0;
    while(i<x) {
        j++;
        i*=2;
    }
    return j;
}


// Note, requires l.size() be power of two. 
// Based on Guy's algorithm as cited in paper: Scan Primites for GPU Computing
void parallel_scan_memoryefficient(std::vector<double> &l, std::vector<double> &A) {
    
    int depth = LOG2(l.size());
    int n = l.size();
    for(int i=0; i<n; i++) A[i] = l[i];
    
    // Up-sweep / reduce
    for(int k=0; k<depth; k++) {
        int h = (1<<(k+1));
        #pragma omp parallel for
        for(int i=0; i<n; i+=h) {
           A[i + h -1] += A[i + (h>>1)-1];
        }
    }
    
    // Down-sweep
    A[n-1] = 0;
    for(int d=depth-1; d>=0; d--) {
        int h = (1<<(d+1));
        #pragma omp parallel for
        for(int i=0; i<n; i+=h) {
            double t = A[i + (h>>1) - 1];
            A[i + (h>>1)-1] = A[i + h-1];
            A[i + h-1] = t + A[i + h-1];
        }
       
    }
}


// Note, requires l.size() be power of two. 
void parallel_scan(std::vector<double> &l, std::vector<double> & workl) {
    if (l.size() <= 1) {
        workl[0] = 0;
    } if (l.size() < 256) {
        workl[0] = 0;
        for(int i=1; i<workl.size(); i++) {
            workl[i] = workl[i-1]+l[i-1];
        }
    } else {
        int n = l.size();
        std::vector<double> sums(n/2);
        #pragma omp parallel for
        for(int i=0; i<n/2; i++) {
            sums[i] = l[i*2] + l[i*2+1];
        }
        // Recurse
        std::vector<double> evens(sums.size());
        parallel_scan(sums, evens);
        // Odds (now reuse sums)
        std::vector<double> & odds = sums;
        #pragma omp parallel for
        for(int i=0; i<n/2; i++) {
            odds[i] = evens[i] + l[i*2];
        }
        // Interleave
        #pragma omp parallel for
        for(int i=0; i<n/2; i++) {
            workl[i*2] = evens[i];
            workl[i*2+1] = odds[i];
        }
       
    }
}

#define CHUNKSIZE 2048
#define PAD 8
void parallel_scan_simple(std::vector<double> &l, std::vector<double> & A) {
     int n = l.size();
     A[0] = 0;
     
     int nchunks = n/CHUNKSIZE;          
     if ((n%CHUNKSIZE) > 0) nchunks++;
     if (nchunks < 2) nchunks = 2;
     std::vector<double> chunksums(nchunks*PAD,0);

     #pragma omp parallel for
        for(int c=0; c<nchunks-1; c++) {
         int chunkstart = c*CHUNKSIZE;
         int chunkend = (c+1)*CHUNKSIZE;
         if (c==0) chunkstart++;
         double sum = 0;
         for(int i=chunkstart; i<chunkend; i++) {
           sum += l[i-1];
         }
         chunksums[c*PAD] = sum;
     }
     // Update pivot points
     for(int c=1; c<nchunks; c++) {
        chunksums[c*PAD] += chunksums[(c-1)*PAD];
     }
     
     // FIll in (first chunk already filled)
     #pragma omp parallel for
        for(int c=0; c<nchunks; c++) {
         int chunkstart = c*CHUNKSIZE;
         int chunkend = (c+1)*CHUNKSIZE;
         if (chunkend>n) chunkend=n;
         if (c==0) chunkstart++;
         else A[chunkstart-1] = chunksums[(c-1)*PAD];
         for(int i=chunkstart; i<chunkend; i++) {
            A[i] = A[i-1]+l[i-1];
         }
     }
}



int main(int argc, char* argv[]) {
      _argc = argc - 1; 
     _argv = argv + 1;
     
     int N =  1<<get_option_int("e", 8);
     create_list(N);
     print(list);
     assert(get_option_int("e", 8)  == LOG2(N));
     
     create_ground_truth();

     std::vector<double> res(list.size());
     start_timing(1);
     parallel_scan(list, res);
     if (verify(res)) {
         std::cout << "SUCCESS." << std::endl;
     } else {
        std::cout << "FAILURE!." << std::endl;
     }
     stop_timing(1);
     
     std::vector<double> res2(list.size());
     start_timing(2);
     parallel_scan_memoryefficient(list, res2);
     print(res2);
     
     if (verify(res2)) {
        std::cout << "SUCCESS." << std::endl;
     } else {
        std::cout << "FAILURE!." << std::endl;
     }
     
     stop_timing(2);
     
     std::vector<double> res3(list.size());
     start_timing(3);
     parallel_scan_simple(list, res3);
     stop_timing(3);
     print(res3);
     
     if (verify(res3)) {
        std::cout << "SUCCESS." << std::endl;
     } else {
        std::cout << "FAILURE!." << std::endl;
     }
     
     
     std::cout << "Ground truth:" << std::endl;
     print(ground_truth);
     
     std::cout << "Parallel result: (mem eff)" << std::endl;
    
     
     print_timings();
     std::cout << "Speedup: " << (timings[0].cumulative_secs/timings[1].cumulative_secs) << std::endl
                    << " memory efficient version: " <<  (timings[0].cumulative_secs/timings[2].cumulative_secs)  << std::endl 
                        << " simple: " <<  (timings[0].cumulative_secs/timings[3].cumulative_secs) << "\n";
}

