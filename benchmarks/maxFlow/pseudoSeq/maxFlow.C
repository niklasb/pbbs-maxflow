#include <cstring>
#include <cassert>
#include <iostream>
#include "graph.h"
#include "gettime.h"
#include "utils.h"
#include "maxFlow.h"

using namespace std;

#define STATS
#define FIFO_BUCKET
//#define LIFO_BUCKET
//#define LOWEST_LABEL // highest label is the default

// implementation copied and adapted from
// http://riot.ieor.berkeley.edu/Applications/Pseudoflow/maxflow.html
// http://riot.ieor.berkeley.edu/Applications/Pseudoflow/pseudo-max-3.23_2.tar
struct Node;
struct Arc
{
  Node *from;
  Node *to;
  intT flow;
  intT capacity;
  intT direction;
};

struct Node
{
  intT visited;
  intT numAdjacent;
  intT number;
  intT label;
  intT excess;
  Node *parent;
  Node *childList;
  Node *nextScan;
  intT numOutOfTree;
  Arc **outOfTree;
  intT nextArc;
  Arc *arcToParent;
  Node *next;
};


struct Root
{
  Node *start;
  Node *end;
};

intT numNodes = 0;
intT numArcs = 0;
intT source = 0;
intT sink = 0;

#ifdef LOWEST_LABEL
intT lowestStrongLabel = 1;
#else
intT highestStrongLabel = 1;
#endif

Node *adjacencyList = NULL;
Root *strongRoots = NULL;
intT *labelCount = NULL;
Arc *arcList = NULL;

#ifdef STATS
intT numPushes = 0;
intT numMergers = 0;
intT numRelabels = 0;
intT numGaps = 0;
intT numArcScans = 0;
#endif

void
initializeNode (Node *nd, const uintT n)
{
  nd->label = 0;
  nd->excess = 0;
  nd->parent = NULL;
  nd->childList = NULL;
  nd->nextScan = NULL;
  nd->nextArc = 0;
  nd->numOutOfTree = 0;
  nd->arcToParent = NULL;
  nd->next = NULL;
  nd->visited = 0;
  nd->numAdjacent = 0;
  nd->number = n;
  nd->outOfTree = NULL;
}

void
initializeRoot (Root *rt)
{
  rt->start = NULL;
  rt->end = NULL;
}

void
freeRoot (Root *rt)
{
  rt->start = NULL;
  rt->end = NULL;
}

void
liftAll (Node *rootNode)
{
  Node *temp, *current=rootNode;

  current->nextScan = current->childList;

  -- labelCount[current->label];
  current->label = numNodes;

  for ( ; (current); current = current->parent)
  {
    while (current->nextScan)
    {
      temp = current->nextScan;
      current->nextScan = current->nextScan->next;
      current = temp;
      current->nextScan = current->childList;

      -- labelCount[current->label];
      current->label = numNodes;
    }
  }
}

#ifdef FIFO_BUCKET
void
addToStrongBucket (Node *newRoot, Root *rootBucket)
{
  if (rootBucket->start)
  {
    rootBucket->end->next = newRoot;
    rootBucket->end = newRoot;
    newRoot->next = NULL;
  }
  else
  {
    rootBucket->start = newRoot;
    rootBucket->end = newRoot;
    newRoot->next = NULL;
  }
}

#else

void
addToStrongBucket (Node *newRoot, Root *rootBucket)
{
  newRoot->next = rootBucket->start;
  rootBucket->start = newRoot;
}
#endif

void
createOutOfTree (Node *nd)
{
  if (nd->numAdjacent)
  {
    if ((nd->outOfTree = (Arc **) malloc (nd->numAdjacent * sizeof (Arc *))) == NULL)
    {
      printf ("%s Line %d: Out of memory\n", __FILE__, __LINE__);
      exit (1);
    }
  }
}

void
initializeArc (Arc *ac)
{
  ac->from = NULL;
  ac->to = NULL;
  ac->capacity = 0;
  ac->flow = 0;
  ac->direction = 1;
}

