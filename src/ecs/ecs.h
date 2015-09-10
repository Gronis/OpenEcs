#pragma once

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
        static_assert(details::is_callable<T>::value,                                       \
        "Provide a function or lambda expression");                                         \

#define ECS_ASSERT_IS_ENTITY(T)                                                             \
        static_assert(std::is_base_of<BaseEntityAlias, T>::value ||                         \
                  std::is_same<Entity, T>::value ,                                          \
        #T " does not inherit EntityInterface.");

#define ECS_ASSERT_ENTITY_CORRECT_SIZE(T)                                                   \
        static_assert(sizeof(BaseEntityAlias) == sizeof(T),                                 \
        #T " should not include new variables, add them as Components instead.");           \

#define ECS_ASSERT_VALID_ENTITY(E)                                                          \
        ECS_ASSERT(is_valid(E), "Entity is no longer valid");                               \

#define ECS_ASSERT_IS_SYSTEM(S)                                                             \
            static_assert(std::is_base_of<System, S>::value,                                \
            "DirivedSystem must inherit System.");                                          \

#define ECS_ASSERT_MSG_ONLY_ONE_ARGS_PROPERTY_CONSTRUCTOR                                   \
            "Creating a property component should only take 1 argument. "                   \
            "If component should initilize more members, provide a "                        \
            "constructor to initilize property component correctly"                         \

namespace ecs{

/// Type used for entity index
typedef uint32_t index_t;

/// Type used for entity version
typedef uint8_t version_t;

namespace details{

///--------------------------------------------------------------------
/// function_traits is used to determine function properties
/// such as parameter types (arguments) and return type.
///--------------------------------------------------------------------
template <typename T>
struct function_traits
    : public function_traits<decltype(&T::operator())>
{};
// For generic types, directly use the result of the signature of its 'operator()'

template <typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
  // we specialize for pointers to member function
{
  enum { arg_count = sizeof...(Args) };

  typedef ReturnType return_type;

  template <size_t i>
  struct arg_t
  {
    typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
  };
  template <size_t i>
  using arg = typename arg_t<i>::type;

  template <size_t i>
  struct arg_remove_ref_t
  {
    typedef typename std::remove_reference<arg<i>>::type type;
  };
  template <size_t i>
  using arg_remove_ref = typename arg_remove_ref_t<i>::type;

  typedef std::tuple<Args...> args;
};


///---------------------------------------------------------------------
/// Determine if type is part of a list of types
///---------------------------------------------------------------------
template<typename T, typename... Ts>
struct is_type;

template<typename T, typename... Ts>
struct is_type<T, T, Ts...> : std::true_type{};

template<typename T>
struct is_type<T, T> : std::true_type{};

template<typename T, typename Tail>
struct is_type<T, Tail> : std::false_type{};

template<typename T, typename Tail, typename... Ts>
struct is_type<T, Tail, Ts...> : is_type<T, Ts...>::type {};



///---------------------------------------------------------------------
/// Check if a class has implemented operator()
///
/// Mostly used for checking if provided type is a lambda expression
///---------------------------------------------------------------------
///
template<typename T>
struct is_callable {
  template<typename U>
  static char test( decltype(&U::operator()) );

  template<typename U>
  static int test( ... );

  enum{ value = sizeof(test<T>(0)) == sizeof(char) };
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
class BasePool : forbid_copies {
 public:
  explicit BasePool(size_t element_size, size_t chunk_size = ECS_DEFAULT_CHUNK_SIZE) :
      size_(0),
      capacity_(0),
      element_size_(element_size),
      chunk_size_(chunk_size) {
  };

  virtual ~BasePool() {
    for (char *ptr : chunks_) {
      delete[] ptr;
    }
  }

  inline index_t size() const {
    return size_;
  }

  inline index_t capacity() const {
    return capacity_;
  }

  inline size_t chunks() const {
    return chunks_.size();
  }

  inline void ensure_min_size(std::size_t size) {
    if (size >= size_) {
      if (size >= capacity_) ensure_min_capacity(size);
      size_ = size;
    }
  }

  inline void ensure_min_capacity(size_t min_capacity) {
    while (min_capacity >= capacity_) {
      char *chunk = new char[element_size_ * chunk_size_];
      chunks_.push_back(chunk);
      capacity_ += chunk_size_;
    }
  }


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
class Pool : public BasePool {
 public:
  Pool(size_t chunk_size) : BasePool(sizeof(T), chunk_size) { };

  virtual void destroy(index_t index) override {
    ECS_ASSERT(index < size_, "Pool has not allocated memory for this index.");
    T *ptr = get_ptr(index);
    ptr->~T();
  }

  inline T *get_ptr(index_t index) {
    ECS_ASSERT(index < capacity_, "Pool has not allocated memory for this index.");
    return reinterpret_cast<T *>(chunks_[index / chunk_size_] + (index % chunk_size_) * element_size_);
  }

  const inline T *get_ptr(index_t index) const {
    ECS_ASSERT(index < this->capacity_, "Pool has not allocated memory for this index.");
    return reinterpret_cast<T *>(chunks_[index / chunk_size_] + (index % chunk_size_) * element_size_);
  }

  inline virtual T& get(index_t index) {
    return *get_ptr(index);
  }

  inline T& operator[] (size_t index){
    return get(index);
  }
};

///---------------------------------------------------------------------
/// Every Property must derive from the BaseProperty somehow used for
/// compile-time calculations
///---------------------------------------------------------------------
struct BaseProperty {};

///---------------------------------------------------------------------
/// A StandardProperty is a helper class for Component with only one
/// property of any type
///---------------------------------------------------------------------
///
/// A StandardProperty is a helper class for Component with only one
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
///
/// TODO: Add more operators?
///---------------------------------------------------------------------
template<typename T>
struct StandardProperty : BaseProperty{
  StandardProperty(){}

  StandardProperty(const T& value) : value(value){}

  inline operator const T&() const{
    return value;
  }

  inline operator T&(){
    return value;
  }

  template<typename E>
  inline bool operator == (const E& rhs) const{
    return value == rhs;
  }

  template<typename E>
  inline bool operator != (const E& rhs) const{
    return value != rhs;
  }

  template<typename E>
  inline bool operator >= (const E& rhs) const{
    return value >= rhs;
  }

  template<typename E>
  inline bool operator > (const E& rhs) const{
    return value > rhs;
  }

  template<typename E>
  inline bool operator <= (const E& rhs) const{
    return value <= rhs;
  }

  template<typename E>
  inline bool operator < (const E& rhs) const{
    return value < rhs;
  }

  template<typename E>
  inline T& operator += (const E& rhs) {
    return value += rhs;
  }

  template<typename E>
  inline T& operator -= (const E& rhs) {
    return value -= rhs;
  }

  template<typename E>
  inline T& operator *= (const E& rhs) {
    return value *= rhs;
  }

  template<typename E>
  inline T& operator /= (const E& rhs) {
    return value /= rhs;
  }

  template<typename E>
  inline T& operator %= (const E& rhs) {
    return value %= rhs;
  }

  template<typename E>
  inline T& operator &= (const E& rhs) {
    return value &= rhs;
  }

  template<typename E>
  inline T& operator |= (const E& rhs) {
    return value |= rhs;
  }

  template<typename E>
  inline T& operator ^= (const E& rhs) {
    return value ^= rhs;
  }

  template<typename E>
  inline T operator + (const E& rhs) {
    return value + rhs;
  }

  template<typename E>
  inline T operator - (const E& rhs) {
    return value - rhs;
  }

  template<typename E>
  inline T operator * (const E& rhs) {
    return value * rhs;
  }

  template<typename E>
  inline T operator / (const E& rhs) {
    return value / rhs;
  }

  template<typename E>
  inline T operator & (const E& rhs) {
    return value & rhs;
  }

  template<typename E>
  inline T operator | (const E& rhs) {
    return value | rhs;
  }

  template<typename E>
  inline T operator ^ (const E& rhs) {
    return value ^ rhs;
  }

  template<typename E>
  inline T operator ~ () {
    return ~value;
  }

  template<typename E>
  inline T operator >> (const E& rhs) {
    return value >> rhs;
  }

  template<typename E>
  inline T operator << (const E& rhs) {
    return value << rhs;
  }
  T value;
  typedef T ValueType;
};

///---------------------------------------------------------------------
/// A IntegerProperty is a helper class for Component with only one
/// property of any type.
///
/// it implements standard operators:  ++  --
///---------------------------------------------------------------------
template<typename T>
struct IntegerProperty : StandardProperty<T> {
  IntegerProperty(){}

  IntegerProperty(const T& value) : StandardProperty<T>(value){}

  inline T& operator -- () {
    --this->value;
    return this->value;
  }

  inline T operator -- (int) {
    T copy = *this;
    operator--();
    return copy;
  }

  inline T& operator ++ () {
    ++this->value;
    return this->value;
  }

  inline T operator ++ (int) {
    T copy = *this;
    operator++();
    return copy;
  }
};

} // namespace details

///---------------------------------------------------------------------
/// A Property is a sample Component with only one property of any type
///---------------------------------------------------------------------
///
/// A Property is a sample Component with only one property of any type.
/// it implements standard constructors, type conversations and
/// operators which might be useful.
///
///---------------------------------------------------------------------
template<typename T>
class Property : public std::conditional<std::is_integral<T>::value,
                                         details::IntegerProperty<T>,
                                         details::StandardProperty<T>>::type {
  typedef typename
  std::conditional<std::is_integral<T>::value,
                   details::IntegerProperty<T>,
                   details::StandardProperty<T>>
  ::type Base;
 public:
  Property(){}
  Property(const T& value) : Base(value){}
};

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
class EntityManager : details::forbid_copies {
 public:
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
  class Entity;

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
  class EntityAlias;

  ///---------------------------------------------------------------------
  /// Iterator is an iterator for iterating through the entity manager
  ///---------------------------------------------------------------------
  template<typename T>
  class Iterator;

  ///---------------------------------------------------------------------
  /// Helper class that is used to access the iterator
  ///---------------------------------------------------------------------
  /// @usage Calling entities.with<Components>(), returns a view
  ///        that can be used to access the iterator with begin() and
  ///        end() that iterates through all entities with specified
  ///        Components
  ///---------------------------------------------------------------------
  template<typename T>
  class View;

 private:
  ///---------------------------------------------------------------------
  /// ComponentMask is a mask defining what components and entity has.
  ///---------------------------------------------------------------------
  ///
  ///---------------------------------------------------------------------
  typedef std::bitset<ECS_MAX_NUM_OF_COMPONENTS> ComponentMask;

  ///---------------------------------------------------------------------
  /// Helper class
  ///---------------------------------------------------------------------
  class BaseEntityAlias;

  ///---------------------------------------------------------------------
  /// Helper class
  ///---------------------------------------------------------------------
  class BaseManager;

  ///---------------------------------------------------------------------
  /// Helper class,  This is the main class for holding many Component of
  /// a specified type.
  ///---------------------------------------------------------------------
  template<typename T>
  class ComponentManager;

  ///////////////////////////////////////////////////////////////////////
  ///
  /// ----------------------Implementation-------------------------------
  ///
  ///////////////////////////////////////////////////////////////////////
 private:
  class BaseManager{
   public:
    virtual ~BaseManager() {};
    virtual void remove(index_t index) = 0;
    virtual ComponentMask mask() = 0;
  };

  template<typename C>
  class ComponentManager : public BaseManager, details::forbid_copies {
   public:
    ComponentManager(EntityManager& manager, size_t chunk_size = ECS_DEFAULT_CHUNK_SIZE) :
        manager_(manager),
        pool_(chunk_size){}

    /// Allocate and create at specific index, using constructor
    template<typename ...Args>
    auto create(index_t index, Args && ... args) ->
    typename std::enable_if<std::is_constructible<C,Args...>::value, C&>::type {
      pool_.ensure_min_size(index + 1);
      new(get_ptr(index)) C(std::forward<Args>(args)...);
      return get(index);
    }

    template<typename ...Args>
    auto create(index_t index, Args && ... args) ->
    typename std::enable_if<
        !std::is_constructible<C,Args...>::value &&
            !std::is_base_of<details::BaseProperty, C>::value, C&>::type {
      pool_.ensure_min_size(index + 1);
      new(get_ptr(index)) C{std::forward<Args>(args)...};
      return get(index);
    }


    template<typename ...Args>
    auto create(index_t index, Args && ... args) ->
    typename std::enable_if<
        !std::is_constructible<C,Args...>::value &&
            std::is_base_of<details::BaseProperty, C>::value, C&>::type {
      static_assert(sizeof...(Args) <= 1 , ECS_ASSERT_MSG_ONLY_ONE_ARGS_PROPERTY_CONSTRUCTOR);
      pool_.ensure_min_size(index + 1);
      new(get_ptr(index)) typename C::ValueType(std::forward<Args>(args)...);
      return get(index);
    }

    inline void remove(index_t index){
      pool_.destroy(index);
      manager_.mask(index).reset(component_index<C>());
    }

    inline C& operator[] (index_t index){
      return get(index);
    }

    inline C& get(index_t index){
      return *get_ptr(index);
    }

    inline C const & get(index_t index) const {
      return *get_ptr(index);
    }
   private:
    inline C* get_ptr (index_t index){
      return pool_.get_ptr(index);
    }

    inline C const * get_ptr (index_t index) const {
      return pool_.get_ptr(index);
    }

    inline ComponentMask mask(){
      return component_mask<C>();
    }

    EntityManager &manager_;
    details::Pool<C> pool_;

    friend class EntityManager;
  }; //ComponentManager

 public:
  class Entity{
   public:
    class Id {
     public:
      Id(){}

      Id(index_t index, version_t version) : index_(index), version_(version){}

      inline bool operator == (const Id & rhs) const {
        return index_ == rhs.index_ && version_ == rhs.version_;
      }

      inline bool operator != (const Id & rhs) const {
        return index_ != rhs.index_ || version_ != rhs.version_;
      }

      inline index_t index(){
        return index_;
      }

      inline index_t index() const {
        return index_;
      }

      inline version_t version(){
        return version_;
      }

      inline version_t version() const {
        return version_;
      }

     private:
      index_t index_;
      version_t version_;
      friend class Entity;
      friend class EntityManager;
    };

    Entity(EntityManager * manager, Id id) :
        manager_(manager),
        id_(id){
    }

    Entity(const Entity& other) :
        Entity(other.manager_, other.id_){
    }

    Id & id(){
      return id_;
    };

    Id const & id() const{
      return id_;
    };

    inline Entity& operator = (const Entity& rhs){
      manager_ = rhs.manager_;
      id_ = rhs.id_;
      return *this;
    }

    inline bool operator ==(const Entity& rhs) const {
      return id_ == rhs.id_;
    }

    inline bool operator != (const Entity& rhs) const {
      return id_ != rhs.id_;
    }

    /// Returns the requested component, or error if it doesn't exist
    template<typename C>
    inline C& get(){
      return manager_->get_component<C>(*this);
    }

    template<typename C>
    inline C const & get() const{
      return manager_->get_component<C>(*this);
    }

    /// Set the requested component, if old component exist,
    /// a new one is created. Otherwise, the assignment operator
    /// is used.
    template<typename C, typename ... Args>
    inline C& set(Args && ... args){
      return manager_->set_component<C>(*this, std::forward<Args>(args) ...);
    }

    /// Add the requested component, error if component of the same type exist already
    template<typename C, typename ... Args>
    inline C& add(Args && ... args){
      return manager_->create_component<C>(*this, std::forward<Args>(args) ...);
    }

    /// Access this Entity as an EntityAlias.
    template<typename T>
    inline T& as(){
      ECS_ASSERT_IS_ENTITY(T);
      ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
      ECS_ASSERT(has(T::mask()), "Entity doesn't have required components for this EntityAlias");
      return reinterpret_cast<T&>(*this);
    }

    template<typename T>
    inline T const & as() const {
      ECS_ASSERT_IS_ENTITY(T);
      ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
      ECS_ASSERT(has(T::mask()), "Entity doesn't have required components for this EntityAlias");
      return reinterpret_cast<T const &>(*this);
    }

    /// Assume that this entity has provided Components
    /// Use for faster component access calls
    template<typename ...Components>
    inline EntityAlias<Components...>& assume(){
      return as<EntityAlias<Components...>>();
    }

    template<typename ...Components>
    inline EntityAlias<Components...> const & assume() const{
      return as<EntityAlias<Components...>>();
    }

    /// Removes a component. Error of it doesn't exist
    template<typename C>
    inline void remove(){
      manager_->remove_component<C>(*this);
    }

    /// Removes all components and call destructors
    inline void remove_everything(){
      manager_->remove_all_components(*this);
    }

    /// Clears the component mask without destroying components (faster than remove_everything)
    inline void clear_mask(){
      manager_->clear_mask(*this);
    }

    /// Destroys this entity. Removes all components as well
    inline void destroy(){
      manager_->destroy(*this);
    }
    /// Return true if entity has all specified components. False otherwise
    template<typename... Components>
    inline bool has(){
      return manager_->has_component<Components ...>(*this);
    }

    template<typename... Components>
    inline bool has() const{
      return manager_->has_component<Components ...>(*this);
    }

    /// Returns whether an entity is an entity alias or not
    template<typename T>
    inline bool is(){
      ECS_ASSERT_IS_ENTITY(T);
      ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
      return has(T::mask());
    }

    template<typename T>
    inline bool is() const{
      ECS_ASSERT_IS_ENTITY(T);
      ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
      return has(T::mask());
    }

    /// Returns true if entity has not been destroyed. False otherwise
    inline bool is_valid(){
      return manager_->is_valid(*this);
    }

    inline bool is_valid() const{
      return manager_->is_valid(*this);
    }

    template <typename ...Components>
    std::tuple<Components& ...> unpack() {
      return std::forward_as_tuple(get<Components>()...);
    }

    template <typename ...Components>
    std::tuple<Components const & ...> unpack() const{
      return std::forward_as_tuple(get<Components>()...);
    }

   private:

    /// Return true if entity has all specified compoents. False otherwise
    inline bool has(ComponentMask& check_mask){
      return manager_->has_component(*this, check_mask);
    }
    inline bool has(const ComponentMask& check_mask) const {
      return manager_->has_component(*this, check_mask);
    }

    inline ComponentMask & mask(){
      return manager_->mask(*this);
    }

    inline ComponentMask const & mask() const{
      return manager_->mask(*this);
    }
    EntityManager* manager_;
    Id id_;
    friend class EntityManager;
  }; //Entity

  template<typename T>
  class Iterator : public std::iterator<std::input_iterator_tag, typename std::remove_reference<T>::type>{
    typedef typename std::remove_reference<typename std::remove_const<T>::type>::type T_no_ref;
   public:

    Iterator(EntityManager* manager, ComponentMask mask, bool begin = true) :
        manager_(manager),
        mask_(mask){
      // Must be pool size because of potential holes
      size_ = manager_->entity_versions_.size();
      if(!begin) {
        cursor_ = size_;
      }
      find_next();
    };

    Iterator(const Iterator& it) : Iterator(it.manager_, it.cursor_) {};

    Iterator& operator = (const Iterator& rhs) {
      manager_ = rhs.manager_;
      cursor_ = rhs.cursor_;
      return *this;
    }

    Iterator& operator ++(){
      ++cursor_;
      find_next();
      return *this;
    }

    bool operator==(const Iterator& rhs) const {
      return cursor_ == rhs.cursor_;
    }

    bool operator!=(const Iterator& rhs) const {
      return !(*this == rhs);
    }

    inline T operator*() {
      return entity().template as<T_no_ref>();
    }

    inline T const & operator*() const {
      return entity().template as<T_no_ref>();
    }

    inline index_t index() const {
      return cursor_;
    }

   private:
    inline void find_next(){
      while (cursor_ < size_ && (manager_->component_masks_[cursor_] & mask_) != mask_){
        ++cursor_;
      }
    }

    inline Entity entity() {
      return manager_->get_entity(index());
    }

    inline const Entity entity() const {
      return manager_->get_entity(index());
    }

    EntityManager *manager_;
    ComponentMask mask_;
    size_t cursor_ = 0;
    size_t size_;
  }; //Iterator

  template<typename T>
  class View{
    ECS_ASSERT_IS_ENTITY(T)
   public:
    typedef Iterator<T> iterator;
    typedef Iterator<T const &> const_iterator;

    View(EntityManager* manager, ComponentMask mask) :
        manager_(manager),
        mask_(mask){}

    iterator begin(){
      return iterator(manager_, mask_, true);
    }

    iterator end(){
      return iterator(manager_, mask_, false);
    }

    const_iterator begin() const{
      return const_iterator(manager_, mask_, true);
    }

    const_iterator end() const{
      return const_iterator(manager_, mask_, false);
    }

    inline index_t count(){
      index_t count = 0;
      for (auto it = begin(); it != end(); ++it) {
        ++count;
      }
      return count;
    }

    template<typename ...Components>
    View<T> with(){
      return View<T>(manager_, component_mask <Components...>() | mask_);
    }

   protected:
    EntityManager* manager_;
    ComponentMask mask_;
    friend class EntityManager;
  }; //View

 private:
  class BaseEntityAlias {
   public:
    BaseEntityAlias(Entity& entity) : entity_(entity){}
   protected:
    BaseEntityAlias(){}
    BaseEntityAlias(const BaseEntityAlias & other) : entity_(other.entity_){}

    EntityManager& entities() { return *manager_; }
   private:
    union{
      EntityManager* manager_;
      Entity entity_;
    };
    template<typename ... Components>
    friend class EntityAlias;
  }; //BaseEntityAlias


  // Class for accessing where to put entities with specific components.
  struct IndexAccessor {
    // Used to store next available index there is within each block
    std::vector<index_t> last_index;
    // Used to store all indexes that are free
    std::vector<index_t> free_list;
  };

 public:
  template<typename ...Components>
  class EntityAlias : public BaseEntityAlias {
   private:

    /// Underlying EntityAlias. Used for creating Entity alias without
    /// a provided constructor
    typedef EntityAlias<Components...> Type;

    template<typename C>
    using is_component = details::is_type<C, Components...>;

   public:
    EntityAlias(Entity& entity) : BaseEntityAlias(entity){}

    inline operator Entity & () {
      return entity_;
    }

    inline operator Entity const & () const {
      return entity_;
    }

    Entity::Id & id(){
      return entity_.id();
    };

    Entity::Id const & id() const{
      return entity_.id();
    };

    inline bool operator ==(const Entity& rhs) const {
      return entity_ == rhs;
    }

    inline bool operator != (const Entity& rhs) const {
      return entity_ != rhs;
    }

    /// Returns the requested component, or error if it doesn't exist
    template<typename C>
    inline typename std::enable_if<is_component<C>::value, C&>::type get(){
      return manager_->get_component_fast<C>(entity_);
    }

    template<typename C>
    inline typename std::enable_if<is_component<C>::value, C const &>::type get() const{
      return manager_->get_component_fast<C>(entity_);
    }

    template<typename C>
    inline typename std::enable_if<!is_component<C>::value, C&>::type get(){
      return entity_.get<C>();
    }

    template<typename C>
    inline typename std::enable_if<!is_component<C>::value, C const &>::type get() const{
      return entity_.get<C>();
    }

    /// Set the requested component, if old component exist,
    /// a new one is created. Otherwise, the assignment operator
    /// is used.
    template<typename C, typename ... Args>
    inline typename std::enable_if<is_component<C>::value, C&>::type set(Args && ... args){
      return manager_->set_component_fast<C>(entity_, std::forward<Args>(args)...);
    }

    template<typename C, typename ... Args>
    inline typename std::enable_if<!is_component<C>::value, C&>::type set(Args && ... args){
      return manager_->set_component<C>(entity_, std::forward<Args>(args)...);
    }

    /// Add the requested component, error if component of the same type exist already
    template<typename C, typename ... Args>
    inline C& add(Args && ... args){
      return entity_.add<C>(std::forward<Args>(args)...);
    }

    /// Access this Entity as an EntityAlias.
    template<typename T>
    inline T& as(){
      return entity_.as<T>();
    }

    template<typename T>
    inline T const & as() const{
      return entity_.as<T>();
    }

    /// Assume that this entity has provided Components
    /// Use for faster component access calls
    template<typename ...Components_>
    inline EntityAlias<Components_...>& assume(){
      return entity_.assume<Components_...>();
    }

    template<typename ...Components_>
    inline EntityAlias<Components_...> const & assume() const{
      return entity_.assume<Components_...>();
    }

    /// Removes a component. Error of it doesn't exist. Cannot remove dependent components
    template<typename C>
    inline typename std::enable_if<!is_component<C>::value, void>::type remove(){
      entity_.remove<C>();
    }

    template<typename C>
    inline typename std::enable_if<is_component<C>::value, void>::type remove(){
      manager_->remove_component_fast<C>(entity_);
    }

    /// Removes all components and call destructors
    inline void remove_everything(){
      entity_.remove_everything();
    }

    /// Clears the component mask without destroying components (faster than remove_everything)
    inline void clear_mask(){
      entity_.clear_mask();
    }

    /// Destroys this entity. Removes all components as well
    inline void destroy(){
      entity_.destroy();
    }
    /// Return true if entity has all specified components. False otherwise
    template<typename... Components_>
    inline bool has(){
      return entity_.has<Components_...>();
    }

    template<typename... Components_>
    inline bool has() const{
      return entity_.has<Components_...>();
    }

    /// Returns whether an entity is an entity alias or not
    template<typename T>
    inline bool is(){
      return entity_.is<T>();
    }

    template<typename T>
    inline bool is() const{
      return entity_.is<T>();
    }

    /// Returns true if entity has not been destroyed. False otherwise
    inline bool is_valid(){
      return entity_.is_valid();
    }

    inline bool is_valid() const{
      return entity_.is_valid();
    }

    template <typename ...Components_>
    std::tuple<Components_& ...> unpack() {
      return entity_.unpack<Components_...>();
    }

    template <typename ...Components_>
    std::tuple<Components_ const & ...> unpack() const {
      return entity_.unpack<Components_...>();
    }

    std::tuple<Components& ...> unpack() {
      return entity_.unpack<Components...>();
    }

    std::tuple<Components const & ...> unpack() const {
      return entity_.unpack<Components...>();
    }

   protected:
    EntityAlias(){}

   private:
    // Recursion init components with argument
    template<typename C0, typename Arg>
    inline void init_components(Arg arg){
      add<C0>(arg);
    }

    template<typename C0, typename C1, typename ... Cs, typename Arg0, typename Arg1, typename ... Args>
    inline void init_components(Arg0 arg0, Arg1 arg1, Args... args){
      init_components<C0>(arg0);
      init_components<C1, Cs...>(arg1, args...);
    }
    // Recursion init components without argument
    template<typename C>
    inline void init_components(){
      add<C>();
    }

    template<typename C0, typename C1, typename ... Cs>
    inline void init_components(){
      init_components<C0>();
      init_components<C1, Cs...>();
    }

    template<typename ... Args>
    void init(Args... args){
      init_components<Components...>(args...);
    }

    static ComponentMask mask(){
      return component_mask <Components...>();
    }


    friend class EntityManager;
    friend class Entity;
  }; //EntityAlias


  EntityManager(size_t chunk_size = 8192) {
    entity_versions_.reserve(chunk_size);
    component_masks_.reserve(chunk_size);
  }

  ~EntityManager(){
    for (BaseManager* manager : component_managers_) {
      if(manager) delete manager;
    }
    component_managers_.clear();
    component_masks_.clear();
    entity_versions_.clear();
    next_free_indexes_.clear();
    component_mask_to_index_accessor_.clear();
    index_to_component_mask.clear();
  }

  // Cretate a new Entity and return handle
  Entity create(){
    return create_with_mask(ComponentMask(0));
  }

  std::vector<Entity> create(const size_t num_of_entities){
    return create_with_mask(ComponentMask(0), num_of_entities);
  }

  /// If EntityAlias is constructable with Args...
  template<typename T, typename ...Args>
  auto create(Args... args) ->
  typename std::enable_if<std::is_constructible<T, Args...>::value, T>::type{
    ECS_ASSERT_IS_ENTITY(T);
    ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
    ComponentMask mask = T::mask();
    Entity entity = create_with_mask(mask);
    T* entity_alias = new(&entity) T(std::forward<Args>(args)...);
    ECS_ASSERT(entity.has(mask),
               "Every required component must be added when creating an Entity Alias");
    return *entity_alias;
  }

  /// If EntityAlias is not constructable with Args...
  /// Attempt to create with underlying EntityAlias
  template<typename T, typename ...Args>
  auto create(Args... args) ->
  typename std::enable_if<!std::is_constructible<T, Args...>::value, T>::type{
    ECS_ASSERT_IS_ENTITY(T);
    ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
    typedef typename T::Type Type;
    Entity entity = create_with_mask(T::mask());
    Type* entity_alias = new(&entity) Type();
    entity_alias->init(std::forward<Args>(args)...);
    ECS_ASSERT(entity.has(T::mask()),
               "Every required component must be added when creating an Entity Alias");
    return *reinterpret_cast<T*>(entity_alias);
  }

  template<typename ...Components, typename ...Args>
  EntityAlias<Components...> create_with(Args... args){
    typedef EntityAlias<Components...> Type;
    Entity entity = create_with_mask(component_mask<Components...>());
    Type* entity_alias = new(&entity) Type();
    entity_alias->init(std::forward<Args>(args)...);
    return *entity_alias;
  }

  template<typename ...Components>
  EntityAlias<Components...> create_with(){
    typedef EntityAlias<Components...> Type;
    Entity entity = create_with_mask(component_mask<Components...>());
    Type* entity_alias = new(&entity) Type();
    entity_alias->init();
    return *entity_alias;
  }

  // Access a View of all entities with specified components
  template<typename ...Components>
  View<EntityAlias<Components...>> with(){
    ComponentMask mask = component_mask <Components...>();
    return View<EntityAlias<Components...>>(this, mask);
  }

  // Iterate through all entities with all components, specified as lambda parameters
  // example: entities.with([] (Position& pos) {  });
  template<typename T>
  void with(T lambda){
    ECS_ASSERT_IS_CALLABLE(T);
    with_<T>::for_each(*this, lambda);
  }

  // Access all entities with
  template<typename T>
  View<T> fetch_every(){
    ECS_ASSERT_IS_ENTITY(T);
    return View<T>(this, T::mask());
  }

  template<typename T>
  void fetch_every(T lambda){
    ECS_ASSERT_IS_CALLABLE(T);
    typedef details::function_traits<T> function;
    static_assert(function::arg_count == 1, "Lambda or function must only have one argument");
    typedef typename function::template arg_remove_ref<0> entity_interface_t;
    for(entity_interface_t entityInterface : fetch_every<entity_interface_t>()){
      lambda(entityInterface);
    }
  }

  inline Entity operator[] (index_t index){
    return get_entity(index);
  }

  inline Entity operator[] (Entity::Id id){
    Entity entity = get_entity(id);
    ECS_ASSERT(id == entity.id(), "Id is no longer valid (Entity was destroyed)");
    return entity;
  }

  inline size_t count() {
    return count_;
  }

 private:
  template<size_t N, typename...>
  struct with_t;

  template<size_t N, typename Lambda, typename... Args>
  struct with_t<N, Lambda, Args...> :
      with_t<N - 1, Lambda,
             typename details::function_traits<Lambda>::template arg_remove_ref<N - 1> ,Args...>{};

  template<typename Lambda, typename... Args>
  struct with_t<0, Lambda, Args...>{
    static inline void for_each(EntityManager& manager, Lambda lambda){
      typedef details::function_traits<Lambda> function;
      static_assert(function::arg_count > 0, "Lambda or function must have at least 1 argument.");
      auto view = manager.with<Args...>();
      auto it = view.begin();
      auto end = view.end();
      for(; it != end; ++it){
        lambda(get_arg<Args>(manager, it.index())...);
      }
    }

    //When arg is component, access component
    template<typename C>
    static inline auto get_arg(EntityManager& manager, index_t index) ->
    typename std::enable_if<!std::is_same<C, Entity>::value, C&>::type{
      return manager.get_component_fast<C>(index);
    }

    //When arg is the Entity, access the Entity
    template<typename C>
    static inline auto get_arg(EntityManager& manager, index_t index) ->
    typename std::enable_if<std::is_same<C, Entity>::value, Entity>::type{
      return manager.get_entity(index);
    }
  };

  template<typename Lambda>
  using with_ = with_t<details::function_traits<Lambda>::arg_count, Lambda>;

  Entity create_with_mask(ComponentMask mask){
    ++count_;
    index_t index = find_new_entity_index(mask);
    size_t slots_required = index + 1;
    if(entity_versions_.size() < slots_required){
      entity_versions_.resize(slots_required);
      component_masks_.resize(slots_required, ComponentMask(0));
    }
    return get_entity(index);
  }

  std::vector<Entity> create_with_mask(ComponentMask mask, const size_t num_of_entities){
    std::vector<Entity> new_entities;
    index_t index;
    size_t entities_left = num_of_entities;
    new_entities.reserve(entities_left);
    auto mask_as_ulong = mask.to_ulong();
    IndexAccessor &index_accessor = component_mask_to_index_accessor_[mask_as_ulong];
    //See if we can use old indexes for destroyed entities via free list
    while(!index_accessor.free_list.empty() && entities_left--){
      new_entities.push_back(get_entity(index_accessor.free_list.back()));
      index_accessor.free_list.pop_back();
    }
    index_t block_index = 0;
    index_t current    = ECS_CACHE_LINE_SIZE; // <- if empty, create new block instantly
    size_t slots_required;
    // Are there any unfilled blocks?
    if(!index_accessor.last_index.empty()){
      block_index = index_accessor.last_index[index_accessor.last_index.size() - 1];
      current     = next_free_indexes_[block_index];
      slots_required = block_count_ * ECS_CACHE_LINE_SIZE + current + entities_left;
    } else{
      slots_required = block_count_ * ECS_CACHE_LINE_SIZE + entities_left;
    }
    entity_versions_.resize(slots_required);
    component_masks_.resize(slots_required, ComponentMask(0));

    // Insert until no entity is left or no block remain
    while(entities_left){
      for (;current < ECS_CACHE_LINE_SIZE && entities_left; ++current) {
        new_entities.push_back(get_entity(current + ECS_CACHE_LINE_SIZE * block_index));
        entities_left--;
      }
      // Add more blocks if there are entities left
      if(entities_left){
        block_index = block_count_;
        create_new_block(index_accessor, mask_as_ulong, 0);
        ++block_count_;
        current = 0;
      }
    }
    count_ += num_of_entities;
    return new_entities;
  }


  // Find a proper index for a new entity with components
  index_t find_new_entity_index(ComponentMask mask){
    auto mask_as_ulong = mask.to_ulong();
    IndexAccessor &index_accessor = component_mask_to_index_accessor_[mask_as_ulong];
    //See if we can use old indexes for destroyed entities via free list
    if(!index_accessor.free_list.empty()){
      auto index = index_accessor.free_list.back();
      index_accessor.free_list.pop_back();
      return index;
    }
    // EntityManager has created similar entities already
    if(!index_accessor.last_index.empty()){
      //No free_indexes in free list (removed entities), find a new index
      //at last block, if that block has free slots
      auto& block_index = index_accessor.last_index[index_accessor.last_index.size() - 1];
      auto& current     = next_free_indexes_[block_index];
      // If block has empty slot, use it
      if(ECS_CACHE_LINE_SIZE > current){
        return (current++) + ECS_CACHE_LINE_SIZE * block_index;
      }
    }
    create_new_block(index_accessor, mask_as_ulong, 1);
    return (block_count_++) * ECS_CACHE_LINE_SIZE;
  }

  /// Create a new block for this entity type.
  void create_new_block(IndexAccessor& index_accessor, unsigned long mask_as_ulong, index_t next_free_index){
    index_accessor.last_index.push_back(block_count_);
    next_free_indexes_.resize(block_count_ + 1);
    index_to_component_mask.resize(block_count_ + 1);
    next_free_indexes_[block_count_] = next_free_index;
    index_to_component_mask[block_count_] = mask_as_ulong;
  }

  //C1 should not be Entity
  template<typename C>
  static inline auto component_mask() -> typename
  std::enable_if<!std::is_same<C, Entity>::value, ComponentMask>::type{
    ComponentMask mask = ComponentMask((1UL << component_index<C>()));
    return mask;
  }

  //When C1 is Entity, ignore
  template<typename C>
  static inline auto component_mask() -> typename
  std::enable_if<std::is_same<C, Entity>::value, ComponentMask>::type{
    return ComponentMask(0);
  }

  //recursive function for component_mask creation
  template<typename C1, typename C2, typename ...Cs>
  static inline auto component_mask() -> typename
  std::enable_if<!std::is_same<C1, Entity>::value, ComponentMask>::type{
    ComponentMask mask = component_mask<C1>() | component_mask<C2, Cs...>();
    return mask;
  }

  template<typename C>
  static size_t component_index(){
    static size_t index = inc_component_counter();
    return index;
  }

  static size_t inc_component_counter(){
    size_t index = component_counter()++;
    ECS_ASSERT(index < ECS_MAX_NUM_OF_COMPONENTS, "maximum number of components exceeded.");
    return index;
  }

  static size_t& component_counter(){
    static size_t counter = 0;
    return counter;
  }

  template<typename C, typename ...Args>
  ComponentManager<C>& create_component_manager(Args && ... args){
    ComponentManager<C>* ptr = new ComponentManager<C>(std::forward<EntityManager&>(*this),
                                                       std::forward<Args>(args) ...);
    component_managers_[component_index<C>()] = ptr;
    return *ptr;
  }

  template<typename C>
  inline ComponentManager<C>& get_component_manager_fast(){
    return *reinterpret_cast<ComponentManager<C>*>(component_managers_[component_index<C>()]);
  }

  template<typename C>
  inline ComponentManager<C> const & get_component_manager_fast() const {
    return *reinterpret_cast<ComponentManager<C>*>(component_managers_[component_index<C>()]);
  }

  template<typename C>
  inline ComponentManager<C>& get_component_manager(){
    auto index = component_index<C>();
    if(component_managers_.size() <= index){
      component_managers_.resize(index + 1, nullptr);
      return create_component_manager<C>();
    } else if(component_managers_[index] == nullptr){
      return create_component_manager<C>();
    }
    return *reinterpret_cast<ComponentManager<C>*>(component_managers_[index]);
  }

  template<typename C>
  inline ComponentManager<C> const & get_component_manager() const{
    auto index = component_index<C>();
    ECS_ASSERT(component_managers_.size() > index, component_managers_[index] == nullptr &&
        "Component manager not created");
    return *reinterpret_cast<ComponentManager<C>*>(component_managers_[index]);
  }

  template<typename C>
  inline C& get_component(Entity& entity){
    ECS_ASSERT(has_component<C>(entity), "Entity doesn't have this component attached");
    return get_component_manager<C>().get(entity.id_.index_);
  }

  template<typename C>
  inline C const & get_component(Entity const & entity) const{
    ECS_ASSERT(has_component<C>(entity), "Entity doesn't have this component attached");
    return get_component_manager<C>().get(entity.id_.index_);
  }

  template<typename C>
  inline C& get_component_fast(index_t index){
    return get_component_manager_fast<C>().get(index);
  }

  template<typename C>
  inline C const & get_component_fast(index_t index) const {
    return get_component_manager_fast<C>().get(index);
  }

  /// Get component fast, no error checks. Use if it is already known that Entity has Component
  template<typename C>
  inline C& get_component_fast(Entity& entity){
    return get_component_manager_fast<C>().get(entity.id_.index_);
  }

  template<typename C>
  inline C const & get_component_fast(Entity const & entity) const{
    return get_component_manager_fast<C>().get(entity.id_.index_);
  }

  /// Use to create a component tmp that is assignable. Call the right constructor
  template<typename C, typename ...Args>
  static auto create_tmp_component(Args && ... args) ->
  typename std::enable_if<std::is_constructible<C,Args...>::value, C>::type {
    return C(std::forward<Args>(args) ...);
  }

  /// Use to create a component tmp that is assignable. Call the right constructor
  template<typename C, typename ...Args>
  static auto create_tmp_component(Args && ... args) ->
  typename std::enable_if<
      !std::is_constructible<C,Args...>::value &&
          !std::is_base_of<details::BaseProperty, C>::value,
      C>::type {
    return C{std::forward<Args>(args) ...};
  }

  /// Use to create a component tmp that is assignable. Call the right constructor
  /// Called when component is a property, and no constructor inaccessible.
  template<typename C, typename ...Args>
  static auto create_tmp_component(Args && ... args) ->
  typename std::enable_if<
      !std::is_constructible<C,Args...>::value &&
          std::is_base_of<details::BaseProperty, C>::value,
      C>::type {
    static_assert(sizeof...(Args) == 1 , ECS_ASSERT_MSG_ONLY_ONE_ARGS_PROPERTY_CONSTRUCTOR);
    static_assert(sizeof(C) == sizeof(std::tuple<Args...>),
                  "Cannot initilize component property. Please provide a constructor");
    auto tmp = typename C::ValueType(std::forward<Args>(args)...);
    return *reinterpret_cast<C*>(&tmp);
  }

  template<typename C, typename ...Args>
  inline C& create_component(Entity &entity, Args &&... args){
    ECS_ASSERT_VALID_ENTITY(entity);
    ECS_ASSERT(!has_component<C>(entity), "Entity already has this component attached");
    C& component = get_component_manager<C>().create(entity.id_.index_, std::forward<Args>(args) ...);
    entity.mask().set(component_index<C>());
    return component;
  }

  template<typename C>
  inline void remove_component(Entity& entity){
    ECS_ASSERT_VALID_ENTITY(entity);
    ECS_ASSERT(has_component<C>(entity), "Entity doesn't have component attached");
    get_component_manager<C>().remove(entity.id_.index_);
  }

  template<typename C>
  inline void remove_component_fast(Entity& entity){
    ECS_ASSERT_VALID_ENTITY(entity);
    ECS_ASSERT(has_component<C>(entity), "Entity doesn't have component attached");
    get_component_manager_fast<C>().remove(entity.id_.index_);
  }

  /// Removes all components from a single entity
  inline void remove_all_components(Entity& entity){
    ECS_ASSERT_VALID_ENTITY(entity);
    for (auto componentManager : component_managers_) {
      if(componentManager && has_component(entity, componentManager->mask())) {
        componentManager->remove(entity.id_.index_);
      }
    }
  }

  inline void clear_mask(Entity &entity){
    ECS_ASSERT_VALID_ENTITY(entity);
    component_masks_[entity.id_.index_].reset();
  }

  template<typename C, typename ...Args>
  inline C& set_component(Entity& entity, Args && ... args){
    ECS_ASSERT_VALID_ENTITY(entity);
    if(entity.has<C>()){
      return get_component_fast<C>(entity) = create_tmp_component<C>(std::forward<Args>(args)...);
    }
    else return create_component<C>(entity, std::forward<Args>(args)...);
  }

  template<typename C, typename ...Args>
  inline C& set_component_fast(Entity& entity, Args && ... args){
    ECS_ASSERT_VALID_ENTITY(entity);
    ECS_ASSERT(entity.has<C>(), "Entity does not have component attached");
    return get_component_fast<C>(entity) = create_tmp_component<C>(std::forward<Args>(args)...);
  }

  inline bool has_component(Entity& entity, ComponentMask component_mask){
    ECS_ASSERT_VALID_ENTITY(entity);
    return (mask(entity) & component_mask) == component_mask;
  }

  inline bool has_component(Entity const & entity, ComponentMask const & component_mask) const{
    ECS_ASSERT_VALID_ENTITY(entity);
    return (mask(entity) & component_mask) == component_mask;
  }

  inline bool has_component(index_t index, ComponentMask& component_mask){
    return (mask(index) & component_mask) == component_mask;
  }

  template<typename ...Components>
  inline bool has_component(Entity& entity){
    return has_component(entity, component_mask<Components...>());
  }

  template<typename ...Components>
  inline bool has_component(Entity const & entity) const {
    return has_component(entity, component_mask<Components...>());
  }

  inline bool is_valid(Entity const &entity){
    return  entity.id_.index_ < entity_versions_.size() &&
        entity.id_.version_ == entity_versions_[entity.id_.index_];
  }

  inline bool is_valid(Entity const &entity) const{
    return  entity.id_.index_ < entity_versions_.size() &&
        entity.id_.version_ == entity_versions_[entity.id_.index_];
  }

  inline void destroy(Entity& entity){
    index_t index = entity.id().index_;
    remove_all_components(entity);
    ++entity_versions_[index];
    auto& mask_as_ulong = index_to_component_mask[index / ECS_CACHE_LINE_SIZE];
    component_mask_to_index_accessor_[mask_as_ulong].free_list.push_back(index);
    --count_;
  }

  inline ComponentMask& mask(Entity& entity){
    return mask(entity.id_.index_);
  }

  inline ComponentMask const & mask(Entity const & entity) const{
    return mask(entity.id_.index_);
  }

  inline ComponentMask& mask(index_t index){
    return component_masks_[index];
  }

  inline ComponentMask const & mask(index_t index) const{
    return component_masks_[index];
  }

  inline Entity get_entity(Entity::Id id){
    return Entity(this, id);
  }

  inline Entity get_entity(index_t index){
    return get_entity(Entity::Id(index, entity_versions_[index]));
  }

  inline void sortFreeList(){
    //std::sort(free_list_.begin(), free_list_.end());
  }

  inline size_t capacity() const {
    return entity_versions_.capacity();
  }

  std::vector<BaseManager*> component_managers_;
  std::vector<ComponentMask> component_masks_;
  std::vector<version_t> entity_versions_;
  std::vector<index_t> next_free_indexes_;
  std::vector<size_t> index_to_component_mask;
  std::map<size_t, IndexAccessor> component_mask_to_index_accessor_;

  index_t block_count_ = 0;
  index_t count_ = 0;

  template<typename T>
  friend class Iterator;
  friend class Entity;
  friend class BaseComponent;
};

class SystemManager : details::forbid_copies{
 private:
 public:
  class System{
   public:
    virtual ~System(){}
    virtual void update(float time) = 0;
   protected:
    EntityManager& entities(){
      return *manager_->entities_;
    }
   private:
    friend class SystemManager;
    SystemManager* manager_;
  };

  SystemManager(EntityManager& entities) : entities_(&entities){}

  ~SystemManager(){
    for(auto system : systems_){
      if(system != nullptr) delete system;
    }
    systems_.clear();
    order_.clear();
  }

  template<typename S, typename ...Args>
  S& add(Args &&... args){
    ECS_ASSERT_IS_SYSTEM(S);
    ECS_ASSERT(!exists<S>(), "System already exists");
    systems_.resize(system_index<S>() + 1);
    S* system = new S(std::forward<Args>(args) ...);
    system->manager_ = this;
    systems_[system_index<S>()] = system;
    order_.push_back(system_index<S>());
    return *system;
  }

  template<typename S>
  void remove(){
    ECS_ASSERT_IS_SYSTEM(S);
    ECS_ASSERT(exists<S>(), "System does not exist");
    delete systems_[system_index<S>()];
    systems_[system_index<S>()] = nullptr;
    for (auto it = order_.begin(); it != order_.end(); ++it) {
      if(*it == system_index<S>()){
        order_.erase(it);
      }
    }
  }

  void update(float time){
    for(auto index : order_){
      systems_[index]->update(time);
    }
  }

  template<typename S>
  inline bool exists(){
    ECS_ASSERT_IS_SYSTEM(S);
    return systems_.size() > system_index<S>() && systems_[system_index<S>()] != nullptr;
  }

 private:
  template<typename C>
  static size_t system_index(){
    static size_t index = system_counter()++;
    return index;
  }

  static size_t& system_counter(){
    static size_t counter = 0;
    return counter;
  }

  std::vector<System*> systems_;
  std::vector<size_t> order_;
  EntityManager* entities_;
};

template<typename ...Components>
using EntityAlias = EntityManager::EntityAlias<Components...>;
using Entity = EntityManager::Entity;
using System = SystemManager::System;
} // namespace ecs

///---------------------------------------------------------------------
/// This will allow a property to be streamed into a input and output
/// stream.
///---------------------------------------------------------------------
namespace std{

template<typename T>
ostream& operator<<(ostream& os, const ecs::Property<T>& obj) {
  return os << obj.value;
}

template<typename T>
istream& operator>>(istream& is, ecs::Property<T>& obj) {
  return is >> obj.value;
}


} //namespace std

///---------------------------------------------------------------------
/// This will enable properties to be added to a string
///---------------------------------------------------------------------
template<typename T>
std::string operator+ (const std::string& lhs, const ecs::Property<T>& rhs) { return lhs + rhs.value; }

template<typename T>
std::string operator+ (std::string&&      lhs, ecs::Property<T>&&      rhs) { return lhs + rhs.value; }

template<typename T>
std::string operator+ (std::string&&      lhs, const ecs::Property<T>& rhs) { return lhs + rhs.value; }

template<typename T>
std::string operator+ (const std::string& lhs, ecs::Property<T>&&      rhs) { return lhs + rhs.value; }