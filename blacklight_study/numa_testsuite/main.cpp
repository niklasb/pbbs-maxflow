

#include <iostream>
#include "utils.h"
#include "task_worker.h"
#include "memory_access_tests.h"



class funnytask : public task {
    void execute(int workerid, int cpuid) {
        timeval start_time;
        gettimeofday(&start_time, NULL);
        
        std::cout << " ------------- " << std::endl;
        std::cout << "Executed funny task " << workerid << " on cpu " << cpuid << std::endl;
        sleep(0.1);
        timeval current_time;
        gettimeofday(&current_time, NULL);
        
        std::cout << "After sleep; took " << (current_time.tv_sec - start_time.tv_sec) << " secs." << std::endl;
        
    }
    
    std::string name() {
        return "dummy-test";
    }
};

task * funnytask_constr() { 
    return new funnytask();
}



int main (int argc, char * const argv[]) {
    FILE * f = fopen("statsdump.txt", "w");
    fprintf(f, "Started...%ld\n", time(NULL));
    fclose(f);
    
    // insert code here...
    set_args(argc-1, (char**) (argv+1));
    
    std::string test = get_option_string("testtype", "default");
    std::cout << "Running test: " << test << std::endl;
    int num_workers = get_option_int("num_workers", 32);
    int aff_multiplier = get_option_int("affinity_multiplier", 1);
    std::cout << "Going to start " << num_workers << " workers." << std::endl;
    
    thread_group * workergroup = new thread_group();
    std::vector< task_worker * > workers;
    std::vector< int > all;
    for(int workerid=0; workerid<num_workers; workerid++) {
        int cpuid = workerid*aff_multiplier;
        task_worker * worker = new task_worker(workerid, cpuid);
        workers.push_back(worker);
        workergroup->launch(worker); 
        all.push_back(workerid);
    }
    sleep(2.5);
    
    
    //launch_task(funnytask_constr, all, workers);
    
    init_memaccesstest(workers.size());
    if (test == "default") {
        run_memaccesstest(workers);
    } else if (test == "quickbig") {
        run_quickbigtest(workers);
    }
    finish_all(workers);
    
    workergroup->join();
    return 0;
}