void
addOutOfTreeNode (Node *n, Arc *out)
{
  n->outOfTree[n->numOutOfTree] = out;
  ++ n->numOutOfTree;
}
void
simpleInitialization (void)
{
  intT i, size;
  Arc *tempArc;

  size = adjacencyList[source-1].numOutOfTree;
  for (i=0; i<size; ++i)
  {
    tempArc = adjacencyList[source-1].outOfTree[i];
    tempArc->flow = tempArc->capacity;
    tempArc->to->excess += tempArc->capacity;
  }

  size = adjacencyList[sink-1].numOutOfTree;
  for (i=0; i<size; ++i)
  {
    tempArc = adjacencyList[sink-1].outOfTree[i];
    tempArc->flow = tempArc->capacity;
    tempArc->from->excess -= tempArc->capacity;
  }

  adjacencyList[source-1].excess = 0;
  adjacencyList[sink-1].excess = 0;

  for (i=0; i<numNodes; ++i)
  {
    if (adjacencyList[i].excess > 0)
    {
        adjacencyList[i].label = 1;
      ++ labelCount[1];

      addToStrongBucket (&adjacencyList[i], &strongRoots[1]);
    }
  }

  adjacencyList[source-1].label = numNodes;
  adjacencyList[sink-1].label = 0;
  labelCount[0] = (numNodes - 2) - labelCount[1];
}

inline intT
addRelationship (Node *newParent, Node *child)
{
  child->parent = newParent;
  child->next = newParent->childList;
  newParent->childList = child;

  return 0;
}

inline void
breakRelationship (Node *oldParent, Node *child)
{
  Node *current;

  child->parent = NULL;

  if (oldParent->childList == child)
  {
    oldParent->childList = child->next;
    child->next = NULL;
    return;
  }

  for (current = oldParent->childList; (current->next != child); current = current->next);

  current->next = child->next;
  child->next = NULL;
}

void
merge (Node *parent, Node *child, Arc *newArc)
{
  Arc *oldArc;
  Node *current = child, *oldParent, *newParent = parent;

#ifdef STATS
  ++ numMergers;
#endif

  while (current->parent)
  {
    oldArc = current->arcToParent;
    current->arcToParent = newArc;
    oldParent = current->parent;
    breakRelationship (oldParent, current);
    addRelationship (newParent, current);
    newParent = current;
    current = oldParent;
    newArc = oldArc;
    newArc->direction = 1 - newArc->direction;
  }

  current->arcToParent = newArc;
  addRelationship (newParent, current);
}


inline void
pushUpward (Arc *currentArc, Node *child, Node *parent, const intT resCap)
{
#ifdef STATS
  ++ numPushes;
#endif

  if (resCap >= child->excess)
  {
    parent->excess += child->excess;
    currentArc->flow += child->excess;
    child->excess = 0;
    return;
  }

  currentArc->direction = 0;
  parent->excess += resCap;
  child->excess -= resCap;
  currentArc->flow = currentArc->capacity;
  parent->outOfTree[parent->numOutOfTree] = currentArc;
  ++ parent->numOutOfTree;
  breakRelationship (parent, child);

#ifdef LOWEST_LABEL
  lowestStrongLabel = child->label;
#endif

  addToStrongBucket (child, &strongRoots[child->label]);
}


inline void
pushDownward (Arc *currentArc, Node *child, Node *parent, intT flow)
{
#ifdef STATS
  ++ numPushes;
#endif

  if (flow >= child->excess)
  {
    parent->excess += child->excess;
    currentArc->flow -= child->excess;
    child->excess = 0;
    return;
  }

  currentArc->direction = 1;
  child->excess -= flow;
  parent->excess += flow;
  currentArc->flow = 0;
  parent->outOfTree[parent->numOutOfTree] = currentArc;
  ++ parent->numOutOfTree;
  breakRelationship (parent, child);

#ifdef LOWEST_LABEL
  lowestStrongLabel = child->label;
#endif

  addToStrongBucket (child, &strongRoots[child->label]);
}

