/*
 *  task_worker.h
 *  numa_testsuite
 *
 *  Created by Aapo Kyrola on 4/24/11.
 *  Copyright 2011 Carnegie Mellon University. All rights reserved.
 *
 */


#ifndef __DEF_TASK_WORKER_
#define __DEF_TASK_WORKER_

#include "pthread_tools.hpp"
#include <sys/time.h>
#include <iostream>
#include <cstdio>
#include <queue>
#include <vector>

class task {
public:
    bool used;
    barrier * bar;
    double exectime;
    int _worker;
    int _cpu_id;
    std::string tag;
    
    task() {
        used = false;
        bar = NULL;
        tag = "";
    }
    
    void set_barrier(barrier * b) {
        this->bar = b;
    }
    
    virtual void execute(int worker_id, int cpu_id) {  
        
    }
    
    void _execute(int worker_id, int cpu_id) {
        assert(!used);
        used = true;
        _worker = worker_id;
        _cpu_id = cpu_id;
        timeval current_time, start;
        gettimeofday(&start, NULL);
        execute(worker_id, cpu_id);
        gettimeofday(&current_time, NULL);
        exectime = current_time.tv_sec-start.tv_sec+ ((double)(current_time.tv_usec-start.tv_usec))/1.0E6;
        
        if (bar != NULL) {
            bar->wait();
        }   
    }
    
    virtual bool quit() {
        return false;
    }
    
    virtual std::string name() {
        return "no name";
    }
    
    virtual std::string extrastats() {
        return "";
    }
    
    virtual std::string stats() { 
        char s[1024];
        sprintf(s, "%s:%s,%d,%d,%lf,%s", name().c_str(), tag.c_str(), _worker, _cpu_id, exectime, extrastats().c_str());
        return std::string(s);
    }
};


class last_task : public task {
public:
    virtual bool quit() { return true; }
    std::string name() {
        return "last-task";
    }
};  


class task_worker : public runnable {
    
public:
    
    int worker_id, cpu_id;
    std::queue< task* > taskqueue;
    std::vector< task*> finished;
    mutex sync;
    task_worker(int wid, int cpuid):worker_id(wid), cpu_id(cpuid)  {
    }
    
    void run() {
        std::cout << "Worker " << worker_id << " starting." << std::endl;
        do {
            if (!taskqueue.empty() ) {
                sync.lock();
                task * t = taskqueue.front();
                taskqueue.pop();
                sync.unlock();
                t->_execute(worker_id, cpu_id);
                if (t->quit()) break;
                else {
                    finished.push_back(t);   
                }
            }
            sleep(0.1);
        } while (true);
        std::cout << "Worker " << worker_id << " finished." << std::endl;
    }
    
    void finish_next() {
        taskqueue.push(new last_task());
    }
    
    void add_task(task * t, barrier * b) {
        sync.lock();
        if (b != NULL) t->set_barrier(b);
        taskqueue.push(t);
        sync.unlock();
    }
    
    std::vector< task *> finished_tasks() {
        return finished;
    }
    
};

typedef task *(*task_constructor)(void);


static void report(std::vector<task *> & finished) {
    FILE * f = fopen("statsdump.txt", "a");
    for(int j=0; j<finished.size(); j++) {
        std::string s = finished[j]->stats();
        std::cout << s << std::endl;
        fprintf(f, "%s\n", s.c_str());
    }    
    fclose(f);
}


static void launch_task(task_constructor cotr, std::vector<int> launchw, std::vector<task_worker *> & workers) {
    barrier * b = new barrier(launchw.size()+1);
    std::vector<task *> tasks;
    for(int i=0; i<launchw.size(); i++) {
        task * ntask = cotr();
        tasks.push_back(ntask);
        workers[launchw[i]]->add_task(ntask, b); 
    }
    b->wait();
    sleep(0.2);
    
    //delete(b);  // MEMORY LEAK! For some reason this crashes on Mac OS X.
    
    report(tasks);
}

static void finish_all( std::vector<task_worker *> & workers) {
    for(int i=0; i<workers.size(); i++) {
        workers[i]->add_task(new last_task(), NULL);
    }
}


#endif


