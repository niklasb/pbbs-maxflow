#ifndef __SUFFIX_UTILS__
#define __SUFFIX_UTILS__
#include "hash.h"

struct notNeg { bool operator () (int i) {return (i >= 0);}};

template <class strElt>
struct stNode {
  int parentID;
  strElt edgeFirstChar;
  int pointingTo;
  int locationInOriginalArray;
  int edgeLength;

  void setValues(int _parentID,strElt _edgeFirstChar,
		 int _pointingTo, int _location, int _edgeLength){
    parentID = _parentID;
    edgeFirstChar = _edgeFirstChar;
    pointingTo = _pointingTo;
    locationInOriginalArray = _location;
    edgeLength = _edgeLength;
  }
};

struct stNodeHash {
  unsigned int operator() (stNode<int>* stn) {
    unsigned int seed = utils::hash(stn->parentID);
    seed ^= utils::hash(stn->edgeFirstChar) +  
      0x9e3779b9 + (seed << 6) + (seed >> 2);
    return seed;
  }
};

struct stNodeCmp {
  int operator() (stNode<int>* a, stNode<int>* b){
    if(a->parentID > b->parentID) return 1;
    else if(a->parentID < b->parentID) return -1;
    else return (a->edgeFirstChar > b->edgeFirstChar) 
	   ? 1 : ((a->edgeFirstChar == b->edgeFirstChar) ? 0 : -1); }
};

typedef Table<stNode<int>*, stNodeCmp, stNodeHash> stNodeTable;

static stNodeTable makeStNodeTable(int m) {
  return stNodeTable(m,stNodeCmp(),stNodeHash(),(stNode<int>*) NULL);
}

struct suffixTree {
  int n; // number of leaves
  int m; // total number of nodes (leaves and internal)
  stNodeTable table;
  int* s;
  stNode<int>* nodes;
  suffixTree(int _n, int _m, stNodeTable _table, int* _s, stNode<int>* _nodes) :
    n(_n), m(_m), table(_table), s(_s), nodes(_nodes) {}
  void del() {
    table.del();
    free(s);
    free(nodes);
  }

  int search(int* string) {
    //stNode for searching
    stNode<int> currentNode;
    int position = 0;
    currentNode.parentID = 0;  
    if (string[0] == 0) return 0;

    while (1) {
      currentNode.edgeFirstChar = string[position];
      stNode<int>* b = table.find(&currentNode);
      if (b == NULL) return -1;
      else {
	int length = b->edgeLength;
	int stLocation = b->locationInOriginalArray;

        // don't need to test first position since matched in hash table
	for (int i=1; i<length; i++) {
	  if (string[position+i] == 0) return stLocation-position;
	  if (string[position+i] != s[stLocation+i]) return -1;
	}

	if (string[position+length] == 0) return stLocation-position;
	position += length;
	currentNode.parentID = b->pointingTo; 
      }
    }
  }
};

suffixTree suffixArrayToTree (int* SA, int* LCP, int n, int* s);
pair<int*,int*> suffixArray(int* s, int n, bool findLCPs);

#endif