void
pushExcess (Node *strongRoot)
{
  Node *current, *parent;
  Arc *arcToParent;
  intT prevEx=1;

  for (current = strongRoot; (current->excess && current->parent); current = parent)
  {
    parent = current->parent;
    prevEx = parent->excess;

    arcToParent = current->arcToParent;

    if (arcToParent->direction)
    {
      pushUpward (arcToParent, current, parent, (arcToParent->capacity - arcToParent->flow));
    }
    else
    {
      pushDownward (arcToParent, current, parent, arcToParent->flow);
    }
  }

  if ((current->excess > 0) && (prevEx <= 0))
  {

#ifdef LOWEST_LABEL
    lowestStrongLabel = current->label;
#endif
      addToStrongBucket (current, &strongRoots[current->label]);
  }
}


Arc *
findWeakNode (Node *strongNode, Node **weakNode)
{
  intT i, size;
  Arc *out;

  size = strongNode->numOutOfTree;

  for (i=strongNode->nextArc; i<size; ++i)
  {

#ifdef STATS
    ++ numArcScans;
#endif

#ifdef LOWEST_LABEL
    if (strongNode->outOfTree[i]->to->label == (lowestStrongLabel-1))
#else
    if (strongNode->outOfTree[i]->to->label == (highestStrongLabel-1))
#endif
    {
      strongNode->nextArc = i;
      out = strongNode->outOfTree[i];
      (*weakNode) = out->to;
      -- strongNode->numOutOfTree;
      strongNode->outOfTree[i] = strongNode->outOfTree[strongNode->numOutOfTree];
      return (out);
    }
#ifdef LOWEST_LABEL
    else if (strongNode->outOfTree[i]->from->label == (lowestStrongLabel-1))
#else
    else if (strongNode->outOfTree[i]->from->label == (highestStrongLabel-1))
#endif
    {
      strongNode->nextArc = i;
      out = strongNode->outOfTree[i];
      (*weakNode) = out->from;
      -- strongNode->numOutOfTree;
      strongNode->outOfTree[i] = strongNode->outOfTree[strongNode->numOutOfTree];
      return (out);
    }
  }

  strongNode->nextArc = strongNode->numOutOfTree;

  return NULL;
}


void
checkChildren (Node *curNode)
{
  for ( ; (curNode->nextScan); curNode->nextScan = curNode->nextScan->next)
  {
    if (curNode->nextScan->label == curNode->label)
    {
      return;
    }

  }

  -- labelCount[curNode->label];
  ++  curNode->label;
  ++ labelCount[curNode->label];

#ifdef STATS
  ++ numRelabels;
#endif

  curNode->nextArc = 0;
}

void
processRoot (Node *strongRoot)
{
  Node *temp, *strongNode = strongRoot, *weakNode;
  Arc *out;

  strongRoot->nextScan = strongRoot->childList;

  if ((out = findWeakNode (strongRoot, &weakNode)))
  {
    merge (weakNode, strongNode, out);
    pushExcess (strongRoot);
    return;
  }

  checkChildren (strongRoot);

  while (strongNode)
  {
    while (strongNode->nextScan)
    {
      temp = strongNode->nextScan;
      strongNode->nextScan = strongNode->nextScan->next;
      strongNode = temp;
      strongNode->nextScan = strongNode->childList;

      if ((out = findWeakNode (strongNode, &weakNode)))
      {
        merge (weakNode, strongNode, out);
        pushExcess (strongRoot);
        return;
      }

      checkChildren (strongNode);
    }

    if ((strongNode = strongNode->parent))
    {
      checkChildren (strongNode);
    }
  }

  addToStrongBucket (strongRoot, &strongRoots[strongRoot->label]);

#ifndef LOWEST_LABEL
  ++ highestStrongLabel;
#endif
}

