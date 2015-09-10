namespace ecs{


template<typename T, typename E>
inline bool operator==(Property<T> const &lhs, const E &rhs) {
  return lhs.value == rhs;
}

template<typename T, typename E>
inline bool operator!=(Property<T> const &lhs, const E &rhs) {
  return lhs.value != rhs;
}

template<typename T, typename E>
inline bool operator>=(Property<T> const &lhs, const E &rhs) {
  return lhs.value >= rhs;
}

template<typename T, typename E>
inline bool operator>(Property<T> const &lhs, const E &rhs) {
  return lhs.value > rhs;
}

template<typename T, typename E>
inline bool operator<=(Property<T> const &lhs, const E &rhs) {
  return lhs.value <= rhs;
}

template<typename T, typename E>
inline bool operator<(Property<T> const &lhs, const E &rhs) {
  return lhs.value < rhs;
}

template<typename T, typename E>
inline T &operator+=(Property<T> &lhs, const E &rhs) {
  return lhs.value += rhs;
}

template<typename T, typename E>
inline T &operator-=(Property<T> &lhs, const E &rhs) {
  return lhs.value -= rhs;
}

template<typename T, typename E>
inline T &operator*=(Property<T> &lhs, const E &rhs) {
  return lhs.value *= rhs;
}

template<typename T, typename E>
inline T &operator/=(Property<T> &lhs, const E &rhs) {
  return lhs.value /= rhs;
}

template<typename T, typename E>
inline T &operator%=(Property<T> &lhs, const E &rhs) {
  return lhs.value %= rhs;
}

template<typename T, typename E>
inline T &operator&=(Property<T> &lhs, const E &rhs) {
  return lhs.value &= rhs;
}

template<typename T, typename E>
inline T &operator|=(Property<T> &lhs, const E &rhs) {
  return lhs.value |= rhs;
}

template<typename T, typename E>
inline T &operator^=(Property<T> &lhs, const E &rhs) {
  return lhs.value ^= rhs;
}

template<typename T, typename E>
inline T operator+(Property<T> &lhs, const E &rhs) {
  return lhs.value + rhs;
}

template<typename T, typename E>
inline T operator-(Property<T> &lhs, const E &rhs) {
  return lhs.value - rhs;
}

template<typename T, typename E>
inline T operator*(Property<T> &lhs, const E &rhs) {
  return lhs.value * rhs;
}

template<typename T, typename E>
inline T operator/(Property<T> &lhs, const E &rhs) {
  return lhs.value / rhs;
}

template<typename T, typename E>
inline T operator%(Property<T> &lhs, const E &rhs) {
  return lhs.value % rhs;
}

template<typename T>
inline T &operator++(Property<T> &lhs) {
  ++lhs.value;
  return lhs.value;
}

template<typename T>
inline T operator++(Property<T> &lhs, int) {
  T copy = lhs;
  ++lhs;
  return copy;
}

template<typename T>
inline T& operator--(Property<T> &lhs) {
  --lhs.value;
  return lhs.value;
}

template<typename T>
inline T operator--(Property<T> &lhs, int) {
  T copy = lhs;
  --lhs;
  return copy;
}

template<typename T, typename E>
inline T operator&(Property<T> &lhs, const E &rhs) {
  return lhs.value & rhs;
}

template<typename T, typename E>
inline T operator|(Property<T> &lhs, const E &rhs) {
  return lhs.value | rhs;
}

template<typename T, typename E>
inline T operator^(Property<T> &lhs, const E &rhs) {
  return lhs.value ^ rhs;
}

template<typename T, typename E>
inline T operator~(Property<T> &lhs) {
  return ~lhs.value;
}

template<typename T, typename E>
inline T operator>>(Property<T> &lhs, const E &rhs) {
  return lhs.value >> rhs;
}

template<typename T, typename E>
inline T operator<<(Property<T> &lhs, const E &rhs) {
  return lhs.value << rhs;
}

} // namespace ecs

///---------------------------------------------------------------------
/// This will allow a property to be streamed into a input and output
/// stream.
///---------------------------------------------------------------------
namespace std {

template<typename T>
ostream &operator<<(ostream &os, const ecs::Property<T> &obj) {
  return os << obj.value;
}

template<typename T>
istream &operator>>(istream &is, ecs::Property<T> &obj) {
  return is >> obj.value;
}

} //namespace std

///---------------------------------------------------------------------
/// This will enable properties to be added to a string
///---------------------------------------------------------------------
template<typename T>
std::string operator+(const std::string &lhs, const ecs::Property<T> &rhs) { return lhs + rhs.value; }

template<typename T>
std::string operator+(std::string &&lhs, ecs::Property<T> &&rhs) { return lhs + rhs.value; }

template<typename T>
std::string operator+(std::string &&lhs, const ecs::Property<T> &rhs) { return lhs + rhs.value; }

template<typename T>
std::string operator+(const std::string &lhs, ecs::Property<T> &&rhs) { return lhs + rhs.value; }