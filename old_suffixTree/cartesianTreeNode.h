#ifndef _CTNODE_
#define _CTNODE_

struct node_c { 
  intT value;
  intT parent; 
  //intT leftChild;//if comment leftChild out and then call delete[], then memory problems. why??
  intT rightChild;
};

#endif
