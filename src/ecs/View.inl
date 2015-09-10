#include "EntityManager.h"

namespace ecs{

template<typename T>
View<T>::View(EntityManager *manager, details::ComponentMask mask)  :
    manager_(manager),
    mask_(mask) { }


template<typename T>
typename View<T>::iterator View<T>::begin() {
  return iterator(manager_, mask_, true);
}

template<typename T>
typename View<T>::iterator View<T>::end() {
  return iterator(manager_, mask_, false);
}

template<typename T>
typename View<T>::const_iterator View<T>::begin() const {
  return const_iterator(manager_, mask_, true);
}

template<typename T>
typename View<T>::const_iterator View<T>::end() const {
  return const_iterator(manager_, mask_, false);
}

template<typename T>
inline index_t View<T>::count() {
  index_t count = 0;
  for (auto it = begin(); it != end(); ++it) {
    ++count;
  }
  return count;
}

template<typename T> template<typename ...Components>
View<T>&& View<T>::with() {
  mask_ |= details::component_mask<Components...>();
  return *this;
}

} // namespace ecs