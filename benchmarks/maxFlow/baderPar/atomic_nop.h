#include <iostream>

namespace std {
  template <typename T>
  struct atomic {
    T x;
    atomic(T x) : x(x) {}
    atomic() : x() {}
    const atomic<T>& operator=(T _x) { x = _x; return *this; }
    const atomic<T>& operator=(const atomic<T>& _x) { x = _x.x; return *this; }
    T operator+=(T y) { return x += y; }
    T operator++() { return ++x; }
    T operator++(int) { return x++; }
    T operator-=(T y) { return x -= y; }
    T operator--() { return --x; }
    T operator--(int) { return x--; }
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
    bool x;
    void clear() { x = 0; }
    bool test_and_set() {
      bool old = x;
      x = 1;
      return old;
    }
  };
}

template <typename T>
inline bool CAS(T* x, T y, T z) {
  if (*x != y) return 0;
  *x = z;
  return 1;
}
