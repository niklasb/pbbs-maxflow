void merge(node* N, intT left, intT right) {
  intT head;
  if (N[left].value > N[right].value) {
    head = left; left = N[left].parent;}
  else {head = right; right= N[right].parent;}
  
  while(1) {
    if (left == 0) {N[head].parent = right; break;}
    if (right == 0) {N[head].parent = left; break;}
    if (N[left].value > N[right].value) {
      N[head].parent = left; left = N[left].parent;}
    else {N[head].parent = right; right = N[right].parent;}
    head = N[head].parent;}}

void cartesianTree(node* Nodes, intT s, intT n) { 
  if (n < 2) return;
  if(n == 2) {
    if (Nodes[s].value > Nodes[s+1].value) Nodes[s].parent=s+1;
    else Nodes[s+1].parent=s;
    return;
  }
  if (n > 1000){
    cilk_spawn cartesianTree(Nodes,s,n/2);
    cartesianTree(Nodes,s+n/2,n-n/2);
    cilk_sync;
  } else {
    cartesianTree(Nodes,s,n/2);
    cartesianTree(Nodes,s+n/2,n-n/2);
  }
  merge(Nodes,s+n/2-1,s+n/2);}