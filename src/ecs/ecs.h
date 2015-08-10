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
#include <exception>
#include <string>
#include <functional>
#include <assert.h>

#define ECS_ASSERT_IS_ENTITY(T)                                                             \
        static_assert(std::is_base_of<BaseEntityAlias, T>::value ||                         \
                  std::is_same<Entity, T>::value ,                                          \
        #T " does not inherit EntityInterface.");

#define ECS_ASSERT_ENTITY_CORRECT_SIZE(T)                                                   \
        static_assert(sizeof(BaseEntityAlias) == sizeof(T),                                 \
        #T " should not include new variables, add them as Components instead.");           \

#define ECS_ASSERT_VALID_ENTITY(E)                                                          \
        assert(valid(E) && "Entity is no longer valid");                                    \

#define ECS_ASSERT_IS_SYSTEM(S)                                                             \
            static_assert(std::is_base_of<System, S>::value,                                \
            "DirivedSystem must inherit System.");                                          \

#define ECS_MAX_NUM_OF_COMPONENTS 32
#define ECS_DEFAULT_CHUNK_SIZE 8192

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
        enum { arity = sizeof...(Args) };
        // arity is the number of arguments.

        typedef ReturnType return_type;

        template <size_t i>
        struct arg_t
        {
            typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
            // the i-th argument is equivalent to the i-th tuple element of a tuple
            // composed of those arguments.
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
    /// Any class that should not be able to copy itself inherit this class
    ///---------------------------------------------------------------------
    ///
    class NonCopyable {
    protected:
        NonCopyable() = default;
        ~NonCopyable() = default;
        NonCopyable(const NonCopyable &) = delete;
        NonCopyable &operator=(const NonCopyable &) = delete;
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
    class BasePool {
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
            assert(index < size_ && "Pool has not allocated memory for this index.");
            T *ptr = get_ptr(index);
            ptr->~T();
        }

        inline T *get_ptr(index_t index) {
            assert(index < capacity_ && "Pool has not allocated memory for this index.");
            return reinterpret_cast<T *>(chunks_[index / chunk_size_] + (index % chunk_size_) * element_size_);
        }

        const inline T *get_ptr(index_t index) const {
            assert(index < capacity_ && "Pool has not allocated memory for this index.");
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
    struct StandardProperty{
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

    public:
        T value;
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



    ///---------------------------------------------------------------------
    /// Determine whether a class overrides the ++ and -- operators
    ///---------------------------------------------------------------------
    template <typename T>
    class has_integer_operators
    {
        template <typename C> static char pp(decltype(&C::operator++)) ;
        template <typename C> static long pp(...);
        template <typename C> static char mm( decltype(&C::operator--) ) ;
        template <typename C> static long mm(...);

    public:
        enum { value = sizeof(pp<T>(0)) == sizeof(char)  &&
                       sizeof(mm<T>(0)) == sizeof(char)};
    };


    ///---------------------------------------------------------------------
    /// BaseProperty
    ///
    /// First template argument is property type. Second argument is if
    /// type does implement ++i i++ --i i-- operators.
    /// Use has_integer_operators to determine if this class should be used.
    ///---------------------------------------------------------------------
    template<typename T, bool>
    struct BaseProperty;

    template<typename T>
    struct BaseProperty<T, false> : details::StandardProperty<T> {
        BaseProperty(){}
        BaseProperty(const T& value) : details::StandardProperty<T>(value){}
    };

    template<typename T>
    struct BaseProperty<T, true> : details::IntegerProperty<T> {
        BaseProperty(){}
        BaseProperty(const T& value) : details::IntegerProperty<T>(value){}
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
    using Property = details::BaseProperty<T, details::has_integer_operators<T>::value>;

    ///---------------------------------------------------------------------
    /// This is the main class for holding all Entities and Components
    ///---------------------------------------------------------------------
    ///
    /// It uses 1 memory pool for holding entities, 1 pool for holding masks,
    /// and 1 memory pool for each additional component.
    ///
    /// A component is placed at the same index in the pool as the entity is
    /// placed.
    ///
    /// The Entity id is the same as the index. When an Entity is removed,
    /// the id is added to the free list. A free list knows where spaces are
    /// left open for new entities (to ensure no holes).
    ///
    ///---------------------------------------------------------------------
    class EntityManager : details::NonCopyable {
    public:
        ///---------------------------------------------------------------------
        /// Entity is the identifier of an identity
        ///---------------------------------------------------------------------
        ///
        /// An entity consists of an id and version. The version is used to
        /// ensure that new entites allocated with the same id are separable.
        ///
        /// An entity becomes invalid when destroyed.
        ///
        ///---------------------------------------------------------------------
        class Entity;

        ///---------------------------------------------------------------------
        /// EntityAlias is a wrapper around an Entity
        ///---------------------------------------------------------------------
        ///
        /// An entity interface makes modification of the entity and other
        /// entities much easier when performing actions. It acts soley as an
        /// abstraction layer between the entity and different actions.
        ///
        ///---------------------------------------------------------------------
        template<typename ...Components>
        class EntityAlias;

        ///---------------------------------------------------------------------
        /// Iterator is an iterator for iterating through the entity manager
        ///---------------------------------------------------------------------
        /// @usage Calling entities.with<Components>(), returns an iterator
        ///        that iterates through all entities with specified Components
        ///---------------------------------------------------------------------
        template<typename T>
        class Iterator;

        ///---------------------------------------------------------------------
        /// Helper class that is used to access the iterator
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
        class ComponentManager : public BaseManager, details::NonCopyable{
        public:
            ComponentManager(EntityManager& manager, size_t chunk_size = ECS_DEFAULT_CHUNK_SIZE) :
                    manager_(manager),
                    pool_(chunk_size){}

            /// Allocate and create at specific index
            template<typename ...Args>
            C& create(index_t index, Args && ... args){
                pool_.ensure_min_size(index + 1);
                new(get_ptr(index)) C(std::forward<Args>(args) ...);
                return get(index);
            }

            void remove(index_t index){
                pool_.destroy(index);
                manager_.mask(index).reset(component_index<C>());
            }

            inline C& operator[] (index_t index){
                return get(index);
            }

            inline C& get(index_t index){
                return *get_ptr(index);
            }
        private:
            inline C* get_ptr (index_t index){
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

                inline bool operator ==(const Id & rhs){
                    return index_ == rhs.index_ && version_ == rhs.version_;
                }

                inline bool operator != (const Id & rhs){
                    return index_ != rhs.index_ || version_ != rhs.version_;
                }

                inline index_t index(){
                    return index_;
                }

                inline version_t version(){
                    return version_;
                }

            private:
                index_t index_;
                version_t version_;
                friend class Entity;
                friend class EntityManager;
            };

            Entity(EntityManager* manager, Id id) :
                    manager_(manager),
                    id_(id){
            }

            Entity(const Entity& other) :
                    Entity(other.manager_, other.id_){
            }

            Id & id(){
                return id_;
            };

            inline Entity& operator =(const Entity& rhs){
                manager_ = rhs.manager_;
                id_ = rhs.id_;
                return *this;
            }

            inline bool operator ==(const Entity& rhs){
                return id_ == rhs.id_;
            }

            inline bool operator != (const Entity& rhs){
                return id_ != rhs.id_;
            }

            /// Returns the requested component, or error if it doesn't exist
            template<typename C>
            inline C& get(){
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
                assert(has(T::mask()) && "Entity doesn't have required components for this EntityAlias");
                return reinterpret_cast<T&>(*this);
            }

            /// Assume that this entity has provided Components
            /// Use for faster component access calls
            template<typename ...Components>
            inline EntityAlias<Components...>& assume(){
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

            /// Returns whether an entity is an entity alias or not
            template<typename T>
            inline bool is(){
                ECS_ASSERT_IS_ENTITY(T);
                ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
                return has(T::mask());
            }

            /// Returns true if entity has not been destroyed. False otherwise
            inline bool valid(){
                return manager_->valid(*this);
            }

            template <typename ...Components>
            std::tuple<Components& ...> unpack() {
                return std::forward_as_tuple(get<Components>()...);
            }

        private:

            /// Return true if entity has all specified compoents. False otherwise
            inline bool has(ComponentMask check_mask){
                return manager_->has_component(*this, check_mask);
            }
            inline ComponentMask& mask(){
                return manager_->mask(*this);
            }
            EntityManager* manager_;
            Id id_;
            friend class EntityManager;
        }; //Entity

        template<typename T>
        class Iterator : public std::iterator<std::input_iterator_tag, Entity::Id>{
        public:

            Iterator(EntityManager* manager, ComponentMask mask, bool begin = true) :
                manager_(manager),
                mask_(mask){
                // Must be pool size because of potential holes
                size_ = manager_->entity_versions_.size();
                if(!begin) cursor_ = size_;
                next();
            };

            Iterator(const Iterator& it) :
                manager_(it.manager_),
                cursor_(it.cursor_) {};

            Iterator& operator ++(){
                ++cursor_;
                next();
                return *this;
            }

            bool operator==(const Iterator& rhs) const {
                return cursor_ == rhs.cursor_;
            }

            bool operator!=(const Iterator& rhs) const {
                return cursor_ != rhs.cursor_;
            }

            inline T operator*() {
                return entity().template as<T>();
            }

            inline index_t index(){
                return cursor_;
            }

        private:
            void next(){
                //find next component with components
                while (cursor_ < size_ && (manager_->component_masks_[cursor_] & mask_) != mask_){
                    ++cursor_;
                }
            }

            inline Entity entity() {
                return manager_->get(index());
            }

            EntityManager *manager_;
            ComponentMask mask_;
            index_t cursor_ = 0;
            size_t size_;
        }; //Iterator

        template<typename T>
        class View{
            ECS_ASSERT_IS_ENTITY(T)
        public:
            View(EntityManager* manager, ComponentMask mask) :
                    manager_(manager),
                    mask_(mask){}

            Iterator<T> begin(){
                return Iterator<T>(manager_, mask_, true);
            }

            Iterator<T> end(){
                return Iterator<T>(manager_, mask_, false);
            }

            const Iterator<T> begin() const{
                return Iterator<T>(manager_, mask_, true);
            }

            const Iterator<T> end() const{
                return Iterator<T>(manager_, mask_, false);
            }

            inline index_t count(){
                index_t count = 0;
                for (auto it = begin(); it != end(); ++it) {
                    count ++;
                }
                return count;
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

            EntityManager& manager() { return *manager_; }
        private:
            union{
                EntityManager* manager_;
                Entity entity_;
            };
            template<typename ... Components>
            friend class EntityAlias;
        }; //BaseEntityAlias

    public:
        template<typename ...Components>
        class EntityAlias : public BaseEntityAlias {
        private:

            template<typename C>
            using is_component = details::is_type<C, Components...>;

        public:
            EntityAlias(Entity& entity) : BaseEntityAlias(entity){}

            inline operator Entity & () {
                return entity_;
            }

            Entity::Id & id(){
                return entity_.id();
            };

            inline Entity& operator = (const Entity& rhs){
                entity_ = rhs;
                return *this;
            }

            inline bool operator ==(const Entity& rhs){
                return entity_ == rhs;
            }

            inline bool operator != (const Entity& rhs){
                return entity_ != rhs;
            }

            /// Returns the requested component, or error if it doesn't exist
            template<typename C>
            inline typename std::enable_if<is_component<C>::value, C&>::type get(){
                return manager_->get_component_fast<C>(entity_);
            }

            template<typename C>
            inline typename std::enable_if<!is_component<C>::value, C&>::type get(){
                return manager_->get_component<C>(entity_);
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

            /// Assume that this entity has provided Components
            /// Use for faster component access calls
            template<typename ...Components_>
            inline EntityAlias<Components_...>& assume(){
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

            /// Returns whether an entity is an entity alias or not
            template<typename T>
            inline bool is(){
                return entity_.is<T>();
            }

            /// Returns true if entity has not been destroyed. False otherwise
            inline bool valid(){
                return entity_.valid();
            }

            template <typename ...Components_>
            std::tuple<Components_& ...> unpack() {
                return entity_.unpack<Components_...>();
            }

            std::tuple<Components& ...> unpack() {
                return entity_.unpack<Components...>();
            }

        protected:
            EntityAlias(){}

        private:
            static ComponentMask mask(){
                return component_mask <Components...>();
            }

            void fill_empty(){
                if(!entity_.has(mask())){
                    fill_empty_components<Components...>();
                }
            }

            template<typename C>
            void fill_empty_components(){
                if(!has<C>()) {
                    add<C>();
                }
            }

            template<typename C, typename C1, typename ...Cs>
            void fill_empty_components(){
                fill_empty_components<C>();
                fill_empty_components<C1, Cs...>();
            }

            friend class EntityManager;
            friend class EntityAlias;
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
            free_list_.clear();
        }

        // Cretate a new Entity and return handle
        Entity create(){
            index_t index;
            size_t entity_count = count();
            entity_versions_.resize(entity_count + 1);
            component_masks_.resize(entity_count + 1, ComponentMask(0));
            //see if there are holes in memory, allocate new element at first hole
            if (free_list_.empty()) {
                index = entity_count;
            } else{
                index = free_list_.back();
                free_list_.pop_back();
            }
            return get(index);
        }

        std::vector<Entity> create(const size_t num_of_entities){
            std::vector<Entity> new_entities;
            index_t index;
            size_t entities_left = num_of_entities;
            size_t entity_count = count();
            new_entities.reserve(entities_left);
            entity_versions_.resize(entity_count + entities_left);
            component_masks_.resize(entity_count + entities_left, ComponentMask(0));
            //first, allocate where there are holes
            while (entities_left && !free_list_.empty()) {
                index = free_list_.back();
                free_list_.pop_back();
                ++entity_count;
                new_entities.push_back(get(index));
                --entities_left;
            }
            while(entities_left){
                index = entity_count++;
                new_entities.push_back(get(index));
                --entities_left;
            }
            return new_entities;
        }

        template<typename T, typename ...Args>
        T create(Args... args){
            ECS_ASSERT_IS_ENTITY(T);
            ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
            Entity entity = create();
            T* entity_alias = new(&entity) T(std::forward<Args>(args)...);
            assert(entity.has(T::mask()));
            return *entity_alias;
        }

        // Access a View of all entities with specified components
        template<typename ...Components>
        View<EntityAlias<Components...>> with(){
            ComponentMask mask = component_mask <Components...>();
            return View<EntityAlias<Components...>>(this, mask);
        }

        // Iterate through all entities with all components, specified as lambda parameters
        // example: entities.with([] (Position& pos) { All position components });
        template<typename T>
        void with(T lambda){
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
            typedef details::function_traits<T> function;
            static_assert(function::arity == 1, "Lambda must only have one argument");
            typedef typename function::template arg_remove_ref<0> entity_interface_t;
            for(entity_interface_t entityInterface : fetch_every<entity_interface_t>()){
                lambda(entityInterface);
            }
        }

        inline Entity operator[] (index_t index){
            return get(index);
        }

        inline Entity operator[] (Entity::Id id){
            Entity entity = get(id);
            assert(id == entity.id() && "Id is no longer valid (Entity was destroyed)");
            return entity;
        }

        inline size_t count() {
            return entity_versions_.size() - free_list_.size();
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
                return manager.get(index);
            }
        };

        template<typename Lambda>
        using with_ = with_t<details::function_traits<Lambda>::arity, Lambda>;

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
            assert(index < ECS_MAX_NUM_OF_COMPONENTS && "maximum number of components exceeded.");
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
        };

        template<typename C>
        inline ComponentManager<C>& get_component_manager_fast(){
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
        inline C& get_component(Entity& entity){
            assert(has_component<C>(entity) && "Entity doesn't have this component attached");
            return get_component_manager<C>().get(entity.id_.index_);
        }

        /// Get component fast, no error checks. Use if it is already known that Entity has Component
        template<typename C>
        inline C& get_component_fast(Entity& entity){
            return get_component_fast<C>(entity.id_.index_);
        }

        /// Get component fast, no error checks. Use if it is already known that Entity has Component
        template<typename C>
        inline C& get_component_fast(index_t index){
            return get_component_manager_fast<C>().get(index);
        }

        template<typename C, typename ...Args>
        inline C& create_component(Entity &entity, Args &&... args){
            ECS_ASSERT_VALID_ENTITY(entity);
            assert(!has_component<C>(entity) && "Entity already has this component attached");
            C& component = get_component_manager<C>().create(entity.id_.index_, std::forward<Args>(args) ...);
            entity.mask().set(component_index<C>());
            return component;
        }

        template<typename C>
        inline void remove_component(Entity& entity){
            ECS_ASSERT_VALID_ENTITY(entity);
            assert(has_component<C>(entity) && "Entity doesn't have component attached");
            get_component_manager<C>().remove(entity.id_.index_);
        }

        template<typename C>
        inline void remove_component_fast(Entity& entity){
            ECS_ASSERT_VALID_ENTITY(entity);
            assert(has_component<C>(entity) && "Entity doesn't have component attached");
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
            if(entity.has<C>()) return get_component_fast<C>(entity) = C(args...);
            else return create_component<C>(entity, std::forward<Args>(args)...);
        }

        template<typename C, typename ...Args>
        inline C& set_component_fast(Entity& entity, Args && ... args){
            ECS_ASSERT_VALID_ENTITY(entity);
            assert(entity.has<C>() && "Entity does not have component attached");
            return get_component_fast<C>(entity) = C(args...);
        }

        inline bool has_component(Entity& entity, ComponentMask component_mask){
            ECS_ASSERT_VALID_ENTITY(entity);
            return (mask(entity) & component_mask) == component_mask;
        }

        inline bool has_component(index_t index, ComponentMask component_mask){
            return (mask(index) & component_mask) == component_mask;
        }

        template<typename ...Components>
        inline bool has_component(Entity& entity){
            return has_component(entity, component_mask<Components...>());
        }

        template<typename ...Components>
        inline bool has_component(index_t index){
            return has_component(index, component_mask<Components...>());
        }

        inline bool valid(Entity& entity){
            sortFreeList();
            return !binary_search(free_list_.begin(), free_list_.end(), entity.id().index_) &&
            entity.id().index_ < entity_versions_.size() && entity == get(entity.id().index_);
        }

        inline void destroy(Entity& entity){
            index_t index = entity.id().index_;
            remove_all_components(entity);
            ++entity_versions_[index];
            free_list_.push_back(index);
        }

        inline ComponentMask& mask(Entity& entity){
            return mask(entity.id_.index_);
        }

        inline ComponentMask& mask(index_t index){
            return component_masks_[index];
        }

        inline Entity get(Entity::Id id){
            return Entity(this, id);
        }

        inline Entity get(index_t index){
            return get(Entity::Id(index, entity_versions_[index]));
        }

        inline void sortFreeList(){
            std::sort(free_list_.begin(), free_list_.end());
        }

        inline size_t capacity() const {
            return entity_versions_.capacity();
        }

        std::vector<BaseManager*> component_managers_;
        std::vector<ComponentMask> component_masks_;
        std::vector<version_t> entity_versions_;
        std::vector<index_t> free_list_;

        template<typename T>
        friend class Iterator;
        friend class Entity;
        friend class BaseComponent;
    };

    class SystemManager{
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
        }

        template<typename S, typename ...Args>
        S& add(Args &&... args){
            ECS_ASSERT_IS_SYSTEM(S);
            assert(!exists<S>() && "System already exists");
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
            assert(exists<S>() && "System does not exist");
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
