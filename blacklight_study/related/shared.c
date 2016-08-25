

#include <map>
#include <cstdlib>
#include <iostream>
#include <sys/time.h>
#include <cstring>

// Timing
struct timing {
    timeval start;
    float cumulative_secs;
    timing() {cumulative_secs = 0.0;}
};
 
static int _argc;
static char **_argv;

 
std::map<int, timing> timings;

void start_timing(int key) {
    if (timings.count(key) == 0) {
       timings[key] = timing();
    }
    gettimeofday(&timings[key].start, NULL);
}

void stop_timing(int key) {
    timing& t = timings[key];
    timeval current_time;
    gettimeofday(&current_time, NULL);
    t.cumulative_secs += current_time.tv_sec-t.start.tv_sec+ ((float)(current_time.tv_usec-t.start.tv_usec))/1.0E6;
}

void print_timings() {
    std::map<int, timing>::iterator iter;
    for( iter = timings.begin(); iter != timings.end(); ++iter ) {
      std::cout << "timing_key: " << iter->first << ", seconds: " << iter->second.cumulative_secs << std::endl;
    }
}




// Very simple option handling. Stolen from Aapo's 15-740 homework 2,
// probably written by Hyeontaek Lim


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


int get_option_long(const char *option_name, long long int default_value)
{
    int i;

    for (i = _argc - 2; i >= 0; i -= 2)
	if (strcmp(_argv[i], option_name) == 0)
	    return atol(_argv[i + 1]);
    return default_value;
}




float get_option_float(const char *option_name, float default_value)
{
    int i;

    for (i = _argc - 2; i >= 0; i -= 2)
	if (strcmp(_argv[i], option_name) == 0)
	    return (float)atof(_argv[i + 1]);
    return default_value;
}

