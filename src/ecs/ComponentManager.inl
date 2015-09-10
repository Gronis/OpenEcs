#include "EntityManager.h"
#include "Defines.h"

namespace ecs{

namespace details{

template<typename C>
ComponentManager<C>::ComponentManager(EntityManager &manager, size_t chunk_size)  :
    manager_(manager),
    pool_(chunk_size)
{ }

// Creating a component that has a defined ctor
template<typename C, typename ...Args>
auto create_component(void* ptr, Args && ... args) ->
typename std::enable_if<std::is_constructible<C, Args...>::value, void>::type {
  new(ptr) C(std::forward<Args>(args)...);
}

// Creating a component that doesn't have ctor, and is not a property -> create using uniform initialization
template<typename C, typename ...Args>
auto create_component(void* ptr, Args && ... args) ->
typename std::enable_if<!std::is_constructible<C, Args...>::value &&
    !std::is_base_of<details::BaseProperty, C>::value, void>::type {
  new(ptr) C{std::forward<Args>(args)...};
}

// Creating a component that doesn't have ctor, and is a property -> create using underlying Property ctor
template<typename C, typename ...Args>
auto create_component(void* ptr, Args && ... args) ->
typename std::enable_if<
    !std::is_constructible<C, Args...>::value &&
        std::is_base_of<details::BaseProperty, C>::value, void>::type {
  static_assert(sizeof...(Args) <= 1, ECS_ASSERT_MSG_ONLY_ONE_ARGS_PROPERTY_CONSTRUCTOR);
  new(ptr) typename C::ValueType(std::forward<Args>(args)...);
}

template<typename C> template<typename ...Args>
C& ComponentManager<C>::create(index_t index, Args &&... args) {
  pool_.ensure_min_size(index + 1);
  create_component<C>(get_ptr(index), std::forward<Args>(args)...);
  return get(index);
}

template<typename C>
void ComponentManager<C>::remove(index_t index) {
  pool_.destroy(index);
  manager_.mask(index).reset(component_index<C>());
}

template<typename C>
C &ComponentManager<C>::operator[](index_t index){
  return get(index);
}

template<typename C>
C &ComponentManager<C>::get(index_t index) {
  return *get_ptr(index);
}

template<typename C>
C const &ComponentManager<C>::get(index_t index) const {
  return *get_ptr(index);
}

template<typename C>
C *ComponentManager<C>::get_ptr(index_t index)  {
  return pool_.get_ptr(index);
}

template<typename C>
C const *ComponentManager<C>::get_ptr(index_t index) const {
  return pool_.get_ptr(index);
}

template<typename C>
ComponentMask ComponentManager<C>::mask() {
  return component_mask<C>();
}

} // namespace details

} // namespace ecs