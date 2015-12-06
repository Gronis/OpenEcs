#ifndef ECS_COMPONENTMANAGER_H
#define ECS_COMPONENTMANAGER_H

#include "Defines.h"
namespace ecs{

// Forward declarations
class EntityManager;

namespace details{

// Forward declarations
class BaseProperty;

///-----------------------------------------------------------------------
/// global function for creating a component at a specific location
///-----------------------------------------------------------------------
template<typename C, typename ...Args>
auto create_component(void* ptr, Args && ... args) ->
typename std::enable_if<std::is_constructible<C, Args...>::value, C&>::type;

// Creating a component that doesn't have ctor, and is not a property -> create using uniform initialization
template<typename C, typename ...Args>
auto create_component(void* ptr, Args && ... args) ->
typename std::enable_if<!std::is_constructible<C, Args...>::value &&
    !std::is_base_of<details::BaseProperty, C>::value, C&>::type;

// Creating a component that doesn't have ctor, and is a property -> create using underlying Property ctor
template<typename C, typename ...Args>
auto create_component(void* ptr, Args && ... args) ->
typename std::enable_if<
    !std::is_constructible<C, Args...>::value &&
        std::is_base_of<details::BaseProperty, C>::value, C&>::type;


///---------------------------------------------------------------------
/// Helper class, all ComponentManager are a BaseManager
///---------------------------------------------------------------------
class BaseManager {
 public:
  virtual ~BaseManager() { };
  virtual void remove(index_t index) = 0;
  virtual ComponentMask mask() = 0;
  virtual void* get_void_ptr(index_t index) = 0;
  virtual void const* get_void_ptr(index_t index) const = 0;
  virtual void ensure_min_size(index_t size) = 0;
};

///---------------------------------------------------------------------
/// Helper class, This is the main class for holding many Component of
/// a specified type. It uses a memory pool to store the components
///---------------------------------------------------------------------
template<typename C>
class ComponentManager: public BaseManager, details::forbid_copies {
 public:
  ComponentManager(EntityManager &manager, size_t chunk_size = ECS_DEFAULT_CHUNK_SIZE);

  /// Allocate and create at specific index, using constructor
  template<typename ...Args>
  C& create(index_t index, Args &&... args);

  /// Remove component at specific index and call destructor
  void remove(index_t index);

  /// Access a component given a specific index
  C& operator[](index_t index);
  C& get(index_t index);
  C const& get(index_t index) const;

  /// Access a ptr to a component given a specific index
  C* get_ptr(index_t index);
  C const* get_ptr(index_t index) const;

  /// Access a raw void ptr to a component given a specific index
  void* get_void_ptr(index_t index);
  void const* get_void_ptr(index_t index) const;

  // Ensures the pool that at it has the size of at least size
  void ensure_min_size(index_t size);

  /// Get the bitmask for the component this ComponentManger handles
  ComponentMask mask();

 private:
  EntityManager &manager_;
  details::Pool<C> pool_;
}; //ComponentManager

} // namespace details

} // namespace ecs

#include "ComponentManager.inl"

#endif //ECS_COMPONENTMANAGER_H
