CILKC = cilk++
CILKFLAGS = -O3  -g -lcilkutil -lmiser -m64 -Imetis/metis/Lib/
CPP = g++


all:   scan scanomp

scan: parallel_scan.cilk shared.c
	$(CILKC) parallel_scan.cilk -o scan $(CILKFLAGS)

scanomp: parallel_scan_omp.c 
	$(CPP) -fopenmp -O3 -m64 parallel_scan_omp.c -o scanomp








