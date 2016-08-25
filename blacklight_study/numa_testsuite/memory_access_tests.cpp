/*
 *  memory_access_tests.cpp
 *  numa_testsuite
 *
 *  Created by Aapo Kyrola on 4/24/11.
 *  Copyright 2011 Carnegie Mellon University. All rights reserved.
 *
 */

#include "memory_access_tests.h"

/*
 *  memory_access_tests.h
 *  numa_testsuite
 *
 *  Created by Aapo Kyrola on 4/24/11.
 *  Copyright 2011 Carnegie Mellon University. All rights reserved.
 *
 */

#include <iostream>
#include <algorithm>
#include "utils.h"
#include "task_worker.h"

int megabytes;
int _num_workers;
int ** allocs;

int round;
size_t numints; 


class alloc_task : public task {
public:    
    void execute(int worker_id, int cpu_id) {
        if (allocs[worker_id] != NULL) free(allocs[worker_id]);
        allocs[worker_id] = (int *) malloc(megabytes * 1024 * 1024);
        for(size_t i=0; i<numints; i++) {
            allocs[worker_id][i] = i%1000;
        }
        std::cout << "Allocation done. " << std::endl;
    }
    virtual std::string name() {
        return "allocate+init";
    }
};

task * create_alloc_task() { return new alloc_task(); }

class access_task : public task {
public:
    bool seq;
    bool reallocated;
    int target;
    int _round;
    access_task(bool _seq, bool _realloc, int r) : seq(_seq), reallocated(_realloc) {
        _round = r;
    }
    
    void execute(int worker_id, int cpu_id) {
        target = (worker_id+_round)%_num_workers;
        size_t sum = 0, checksum = 0;
        for(size_t i=0; i<numints; i+=16) {
            checksum += i%1000;
            sum += allocs[target][i];
        }
        assert(checksum == sum);
    }
    
    virtual std::string extrastats() {
        char s[64];
        sprintf(s, "%d", target);
        return std::string(s);
    }
    
    virtual std::string name() {
        return std::string("access:")+ (seq ? "seq" : "parallel")+":"+(reallocated? "realloc" : "oldalloc");
    }
};


class random_access_task : public task {
public:
    bool seq;
    int target;
    int _round;
    random_access_task(bool _seq, int r) : seq(_seq) {
        _round = r;
    }
    
    void execute(int worker_id, int cpu_id) {
        target = (worker_id+_round)%_num_workers;
        size_t sum = 0;
        size_t j = numints/2;
        for(size_t i=0; i<numints; i+=16) {
            sum += allocs[target][j];
            j = (j+5*(numints/9+3))%numints;
        }
    }
    
    virtual std::string extrastats() {
        char s[64];
        sprintf(s, "%d", target);
        return std::string(s);
    }
    
    virtual std::string name() {
        return std::string("randomacc:")+ (seq ? "seq" : "parallel");
    }
};


task * create_individual_access_task() { task * t =  new access_task(true, false, round);
                                      t->tag = "indiv";
                                      return t;
                                    }

task * create_seq_access_task_realloc() { return new access_task(true, true, round); } 
task * create_seq_access_task_oldalloc() { return new access_task(true, false, round); } 

task * create_par_access_task() { return new access_task(false, false, round); } 
task * create_seq_random_access_task() { return new random_access_task(true, round); } 
task * create_par_random_access_task() { return new random_access_task(false, round); } 

void init_memaccesstest(int num_workers) {
    megabytes = get_option_int("memacc.mb", 32);
    std::cout << "Memory access going to allocate " << megabytes << " mb." << std::endl;
    numints  = megabytes * 1024 * 1024 / sizeof(int);
    _num_workers = num_workers;
    allocs = (int **) malloc(sizeof(int*) * num_workers);
    for(int i=0; i<num_workers; i++) allocs[i] = NULL;
    round = 0;
}

void run_quickbigtest(std::vector<task_worker *> & workers) {
    
    if (workers.size() > 54) {
        launch_task(create_alloc_task, single_worker(54), workers);
        // access from 54 to 54
        round = 0;
        launch_task(create_individual_access_task, single_worker(54), workers);
        round = 54-4; // So 4->54 access (this is cumbersome)
        launch_task(create_individual_access_task, single_worker(4), workers);
        // again
        launch_task(create_individual_access_task, single_worker(4), workers);
        
        // then access from 54 to 54
        round = 0;
        launch_task(create_individual_access_task, single_worker(54), workers);
        launch_task(create_individual_access_task, single_worker(54), workers);
        
    }
    
}

void run_memaccesstest(std::vector<task_worker *> & workers) {
    // Stage 1 indivual access tests
    // First: alloc on 54, access from 4
    // Hypothesis that memory ownership moves according to last access. 
    
    if (workers.size() > 54) {
        launch_task(create_alloc_task, single_worker(54), workers);
        // access from 54 to 54
        round = 0;
        launch_task(create_individual_access_task, single_worker(54), workers);
        round = 54-4; // So 4->54 access (this is cumbersome)
        launch_task(create_individual_access_task, single_worker(4), workers);
        // again
        launch_task(create_individual_access_task, single_worker(4), workers);

        // then access from 54 to 54
        round = 0;
        launch_task(create_individual_access_task, single_worker(54), workers);
        launch_task(create_individual_access_task, single_worker(54), workers);

    }
    
    
    // Stage 2 sequential access tests
    std::cout << "Starting sequential access test. " << std::endl;
    int step = workers.size()/8;
    if (step<4) step = 4;
    std::cout << " ... step=" << step << std::endl;
    for(int w=0; w<workers.size(); w+=step) {
        std::cout << "Reallocations... " << std::endl;
        launch_task(create_alloc_task, allworkers_mod(workers.size(), 4), workers);
        for(round=0; round<workers.size(); round+=4) {
            launch_task(create_seq_access_task_realloc, single_worker(w), workers);
        }
    }
    
    launch_task(create_alloc_task, allworkers_mod(workers.size(), 4), workers);

    // Same without reallocations
    for(int w=0; w<workers.size(); w+=step) {
        for(round=0; round<workers.size(); round+=4) {
            launch_task(create_seq_access_task_oldalloc, single_worker(w), workers);
        }
    }
    
    // Stage 3 parallel access tests. First alloc
    launch_task(create_alloc_task, allworkers_mod(workers.size(), 1), workers);
    std::cout << "Starting parallel access tests. " << std::endl;
    for(round=0; round<workers.size(); round+=4) {
        launch_task(create_par_access_task, allworkers(workers.size()), workers);
    }
    
    // Stage 4 sequential random access test
    for(int w=0; w<workers.size(); w+=step) {
        // Reallocate before each test
        std::cout << "Reallocate" << std::endl;
        launch_task(create_alloc_task, allworkers_mod(workers.size(), 4), workers);
        for(round=0; round<workers.size(); round+=4) {
            launch_task(create_seq_random_access_task, single_worker(w), workers);
        }
    }
}
