#ifndef BYTECODE_H
#define BYTECODE_H

#include <iostream>
#include <fstream>
#include <stdlib.h>
#include <cmath>
#include "parallel.h"
#include "utils.h"
#include <stdio.h>
#include <string.h>

#define LAST_BIT_SET(b) (b & (0x80))
#define EDGE_SIZE_PER_BYTE 7

/* Reads the first edge of an out-edge list, which is the signed
   difference between the target and source. 
*/
inline intE eatFirstEdge(char* &start, uintE source) {
  intE edgeRead = 0;
  int bytesEaten = 0;
  int shiftAmount = 0;

  int sign = 1;

  char fb = *start;
  if ((fb & 0x40) == 0x40) {
    sign = -1;
  }
  edgeRead += (fb & 0x3f);
  if (LAST_BIT_SET(fb)) {
    shiftAmount += 6;
  }
  bytesEaten++;
  start++;

  if (LAST_BIT_SET(fb)) {
    while (1) {
      char b = *start;
      edgeRead |= ((b & 0x7f) << shiftAmount);
      start++;
      bytesEaten++;
      if (LAST_BIT_SET(b))
        shiftAmount += EDGE_SIZE_PER_BYTE;
      else 
        break;
    }
  }
  edgeRead *= sign;
  return (source + edgeRead);
}

/*
  Reads any edge of an out-edge list after the first edge. 
*/
inline uintE eatEdge(char* &start) {
  int bytesEaten = 0;
  uintE edgeRead = 0;
  int shiftAmount = 0;

  while (1) {
    char b = *start;
    edgeRead += ((b & 0x7f) << shiftAmount);
    start++;
    bytesEaten++;
    if (LAST_BIT_SET(b))
      shiftAmount += EDGE_SIZE_PER_BYTE;
    else 
      break;
  } 
  return edgeRead;
}

/*
  The main decoding work-horse. First eats the specially coded first 
  edge, and then eats the remaining |d-1| many edges that are normally
  coded. 
*/
template <class T, class F>
  inline void decode(T t, F f, char* edgeStart, const uintE &source, const uintT &degree) {
  uintE edgesRead = 0;
  if (degree > 0) {
    // Eat first edge, which is compressed specially 
    uintE startEdge = eatFirstEdge(edgeStart,source);
    if (!t.srcTarg(f, source,startEdge,edgesRead)) {
      return;
    }
    for (edgesRead = 1; edgesRead < degree; edgesRead++) {
      // Eat the next 'edge', which is a difference, and reconstruct edge.
      uintE edgeRead = eatEdge(edgeStart);
      uintE edge = startEdge + edgeRead;
      startEdge = edge;
      if (!t.srcTarg(f, source, edge, edgesRead)) {
        break; 
      }
    }
  }
}

//decode edges for weighted graph
template <class T, class F>
  inline void decodeWgh(T t, F f, char* edgeStart, const uintE &source,const uintT &degree) {
  uintT edgesRead = 0;
  if (degree > 0) {
    // Eat first edge, which is compressed specially 
    uintE startEdge = eatFirstEdge(edgeStart,source);
    intE weight = eatFirstEdge(edgeStart,0);
    if (!t.srcTarg(f, source,startEdge, weight, edgesRead)) {
      return;
    }
    for (edgesRead = 1; edgesRead < degree; edgesRead++) {
      uintE edgeRead = eatEdge(edgeStart);
      uintE edge = startEdge + edgeRead;
      startEdge = edge;
      intE weight = eatFirstEdge(edgeStart,0);
      if (!t.srcTarg(f, source, edge, weight, edgesRead)) {
        break; 
      }
    }
  }
}


