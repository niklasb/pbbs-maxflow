#include "utils.h"
#include "graph.h"
#include "parallel.h"
#include "IO.h"
#include "parseCommandLine.h"

int parallel_main(int argc, char* argv[]) {
  commandLine P(argc, argv, " input output");
  char* iFile = P.getArgument(1);
  char* oFile = P.getArgument(0);
  
  bool streaming = P.getOption("-streaming");
  bool remDups = P.getOption("-remDups");

  ifstream file (iFile, ios::in | ios::binary);
  if (!file.is_open()) {
    std::cout << "Unable to open file: " << iFile << std::endl;
    abort();
  }
  cout << "Streaming file\n";
  string s;
  getline(file,s); //get header
  if(s != "AdjacencyGraph"){
    cout << "Bad matrix file" << endl;
    abort();
  }
  char ss[256];
  file >> ss;
  intT n = atol(ss);
  file >> ss;
  intT m = atol(ss);

  cout<<"n = " << n << " m = "<<m << endl;

  //intT* In = newA(intT,n+m);
  intT* offsets = newA(intT,n);
  uint* edges = newA(uint,m);
  intT i=0;
  while(i < n) {
    file >> ss;
    offsets[i++] = atol(ss);
  }
  i=0;
  while(file >> ss){
    if(i > m) {cout << "Bad input file" << endl; abort();}
    edges[i++] = atoi(ss);
  }
  file.close();
  ofstream file2(oFile, ios::out | ios::binary);
  file2.write((char*)&n,sizeof(intT));

  file2.write((char*)&m,sizeof(intT));

  file2.write((char *)offsets,sizeof(intT)*(n));
  file2.write((char *)edges,sizeof(uint)*(m));

  file2.close();


  free(offsets); free(edges);

}
