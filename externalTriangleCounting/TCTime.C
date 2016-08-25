// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch and the PBBS team
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
#include "parallel.h"
#include "gettime.h"
#include "parseCommandLine.h"
#include "TC.h"
#include <stxxl.h> // STXXL header

using namespace std;

void writeAnswer(intT ans, char *oFile)
{
  ofstream outF(oFile);
  
  if (outF.is_open()) {
    outF << ans << endl;
  }
  else {
    cerr << "ERROR: Cannot open output file" << endl;
    exit(0xff);
  }
}

void timeTC(stxxl::vector<intT> &vertices, stxxl::vector<intT> &edges, int rounds, char* outFile) {
  intT ans;
  //ans = countTriangle(vertices,edges);
  for (int i=0; i < rounds; i++) {
    startTime();
    ans = countTriangle(vertices,edges);
    nextTimeN();
  }
  cout << endl;

  if (outFile != NULL) writeAnswer(ans, outFile);

}

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc,argv,"[-o <outFile>] [-r <rounds>] <inFile>");
  char* iFile = P.getArgument(0);
  char* oFile = P.getOptionValue("-o");
  int rounds = P.getOptionIntValue("-r",1);

  ifstream file(iFile);
  string line;
  file >> line;
  if(line != (string) "AdjacencyGraph") {
    cout << "Bad input file: missing header: " << "AdjacencyGraph" << endl;
    abort();
  }

  long n, m;
  file >> n; file >> m;

  cout<<n<<" "<<m<<endl;

  stxxl::vector<intT> vertices(n+3);
  stxxl::vector<intT> edges(m);

  for(intT i=0;i<n;i++) file >> vertices[i];
  vertices[n] = m;
  vertices[n+1] = m-1; //hack used for external sort
  vertices[n+2] = 2*m; //hack used for external sort

  //cout<<m<<" "<<vertices[n]<<" "<<vertices[n+1]<<" "<<vertices[n+2]<<endl;

  for(intT i=0;i<m;i++) file >> edges[i];

  // for(int i=0;i<n;i++)cout<<vertices[i]<<" ";cout<<endl;
  // for(int i=0;i<m;i++)cout<<edges[i]<<" ";cout<<endl;

  file.close();

  timeTC(vertices, edges, rounds, oFile);
}
