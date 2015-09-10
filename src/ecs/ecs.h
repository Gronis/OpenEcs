#pragma once
#ifndef ECS_SINGLE_HEADER
#define ECS_SINGLE_HEADER

/// --------------------------------------------------------------------------
/// Copyright (C) 2015  Robin Grönberg
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#include <bitset>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <cassert>
#include <iostream>

/// The cache line size for the processor. Usually 64 bytes
#ifndef ECS_CACHE_LINE_SIZE
#define ECS_CACHE_LINE_SIZE 64
#endif

/// This is how an assertion is done. Can be defined with something else if needed.
#ifndef ECS_ASSERT
#define ECS_ASSERT(Expr, Msg) assert(Expr && Msg)
#endif

/// The maximum number of component types the EntityManager can handle
#ifndef ECS_MAX_NUM_OF_COMPONENTS
#define ECS_MAX_NUM_OF_COMPONENTS 64
#endif

/// How many components each block of memory should contain
/// By default, this is divided into the same size as cache-line size
#ifndef ECS_DEFAULT_CHUNK_SIZE
#define ECS_DEFAULT_CHUNK_SIZE ECS_CACHE_LINE_SIZE
#endif

#define ECS_ASSERT_IS_CALLABLE(T)                                                           \
            static_assert(details::is_callable<T>::value,                                   \
            "Provide a function or lambda expression");                                     \

