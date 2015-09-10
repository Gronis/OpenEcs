#ifndef ECS_ITERATOR_H
#define ECS_ITERATOR_H

namespace ecs{

///---------------------------------------------------------------------
/// Iterator is an iterator for iterating through the entity manager.
/// The iterator should fulfill the c++ standards for an input iterator,
///---------------------------------------------------------------------
template<typename T>
class Iterator: public std::iterator<std::input_iterator_tag, typename std::remove_reference<T>::type> {
  using T_no_ref = typename std::remove_reference<typename std::remove_const<T>::type>::type;

 public:
  Iterator(EntityManager *manager, details::ComponentMask mask, bool begin = true);
  Iterator(const Iterator &it);
  Iterator &operator=(const Iterator &rhs) = default;

  // Get the current index (cursor)
  index_t index() const;
  Iterator &operator++();

  // The the current entity
  T       entity();
  T const entity() const;

 private:
  // find next entity withing the EntityManager which has the correct components
  void find_next();

  EntityManager         *manager_;
  details::ComponentMask mask_;
  index_t                cursor_;
  size_t                 size_;
}; //Iterator

template<typename T> bool operator==(Iterator<T> const &lhs, Iterator<T> const &rhs);
template<typename T> bool operator!=(Iterator<T> const &lhs, Iterator<T> const &rhs);
template<typename T> inline T operator*(Iterator<T> &lhs);
template<typename T> inline T const &operator*(Iterator<T> const &lhs);

} // namespace ecs

#include "Iterator.inl"

#endif //ECS_ITERATOR_H
