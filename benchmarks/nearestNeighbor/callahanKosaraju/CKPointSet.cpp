/*
 *  CKPointSet.cpp
 *  CallahanKosaraju
 *
 *  Created by Aapo Kyrola on 11/9/10.
 *  Copyright 2010 Carnegie Mellon University. All rights reserved.
 *
 */

#include "CKPointSet.h"
#include "PointGenerator.h"

CKPointSet * ckPointSet = NULL;
 
int _npoints;

void prepareForPoints(size_t numOfPoints) {
    if ( ckPointSet != NULL ) delete(ckPointSet);
    ckPointSet = new CKPointSet();
    ckPointSet->reserve(numOfPoints);
    _npoints = numOfPoints;
}

void addPoint(int i, double3 p) {
    ckPointSet->addPoint(i, p[0],p[1],p[2]);
}

void computeNN(int k) {
    assert(k==1);
    ckPointSet->recompute();
    ckPointSet->compute_wsr_nn();
}

// Used for correctness check.
double3 getNearestPoint(int pointidx) {
    point p = ckPointSet->getPoints()[ckPointSet->getPoints()[pointidx].nn];
    return double3(p.coord[0], p.coord[1], p.coord[2]);
}
 
 