#ifdef LOWEST_LABEL
Node *
getLowestStrongRoot (void)
{
  intT i;
  Node *strongRoot;

  if (lowestStrongLabel == 0)
  {
    while (strongRoots[0].start)
    {
      strongRoot = strongRoots[0].start;
      strongRoots[0].start = strongRoot->next;
      strongRoot->next = NULL;

      strongRoot->label = 1;

#ifdef STATS
      ++ numRelabels;
#endif

      -- labelCount[0];
      ++ labelCount[1];

      addToStrongBucket (strongRoot, &strongRoots[strongRoot->label]);
    }
    lowestStrongLabel = 1;
  }

  for (i=lowestStrongLabel; i<numNodes; ++i)
  {
    if (strongRoots[i].start)
    {
      lowestStrongLabel = i;

      if (labelCount[i-1] == 0)
      {
#ifdef STATS
        ++ numGaps;
#endif
        return NULL;
      }

      strongRoot = strongRoots[i].start;
      strongRoots[i].start = strongRoot->next;
      strongRoot->next = NULL;
      return strongRoot;
    }
  }

  lowestStrongLabel = numNodes;
  return NULL;
}

#else

Node *
getHighestStrongRoot (void)
{
  intT i;
  Node *strongRoot;

  for (i=highestStrongLabel; i>0; --i)
  {
    if (strongRoots[i].start)
    {
      highestStrongLabel = i;
      if (labelCount[i-1])
      {
        strongRoot = strongRoots[i].start;
        strongRoots[i].start = strongRoot->next;
        strongRoot->next = NULL;
        return strongRoot;
      }

      while (strongRoots[i].start)
      {

#ifdef STATS
        ++ numGaps;
#endif
        strongRoot = strongRoots[i].start;
        strongRoots[i].start = strongRoot->next;
        liftAll (strongRoot);
      }
    }
  }

  if (!strongRoots[0].start)
  {
    return NULL;
  }

  while (strongRoots[0].start)
  {
    strongRoot = strongRoots[0].start;
    strongRoots[0].start = strongRoot->next;
    strongRoot->label = 1;
    -- labelCount[0];
    ++ labelCount[1];

#ifdef STATS
    ++ numRelabels;
#endif

    addToStrongBucket (strongRoot, &strongRoots[strongRoot->label]);
  }

  highestStrongLabel = 1;

  strongRoot = strongRoots[1].start;
  strongRoots[1].start = strongRoot->next;
  strongRoot->next = NULL;

  return strongRoot;
}

#endif

void
pseudoflowPhase1 (void)
{
  Node *strongRoot;

#ifdef LOWEST_LABEL
  while ((strongRoot = getLowestStrongRoot ()))
#else
  while ((strongRoot = getHighestStrongRoot ()))
#endif
  {
    processRoot (strongRoot);
  }
}

void
quickSort (Arc **arr, const intT first, const intT last)
{
  intT i, j, left=first, right=last, x1, x2, x3, mid, pivot, pivotval;
  Arc *swap;

  if ((right-left) <= 5)
  {// Bubble sort if 5 elements or less
    for (i=right; (i>left); --i)
    {
      swap = NULL;
      for (j=left; j<i; ++j)
      {
        if (arr[j]->flow < arr[j+1]->flow)
        {
          swap = arr[j];
          arr[j] = arr[j+1];
          arr[j+1] = swap;
        }
      }

      if (!swap)
      {
        return;
      }
    }

    return;
  }

  mid = (first+last)/2;

  x1 = arr[first]->flow;
  x2 = arr[mid]->flow;
  x3 = arr[last]->flow;

  pivot = mid;

  if (x1 <= x2)
  {
    if (x2 > x3)
    {
      pivot = left;

      if (x1 <= x3)
      {
        pivot = right;
      }
    }
  }
  else
  {
    if (x2 <= x3)
    {
      pivot = right;

      if (x1 <= x3)
      {
        pivot = left;
      }
    }
  }

  pivotval = arr[pivot]->flow;

  swap = arr[first];
  arr[first] = arr[pivot];
  arr[pivot] = swap;

  left = (first+1);

  while (left < right)
  {
    if (arr[left]->flow < pivotval)
    {
      swap = arr[left];
      arr[left] = arr[right];
      arr[right] = swap;
      -- right;
    }
    else
    {
      ++ left;
    }
  }

  swap = arr[first];
  arr[first] = arr[left];
  arr[left] = swap;

  if (first < (left-1))
  {
    quickSort (arr, first, (left-1));
  }

  if ((left+1) < last)
  {
    quickSort (arr, (left+1), last);
  }
}

