#ifndef ECS_ENTITY_H
#define ECS_ENTITY_H

#include "Id.h"

namespace ecs{
///---------------------------------------------------------------------
/// Entity is the identifier of an identity
///---------------------------------------------------------------------
///
/// An entity consists of an id and version. The version is used to
/// ensure that new entities allocated with the same id are separable.
///
/// An entity becomes invalid when destroyed.
///
///---------------------------------------------------------------------
class Entity {
 public:
  inline Entity(EntityManager *manager, Id id);
  inline Entity &operator=(const Entity &rhs);

  inline Id &id() { return id_; }
  inline Id const &id() const { return id_; }

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
  template<typename T> inline T const &as() const;

  /// Assume that this entity has provided Components
  /// Use for faster component access calls
  template<typename ...Components> inline EntityAlias<Components...> &assume();
  template<typename ...Components> inline EntityAlias<Components...> const &assume() const;

  /// Removes a component. Error of it doesn't exist
  template<typename C> inline void remove();

  /// Removes all components and call destructors
  void inline remove_everything();

  /// Clears the component mask without destroying components (faster than remove_everything)
  void inline clear_mask();

  /// Destroys this entity. Removes all components as well
  void inline destroy();

  /// Return true if entity has all specified components. False otherwise
  template<typename... Components> inline bool has();
  template<typename... Components> inline bool has() const;

  /// Returns whether an entity is an entity alias or not
  template<typename T> inline bool is();
  template<typename T> inline bool is() const;

  /// Returns true if entity has not been destroyed. False otherwise
  bool inline is_valid();
  bool inline is_valid() const;

 private:
  /// Return true if entity has all specified compoents. False otherwise
  inline bool has(details::ComponentMask &check_mask);
  inline bool has(details::ComponentMask const &check_mask) const;

  inline details::ComponentMask &mask();
  inline details::ComponentMask const &mask() const;

  inline static details::ComponentMask static_mask();

  EntityManager *manager_;
  Id             id_;

  friend class EntityManager;
}; //Entity

inline bool operator==(const Entity &lhs, const Entity &rhs);
inline bool operator!=(const Entity &lhs, const Entity &rhs);

} // namespace ecs

#include "Entity.inl"

#endif //ECS_ENTITY_H
