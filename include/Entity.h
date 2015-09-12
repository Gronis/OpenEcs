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
  Entity(EntityManager *manager, Id id);
  Entity(const Entity &other);
  Entity &operator=(const Entity &rhs);

  Id &id() { return id_; }
  Id const &id() const { return id_; }

  /// Returns the requested component, or error if it doesn't exist
  template<typename C> C &get();
  template<typename C> C const &get() const;

  /// Set the requested component, if old component exist,
  /// a new one is created. Otherwise, the assignment operator
  /// is used.
  template<typename C, typename ... Args> C &set(Args &&... args);

  /// Add the requested component, error if component of the same type exist already
  template<typename C, typename ... Args> C &add(Args &&... args);

  /// Access this Entity as an EntityAlias.
  template<typename T> T &as();
  template<typename T> T const &as() const;

  /// Assume that this entity has provided Components
  /// Use for faster component access calls
  template<typename ...Components> EntityAlias<Components...> &assume();
  template<typename ...Components> EntityAlias<Components...> const &assume() const;

  /// Removes a component. Error of it doesn't exist
  template<typename C> void remove();

  /// Removes all components and call destructors
  void remove_everything();

  /// Clears the component mask without destroying components (faster than remove_everything)
  void clear_mask();

  /// Destroys this entity. Removes all components as well
  void destroy();

  /// Return true if entity has all specified components. False otherwise
  template<typename... Components> bool has();
  template<typename... Components> bool has() const;

  /// Returns whether an entity is an entity alias or not
  template<typename T> bool is();
  template<typename T> bool is() const;

  /// Returns true if entity has not been destroyed. False otherwise
  bool is_valid();
  bool is_valid() const;

  template<typename ...Components> std::tuple<Components &...> unpack();
  template<typename ...Components> std::tuple<Components const &...> unpack() const;

 private:
  /// Return true if entity has all specified compoents. False otherwise
  bool has(details::ComponentMask &check_mask);
  bool has(details::ComponentMask const &check_mask) const;

  details::ComponentMask &mask();
  details::ComponentMask const &mask() const;

  EntityManager *manager_;
  Id             id_;

  friend class EntityManager;
}; //Entity

inline bool operator==(const Entity &lhs, const Entity &rhs);
inline bool operator!=(const Entity &lhs, const Entity &rhs);

} // namespace ecs

#include "Entity.inl"

#endif //ECS_ENTITY_H