#define ECS_ASSERT_IS_ENTITY(T)                                                             \
            static_assert(std::is_base_of<details::BaseEntityAlias, T>::value ||            \
                      std::is_same<Entity, T>::value ,                                      \
            #T " does not inherit EntityInterface.");

#define ECS_ASSERT_ENTITY_CORRECT_SIZE(T)                                                   \
            static_assert(sizeof(details::BaseEntityAlias) == sizeof(T),                    \
            #T " should not include new variables, add them as Components instead.");       \

#define ECS_ASSERT_VALID_ENTITY(E)                                                          \
            ECS_ASSERT(is_valid(E), "Entity is no longer valid");                           \

#define ECS_ASSERT_IS_SYSTEM(S)                                                             \
            static_assert(std::is_base_of<System, S>::value,                                \
            "DirivedSystem must inherit System.");                                          \

#define ECS_ASSERT_MSG_ONLY_ONE_ARGS_PROPERTY_CONSTRUCTOR                                   \
            "Creating a property component should only take 1 argument. "                   \
            "If component should initilize more members, provide a "                        \
            "constructor to initilize property component correctly"                         \

namespace ecs {

/// Type used for entity index
typedef uint32_t index_t;

/// Type used for entity version
typedef uint8_t version_t;

//TODO: remove
class EntityManager;
class Entity;
template<typename ...Components>
class EntityAlias;

namespace details {

///--------------------------------------------------------------------
/// function_traits is used to determine function properties
/// such as parameter types (arguments) and return type.
///--------------------------------------------------------------------
template<typename T>
struct function_traits
    : public function_traits<decltype(&T::operator())> {
};
// For generic types, directly use the result of the signature of its 'operator()'

template<typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
  // we specialize for pointers to member function
{
  enum { arg_count = sizeof...(Args) };

  typedef ReturnType return_type;

  template<size_t i>
  struct arg_t {
    typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
  };
  template<size_t i>
  using arg = typename arg_t<i>::type;

  template<size_t i>
  struct arg_remove_ref_t {
    typedef typename std::remove_reference<arg<i>>::type type;
  };
  template<size_t i>
  using arg_remove_ref = typename arg_remove_ref_t<i>::type;

  typedef std::tuple<Args...> args;
};


///---------------------------------------------------------------------
/// Determine if type is part of a list of types
///---------------------------------------------------------------------
template<typename T, typename... Ts>
struct is_type;

template<typename T, typename... Ts>
struct is_type<T, T, Ts...>: std::true_type { };

template<typename T>
struct is_type<T, T>: std::true_type { };

template<typename T, typename Tail>
struct is_type<T, Tail>: std::false_type { };

template<typename T, typename Tail, typename... Ts>
struct is_type<T, Tail, Ts...>: is_type<T, Ts...>::type { };


///---------------------------------------------------------------------
/// Check if a class has implemented operator()
///
/// Mostly used for checking if provided type is a lambda expression
///---------------------------------------------------------------------
///
template<typename T>
struct is_callable {
  template<typename U>
  static char test(decltype(&U::operator()));

  template<typename U>
  static int test(...);

  enum { value = sizeof(test<T>(0)) == sizeof(char) };
};


///---------------------------------------------------------------------
/// Any class that should not be able to copy itself inherit this class
///---------------------------------------------------------------------
///
class forbid_copies {
 protected:
  forbid_copies() = default;
  ~forbid_copies() = default;
  forbid_copies(const forbid_copies &) = delete;
  forbid_copies &operator=(const forbid_copies &) = delete;
};

///---------------------------------------------------------------------
/// A Pool is a memory pool.
/// See link for more info: http://en.wikipedia.org/wiki/Memory_pool
///---------------------------------------------------------------------
///
/// The BasePool is used to store void*. Use Pool<T> for a generic
/// Pool allocation class.
///
///---------------------------------------------------------------------
class BasePool: forbid_copies {
 public:
  explicit BasePool(size_t element_size, size_t chunk_size = ECS_DEFAULT_CHUNK_SIZE);

  virtual ~BasePool();

  inline index_t size() const { return size_; }

  inline index_t capacity() const { return capacity_; }

  inline size_t chunks() const { return chunks_.size(); }

  inline void ensure_min_size(std::size_t size);

  inline void ensure_min_capacity(size_t min_capacity);

  virtual void destroy(index_t index) = 0;

 protected:
  index_t size_;
  index_t capacity_;
  size_t element_size_;
  size_t chunk_size_;
  std::vector<char *> chunks_;
};

///---------------------------------------------------------------------
/// A Pool is a memory pool.
/// See link for more info: http://en.wikipedia.org/wiki/Memory_pool
///---------------------------------------------------------------------
///
/// The Pool is used to store * of any class. Destroying an object calls
/// destructor. The pool doesn't know where there is data allocated.
/// This must be done from outside.
///
///---------------------------------------------------------------------
template<typename T>
class Pool: public BasePool {
 public:
  Pool(size_t chunk_size);

  virtual void destroy(index_t index) override;

  T* get_ptr(index_t index);
  const T* get_ptr(index_t index) const;

  T& get(index_t index);
  const T& get(index_t index) const;

  T& operator[](size_t index);
  const T& operator[](size_t index) const;
};

///---------------------------------------------------------------------
/// ComponentMask is a mask defining what components and entity has.
///---------------------------------------------------------------------
///
///---------------------------------------------------------------------
typedef std::bitset<ECS_MAX_NUM_OF_COMPONENTS> ComponentMask;

///---------------------------------------------------------------------
/// Helper class used for compile-time checks to determine if a
/// component is a propery.
///---------------------------------------------------------------------
class BaseProperty{};

///---------------------------------------------------------------------
/// Helper class
///---------------------------------------------------------------------
class BaseEntityAlias;

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
  C&create(index_t index, Args &&... args);
  void remove(index_t index);

  C &operator[](index_t index);

  C &get(index_t index);
  C const &get(index_t index) const;

  C *get_ptr(index_t index);
  C const *get_ptr(index_t index) const;

  ComponentMask mask();

 private:
  EntityManager &manager_;
  details::Pool<C> pool_;
}; //ComponentManager


} // namespace details

///---------------------------------------------------------------------
/// A Property is a helper class for Component with only one
/// property of any type
///---------------------------------------------------------------------
///
/// A Property is a helper class for Component with only one
/// property of any type.
///
/// it implements standard constructors, type conversations and
/// operators:
///
///     ==, !=
///     >=, <=, <, >
///     +=, -=, *=, /=, %=
///     &=, |=, ^=
///     +, -, *; /, %
///     &, |, ^, ~
///     >>, <<
///     ++, --
///
/// TODO: Add more operators?
///---------------------------------------------------------------------
template<typename T>
struct Property : details::BaseProperty{
  Property() { }
  Property(const T &value) : value(value) { }

  operator const T &() const { return value; }
  operator T &() { return value; }

  T value;
  using ValueType = T;
};

/// Comparision operators
template<typename T, typename E> bool operator==(Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator!=(Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator>=(Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator> (Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator<=(Property<T> const &lhs, const E &rhs);
template<typename T, typename E> bool operator< (Property<T> const &lhs, const E &rhs);

/// Compound assignment operators
template<typename T, typename E> T& operator+=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator-=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator*=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator/=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator%=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator&=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator|=(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T& operator^=(Property<T> &lhs, const E &rhs);

/// Arithmetic operators
template<typename T, typename E> T  operator+ (Property<T> &lhs, const E &rhs);
template<typename T, typename E> T  operator- (Property<T> &lhs, const E &rhs);
template<typename T, typename E> T  operator* (Property<T> &lhs, const E &rhs);
template<typename T, typename E> T  operator/ (Property<T> &lhs, const E &rhs);
template<typename T, typename E> T  operator% (Property<T> &lhs, const E &rhs);
template<typename T>             T& operator++(Property<T> &lhs);
template<typename T>             T  operator++(Property<T> &lhs, int);
template<typename T>             T& operator--(Property<T> &lhs);
template<typename T>             T  operator--(Property<T> &lhs, int);

/// Bitwise operators
template<typename T, typename E> T operator&(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T operator|(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T operator^(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T operator~(Property<T> &lhs);
template<typename T, typename E> T operator>>(Property<T> &lhs, const E &rhs);
template<typename T, typename E> T operator<<(Property<T> &lhs, const E &rhs);

///---------------------------------------------------------------------
/// Describe Id here
///---------------------------------------------------------------------
///
///
///---------------------------------------------------------------------
class Id {
 public:
  Id();
  Id(index_t index, version_t version);

  inline index_t index() { return index_; }
  inline index_t index() const { return index_; }

  inline version_t version() { return version_; }
  inline version_t version() const { return version_; }

 private:
  index_t index_;
  version_t version_;
  friend class Entity;
  friend class EntityManager;
};

bool operator==(const Id& lhs, const Id &rhs);
bool operator!=(const Id& lhs, const Id &rhs);

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
  template<typename C> inline C &get();
  template<typename C> inline C const &get() const;

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


namespace details{

class BaseEntityAlias {
 public:
  BaseEntityAlias(Entity &entity);
 protected:
  BaseEntityAlias();
  BaseEntityAlias(const BaseEntityAlias &other);

  EntityManager &entities() { return *manager_; }
 private:
  union {
    EntityManager *manager_;
    Entity entity_;
  };
  template<typename ... Components>
  friend class ecs::EntityAlias;
}; //BaseEntityAlias

} // namespace details

inline bool operator==(const Entity &lhs, const Entity &rhs);
inline bool operator!=(const Entity &lhs, const Entity &rhs);

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
class EntityAlias : public details::BaseEntityAlias {
 private:
  /// Underlying EntityAlias. Used for creating Entity alias without
  /// a provided constructor
  using Type = EntityAlias<Components...>;
  template<typename C>
  using is_component = details::is_type<C, Components...>;

 public:
  EntityAlias(Entity &entity);

  operator Entity &();
  operator Entity const &() const;

  Id &id();
  Id const &id() const;

  /// Returns the requested component, or error if it doesn't exist
  template<typename C> inline C & get();
  template<typename C> inline C const & get() const;

  /// Set the requested component, if old component exist,
  /// a new one is created. Otherwise, the assignment operator
  /// is used.
  template<typename C, typename ... Args>
  inline C& set(Args &&... args);

  /// Add the requested component, error if component of the same type exist already
  template<typename C, typename ... Args>
  inline C &add(Args &&... args);

  /// Access this Entity as an EntityAlias.
  template<typename T> T &as();
  template<typename T> T const &as() const;

  /// Assume that this entity has provided Components
  /// Use for faster component access calls
  template<typename ...Components_>
  inline EntityAlias<Components_...> &assume();

  template<typename ...Components_>
  inline EntityAlias<Components_...> const &assume() const;

  /// Removes a component. Error of it doesn't exist. Cannot remove dependent components
  template<typename C>
  void remove();

  /// Removes all components and call destructors
  void remove_everything();

  /// Clears the component mask without destroying components (faster than remove_everything)
  inline void clear_mask();

  /// Destroys this entity. Removes all components as well
  inline void destroy();
  /// Return true if entity has all specified components. False otherwise
  template<typename... Components_> bool has();
  template<typename... Components_> bool has() const;

  /// Returns whether an entity is an entity alias or not
  template<typename T> bool is();
  template<typename T> bool is() const;

  /// Returns true if entity has not been destroyed. False otherwise
  bool is_valid();
  bool is_valid() const;

  template<typename ...Components_> std::tuple<Components_ &...> unpack();
  template<typename ...Components_> std::tuple<Components_ const &...> unpack() const;

  std::tuple<Components &...> unpack();
  std::tuple<Components const &...> unpack() const;

 protected:
  EntityAlias();

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
  void init(Args... args) {
    init_components<Components...>(args...);
  }

  static details::ComponentMask mask();


  friend class EntityManager;
  friend class Entity;
}; //EntityAlias

template<typename ... Cs> bool operator==(const EntityAlias<Cs...> &lhs, const Entity &rhs);
template<typename ... Cs> bool operator!=(const EntityAlias<Cs...> &lhs, const Entity &rhs);

///---------------------------------------------------------------------
/// Iterator is an iterator for iterating through the entity manager
///---------------------------------------------------------------------
template<typename T>
class Iterator: public std::iterator<std::input_iterator_tag, typename std::remove_reference<T>::type> {
  using T_no_ref = typename std::remove_reference<typename std::remove_const<T>::type>::type;

 public:
  Iterator(EntityManager *manager, details::ComponentMask mask, bool begin = true);
  Iterator(const Iterator &it);
  Iterator &operator=(const Iterator &rhs) = default;

  index_t index() const;
  Iterator &operator++();
  T       entity();
  T const entity() const;

 private:
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

///---------------------------------------------------------------------
/// Helper class that is used to access the iterator
///---------------------------------------------------------------------
/// @usage Calling entities.with<Components>(), returns a view
///        that can be used to access the iterator with begin() and
///        end() that iterates through all entities with specified
///        Components
///---------------------------------------------------------------------
template<typename T>
class View {
  ECS_ASSERT_IS_ENTITY(T)
 public:
  using iterator        = Iterator<T>;
  using const_iterator = Iterator<T const &>;

  View(EntityManager *manager, details::ComponentMask mask);

  iterator begin();
  iterator end();
  const_iterator begin() const;
  const_iterator end() const;
  index_t count();
  template<typename ...Components>
  View<T>&& with();

 private:
  EntityManager         *manager_;
  details::ComponentMask mask_;

  friend class EntityManager;
}; //View

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
    std::vector<index_t> block_index;
    // Used to store all indexes that are free
    std::vector<index_t> free_list;
  };

 public:
  EntityManager(size_t chunk_size = 8192);
  ~EntityManager();

  /// Create a new Entity
  Entity create();

  /// Create a specified number of new entities.
  std::vector<Entity> create(const size_t num_of_entities);

  /// Create, using EntityAlias
  template<typename T, typename ...Args>
  T create(Args && ... args);

  /// Create an entity with components assigned
  template<typename ...Components, typename ...Args>
  EntityAlias<Components...> create_with(Args && ... args);

  /// Create an entity with components assigned, using default values
  template<typename ...Components>
  EntityAlias<Components...> create_with();

  // Access a View of all entities with specified components
  template<typename ...Components>
  View<EntityAlias<Components...>> with();

  // Iterate through all entities with all components, specified as lambda parameters
  // example: entities.with([] (Position& pos) {  });
  template<typename T>
  void with(T lambda);

  // Access a View of all entities that has every component as Specified EntityAlias
  template<typename T>
  View<T> fetch_every();

  // Access a View of all entities that has every component as Specified EntityAlias specified as lambda parameters
  // example: entities.fetch_every([] (EntityAlias<Position>& entity) {  });
  template<typename T>
  void fetch_every(T lambda);

  // Get an Entity at specified index
  Entity operator[](index_t index);

  // Get an Entity with a specific Id. Id must be valid
  Entity operator[](Id id);

  // Get the Entity count for this EntityManager
  size_t count();

 private:
  template<size_t N, typename...>
  struct with_t;

  template<size_t N, typename Lambda, typename... Args>
  struct with_t<N, Lambda, Args...>:
      with_t<N - 1, Lambda,
             typename details::function_traits<Lambda>::template arg_remove_ref<N - 1>, Args...> {
  };

  template<typename Lambda, typename... Args>
  struct with_t<0, Lambda, Args...> {
    static inline void for_each(EntityManager &manager, Lambda lambda) {
      typedef details::function_traits<Lambda> function;
      static_assert(function::arg_count > 0, "Lambda or function must have at least 1 argument.");
      auto view = manager.with<Args...>();
      auto it = view.begin();
      auto end = view.end();
      for (; it != end; ++it) {
        lambda(get_arg<Args>(manager, it.index())...);
      }
    }

    //When arg is component, access component
    template<typename C>
    static inline auto get_arg(EntityManager &manager, index_t index) ->
    typename std::enable_if<!std::is_same<C, Entity>::value, C &>::type {
      return manager.get_component_fast<C>(index);
    }

    //When arg is the Entity, access the Entity
    template<typename C>
    static inline auto get_arg(EntityManager &manager, index_t index) ->
    typename std::enable_if<std::is_same<C, Entity>::value, Entity>::type {
      return manager.get_entity(index);
    }
  };

  template<typename Lambda>
  using with_ = with_t<details::function_traits<Lambda>::arg_count, Lambda>;

  Entity create_with_mask(details::ComponentMask mask);

  std::vector<Entity> create_with_mask(details::ComponentMask mask, const size_t num_of_entities);

  // Find a proper index for a new entity with components
  index_t find_new_entity_index(details::ComponentMask mask);

  /// Create a new block for this entity type.
  void create_new_block(IndexAccessor &index_accessor, unsigned long mask_as_ulong, index_t next_free_index);

  template<typename C, typename ...Args> details::ComponentManager<C> &create_component_manager(Args &&... args);

  template<typename C> details::ComponentManager<C> &      get_component_manager_fast();
  template<typename C> details::ComponentManager<C> const &get_component_manager_fast() const;

  template<typename C> details::ComponentManager<C> &       get_component_manager();
  template<typename C> details::ComponentManager<C> const &get_component_manager() const;

  template<typename C> C &      get_component(Entity &entity);
  template<typename C> C const &get_component(Entity const &entity) const;

  template<typename C> C &      get_component_fast(index_t index);
  template<typename C> C const &get_component_fast(index_t index) const;

  /// Get component fast, no error checks. Use if it is already known that Entity has Component
  template<typename C> C &      get_component_fast(Entity &entity);
  template<typename C> C const &get_component_fast(Entity const &entity) const;

  /// Use to create a component tmp that is assignable. Call the right constructor
  template<typename C, typename ...Args>
  static auto create_tmp_component(Args &&... args) ->
  typename std::enable_if<std::is_constructible<C, Args...>::value, C>::type {
    return C(std::forward<Args>(args) ...);
  }

  /// Use to create a component tmp that is assignable. Call the right constructor
  template<typename C, typename ...Args>
  static auto create_tmp_component(Args &&... args) ->
  typename std::enable_if<
      !std::is_constructible<C, Args...>::value &&
          !std::is_base_of<details::BaseProperty, C>::value,
      C>::type {
    return C{std::forward<Args>(args) ...};
  }

  /// Use to create a component tmp that is assignable. Call the right constructor
  /// Called when component is a property, and no constructor inaccessible.
  template<typename C, typename ...Args>
  static auto create_tmp_component(Args &&... args) ->
  typename std::enable_if<
      !std::is_constructible<C, Args...>::value &&
          std::is_base_of<details::BaseProperty, C>::value,
      C>::type {
    static_assert(sizeof...(Args) == 1, ECS_ASSERT_MSG_ONLY_ONE_ARGS_PROPERTY_CONSTRUCTOR);
    static_assert(sizeof(C) == sizeof(std::tuple<Args...>),
                  "Cannot initilize component property. Please provide a constructor");
    auto tmp = typename C::ValueType(std::forward<Args>(args)...);
    return *reinterpret_cast<C *>(&tmp);
  }

  template<typename C, typename ...Args>
  inline C &create_component(Entity &entity, Args &&... args);

  template<typename C>
  inline void remove_component(Entity &entity);

  template<typename C>
  inline void remove_component_fast(Entity &entity);

  /// Removes all components from a single entity
  inline void remove_all_components(Entity &entity);

  inline void clear_mask(Entity &entity);

  template<typename C, typename ...Args>
  inline C &set_component(Entity &entity, Args &&... args);

  template<typename C, typename ...Args>
  inline C &set_component_fast(Entity &entity, Args &&... args);

  bool has_component(Entity &       entity, details::ComponentMask        component_mask);
  bool has_component(Entity const & entity, details::ComponentMask const &component_mask) const;
  bool has_component(index_t        index,  details::ComponentMask &      component_mask);

  template<typename ...Components>
  bool has_component(Entity &      entity);
  template<typename ...Components>
  bool has_component(Entity const &entity) const;

  bool is_valid(Entity &entity);
  bool is_valid(Entity const &entity) const;

  void destroy(Entity &entity);

  details::ComponentMask &      mask(Entity       &entity);
  details::ComponentMask const &mask(Entity const &entity) const;
  details::ComponentMask &      mask(index_t       index);
  details::ComponentMask const &mask(index_t       index) const;

  Entity get_entity(Id id);
  Entity get_entity(index_t index);

  size_t capacity() const;

  std::vector<details::BaseManager *> component_managers_;
  std::vector<details::ComponentMask> component_masks_;
  std::vector<version_t>              entity_versions_;
  std::vector<index_t>                next_free_indexes_;
  std::vector<size_t>                 index_to_component_mask;
  std::map<size_t, IndexAccessor>     component_mask_to_index_accessor_;

  index_t block_count_ = 0;
  index_t count_ = 0;

  template<typename T>
  friend class details::ComponentManager;

  template<typename T>
  friend class Iterator;
  friend class Entity;
  friend class BaseComponent;
};

class SystemManager: details::forbid_copies {
 private:
 public:
  class System {
   public:
    virtual ~System() { }
    virtual void update(float time) = 0;
   protected:
    EntityManager &entities() {
      return *manager_->entities_;
    }
   private:
    friend class SystemManager;
    SystemManager *manager_;
  };

  SystemManager(EntityManager &entities) : entities_(&entities) { }

  ~SystemManager() {
    for (auto system : systems_) {
      if (system != nullptr) delete system;
    }
    systems_.clear();
    order_.clear();
  }

  template<typename S, typename ...Args>
  S &add(Args &&... args) {
    ECS_ASSERT_IS_SYSTEM(S);
    ECS_ASSERT(!exists<S>(), "System already exists");
    systems_.resize(system_index<S>() + 1);
    S *system = new S(std::forward<Args>(args) ...);
    system->manager_ = this;
    systems_[system_index<S>()] = system;
    order_.push_back(system_index<S>());
    return *system;
  }

  template<typename S>
  void remove() {
    ECS_ASSERT_IS_SYSTEM(S);
    ECS_ASSERT(exists<S>(), "System does not exist");
    delete systems_[system_index<S>()];
    systems_[system_index<S>()] = nullptr;
    for (auto it = order_.begin(); it != order_.end(); ++it) {
      if (*it == system_index<S>()) {
        order_.erase(it);
      }
    }
  }

  void update(float time) {
    for (auto index : order_) {
      systems_[index]->update(time);
    }
  }

  template<typename S>
  inline bool exists() {
    ECS_ASSERT_IS_SYSTEM(S);
    return systems_.size() > system_index<S>() && systems_[system_index<S>()] != nullptr;
  }

 private:
  template<typename C>
  static size_t system_index() {
    static size_t index = system_counter()++;
    return index;
  }

  static size_t &system_counter() {
    static size_t counter = 0;
    return counter;
  }

  std::vector<System *> systems_;
  std::vector<size_t> order_;
  EntityManager *entities_;
};

using System = SystemManager::System;
} // namespace ecs

///---------------------------------------------------------------------
/// This will allow a property to be streamed into a input and output
/// stream.
///---------------------------------------------------------------------
namespace std {

template<typename T>
ostream &operator<<(ostream &os, const ecs::Property<T> &obj) {
  return os << obj.value;
}

template<typename T>
istream &operator>>(istream &is, ecs::Property<T> &obj) {
  return is >> obj.value;
}

} //namespace std

///---------------------------------------------------------------------
/// This will enable properties to be added to a string
///---------------------------------------------------------------------
template<typename T>
std::string operator+(const std::string &lhs, const ecs::Property<T> &rhs) { return lhs + rhs.value; }

template<typename T>
std::string operator+(std::string &&lhs, ecs::Property<T> &&rhs) { return lhs + rhs.value; }

template<typename T>
std::string operator+(std::string &&lhs, const ecs::Property<T> &rhs) { return lhs + rhs.value; }

template<typename T>
std::string operator+(const std::string &lhs, ecs::Property<T> &&rhs) { return lhs + rhs.value; }

#endif //Header guard