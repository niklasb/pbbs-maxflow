/*
    Copyright 2005-2014 Intel Corporation.  All Rights Reserved.

    This file is part of Threading Building Blocks.

    Threading Building Blocks is free software; you can redistribute it
    and/or modify it under the terms of the GNU General Public License
    version 2 as published by the Free Software Foundation.

    Threading Building Blocks is distributed in the hope that it will be
    useful, but WITHOUT ANY WARRANTY; without even the implied warranty
    of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with Threading Building Blocks; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    As a special exception, you may use this file as part of a free software
    library without restriction.  Specifically, if other files instantiate
    templates or use macros or inline functions from this file, or you compile
    this file and link it with other files to produce an executable, this
    file does not by itself cause the resulting executable to be covered by
    the GNU General Public License.  This exception does not however
    invalidate any other reasons why the executable file might be covered by
    the GNU General Public License.
*/

// Workaround for ICC 11.0 not finding __sync_fetch_and_add_4 on some of the Linux platforms.
#if __linux__ && defined(__INTEL_COMPILER)
#define __sync_fetch_and_add(ptr,addend) _InterlockedExchangeAdd(const_cast<void*>(reinterpret_cast<volatile void*>(ptr)), addend)
#endif
#include <string>
#include <cstring>
#include <cctype>
#include <cstdlib>
#include <cstdio>
#include "tbb/concurrent_hash_map.h"
#include "tbb/blocked_range.h"
#include "tbb/parallel_for.h"
#include "tbb/tick_count.h"
#include "tbb/task_scheduler_init.h"
#include "tbb/tbb_allocator.h"
#include "../../common/utility/utility.h"


//! String type with scalable allocator.
/** On platforms with non-scalable default memory allocators, the example scales 
    better if the string allocator is changed to tbb::tbb_allocator<char>. */
typedef std::basic_string<char,std::char_traits<char>,tbb::tbb_allocator<char> > MyString;

using namespace tbb;
using namespace std;

//! Set to true to counts.
static bool verbose = false;
static bool silent = false;
//! Problem size
long N = 1000000;
const int size_factor = 2;

struct HashCompare {
  static size_t hash (int a) {
   a = (a+0x7ed55d16) + (a<<12);
   a = (a^0xc761c23c) ^ (a>>19);
   a = (a+0x165667b1) + (a<<5);
   a = (a+0xd3a2646c) ^ (a<<9);
   a = (a+0xfd7046c5) + (a<<3);
   a = (a^0xb55a4f09) ^ (a>>16);
   return a;
  }
  static bool equal(int a, int b) { return a == b; }
};



//! A concurrent hash table that maps strings to ints.
typedef concurrent_hash_map<int,int,HashCompare> IntTable;

//! Function object for counting occurrences of strings.
struct Tally {
  IntTable& table;
    Tally( IntTable& table_ ) : table(table_) {}
    void operator()( const blocked_range<int*> range ) const {
        for( int* p=range.begin(); p!=range.end(); ++p ) {
            IntTable::accessor a;
            table.insert( a, *p );
            //a->second += 1;
        }
    }
};

static void CountOccurrences(int nthreads, int* Data) {
    IntTable table;    
    tick_count t0 = tick_count::now();
    parallel_for( blocked_range<int*>( Data, Data+N ), Tally(table) );
    tick_count t1 = tick_count::now();

    //int n = 0;
    // for( StringTable::iterator i=table.begin(); i!=table.end(); ++i ) {
    //     if( verbose && nthreads )
    //         printf("%s %d\n",i->first.c_str(),i->second);
    //     n += i->second;
    // }

    if ( !silent ) printf("time = %g\n", (t1-t0).seconds());
}


int main( int argc, char* argv[] ) {
    try {
        tbb::tick_count mainStartTime = tbb::tick_count::now();
        srand(2);

        //! Working threads count
        // The 1st argument is the function to obtain 'auto' value; the 2nd is the default value
        // The example interprets 0 threads as "run serially, then fully subscribed"
        utility::thread_number_range threads(tbb::task_scheduler_init::default_num_threads,0);

        if ( silent ) verbose = false;
	if(argc > 1) N = atol(argv[1]);
	int* Data = new int[N];
	for(long i=0;i<N;i++) Data[i] = rand()%N;

        if ( threads.first ) {
            for(int p = threads.first;  p <= threads.last; p = threads.step(p)) {
                if ( !silent ) printf("threads = %d  ", p );
                task_scheduler_init init( p );
                CountOccurrences( p, Data );
            }
        } else { // Number of threads wasn't set explicitly. Run serial and parallel version
            { // serial run
	      //if ( !silent ) printf("serial run   ");
                //task_scheduler_init Dnit_serial(1);
                //CountOccurrences(1, Data);
            }
            { // parallel run (number of threads is selected automatically)
                if ( !silent ) printf("parallel run ");
                task_scheduler_init init_parallel;
                CountOccurrences(0, Data);
            }
        }

        delete[] Data;

        utility::report_elapsed_time((tbb::tick_count::now() - mainStartTime).seconds());

        return 0;
    } catch(std::exception& e) {
        std::cerr<<"error occurred. error text is :\"" <<e.what()<<"\"\n";
    }
}
