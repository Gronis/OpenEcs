#ifndef ECS_ENTITYMANAGER_H
#define ECS_ENTITYMANAGER_H

namespace ecs {

/// Forward declareations
template <typename...>
class EntityAlias;
class Entity;
template<typename>
class View;
class Id;

namespace details{

/// Used to iterate through the EntityManager with a lambda-expression,
/// with each argument as a specific component type. Used by with
/// function.
template<size_t N, typename...>
struct with_t;

} // namespace details

///---------------------------------------------------------------------
/// This is the main class for holding all Entities and Components
///---------------------------------------------------------------------
///
/// It uses a vector for holding entity versions and another for component
/// masks. It also uses 1 memory pool for each type of component.
///
/// The index of each data structure is used to identify each entity.
/// The version is used for checking if entity is valid.
/// The component mask is used to check what components an entity has
///
/// The Entity id = {index + version}. When an Entity is removed,
/// the index is added to the free list. A free list knows where spaces are
/// left open for new entities (to ensure no holes).
///
///---------------------------------------------------------------------
class EntityManager: details::forbid_copies {
 private:
  // Class for accessing where to put entities with specific components.
  struct IndexAccessor {
    // Used to store next available index there is within each block
    std::vector <index_t> block_index;
    // Used to store all indexes that are free
    std::vector <index_t> free_list;
  };

 public:
  inline EntityManager(size_t chunk_size = 8192);
  inline ~EntityManager();

  /// Create a new Entity
  inline Entity create();

  /// Create a specified number of new entities.
  inline std::vector <Entity> create(const size_t num_of_entities);

  /// Create, using EntityAlias
  template<typename T, typename ...Args>
  inline auto create(Args &&... args) -> typename std::enable_if<!std::is_constructible<T, Args...>::value, T>::type;
  template<typename T, typename ...Args>
  inline auto create(Args &&... args) -> typename std::enable_if<std::is_constructible<T, Args...>::value, T>::type;

  /// Create an entity with components assigned
  template<typename ...Components, typename ...Args>
  inline EntityAlias<Components...> create_with(Args &&... args);

  /// Create an entity with components assigned, using default values
  template<typename ...Components>
  inline EntityAlias<Components...> create_with();

  // Access a View of all entities with specified components
  template<typename ...Components>
  inline View <EntityAlias<Components...>> with();

  // Iterate through all entities with all components, specified as lambda parameters
  // example: entities.with([] (Position& pos) {  });
  template<typename T>
  inline void with(T lambda);

  // Access a View of all entities that has every component as Specified EntityAlias
  template<typename T>
  inline View <T> fetch_every();

  // Access a View of all entities that has every component as Specified EntityAlias specified as lambda parameters
  // example: entities.fetch_every([] (EntityAlias<Position>& entity) {  });
  template<typename T>
  inline void fetch_every(T lambda);

  // Get an Entity at specified index
  inline Entity operator[](index_t index);

  // Get an Entity with a specific Id. Id must be valid
  inline Entity operator[](Id id);

  // Get the Entity count for this EntityManager
  inline size_t count();

 private:

  /// Creates an entity and put it close to entities
  /// with similar components
  inline Entity create_with_mask(details::ComponentMask mask);

  /// Creates any number of entities > 0 and put them close to entities
  /// with similar components
  inline std::vector <Entity> create_with_mask(details::ComponentMask mask, const size_t num_of_entities);

  /// Find a proper index for a new entity with components
  inline index_t find_new_entity_index(details::ComponentMask mask);

  /// Create a new block for this entity type.
  inline void create_new_block(IndexAccessor &index_accessor, unsigned long mask_as_ulong, index_t next_free_index);

  /// Creates a ComponentManager. Mainly used by get_component_manager the first time its called
  template<typename C, typename ...Args>
  inline details::ComponentManager <C> &create_component_manager(Args &&... args);

  /// Get the ComponentManager. Assumes that the component manager already exists.
  template<typename C>
  inline details::ComponentManager <C> &get_component_manager_fast();
  template<typename C>
  inline details::ComponentManager <C> const &get_component_manager_fast() const;

  /// Get the ComponentManager. Creates a component manager if it
  /// doesn't exists for specified component type.
  template<typename C>
  inline details::ComponentManager <C> &get_component_manager();
  template<typename C>
  inline details::ComponentManager <C> const &get_component_manager() const;

  /// Get component for a specific entity or index.
  template<typename C>
  inline C &get_component(Entity &entity);
  template<typename C>
  inline C const &get_component(Entity const &entity) const;

  /// Get component for a specific entity or index. Assumes that a
  /// ComponentManager exists for the specific component type.
  template<typename C>
  inline C &get_component_fast(index_t index);
  template<typename C>
  inline C const &get_component_fast(index_t index) const;
  template<typename C>
  inline C &get_component_fast(Entity &entity);
  template<typename C>
  inline C const &get_component_fast(Entity const &entity) const;

