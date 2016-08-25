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

#include <cmath>
#include <cstdio>
#include <cstring>
#include <vector>
#include <iostream>
#include <stdint.h>

#include "assert.h"

#ifndef __DEF_LASSO_LOAD_
#define __DEF_LASSO_LOAD_

typedef double valuetype_t;


// Simple sparse array
struct sparse_array {
    std::vector<unsigned int> idxs;
    std::vector<valuetype_t> values;   
    
    int length() {
        return idxs.size();
    }
    void add(unsigned int _idx, valuetype_t value) {
        idxs.push_back(_idx);
        values.push_back(value);
    }
};




// Removes \n from the end of line
void FIXLINE(char * s) {
    int len = strlen(s)-1; 	  
    if(s[len] == '\n') s[len] = 0;
}

void load_lasso_data(char * inputfile, std::vector<sparse_array> * cols, std::vector<sparse_array> * rows,
                    std::vector<valuetype_t> * y,   int &nx, int &ny) {
    FILE * f = fopen(inputfile, "r");
    if (f == NULL) {
      std::cout << "File not found! " << std::endl;
      exit(1);
    }
    
    char s[255];
	char delims[] = ",";	
    char *t = NULL;
    unsigned int n = 0; 
    std::string mode;
    while(fgets(s, 255, f) != NULL) {
			FIXLINE(s);
			if (n == 0) {
				t = strtok(s, delims);
				mode = std::string(t);
				if (mode == "y") {
					t = strtok(NULL, delims);
					sscanf(t, "%d", &n);
					if (mode == "y") {
						ny = n;
						if (y != NULL) y->reserve(ny);
					}
				} else if (mode == "A") {
					t = strtok(NULL, delims);
					sscanf(t, "%d", &n);
					t = strtok(NULL, delims);
					sscanf(t, "%d", &nx);
					t = strtok(NULL, delims);
					sscanf(t, "%d", &ny);
					/* Initialize rows and cols */
					if (cols != NULL) cols->resize(nx);
				    if (rows != NULL) rows->resize(ny);
				}
			} else {
				n--;
				if (mode == "y") {
					double f;
					sscanf(s, "%lf", &f);
					if (y != NULL)  y->push_back(f);
	    		} else if (mode == "A") {
					t = strtok(s, delims);
					//uint64_t idx;
					unsigned long idx;
					valuetype_t value;
					sscanf(t, "%ld", &idx);
					idx--;  // Matlab uses 1-index
					t = strtok(NULL, delims);
					sscanf(t, "%lf", &value);
					
					// Create edge from x_i to processing node
					int row_id = idx%ny;
					int col_id = idx/ny;
					assert(col_id < nx);
					if (cols != NULL) (*cols)[col_id].add(row_id, value);
				    if (rows != NULL) (*rows)[row_id].add(col_id, value);
				}
			}
		} 
}

#endif