void
sort (Node * current)
{
  if (current->numOutOfTree > 1)
  {
    quickSort (current->outOfTree, 0, (current->numOutOfTree-1));
  }
}

void
minisort (Node *current)
{
  Arc *temp = current->outOfTree[current->nextArc];
  intT i, size = current->numOutOfTree, tempflow = temp->flow;

  for(i=current->nextArc+1; ((i<size) && (tempflow < current->outOfTree[i]->flow)); ++i)
  {
    current->outOfTree[i-1] = current->outOfTree[i];
  }
  current->outOfTree[i-1] = temp;
}

void
decompose (Node *excessNode, const intT source, intT *iteration)
{
  Node *current = excessNode;
  Arc *tempArc;
  intT bottleneck = excessNode->excess;

  for ( ;(current->number != source) && (current->visited < (*iteration));
        current = tempArc->from)
  {
    current->visited = (*iteration);
    tempArc = current->outOfTree[current->nextArc];

    if (tempArc->flow < bottleneck)
    {
      bottleneck = tempArc->flow;
    }
  }

  if (current->number == source)
  {
    excessNode->excess -= bottleneck;
    current = excessNode;

    while (current->number != source)
    {
      tempArc = current->outOfTree[current->nextArc];
      tempArc->flow -= bottleneck;

      if (tempArc->flow)
      {
        minisort(current);
      }
      else
      {
        ++ current->nextArc;
      }
      current = tempArc->from;
    }
    return;
  }

  ++ (*iteration);

  bottleneck = current->outOfTree[current->nextArc]->flow;

  while (current->visited < (*iteration))
  {
    current->visited = (*iteration);
    tempArc = current->outOfTree[current->nextArc];

    if (tempArc->flow < bottleneck)
    {
      bottleneck = tempArc->flow;
    }
    current = tempArc->from;
  }

  ++ (*iteration);

  while (current->visited < (*iteration))
  {
    current->visited = (*iteration);

    tempArc = current->outOfTree[current->nextArc];
    tempArc->flow -= bottleneck;

    if (tempArc->flow)
    {
      minisort(current);
      current = tempArc->from;
    }
    else
    {
      ++ current->nextArc;
      current = tempArc->from;
    }
  }
}

void
recoverFlow (const intT gap)
{
  intT i, j, iteration = 1;
  Arc *tempArc;
  Node *tempNode;

  for (i=0; i<adjacencyList[sink-1].numOutOfTree; ++i)
  {
    tempArc = adjacencyList[sink-1].outOfTree[i];
    if (tempArc->from->excess < 0)
    {
      if ((tempArc->from->excess + (intT) tempArc->flow)  < 0)
      {
        tempArc->from->excess += (intT) tempArc->flow;
        tempArc->flow = 0;
      }
      else
      {
        tempArc->flow = (intT) (tempArc->from->excess + (intT) tempArc->flow);
        tempArc->from->excess = 0;
      }
    }
  }

  for (i=0; i<adjacencyList[source-1].numOutOfTree; ++i)
  {
    tempArc = adjacencyList[source-1].outOfTree[i];
    addOutOfTreeNode (tempArc->to, tempArc);
  }

  adjacencyList[source-1].excess = 0;
  adjacencyList[sink-1].excess = 0;

  for (i=0; i<numNodes; ++i)
  {
    tempNode = &adjacencyList[i];

    if ((i == (source-1)) || (i == (sink-1)))
    {
      continue;
    }

    if (tempNode->label >= gap)
    {
      tempNode->nextArc = 0;
      if ((tempNode->parent) && (tempNode->arcToParent->flow))
      {
        addOutOfTreeNode (tempNode->arcToParent->to, tempNode->arcToParent);
      }

      for (j=0; j<tempNode->numOutOfTree; ++j)
      {
        if (!tempNode->outOfTree[j]->flow)
        {
          -- tempNode->numOutOfTree;
          tempNode->outOfTree[j] = tempNode->outOfTree[tempNode->numOutOfTree];
          -- j;
        }
      }

      sort(tempNode);
    }
  }

  for (i=0; i<numNodes; ++i)
  {
    tempNode = &adjacencyList[i];
    while (tempNode->excess > 0)
    {
      ++ iteration;
      decompose(tempNode, source, &iteration);
    }
  }
}


