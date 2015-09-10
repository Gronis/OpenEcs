#ifndef ECS_PROPERTY_H
#define ECS_PROPERTY_H

namespace ecs{

namespace details{
///---------------------------------------------------------------------
/// Helper class used for compile-time checks to determine if a
/// component is a propery.
///---------------------------------------------------------------------
class BaseProperty{};

} // namespace details

///---------------------------------------------------------------------
/// A Property is a helper class for Component with only one
/// property of any type
///---------------------------------------------------------------------
///
/// A Property is a helper class for Component with only one
/// property of any type.
///
/// it implements standard constructors, type conversations and
/// operators:
///
///     ==, !=
///     >=, <=, <, >
///     +=, -=, *=, /=, %=
///     &=, |=, ^=
///     +, -, *; /, %
///     &, |, ^, ~
///     >>, <<
///     ++, --
///
/// TODO: Add more operators?
///---------------------------------------------------------------------
template<typename T>
struct Property : details::BaseProperty{
  Property() { }
  Property(const T &value) : value(value) { }

  operator const T &() const { return value; }
  operator T &() { return value; }

  T value;
  using ValueType = T;
};

/// Comparision operators
template<typename T, typename E> bool operator==(Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator!=(Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator>=(Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator> (Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator<=(Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator< (Property<T> const &lhs, const E &rhs);

/// Compound assignment operators
template<typename T, typename E> T& operator+=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator-=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator*=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator/=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator%=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator&=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator|=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator^=(Property<T> &lhs, const E &rhs);

/// Arithmetic operators
template<typename T, typename E> T  operator+ (Property<T> &lhs, const E &rhs);
template<typename T, typename E> T  operator- (Property<T> &lhs, const E &rhs);
template<typename T, typename E> T  operator* (Property<T> &lhs, const E &rhs);
template<typename T, typename E> T  operator/ (Property<T> &lhs, const E &rhs);
template<typename T, typename E> T  operator% (Property<T> &lhs, const E &rhs);
template<typename T>             T& operator++(Property<T> &lhs);
template<typename T>             T  operator++(Property<T> &lhs, int);
template<typename T>             T& operator--(Property<T> &lhs);
template<typename T>             T  operator--(Property<T> &lhs, int);

/// Bitwise operators
template<typename T, typename E> T operator&(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T operator|(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T operator^(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T operator~(Property<T> &lhs);
template<typename T, typename E> T operator>>(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T operator<<(Property<T> &lhs, const E &rhs);

} // namespace ecs

#include "Property.inl"

#endif //ECS_PROPERTY_H