/*
  Compresses the first edge, writing target-source and a sign bit. 
*/
long compressFirstEdge(char *start, long offset, uintE source, uintE target) {
  char* saveStart = start;
  long saveOffset = offset;

  intE preCompress = (intE) target - source;
  int bytesUsed = 0;
  char firstByte = 0;
  intE toCompress = abs(preCompress);
  firstByte = toCompress & 0x3f; // 0011|1111
  if (preCompress < 0) {
    firstByte |= 0x40;
  }
  toCompress = toCompress >> 6;
  if (toCompress > 0) {
    firstByte |= 0x80;
  }
  start[offset] = firstByte;
  offset++;

  char curByte = toCompress & 0x7f;
  while ((curByte > 0) || (toCompress > 0)) {
    bytesUsed++;
    char toWrite = curByte;
    toCompress = toCompress >> 7;
    // Check to see if there's any bits left to represent
    curByte = toCompress & 0x7f;
    if (toCompress > 0) {
      toWrite |= 0x80; 
    }
    start[offset] = toWrite;
    offset++;
  }
  return offset;
}

/*
  Should provide the difference between this edge and the previous edge
*/

long compressEdge(char *start, long curOffset, uintE e) {
  char curByte = e & 0x7f;
  int bytesUsed = 0;
  while ((curByte > 0) || (e > 0)) {
    bytesUsed++;
    char toWrite = curByte;
    e = e >> 7;
    // Check to see if there's any bits left to represent
    curByte = e & 0x7f;
    if (e > 0) {
      toWrite |= 0x80; 
    }
    start[curOffset] = toWrite;
    curOffset++;
  }
  return curOffset;
}

/*
  Takes: 
    1. The edge array of chars to write into
    2. The current offset into this array
    3. The vertices degree
    4. The vertices vertex number
    5. The array of saved out-edges we're compressing
  Returns:
    The new offset into the edge array
*/
long sequentialCompressEdgeSet(char *edgeArray, long currentOffset, uintT degree, 
                                uintE vertexNum, uintE *savedEdges) {
  if (degree > 0) {
    // Compress the first edge whole, which is signed difference coded
    currentOffset = compressFirstEdge(edgeArray, currentOffset, 
                                       vertexNum, savedEdges[0]);
    for (uintT edgeI=1; edgeI < degree; edgeI++) {
      // Store difference between cur and prev edge. 
      uintE difference = savedEdges[edgeI] - 
                        savedEdges[edgeI - 1];
      currentOffset = compressEdge(edgeArray, currentOffset, difference);
    }
    // Increment nWritten after all of vertex n's neighbors are written
  }
  return currentOffset;
}

/*
  Compresses the edge set in parallel. 
*/
uintE *parallelCompressEdges(uintE *edges, uintT *offsets, long n, long m, uintE* Degrees) {
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE*, n);
  uintT *degrees = newA(uintT, n+1);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n+1);
  {parallel_for(long i=0; i<n; i++) { 
      degrees[i] = Degrees[i];
    charsUsedArr[i] = ceil((degrees[i] * 9) / 8) + 4;
  }}
  degrees[n] = 0;
  sequence::plusScan(degrees,degrees, n+1);
  long toAlloc = sequence::plusScan(charsUsedArr,charsUsedArr,n);
  uintE* iEdges = newA(uintE,toAlloc);

  {parallel_for(long i=0; i<n; i++) {
      edgePts[i] = iEdges+charsUsedArr[i];
      long charsUsed = 
	sequentialCompressEdgeSet((char *)(iEdges+charsUsedArr[i]), 
				  0, degrees[i+1]-degrees[i],
				  i, edges + offsets[i]);
      charsUsedArr[i] = charsUsed;
  }}

  // produce the total space needed for all compressed lists in chars. 
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace;
  free(degrees);
  free(charsUsedArr);
  
  char *finalArr = newA(char, totalSpace);
  cout << "total space requested is : " << totalSpace << endl;
  float avgBitsPerEdge = (float)totalSpace*8 / (float)m; 
  cout << "Average bits per edge: " << avgBitsPerEdge << endl;

  {parallel_for(long i=0; i<n; i++) {
      long o = compressionStarts[i];
    memcpy(finalArr + o, (char *)(edgePts[i]), compressionStarts[i+1]-o);
    offsets[i] = o;
  }}
  offsets[n] = totalSpace;
  free(iEdges);
  free(edgePts);
  free(compressionStarts);
  cout << "finished compressing, bytes used = " << totalSpace << endl;
  cout << "would have been, " << (m * 4) << endl;
  return ((uintE *)finalArr);
}