void
freeMemory (void)
{
  intT i;

  for (i=0; i<numNodes; ++i)
  {
    freeRoot (&strongRoots[i]);
  }

  free (strongRoots);

  for (i=0; i<numNodes; ++i)
  {
    if (adjacencyList[i].outOfTree)
    {
      free (adjacencyList[i].outOfTree);
    }
  }

  free (adjacencyList);

  free (labelCount);

  free (arcList);
}

void
initDS (FlowGraph<intT>& g)
{
  intT i, capacity, from, to, first=0, last=0;

  numNodes = g.g.n;
  numArcs = g.g.m;
  adjacencyList = (Node *) malloc (numNodes * sizeof (Node));
  strongRoots = (Root *) malloc (numNodes * sizeof (Root));
  labelCount = (intT *) malloc (numNodes * sizeof (intT));
  arcList = (Arc *) malloc (numArcs * sizeof (Arc));

  for (i=0; i<numNodes; ++i)
  {
    initializeRoot (&strongRoots[i]);
    initializeNode (&adjacencyList[i], (i+1));
    labelCount[i] = 0;
  }

  for (i=0; i<numArcs; ++i)
  {
    initializeArc (&arcList[i]);
  }

  first = 0;
  last = numArcs-1;

  for (intT from = 0; from < numNodes; ++from) {
    FlowVertex& v = g.g.V[from];
    for (intT j = 0; j < v.degree; ++j) {
      intT to = v.Neighbors[j];
      intT c = v.nghWeights[j];

      if ((from+to) % 2)
      {
        arcList[first].from = &adjacencyList[from];
        arcList[first].to = &adjacencyList[to];
        arcList[first].capacity = c;
        v.nghWeights[j] = first;
        ++ first;
      }
      else
      {
        arcList[last].from = &adjacencyList[from];
        arcList[last].to = &adjacencyList[to];
        arcList[last].capacity = c;
        v.nghWeights[j] = last;
        -- last;
      }

      ++ adjacencyList[from].numAdjacent;
      ++ adjacencyList[to].numAdjacent;
    }
  }

  source = g.source + 1;
  sink = g.sink + 1;

  for (i=0; i<numNodes; ++i)
  {
    createOutOfTree (&adjacencyList[i]);
  }

  for (i=0; i<numArcs; i++)
  {
    to = arcList[i].to->number;
    from = arcList[i].from->number;
    capacity = arcList[i].capacity;

    if (!((source == to) || (sink == from) || (from == to)))
    {
      if ((source == from) && (to == sink))
      {
        arcList[i].flow = capacity;
      }
      else if (from == source)
      {
        addOutOfTreeNode (&adjacencyList[from-1], &arcList[i]);
      }
      else if (to == sink)
      {
        addOutOfTreeNode (&adjacencyList[to-1], &arcList[i]);
      }
      else
      {
        addOutOfTreeNode (&adjacencyList[from-1], &arcList[i]);
      }
    }
  }
}

intT maxFlow(FlowGraph<intT>& g) {
  initDS(g);
  beforeHook();
  simpleInitialization();
  pseudoflowPhase1 ();
  intT gap;
#ifdef LOWEST_LABEL
  gap = lowestStrongLabel;
#else
  gap = numNodes;
#endif
  afterHook();
  recoverFlow(gap);

  intT flow = 0;
  for (intT i = 0; i < numNodes; ++i) {
    FlowVertex& v = g.g.V[i];
    for (intT j = 0; j < v.degree; ++j) {
      v.nghWeights[j] = arcList[v.nghWeights[j]].flow;
      if (i == g.source) flow += v.nghWeights[j];
    }
  }

  freeMemory();
  cout << "flow=" << flow << endl;
  return flow;
}
