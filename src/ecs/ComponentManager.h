#ifndef ECS_COMPONENTMANAGER_H
#define ECS_COMPONENTMANAGER_H

namespace ecs{

// Forward declarations
class EntityManager;

namespace details{

class BaseProperty;

///---------------------------------------------------------------------
/// Helper class
///---------------------------------------------------------------------
class BaseManager {
 public:
  virtual ~BaseManager() { };
  virtual void remove(index_t index) = 0;
  virtual ComponentMask mask() = 0;
};

///---------------------------------------------------------------------
/// Helper class,  This is the main class for holding many Component of
/// a specified type.
///---------------------------------------------------------------------
template<typename C>
class ComponentManager: public BaseManager, details::forbid_copies {
 public:
  ComponentManager(EntityManager &manager, size_t chunk_size = ECS_DEFAULT_CHUNK_SIZE);

  /// Allocate and create at specific index, using constructor
  template<typename ...Args>
  C& create(index_t index, Args &&... args);
  void remove(index_t index);

  C& operator[](index_t index);

  C& get(index_t index);
  C const& get(index_t index) const;

  C* get_ptr(index_t index);
  C const* get_ptr(index_t index) const;

  ComponentMask mask();

 private:
  EntityManager &manager_;
  details::Pool<C> pool_;
}; //ComponentManager

} // namespace details

} // namespace ecs

#include "ComponentManager.inl"

#endif //ECS_COMPONENTMANAGER_H
