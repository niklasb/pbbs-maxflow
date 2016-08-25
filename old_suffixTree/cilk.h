#if defined(CILK)
#include <cilk.h>
#define cilk_for_1 _Pragma("cilk_grainsize = 1") cilk_for
#define _cilk_grainsize_1 _Pragma("cilk_grainsize = 1")
#define _cilk_grainsize_2 _Pragma("cilk_grainsize = 2")
#define _cilk_grainsize_256 _Pragma("cilk_grainsize = 256")
#elif defined(IPPROOT)
#define cilk_main main
#define cilk_for_1 _Pragma("cilk grainsize = 1") cilk_for
#define _cilk_grainsize_1 _Pragma("cilk grainsize = 1")
#define _cilk_grainsize_2 _Pragma("cilk grainsize = 2")
#define _cilk_grainsize_256 _Pragma("cilk grainsize = 256")
#elif defined(CILKP)
#define cilk_main main
#define cilk_for_1 _Pragma("cilk grainsize = 1") cilk_for
#define _cilk_grainsize_1 _Pragma("cilk grainsize = 1")
#define _cilk_grainsize_2 _Pragma("cilk grainsize = 2")
#define _cilk_grainsize_256 _Pragma("cilk grainsize = 256")
#include <cilk/cilk.h>
#elif defined(OPENMP)
#define cilk_spawn
#define cilk_sync
#define cilk_for_1 _Pragma("omp parallel for schedule (static,1)") for
#define cilk_for _Pragma("omp parallel for") for
#define cilk_main main
#define _cilk_grainsize_1 
#define _cilk_grainsize_2 
#define _cilk_grainsize_256 
#include <omp.h>
#else
#define cilk_spawn
#define cilk_sync
#define cilk_for_1 for
#define cilk_for for
#define cilk_main main
#define _cilk_grainsize_1 
#define _cilk_grainsize_2 
#define _cilk_grainsize_256 
#endif

#include <limits.h>

#if defined(LONG)
typedef long intT;
typedef unsigned long uintT;
#define INT_T_MAX LONG_MAX
#else
typedef int intT;
typedef unsigned int uintT;
#define INT_T_MAX INT_MAX
#endif