/* Usage : 
   Compile with "icpc -O3 -DCILKP -o write write.C"
   Run with "./write [n] [buckets] [stride] [rounds]
   Example: "./write 100000000 1024 1 1", 
*/

#include <math.h>
#include <iostream>
#include <cilk/cilk.h>
#include <stdio.h>
#include <stdlib.h>
#include <iomanip>
#include <sys/time.h>
#include <limits.h>

using namespace std;

struct timer {
  double totalTime;
  double lastTime;
  double totalWeight;
  bool on;
  struct timezone tzp;
  timer() {
    struct timezone tz = {0, 0};
    totalTime=0.0; 
    totalWeight=0.0;
    on=0; tzp = tz;}
  double getTime() {
    timeval now;
    gettimeofday(&now, &tzp);
    return ((double) now.tv_sec) + ((double) now.tv_usec)/1000000.;
  }
  void start () {
    on = 1;
    lastTime = getTime();
  } 
  double stop () {
    on = 0;
    double d = (getTime()-lastTime);
    totalTime += d;
    return d;
  } 
  double stop (double weight) {
    on = 0;
    totalWeight += weight;
    double d = (getTime()-lastTime);
    totalTime += weight*d;
    return d;
  } 
  double total() {
    if (on) return totalTime + getTime() - lastTime;
    else return totalTime;
  }
 
  void reportT(double time) {
    std::cout << "PBBS-time: " << setprecision(3) << time <<  std::endl;;
  }
};


#define newA(__E,__n) (__E*) malloc((__n)*sizeof(__E))

inline unsigned int hash(unsigned int a)
{
  a = (a+0x7ed55d16) + (a<<12);
  a = (a^0xc761c23c) ^ (a>>19);
  a = (a+0x165667b1) + (a<<5);
  a = (a+0xd3a2646c) ^ (a<<9);
  a = (a+0xfd7046c5) + (a<<3);
  a = (a^0xb55a4f09) ^ (a>>16);
  return a;
}

inline unsigned int hash2(unsigned int a)
{
  return (((unsigned int) 1103515245 * a) + (unsigned int) 12345) %
    (unsigned int) 0xFFFFFFFF;
}

void contend(int factor, int n, int buckets, int stride, int rounds){
  timer tm;
  int* A = newA(int,n);
  int* vals = newA(int,n);
  int* destinations = newA(int,n);
  int* scratch = newA(int,n);
  cilk_for(int i=0;i<n;i++) vals[i] = hash(i);
  
  cilk_for(int i=0;i<n;i++)
    destinations[i] = stride * (hash2(hash(i)) % buckets);

  // make sure buckets are paged in
  cilk_for(int i=0;i<n;i++) A[i] = 0;

  for(int r=0;r<rounds;r++){
    tm.start();
    cilk_for(int i=0;i<n;i++) {
      if(i%factor == 0) // WRITE
	A[destinations[i]] = vals[i];
      else // READ
	scratch[i] = A[destinations[i]];
    }
    tm.stop();
  }
  cout<<"(1/"<<factor<<" writes of rand val to "<< buckets<<" random locations) :";
  
  tm.reportT(tm.total());

  free(A); free(vals); free(destinations); free(scratch);
}


int main(int argc, char* argv[]){
  if (argc != 5)  {
    cout << "write <n> <buckets> <stride> <rounds>" << endl;
    abort();
  }
  int n = atoi(argv[1]);
  int buckets = atoi(argv[2]);
  int stride = atoi(argv[3]);
  int rounds = atoi(argv[4]);
  cout << "n = "<<n
       << " buckets = " << buckets
       << " stride = " << stride
       << " rounds = " << rounds << endl;

  for(int i=1;i<n;i*=2){
    contend(i,n,buckets,stride,rounds);
  }
  contend(n,n,1024,rounds,stride);
}
