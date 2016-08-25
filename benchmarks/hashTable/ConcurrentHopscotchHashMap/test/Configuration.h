#ifndef __CONFIGURATION_HPP__
#define __CONFIGURATION_HPP__

//------------------------------------------------------------------------------
// File    : configuration.h
// Author  : Ms.Moran Tzafrir
// Written : 13 April 2009
// 
// Copyright (C) 2009 Moran Tzafrir.
//
// This program is free software; you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 2 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of 
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU 
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License 
// along with this program; if not, write to the Free Software Foundation
// Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
//------------------------------------------------------------------------------

#include <string>
#include "../framework/cpp_framework.h"

class Configuration {
public:
	char*	algName;
	int   test_no;
	int   no_of_threads;

	int   update_ops;
	float	load_factor;

	int   initial_count;
	int   throughputTime;
	int   is_print_result;

	bool read() {
		try {
			//read configuration from input stream
			int i_load_factor=0;
			int num_read = fscanf(stdin, "%s %d %d %d %d %d %d %d", algName, &test_no, &no_of_threads, &update_ops, &i_load_factor, &initial_count, &throughputTime, &is_print_result);
			initial_count = CMDR::Integer::nearestPowerOfTwo(initial_count);
			load_factor = i_load_factor/100.0f;
			return (14 == num_read);
		} catch (...) {
			return false;
		}
	}

	bool read(int argc, char **argv) {
		try {
			//read configuration from input stream
			int i_load_factor=0;

			int curr_arg=1;
			algName = argv[curr_arg++];
			test_no				= CMDR::Integer::parseInt(argv[curr_arg++]);
			no_of_threads		= CMDR::Integer::parseInt(argv[curr_arg++]);
			update_ops			= CMDR::Integer::parseInt(argv[curr_arg++]);
			i_load_factor		= CMDR::Integer::parseInt(argv[curr_arg++]);
			initial_count		= CMDR::Integer::nearestPowerOfTwo((unsigned int)(CMDR::Integer::parseInt(argv[curr_arg++])));
			throughputTime		= CMDR::Integer::parseInt(argv[curr_arg++]);
			is_print_result	= CMDR::Integer::parseInt(argv[curr_arg++]);
			load_factor = i_load_factor/100.0f;
			return true;
		} catch (...) {
			return false;
		}
	}

	std::string GetAlgName() {return std::string(algName);}
};

#endif

