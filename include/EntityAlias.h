#ifndef ECS_ENTITYALIAS_H
#define ECS_ENTITYALIAS_H

namespace ecs{

namespace details{

class BaseEntity {
 public:
  inline BaseEntity(const Entity &entity);
 protected:
  inline BaseEntity();
  inline BaseEntity(const BaseEntity &other);

  inline EntityManager &entities() { return *manager_; }
  inline Entity &entity() { return entity_; }
 private:
  union {
    EntityManager *manager_;
    Entity entity_;
  };
  template<typename ... Components>
  friend class ecs::EntityAlias;
}; //BaseEntity

} // namespace details


///---------------------------------------------------------------------
/// EntityAlias is a wrapper around an Entity
///---------------------------------------------------------------------
///
/// An EntityAlias makes modification of the entity and other
/// entities much easier when performing actions. It acts solely as an
/// abstraction layer between the entity and different actions.
///
///---------------------------------------------------------------------
template<typename ...Components>
class EntityAlias : public details::BaseEntity {
 private:
  /// Underlying EntityAlias. Used for creating Entity alias without
  /// a provided constructor
  using Type = EntityAlias<Components...>;
  template<typename C>
  using is_component = details::is_type<C, Components...>;

 public:
  inline EntityAlias(const Entity &entity);

  /// Cast to Entity or EntityAlias
  inline operator Entity &();
  inline operator Entity const &() const;
  template<typename T>
  inline operator const T &() const;
  template<typename T>
  inline operator T &();

  inline bool operator==(const Entity &rhs) const;
  inline bool operator!=(const Entity &rhs) const;

  inline Id &id();
  inline Id const &id() const;

  /// Returns the requested component, or error if it doesn't exist
  template<typename C> inline auto get()       -> typename std::enable_if< is_component<C>::value, C &>::type;
  template<typename C> inline auto get()       -> typename std::enable_if<!is_component<C>::value, C &>::type;
  template<typename C> inline auto get() const -> typename std::enable_if< is_component<C>::value, C const &>::type;
  template<typename C> inline auto get() const -> typename std::enable_if<!is_component<C>::value, C const &>::type;

  /// Set the requested component, if old component exist,
  /// a new one is created. Otherwise, the assignment operator
  /// is used.
  template<typename C, typename ... Args> inline auto set(Args &&... args) ->
      typename std::enable_if< is_component<C>::value, C &>::type;
  template<typename C, typename ... Args> inline auto set(Args &&... args) ->
      typename std::enable_if<!is_component<C>::value, C &>::type;

  /// Add the requested component, error if component of the same type exist already
  template<typename C, typename ... Args>
  inline C &add(Args &&... args);

  /// Access this Entity as an EntityAlias.
  template<typename T> inline T &as();
  template<typename T> inline T const &as() const;

  /// Assume that this entity has provided Components
  /// Use for faster component access calls
  template<typename ...Components_>
  inline EntityAlias<Components_...> &assume();

  template<typename ...Components_>
  inline EntityAlias<Components_...> const &assume() const;

  /// Removes a component. Error of it doesn't exist. Cannot remove dependent components
  template<typename C> inline auto remove() -> typename std::enable_if< is_component<C>::value, void>::type;
  template<typename C> inline auto remove() -> typename std::enable_if<!is_component<C>::value, void>::type;

  /// Removes all components and call destructors
  inline void remove_everything();

  /// Clears the component mask without destroying components (faster than remove_everything)
  inline void clear_mask();

  /// Destroys this entity. Removes all components as well
  inline void destroy();
  /// Return true if entity has all specified components. False otherwise
  template<typename... Components_> inline bool has();
  template<typename... Components_> inline bool has() const;

  /// Returns whether an entity is an entity alias or not
  template<typename T> inline bool is();
  template<typename T> inline bool is() const;

  /// Returns true if entity has not been destroyed. False otherwise
  inline bool is_valid();
  inline bool is_valid() const;

 protected:
  inline EntityAlias();

 private:
  // Recursion init components with argument
  template<typename C0, typename Arg>
  inline void init_components(Arg arg) {
    add<C0>(arg);
  }

  template<typename C0, typename C1, typename ... Cs, typename Arg0, typename Arg1, typename ... Args>
  inline void init_components(Arg0 arg0, Arg1 arg1, Args... args) {
    init_components<C0>(arg0);
    init_components<C1, Cs...>(arg1, args...);
  }
  // Recursion init components without argument
  template<typename C>
  inline void init_components() {
    add<C>();
  }

  template<typename C0, typename C1, typename ... Cs>
  inline void init_components() {
    init_components<C0>();
    init_components<C1, Cs...>();
  }

  template<typename ... Args>
  inline void init(Args... args) {
    init_components<Components...>(args...);
  }

  inline static details::ComponentMask static_mask();

  friend class EntityManager;
  friend class Entity;
}; //EntityAlias

} // namespace ecs

#include "EntityAlias.inl"

#endif //ECS_ENTITYALIAS_H
