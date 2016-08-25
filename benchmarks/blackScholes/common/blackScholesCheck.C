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

#include <iostream>
#include <algorithm>
#include <cstring>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "options.h"
using namespace std;

int main(int argc, char* argv[]) {
  if(argc != 3) {
    printf("Usage: ./blackScholesCheck <inputFile> <pricesFile>\n");
    exit(1);
  }
  char *inputFile = argv[1];
  char *pricesFile = argv[2];
  //Read input data from file
  FILE* file = fopen(inputFile, "r");
  if(file == NULL) {
    printf("ERROR: Unable to open file `%s'.\n", inputFile);
    exit(1);
  }
  int rv = fscanf(file, "%i", &numOptions);
  if(rv != 1) {
    printf("ERROR: Unable to read from file `%s'.\n", inputFile);
    fclose(file);
    exit(1);
  }

  // alloc spaces for the option data
  data = (OptionData*)malloc(numOptions*sizeof(OptionData));
  prices = (fptype*)malloc(numOptions*sizeof(fptype));
  for (int loopnum = 0; loopnum < numOptions; ++ loopnum )
    {
      rv = fscanf(file, "%f %f %f %f %f %f %c %f %f", &data[loopnum].s, &data[loopnum].strike, &data[loopnum].r, &data[loopnum].divq, &data[loopnum].v, &data[loopnum].t, &data[loopnum].OptionType, &data[loopnum].divs, &data[loopnum].DGrefval);
      if(rv != 9) {
	printf("ERROR: Unable to read from file `%s'.\n", inputFile);
	fclose(file);
	exit(1);
      }
    }
  rv = fclose(file);
  if(rv != 0) {
    printf("ERROR: Unable to close file `%s'.\n", inputFile);
    exit(1);
  }

  FILE* pfile = fopen(pricesFile, "r");
  if(pfile == NULL) {
    printf("ERROR: Unable to open file `%s'.\n", pricesFile);
    exit(1);
  }
  int numPrices;
  rv = fscanf(pfile, "%i", &numPrices);
  if(rv != 1) {
    printf("ERROR: Unable to read from file `%s'.\n", pricesFile);
    fclose(pfile);
    exit(1);
  }
  
  if(numPrices != numOptions) {
    printf("ERROR: number of options is wrong\n");
    fclose(pfile);
    exit(1);
  }

  for (int loopnum = 0; loopnum < numPrices; ++ loopnum )
    {
      rv = fscanf(file, "%f", &prices[loopnum]);
    }
  rv = fclose(file);
  if(rv != 0) {
    printf("ERROR: Unable to close file `%s'.\n", inputFile);
    exit(1);
  }
  for(int i = 0; i < numPrices; ++i) {
      fptype priceDelta = data[i].DGrefval - prices[i];
      if( fabs(priceDelta) >= 1e-4 ){
        fprintf(stderr,"Error on %d. Computed=%.5f, Ref=%.5f, Delta=%.5f\n",
		i, prices[i], data[i].DGrefval, priceDelta);
        numError ++;
      }
  }
  if(numError == 0) printf("No errors\n");
  return 0;

}
