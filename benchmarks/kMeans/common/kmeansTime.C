#include <iostream>
#include <fstream>
#include <ctime>
#include "parallel.h"
#include "gettime.h"
#include "kmeansIO.h"
#include "parseCommandLine.h"
#include "kmeansdefs.h"
#include "datautils.h"

template <class F>
void kmeansTime (F data[N][D], int num, int dim, int clusters, F error, 
		char* outfile, int irounds, int kmbbrounds, F lfactor, F frac, int rseed) {
	F* score = new F; F* bfscr = new F; int* steps = new int; 
	int sd = rseed; srand (time (NULL)); ofstream out;
	euclid::distance <F> distance (dim);
	euclid::distanceSquare <F> distancesqr (dim);
	euclid::norm <F> norm (dim);
	euclid::normSquare <F> normsqr (dim);
	euclid::innerProduct <F> inprod (dim);
	sd--;

	// Simply erase file
	if (outfile != NULL) { out.open (outfile); out.close(); }

	// Run the algorithm
	for (int i = 0; i < irounds; i++) {
	 	*score = 0; *steps = 0;
		if (rseed == -1) sd = rand();
		else sd++;

		// (Re)Start timer
		startTime();
			
		kmeans (data, num, dim, clusters, error, irounds, kmbbrounds, 
			lfactor, frac, sd, distance, distancesqr, norm, normsqr, inprod, 
			bfscr, score, steps);
		
		// The total run time of the algorithm 
		nextTimeN();
	
		// Write out results to file
		if (outfile != NULL) {
			out.open (outfile, ios::app);
			out << setprecision(10) << *bfscr << " ";
			out << setprecision(10) << *score << " ";
			out << setprecision(10) << *steps << " ";	
			out << "\n";
			out.close();
		}
	}
}
	
int parallel_main (int argc, char** argv) {
	commandLine cmd (argc, argv, "[-k <clusters>] [-s <seeding style>] [-o outfile] [-i <iterations sytle>] [-r <# iterations>] [-R <kmeans|| rounds>] [-e <allowed convergence error>] [-d <random seed>] [-l <l factor>] [-f <seed file>] [-p <fraction of data>] <infile>");
	char* infile = cmd.getArgument (0);
	char* outfile = cmd.getOptionValue ("-o");
	char* seedfile = cmd.getOptionValue ("-f");
	int iteration = (int) cmd.getOptionDoubleValue ("-i",0);
	int irounds = cmd.getOptionDoubleValue ("-r",1);
	int clusters = cmd.getOptionIntValue ("-k",1);
	int seeding = (int) cmd.getOptionDoubleValue ("-s",0);
	int kmbbrounds = cmd.getOptionIntValue ("-R",5);
	flt lfactor = (flt) cmd.getOptionDoubleValue ("-l",1);
	flt error = (flt) cmd.getOptionDoubleValue ("-e",0);
	int rseed = (int) cmd.getOptionDoubleValue ("-d",-1);
	flt frac = (flt) cmd.getOptionDoubleValue ("-p",1000);

	int info[2];
	getinput (infile, data, weights, info);
	int num = info[0]; int dim = info[1];

	frac = frac / num;

	// Print data info.
	printinfo (num, dim, clusters);
	
	// Make sure the number of clusters is less than the size of the input.
	if (num < clusters) {
		cout << "Bad input: number of clusters greater than size of input!\n";
		return 1;
	}

	// Set the flag for whether or not to print the scores during the execution
	// of the algorithms.
	printscores = false;
	printinfos = false;

	// Run the tests on the algorithm.
	kmeansTime (data, num, dim, clusters, error, outfile, irounds, kmbbrounds, 
			lfactor, frac, rseed);

	return 0;
}
