#include <atomic>
#include <mutex>
#include <condition_variable>

template <typename T>
struct SharedPrioQueue {
  std::mutex mx;
  std::condition_variable cv;
  std::priority_queue<T> q;
  int waiting = 0;
  bool finished = 0;
  std::function<bool(int)> should_stop;
  void reset() {
    waiting = 0;
    while (!q.empty()) q.pop();
    finished = 0;
  }
  void restart() {
    std::unique_lock<mutex> lock(mx);
    finished = 0;
    waiting = 0;
  }
  void stop() {
    std::unique_lock<mutex> lock(mx);
    finished = 1;
    cv.notify_all();
  }
  bool dequeue(T& t, int i = 0) {
    std::unique_lock<mutex> lock(mx);
    //cout << "(1) dequeue " << i << endl;
    ++waiting;
    if (q.empty() && !finished) {
      if (should_stop(waiting)) {
        finished = 1;
        cv.notify_all();
      }
      while (q.empty() && !finished) cv.wait(lock);
    }
    --waiting;
    //cout << "(2) dequeue " << i << endl;
    if (finished) return 0;
    t = q.top();
    q.pop();
    return 1;
  }
  void enqueue(T t, int i = 0) {
    std::unique_lock<mutex> lock(mx);
    q.push(t);
    cv.notify_one();
    //cout << "enqueue " << i << endl;
  }
  bool empty() {
    std::unique_lock<mutex> lock(mx);
    return q.empty();
  }
};
