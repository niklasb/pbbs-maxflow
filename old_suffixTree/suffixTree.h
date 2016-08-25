#ifndef __SUFFIX_UTILS__
#define __SUFFIX_UTILS__
#include "hash2.h"
#include "stackSpace.h"

typedef pair<intT,intT> ipair;

struct node { intT firstChar; intT parent; intT value; };

struct suffixTree {
  typedef intT nodeID;
  typedef intT charType;
  typedef pair<nodeID,charType> hashKey;

  intT n; // number of leaves (length of string)
  intT m; // total number of nodes (leaves and internal)
  charType* S;
  node* Nodes;
  ipair* Sizes;

  struct nodeHash {
    typedef nodeID eType;
    typedef hashKey kType;
    intT n;
    charType* S;
    node* Nodes;
    eType empty() {return 0;}

    nodeHash(intT _n, charType* _S, node *_Nodes) : 
      n(_n), S(_S), Nodes(_Nodes) {}

    inline nodeID parent(nodeID node) { return Nodes[node].parent;}
    inline intT value(nodeID node) { return Nodes[node].value;}
    inline intT start(nodeID node) {
      if (node & 1 == 1) return n - value(node)+1;
      else return n - value(node-1)+1;
    }
    inline intT length(nodeID node) {return value(node)-value(parent(node));}
    inline intT offset(nodeID node) {return start(node) + value(parent(node)); }
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

  struct toInt { intT operator() (bool a) {return (intT) a;}};

  #define NEAR 16

  void addToHashTable(intT* space, intT size) {
    bool* FL = newA(bool,m);
    FL[0] = 0;
    cilk_for (intT i=1; i < m; i++) {
      Nodes[i].firstChar = S[offset(i)]; 
      nodeID p = Nodes[i].parent;
      FL[i] = (Nodes[p].value < Nodes[i].value && abs(i-p) > NEAR);
    }
    intT l = sequence::mapReduce<intT>(FL,m,utils::addF<intT>(),toInt());
    cout << "l=" << l << " m="<<m<<endl;
    table = new nodeHashTable(2*l, nodeHash(n,S,Nodes), space, size);
    cilk_for (intT i=1; i < m; i++) {
      if (FL[i]) table->insert(i);
    }
    free(FL);
  }

  suffixTree(intT _n, intT _m, node* _nodes, charType* _s, 
	     intT* space, intT size, ipair* _Sizes) :
  n(_n), m(_m), Nodes(_nodes), S(_s), Sizes(_Sizes)
  {
    addToHashTable(space, size);
  }

  suffixTree() {};

  void del() {
    table->del();
    //free(Nodes);
  }

  inline nodeID parent(nodeID node) { return Nodes[node].parent;}
  inline intT value(nodeID node) { return Nodes[node].value;}
  inline intT start(nodeID node) {
    if (node & 1 == 1) return n - value(node)+1;
    else return n - value(node-1)+1;
  }
  inline intT length(nodeID node) {return value(node)-value(parent(node));}
  inline intT offset(nodeID node) {return start(node) + value(parent(node)); }
  inline charType firstChar(nodeID node) { return Nodes[node].firstChar; }
  //inline charType firstChar(nodeID node) { return S[offset(node)]; }

  nodeID searchNear(hashKey K) {
    nodeID i = K.first;
    charType c = K.second;
    for (intT j = 1; j <= NEAR; j++) {
      if (i-j > 0 && parent(i-j) == i && firstChar(i-j) == c) return i-j;
      if (i+j < m && parent(i+j) == i && firstChar(i+j) == c) return i+j;
    }
    return 0;
  }

  intT search(intT* string) {
    intT position = 0;
    nodeID currentNode = 0;  
    if (string[0] == 0) return 0;

    while (1) {
      hashKey k = hashKey(currentNode,string[position]);
      currentNode = table->find(k);
      if (currentNode == 0) {
	currentNode = searchNear(k);
	if (currentNode == 0) return -1;
      } 
      intT len = length(currentNode);
      intT off = offset(currentNode);

      // don't need to test first position since matched in hash table
      for (intT i=1; i < len; i++) {
	if (string[position+i] == 0) return off-position;
	if (string[position+i] != S[off+i]) return -1;
      }

      if (string[position+len] == 0) return off-position;
      position += len;
    }
  }

};

suffixTree suffixArrayToTree (intT* SA, intT* LCP, intT n, intT* s, stackSpace* stack);
pair<intT*,intT*> suffixArray(intT* s, intT n, bool findLCPs, stackSpace* stack);

#endif
