unset CILK_NWORKERS
rm refine.C
ln -s refineInsert.C refine.C
make clean;make
./refine ../geometryData/data/2DinCubeDelaunay_5000000
rm refine.C
ln -s refineNoInsert.C refine.C
make clean;make
./refine ../geometryData/data/2DinCubeDelaunay_5000000

export CILK_NWORKERS=1
rm refine.C
ln -s refineInsert.C refine.C
make clean;make
./refine ../geometryData/data/2DinCubeDelaunay_5000000
rm refine.C
ln -s refineNoInsert.C refine.C
make clean;make
./refine ../geometryData/data/2DinCubeDelaunay_5000000

unset CILK_NWORKERS
rm refine.C
ln -s refineInsert.C refine.C
make clean;make
./refine ../geometryData/data/2DkuzminDelaunay_5000000
rm refine.C
ln -s refineNoInsert.C refine.C
make clean;make
./refine ../geometryData/data/2DkuzminDelaunay_5000000

export CILK_NWORKERS=1
rm refine.C
ln -s refineInsert.C refine.C
make clean;make
./refine ../geometryData/data/2DkuzminDelaunay_5000000
rm refine.C
ln -s refineNoInsert.C refine.C
make clean;make
./refine ../geometryData/data/2DkuzminDelaunay_5000000
