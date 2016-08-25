

#include <vector>
#include "task_worker.h"

void init_memaccesstest(int num_workers);
void run_memaccesstest(std::vector<task_worker *> & workers);
void run_quickbigtest(std::vector<task_worker *> & workers);