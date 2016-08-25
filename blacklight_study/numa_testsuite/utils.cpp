/*
 *  utils.cpp
 *  numa_testsuite
 *
 *  Created by Aapo Kyrola on 4/24/11.
 *  Copyright 2011 Carnegie Mellon University. All rights reserved.
 *
 */

#include <string.h> 
#include "utils.h"

int _argc;
char **_argv;

void set_args(int ac, char ** av) {
    _argc = ac;
    _argv = av;
}



std::vector<int> single_worker(int wid) {
    std::vector<int> ii;
    ii.push_back(wid);
    return ii;
}

std::vector<int> allworkers(int numw) {
    std::vector<int> v;
    for(int i=0; i<numw; i++) v.push_back(i);
    return v;
}
std::vector<int> allworkers_mod(int numw, int m) {
  std::vector<int> v;
  for(int i=0; i<numw; i+=m) v.push_back(i);
  return v;
}

std::string get_option_string(const char *option_name,
                              std::string default_value)
{
    int i;
    
    for (i = _argc - 2; i >= 0; i -= 2)
        if (strcmp(_argv[i], option_name) == 0)
            return std::string(_argv[i + 1]);
    return default_value;
}


int get_option_int(const char *option_name, int default_value)
{
    int i;
    
    for (i = _argc - 2; i >= 0; i -= 2)
        if (strcmp(_argv[i], option_name) == 0)
            return atoi(_argv[i + 1]);
    return default_value;
}


float get_option_double(const char *option_name, double default_value)
{
    int i;
    
    for (i = _argc - 2; i >= 0; i -= 2)
        if (strcmp(_argv[i], option_name) == 0)
            return (double)atof(_argv[i + 1]);
    return default_value;
}


struct timing {
    timeval start;
    double cumulative_secs;
    timing() {cumulative_secs = 0.0;}
};





