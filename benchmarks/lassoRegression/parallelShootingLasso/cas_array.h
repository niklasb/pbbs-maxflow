#include <pthread.h>
#include <vector>
#include <iterator>
#include "assert.h"
#include "utils.h"
#include <cstdlib>

#ifndef _CAS_ARRAY_
#define _CAS_ARRAY_

// Concurrent array using compare-and-swap operations.
// Implements ONLY add() for mutability, and supports only doubles.

 
#define PADDED_ARRAY 0
#define CACHELINE 8

template <typename T=double>
class cas_array {
    
   public:
   
    T * arr;
    int len;

    cas_array() {
        arr = NULL;
    }
    
    
    
    // Note: padding for cache lines
    cas_array(int sz) {
       assert(sizeof(T)>0);
       #ifndef PADDED_ARRAY
        arr = (T *) calloc(sz, sizeof(T));
       #endif
       #ifdef PADDED_ARRAY
        arr = (T *) calloc(sz*CACHELINE, sizeof(T));
       #endif
       len = sz;
    }
    
    ~cas_array() {
        if (arr != NULL) free(arr);
    }
    
    // Note: also destroys contents
    void resize(int sz) {
        #ifndef PADDED_ARRAY
        arr = (T *) realloc(arr, sz * sizeof(T));
        memset(arr, 0, sz *sizeof(T));
       #endif
       #ifdef PADDED_ARRAY
        arr = (T *) realloc(arr, sz*CACHELINE *sizeof(T));
        memset(arr, 0, sz*CACHELINE *sizeof(T));
       #endif
       len = sz;
    }
    
    T& operator[] (unsigned int idx) {
         #ifdef PADDED_ARRAY
         idx <<= 3;
         #endif
        return arr[idx];
    }
    
    void add(int idx, double delta) {
         assert(idx<len);
         #ifdef PADDED_ARRAY
         idx <<= 3;
         #endif
	 utils::writeAdd(&arr[idx],delta);
    }
    
      void add(int idx, float delta) {
         #ifdef PADDED_ARRAY
         idx <<= 3;
         #endif
        volatile float prev = arr[idx];
        volatile float newval;
        volatile float oldval;
        do {
            oldval = prev;
            newval = prev + delta;
            uint32_t res = (__sync_val_compare_and_swap(reinterpret_cast<uint32_t *>(&arr[idx]),
                                *reinterpret_cast<volatile uint32_t *>(&prev), *reinterpret_cast<volatile uint32_t*>(&newval)));
            prev = *(reinterpret_cast<float *>(&res));
        } while (prev != oldval);
    }
 
};

#endif
