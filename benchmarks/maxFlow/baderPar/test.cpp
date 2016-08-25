#include <bits/stdc++.h>
#include <cassert>

#include "tbb/concurrent_priority_queue.h"

using namespace std;
using namespace tbb;

size_t max_spawn = 10;
concurrent_priority_queue<int> q;

int main() {
  q.push(3);
  q.push(4);
  int x;
  q.try_pop(x); cout << x << endl;
  q.try_pop(x); cout << x << endl;
  q.try_pop(x); cout << x << endl;
}