  /// Use to create a component tmp that is assignable. Calls the constructor.
  template<typename C, typename ...Args>
  inline static auto create_tmp_component(Args &&... args) ->
  typename std::enable_if<std::is_constructible<C, Args...>::value, C>::type {
    return C(std::forward<Args>(args) ...);
  }

  /// Use to create a component tmp that is assignable. Uses uniform initialization.
  template<typename C, typename ...Args>
  inline static auto create_tmp_component(Args &&... args) ->
  typename std::enable_if<
      !std::is_constructible<C, Args...>::value &&
          !std::is_base_of<details::BaseProperty, C>::value,
      C>::type {
    return C{std::forward<Args>(args) ...};
  }

  /// Use to create a component tmp that is assignable. Call the right constructor
  /// Called when component is a property, and no constructor inaccessible.
  template<typename C, typename ...Args>
  inline static auto create_tmp_component(Args &&... args) ->
  typename std::enable_if<
      !std::is_constructible<C, Args...>::value &&
          std::is_base_of<details::BaseProperty, C>::value,
      C>::type {
    static_assert(sizeof...(Args) == 1, ECS_ASSERT_MSG_ONLY_ONE_ARGS_PROPERTY_CONSTRUCTOR);
    static_assert(sizeof(C) == sizeof(std::tuple < Args... > ),
    "Cannot initilize component property. Please provide a constructor");
    auto tmp = typename C::ValueType(std::forward<Args>(args)...);
    return *reinterpret_cast<C *>(&tmp);
  }

  /// Creates a component for a specific entity with Args
  template<typename C, typename ...Args>
  inline C &create_component(Entity &entity, Args &&... args);

  /// Removes a specific component from an entity.
  template<typename C>
  inline void remove_component(Entity &entity);

  /// Removes a specific component from an entity. Assumes that
  /// a ComponentManager exists for component type
  template<typename C>
  inline void remove_component_fast(Entity &entity);

  /// Removes all components from a single entity
  inline void remove_all_components(Entity &entity);

  /// Clears the component mask without removing any components
  inline void clear_mask(Entity &entity);

  /// Set a component for a specific entity. It uses contructor if
  /// entity does not have component. It uses assignment operator
  /// if component already added
  template<typename C, typename ...Args>
  inline C &set_component(Entity &entity, Args &&... args);

  /// Set a component for a specific entity. It assumes that the
  /// entity already has the component and uses the assignment
  /// operator.
  template<typename C, typename ...Args>
  inline C &set_component_fast(Entity &entity, Args &&... args);

  /// Check if component has specified component types attached
  inline bool has_component(Entity &entity, details::ComponentMask component_mask);
  inline bool has_component(Entity const &entity, details::ComponentMask const &component_mask) const;
  template<typename ...Components>
  inline bool has_component(Entity &entity);
  template<typename ...Components>
  inline bool has_component(Entity const &entity) const;

  /// Check if an entity is valid
  inline bool is_valid(Entity &entity);
  inline bool is_valid(Entity const &entity) const;

  /// Destroy an entity. Also removed all added components
  inline void destroy(Entity &entity);

  /// Get the entity mask from a specific entity or index
  inline details::ComponentMask &mask(Entity &entity);
  inline details::ComponentMask const &mask(Entity const &entity) const;
  inline details::ComponentMask &mask(index_t index);
  inline details::ComponentMask const &mask(index_t index) const;

  /// Get an entity given its id or index. When using Id, it must always be valid
  inline Entity get_entity(Id id);
  inline Entity get_entity(index_t index);

  /// Gey how many entities the EntityManager can handle atm
  inline size_t capacity() const;

  std::vector<details::BaseManager *> component_managers_;
  std::vector <details::ComponentMask> component_masks_;
  std::vector <version_t> entity_versions_;
  std::vector <index_t> next_free_indexes_;
  std::vector <size_t> index_to_component_mask;
  std::map <size_t, IndexAccessor> component_mask_to_index_accessor_;

  /// How many blocks of entities there exists. Used when tracking where to put entities in momory
  index_t block_count_ = 0;
  /// How many entities there are atm
  index_t count_ = 0;

  /// The EntityManager want some friends :)
  template<size_t N, typename...>
  friend struct details::with_t;
  template<typename T>
  friend class details::ComponentManager;
  template<typename ...Cs>
  friend class EntityAlias;
  template<typename T>
  friend class Iterator;
  friend class Entity;
  friend class BaseComponent;
};

} // namespace ecs

#include "EntityManager.inl"

#endif //ECS_ENTITYMANAGER_H
