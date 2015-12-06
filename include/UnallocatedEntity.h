//
// Created by Robin Grönberg on 05/12/15.
//

#ifndef OPENECS_UNALLOCATEDENTITY_H
#define OPENECS_UNALLOCATEDENTITY_H

#include <vector>
#include "Entity.h"
#include "Defines.h"

namespace ecs{

///---------------------------------------------------------------------
/// UnallocatedEntity is used when creating an Entity but postponing
/// allocation for memory in the EntityManager
///---------------------------------------------------------------------
///
/// UnallocatedEntity is used when creating an Entity from the
/// EntityManager but does not allocate memory inside the EntityManager
/// until the UnallocatedEntity is assigned, or goes out of scope.
///
/// The purpose of doing this is to postpone the placement of the Entity
/// until components have been attached. Then, the EntityManager knows
/// what components that the entity has, and can place the entity close
/// to similar entities in memory. This leads to better usage of the
/// CPU cache
///
/// An UnallocatedEntity can always be implicitly casted to an Entity.
///
/// Au UnallocatedEntity is assumed unallocated as long as entity_ is
/// a nullptr
///---------------------------------------------------------------------
class UnallocatedEntity {
 private:
  struct ComponentHeader{
    unsigned int index, size;
  };
 public:
  inline UnallocatedEntity(EntityManager &manager);
  inline UnallocatedEntity &operator=(const UnallocatedEntity &rhs);
  inline ~UnallocatedEntity();

  /// Cast to Entity or EntityAlias
  inline operator Entity &();

  inline bool operator==(const UnallocatedEntity &rhs) const;
  inline bool operator!=(const UnallocatedEntity &rhs) const;
  inline bool operator==(const Entity &rhs) const;
  inline bool operator!=(const Entity &rhs) const;

  inline Id &id();

  /// Returns the requested component, or error if it doesn't exist
  template<typename C> inline C &get();
  template<typename C> inline C const &get() const;

  /// Set the requested component, if old component exist,
  /// a new one is created. Otherwise, the assignment operator
  /// is used.
  template<typename C, typename ... Args> inline C &set(Args &&... args);

  /// Add the requested component, error if component of the same type exist already
  template<typename C, typename ... Args> inline C &add(Args &&... args);

  /// Access this Entity as an EntityAlias.
  template<typename T> inline T &as();

  /// Assume that this entity has provided Components
  /// Use for faster component access calls
  template<typename ...Components> inline EntityAlias<Components...> &assume();

  /// Removes a component. Error of it doesn't exist
  template<typename C> inline void remove();

  /// Removes all components and call destructors
  void inline remove_everything();

  /// Clears the component mask without destroying components (faster than remove_everything)
  void inline clear_mask();

  /// Destroys this entity. Removes all components as well
  void inline destroy();

  /// Return true if entity has all specified components. False otherwise
  template<typename... Components> inline bool has() const;

  /// Returns whether an entity is an entity alias or not
  template<typename T> inline bool is() const;

  /// Returns true if entity has not been destroyed. False otherwise
  bool inline is_valid() const;

  /// Allocates memory for this Entity. Once allocated, the Unallocated Entity function like a
  /// normal Entity
  void inline allocate();

 private:

  bool inline is_allocated() const;

  EntityManager * manager_ = nullptr;
  Entity entity_;
  std::vector<char> component_data;
  std::vector<ComponentHeader> component_headers_;
  details::ComponentMask mask_ = details::ComponentMask(0);

}; //UnallocatedEntity

} // namespace ecs


#include "UnallocatedEntity.inl"

#endif //OPENECS_UNALLOCATEDENTITY_H
