#include <iostream>
#include "utils.h"

template <typename T>
inline bool CAS(T* x, T y, T z) {
  if (*x != y) return 0; // double-check
  return __sync_bool_compare_and_swap(x, y, z);
}

namespace std {
  template <typename T>
  struct atomic {
    volatile T x;
    atomic(T x) : x(x) {}
    atomic() : x() {}
    const atomic<T>& operator=(T _x) { x = _x; return *this; }
    const atomic<T>& operator=(const atomic<T>& _x) { x = _x.x; return *this; }
    void operator+=(T y) {
      __sync_fetch_and_add(&x, y);
    }
    void operator-=(T y) {
      __sync_fetch_and_sub(&x, y);
    }
    void operator++() {
      *this += 1;
    }
    void operator--() {
      *this -= 1;
    }
    T operator++(int) {
      return __sync_fetch_and_add(&x, 1);
    }
    T operator--(int) {
      return __sync_fetch_and_sub(&x, 1);
    }
    operator T() const { return x; }
    bool operator==(T y) const { return x == y; }
    bool operator!=(T y) const { return x != y; }
    bool operator<(T y) const { return x < y; }
    bool operator<=(T y) const { return x <= y; }
    bool operator>(T y) const { return x > y; }
    bool operator>=(T y) const { return x >= y; }
    T load() { return x; }
  };

  template<typename T>
  T operator+(T x, const atomic<T>& y) {
    return x + y.x;
  }
  template<typename T>
  T operator-(T x, const atomic<T>& y) {
    return x - y.x;
  }
  template<typename T>
  ostream& operator<<(ostream& out, atomic<T> x) {
    return out << x.x;
  }

  struct atomic_flag {
    int x;
    void clear() { x = 0; }
    bool test_and_set() {
      return !CAS(&x, 0, 1);
      //return __sync_lock_test_and_set(&x, 1);
      //return !__sync_bool_compare_and_swap(&x, 0, 1);
    }
  };
}
