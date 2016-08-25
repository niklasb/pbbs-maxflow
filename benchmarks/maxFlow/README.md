# About

Push-relabel implementations used in the evaluation of [1].

# Code organization

* `baderPar`: attempt at an implementation of [2]. However it is incomplete and
  naive, so cannot be used for comparison
* `boHongPar` attempt at an implementation of [3]. It is rather optimized, but
  we cannot really know how it compares to their (non-public) implementation.
* `hiprSeqGoldberg`/`pseudoSeq`: Code for HI-PR and HPF, integrated into our
  framework
* `goldbergPar` Simple synchronous parallel implementation (see [1], section 3)
  our paper)
* `syncPar` Improved version with multiple relabels per iteration
(see [1], section 3.1)

# How to create an example graph

    $ cd benchmarks/maxFlow/graphData/data
    $ make randLocalGraph_F_5_1000000

Or use one of the traditional flow graph generators, see Makefile. However,
keep in mind that we experimented with much larger, and non-synthetical graphs,
so for small graphs you might see much different results. To convert a DIMACS
flow benchmark to the packed PBBS flow graph format, you can use the
dimacsToFlowGraph tool:

    $ cd benchmarks/maxFlow/graphData
    $ make dimacsToFlowGraph
    $ head ~/graphs/rlg_5_1000.dimacs
    c DIMACS flow network description
    c (problem-id, nodes, arcs)
    p max 1002 10754
    c source
    n 1001 s
    c sink
    n 1002 t
    c arc description (from, to, capacity)
    a 1 137 8
    a 1 201 10

    $ ./dimacsToFlowGraph ~/graphs/rlg_5_1000.dimacs data/rlg_F_5_1000

# How to build and run

    $ cd benchmarks/maxFlow/goldbergPar
    $ make
    $ OMP_NUM_THREADS=X ./maxFlow ../graphData/data/randLocalGraph_F_5_1000000

Same for syncPar. Keep in mind that configuration is done at the top of the
corresponding maxFlow.C main C++ files. For example, use `#define STATS 1`
to get refined statistics at the cost of run time.

Please note that we only compute a maximum preflow, not a maximum flow, and
that we do not currently store the flow assignment at the end of the program.

`pseudoSeq` and `hiprSeqGoldberg` are also modified so that they do not perform
the second step of flow reconstruction, after finding a maximum preflow.

---

[1] "Efficient Implementation of a Synchronous Parallel Push-Relabel Algorithm", ESA 2015
[2] "A Cache-Aware Parallel Implementation of the Push-Relabel Network
Flow Algorithm and Experimental Evaluation of the Gap Relabeling
Heuristic", PDCS 2005
[3] "An Asynchronous Multithreaded Algorithm for the Maximum Network Flow
Problem with Nonblocking Global Relabeling Heuristic", IEEE TPDS 2011