typedef pair<uintE,intE> intEPair;

/*
  Takes: 
    1. The edge array of chars to write into
    2. The current offset into this array
    3. The vertices degree
    4. The vertices vertex number
    5. The array of saved out-edges we're compressing
  Returns:
    The new offset into the edge array
*/
long sequentialCompressWeightedEdgeSet
(char *edgeArray, long currentOffset, uintT degree, 
 uintE vertexNum, intEPair *savedEdges) {
  if (degree > 0) {
    // Compress the first edge whole, which is signed difference coded
    //target ID
    currentOffset = compressFirstEdge(edgeArray, currentOffset, 
                                       vertexNum, savedEdges[0].first);
    //weight
    currentOffset = compressFirstEdge(edgeArray, currentOffset, 
				      0,savedEdges[0].second);

    for (uintT edgeI=1; edgeI < degree; edgeI++) {
      // Store difference between cur and prev edge. 
      uintE difference = savedEdges[edgeI].first - 
                        savedEdges[edgeI - 1].first;

      //compress difference
      currentOffset = compressEdge(edgeArray, currentOffset, difference);

      //compress weight
      currentOffset = compressFirstEdge(edgeArray, currentOffset, 0, savedEdges[edgeI].second);      
    }
    // Increment nWritten after all of vertex n's neighbors are written
  }
  return currentOffset;
}

/*
  Compresses the weighted edge set in parallel. 
*/
char *parallelCompressWeightedEdges(intEPair *edges, uintT *offsets, long n, long m, uintE* Degrees) {
  cout << "parallel compressing, (n,m) = (" << n << "," << m << ")" << endl;
  uintE **edgePts = newA(uintE*, n);
  uintT *degrees = newA(uintT, n+1);
  long *charsUsedArr = newA(long, n);
  long *compressionStarts = newA(long, n+1);
  {parallel_for(long i=0; i<n; i++) { 
    degrees[i] = Degrees[i];
    charsUsedArr[i] = 2*(ceil((degrees[i] * 9) / 8) + 4); //to change
  }}
  degrees[n] = 0;
  sequence::plusScan(degrees,degrees, n+1);
  long toAlloc = sequence::plusScan(charsUsedArr,charsUsedArr,n);
  uintE* iEdges = newA(uintE,toAlloc);

  {parallel_for(long i=0; i<n; i++) {
    edgePts[i] = iEdges+charsUsedArr[i];
    long charsUsed = 
      sequentialCompressWeightedEdgeSet((char *)(iEdges+charsUsedArr[i]), 0, degrees[i+1]-degrees[i],i, edges + offsets[i]);
    charsUsedArr[i] = charsUsed;
  }}

  // produce the total space needed for all compressed lists in chars. 
  long totalSpace = sequence::plusScan(charsUsedArr, compressionStarts, n);
  compressionStarts[n] = totalSpace;
  free(degrees);
  free(charsUsedArr);

  char *finalArr = newA(char, totalSpace);
  cout << "total space requested is : " << totalSpace << endl;
  float avgBitsPerEdge = (float)totalSpace*8 / (float)m; 
  cout << "Average bits per edge: " << avgBitsPerEdge << endl;

  {parallel_for(long i=0; i<n; i++) {
      long o = compressionStarts[i];
    memcpy(finalArr + o, (char *)(edgePts[i]), compressionStarts[i+1]-o);
    offsets[i] = o;
  }}
  offsets[n] = totalSpace;
  free(iEdges);
  free(edgePts);
  free(compressionStarts);
  cout << "finished compressing, bytes used = " << totalSpace << endl;
  cout << "would have been, " << (m * 8) << endl;
  return finalArr;
}

#endif