#export CILK_NWORKERS=1;
unset CILK_NWORKERS
./remDups ../sequenceData/data/randomSeq_100000000
./remDups ../sequenceData/data/randSeqPair_100000000
./remDups ../sequenceData/data/trigramSeq_100000000
./remDups ../sequenceData/data/trigramSeq_pairInt_100000000
./remDups ../sequenceData/data/exptSeq_100000000
./remDups ../sequenceData/data/exptSeq_pairInt_100000000