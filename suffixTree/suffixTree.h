// This code is part of the Problem Based Benchmark Suite (PBBS)
// Copyright (c) 2011 Guy Blelloch, Julian Shun and the PBBS team
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal in the Software without restriction, including
// without limitation the rights (to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// The above copyright notice and this permission notice shall be included
// in all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
// NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
// LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
// OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
// WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#ifndef __SUFFIX_UTILS__
#define __SUFFIX_UTILS__
#include "hash.h"

typedef pair<uintT,uintT> ipair;

struct node { uintT firstChar; uintT parent; uintT value; };

struct suffixTree {
  typedef uintT nodeID;
  typedef uintT charType;
  typedef pair<nodeID,charType> hashKey;

  long n; // number of leaves (length of string)
  long m; // total number of nodes (leaves and internal)
  charType* S;
  node* Nodes;

  struct nodeHash {
    typedef nodeID eType;
    typedef hashKey kType;
    long n;
    charType* S;
    node* Nodes;
    eType empty() {return 0;}

    nodeHash(long _n, charType* _S, node *_Nodes) : 
      n(_n), S(_S), Nodes(_Nodes) {}

    inline nodeID parent(nodeID node) { return Nodes[node].parent;}
    inline uintT value(nodeID node) { return Nodes[node].value;}
    inline uintT start(nodeID node) {
      if (node & 1 == 1) return n - value(node)+1;
      else return n - value(node-1)+1;
    }
    inline uintT length(nodeID node) {return value(node)-value(parent(node));}
    inline uintT offset(nodeID node) {return start(node) + value(parent(node)); }
    //inline charType firstChar(nodeID node) { return S[offset(node)]; }
    inline charType firstChar(nodeID node) { return Nodes[node].firstChar; }

    kType getKey(eType v) { return kType(parent(v),firstChar(v));}

    uintT hash(kType key) {
      uintT seed = utils::hash(key.first);
      seed ^= utils::hash(key.second) + 0x9e3779b9 + (seed << 6) + (seed >> 2);
      return seed;
    }

    intT cmp(kType a, kType b) {
      if (a.first > b.first) return 1;
      else if (a.first < b.first) return -1;
      else return ((a.second > b.second) ? 1 : 
		   ((a.second == b.second) ? 0 : -1));
    }
    
    bool replaceQ(eType a, eType b) { return false;}
  };

  typedef Table<nodeHash> nodeHashTable;

  nodeHashTable *table;

  struct toInt { uintT operator() (bool a) {return (uintT) a;}};

  #define NEAR 16

  /* inline bool absDiff(uintT a, uintT b) {  */
  /*   if (a < b) return b-a; else return a-b;} */

  void addToHashTable() {
    bool* FL = newA(bool,m);
    FL[0] = 0;
    cilk_for (long i=1; i < m; i++) {
      Nodes[i].firstChar = S[offset(i)]; 
      nodeID p = Nodes[i].parent;
      FL[i] = (Nodes[p].value < Nodes[i].value && 
	       (i > p + NEAR || p > i + NEAR));
    }
    //for(int i=0;i<m;i++)cout<<(intT)FL[i]<<" ";cout<<endl;
    long l = sequence::mapReduce<uintT>(FL,m,utils::addF<uintT>(),toInt());
    cout << "l=" << l << " m="<<m<<endl;
    table = new nodeHashTable(2*l, nodeHash(n,S,Nodes));
    cilk_for (long i=1; i < m; i++) {
      if (FL[i]) table->insert(i);
    }
    free(FL);
  }

  suffixTree(long _n, long _m, node* _nodes, charType* _s) :
  n(_n), m(_m), Nodes(_nodes), S(_s)
  {
    addToHashTable();
  }

  suffixTree() {};

  void del() {
    table->del();
    free(Nodes);
  }

  inline nodeID parent(nodeID node) { return Nodes[node].parent;}
  inline uintT value(nodeID node) { return Nodes[node].value;}
  inline uintT start(nodeID node) {
    if (node & 1 == 1) return n - value(node)+1;
    else return n - value(node-1)+1;
  }
  inline uintT length(nodeID node) {return value(node)-value(parent(node));}
  inline uintT offset(nodeID node) {return start(node) + value(parent(node)); }
  inline charType firstChar(nodeID node) { return Nodes[node].firstChar; }
  //inline charType firstChar(nodeID node) { return S[offset(node)]; }

  nodeID searchNear(hashKey K) {
    nodeID i = K.first;
    charType c = K.second;
    for (uintT j = 1; j <= NEAR; j++) {
      if (i > j && parent(i-j) == i && firstChar(i-j) == c) return i-j;
      if (i+j < m && parent(i+j) == i && firstChar(i+j) == c) return i+j;
    }
    return 0;
  }

  uintT search(uintT* string) {
    uintT position = 0;
    nodeID currentNode = 0;  
    if (string[0] == 0) return 0;

    while (1) {
      hashKey k = hashKey(currentNode,string[position]);
      currentNode = table->find(k); 
      
      if (currentNode == 0) {
	currentNode = searchNear(k);
	if (currentNode == 0) return UINT_T_MAX;
      } 
      uintT len = length(currentNode);
      uintT off = offset(currentNode);

      // don't need to test first position since matched in hash table
      for (long i=1; i < len; i++) {
	if (string[position+i] == 0) return off-position;
	if (string[position+i] != S[off+i]) return UINT_T_MAX;
      }

      if (string[position+len] == 0) return off-position;
      position += len;
    }
  }

};

suffixTree suffixArrayToTree (uintT* SA, uintT* LCP, long n, uintT* s);
pair<uintT*,uintT*> suffixArray(uintT* s, long n, bool findLCPs);

#endif
