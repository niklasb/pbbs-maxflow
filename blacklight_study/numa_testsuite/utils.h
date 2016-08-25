/*
 *  utils.h
 *  numa_testsuite
 *
 *  Created by Aapo Kyrola on 4/24/11.
 *  Copyright 2011 Carnegie Mellon University. All rights reserved.
 *
 */


// Very simple option handling. Stolen from Aapo's 15-740 homework 2,
// probably written by Hyeontaek Lim



#ifndef _DEF_UTILS___
#define _DEF_UTILS___

#include <sys/time.h>
#include <map>
#include <iostream>
#include <cstdlib>
#include "pthread_tools.hpp"

 
void set_args(int ac, char ** av);

std::vector<int> single_worker(int wid);
std::vector<int> allworkers(int numw);
std::vector<int> allworkers_mod(int numw, int m);

std::string get_option_string(const char *option_name,
                              std::string default_value);

int get_option_int(const char *option_name, int default_value);

float get_option_double(const char *option_name, double default_value);



#endif



