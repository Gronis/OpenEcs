#pragma once

/// --------------------------------------------------------------------------
/// OpenEcs - A fast, typesafe, C++11, configurable, header only, ECS
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

namespace ecs{

    #define ASSERT_IS_ENTITY(T)                                                             \
        static_assert(std::is_base_of<BaseEntityAlias, T>::value ||                         \
                  std::is_same<Entity, T>::value ,                                          \
        #T " does not inherit EntityInterface.");

    #define ASSERT_ENTITY_CORRECT_SIZE(T)                                                   \
        static_assert(sizeof(BaseEntityAlias) == sizeof(T),                                 \
        #T " should not include new variables, add them as Components instead.");           \

    #define MAX_NUM_OF_COMPONENTS 32
    #define DEFAULT_CHUNK_SIZE 8192

    /// Type used for entity index
    typedef u_int32_t index_t;

    /// Type used for entity version
    typedef uint8_t version_t;

namespace exceptions{
    ///---------------------------------------------------------------------
    /// Exceptions used by the ecs
    ///---------------------------------------------------------------------
    ///
    class RedundantComponentException : public std::exception{
        virtual const char* what() const throw(){
            return "Entity already has this component attached.";
        }
    };
    class RedundantSystemException : public std::exception{
        virtual const char* what() const throw(){
            return "SystemManager already has system of that type.";
        }
    };
    class MissingComponentException : public std::exception{
        virtual const char* what() const throw(){
            return "Entity has no component of this type attached.";
        }
    };
    class MissingSystemException : public std::exception{
        virtual const char* what() const throw(){
            return "SystemManager does not have system of that type.";
        }
    };
    class InvalidEntityException : public std::exception{
        virtual const char* what() const throw(){
            return "This entity is invalid.";
        }
    };
}; //namespace exceptions

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




    ///--------------------------------------------------------------------
    /// Call different functions depending on a compile-time condition
    ///
    /// if condition is true, call first function provided
    /// else call second function provided
    ///
    /// NOTE: function must me lambdas for now, TODO: fix
    ///--------------------------------------------------------------------
    template<bool value>
    struct call_if_t;

    template<>
    struct call_if_t<true>{
        template<typename F1, typename F2, typename ...Args>
        static inline typename function_traits<F1>::return_type call(F1 f1,
                                                                     F2 f2,
                                                                     Args && ... args){
            return f1(std::forward<Args>(args)...);
        }
    };

    template<>
    struct call_if_t<false>{
        template<typename F1, typename F2, typename ...Args>
        static inline typename function_traits<F2>::return_type call(F1 f1,
                                                                     F2 f2,
                                                                     Args && ... args){
            return f2(std::forward<Args>(args)...);
        }
    };

    template<bool value, typename F1, typename F2, typename ...Args>
    typename function_traits<F1>::return_type call_if(F1 f1, F2 f2, Args && ... args){
        static_assert(std::is_same<typename function_traits<F1>::return_type,
                                   typename function_traits<F2>::return_type>::value,
            "Both functions must have the same return type");
        return call_if_t<value>::call(f1, f2, std::forward<Args>(args)...);
    }




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
        explicit BasePool(size_t element_size, size_t chunk_size = DEFAULT_CHUNK_SIZE) :
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
} // namespace details

    ///---------------------------------------------------------------------
    /// A Property is a sample Component with only one property of any type
    ///---------------------------------------------------------------------
    ///
    /// A Property is a sample Component with only one property of any type.
    /// it implements standard constructors, type conversations and
    /// operators which might be useful.
    ///
    /// TODO: Add more operators
    ///
    ///---------------------------------------------------------------------
    template<typename T>
    struct Property{
        Property(){}

        Property(const Property<T>& rhs) : Property(rhs.value){}

        Property(const T& value) : value(value){}

        inline operator T&(){
            return value;
        }

        template<typename E>
        inline bool operator == (E& rhs) {
            return value == rhs.value;
        }

        inline bool operator == (T& rhs) {
            return value == rhs;
        }

        template<typename E>
        inline bool operator != (E& rhs) {
            return value != rhs.value;
        }

        inline bool operator != (T& rhs) {
            return value != rhs;
        }
        T value;
    };

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
        typedef std::bitset<MAX_NUM_OF_COMPONENTS> ComponentMask;

        ///---------------------------------------------------------------------
        /// Helper class
        ///---------------------------------------------------------------------
        class BaseEntityAlias;

        ///---------------------------------------------------------------------
        /// Helper class
        ///---------------------------------------------------------------------
        class BaseManager;

        ///---------------------------------------------------------------------
        /// Helper class
        ///---------------------------------------------------------------------
        class BaseComponent;
        ///---------------------------------------------------------------------
        /// Component is a wrapper class around a component instance
        /// This class is mainly used to give different Components its
        /// own mask index.
        ///---------------------------------------------------------------------
        template<typename T>
        class Component;

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

        class BaseComponent {
        public:
            typedef std::size_t Family;

        protected:
            static Family& family_counter(){ static Family counter = 0; return counter; };
        private:
            BaseComponent() = delete;
        }; //BaseComponent


        template <typename C>
        class Component : public BaseComponent {
        public:
            static Family family() {
                static Family family = family_counter()++;
                return family;
            }
        private:
            Component() = delete;
        }; //Component

        template<typename C>
        class ComponentManager : public BaseManager, details::NonCopyable{
        public:
            ComponentManager(EntityManager& manager, size_t chunk_size = DEFAULT_CHUNK_SIZE) :
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
                if(!manager_.has_component<C>(index)) throw exceptions::MissingComponentException();
                pool_.destroy(index);
                manager_.mask(index).reset(Component<C>::family());
            }

            void force_remove(index_t index){
                pool_.destroy(index);
                manager_.mask(index).reset(Component<C>::family());
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
                return componentMask<C>();
            }

            EntityManager &manager_;
            details::Pool<C> pool_;

            friend class EntityManager;
            friend class Component<C>;
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
                ASSERT_IS_ENTITY(T);
                ASSERT_ENTITY_CORRECT_SIZE(T);
                if(!has(T::mask())) throw exceptions::MissingComponentException();
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
            /// Return true if entity has all specified compoents. False otherwise
            template<typename... Components>
            inline bool has(){
                return manager_->has_component<Components ...>(*this);
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
            /// Removes a component. Error of it doesn't exist
            template<typename C>
            inline void force_remove(){
                manager_->remove_component_fast<C>(*this);
            }

            /// Access this Entity as an EntityAlias. Ignores entity mask.
            template<typename T>
            inline T&force_as(){
                ASSERT_IS_ENTITY(T);
                ASSERT_ENTITY_CORRECT_SIZE(T);
                return reinterpret_cast<T&>(*this);
            }

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
                return entity().template force_as<T>();
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
            ASSERT_IS_ENTITY(T)
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
            inline C& get(){
                return details::call_if<is_component<C>::value>(
                        [&]() -> C& { return manager_->get_component_fast<C>(entity_); },
                        [&]() -> C& { return manager_->get_component<C>(entity_); });
            }

            /// Set the requested component, if old component exist,
            /// a new one is created. Otherwise, the assignment operator
            /// is used.
            template<typename C, typename ... Args>
            inline C& set(Args && ... args){
                return details::call_if<is_component<C>::value>(
                        [&]() -> C& { return manager_->set_component_fast<C>(entity_, std::forward<Args>(args)...); } ,
                        [&]() -> C& { return manager_->set_component<C>(entity_, std::forward<Args>(args)...); });
            }

            /// Add the requested component, error if component of the same type exist already
            template<typename C, typename ... Args>
            inline C& add(Args && ... args){
                static_assert(!is_component<C>::value,
                              "Cannot add component, it already exist. Use set to override old component.");
                return entity_.add(std::forward<Args>(args)...);
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
            inline void remove(){
                static_assert(!is_component<C>::value,
                              "Cannot remove dependent component. Use force_remove instead.");
                entity_.remove<C>();
            }

            template<typename C>
            inline void force_remove(){
                entity_.force_remove<C>();
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
                entity_.destroy();
            }
            /// Return true if entity has all specified components. False otherwise
            template<typename... Components_>
            inline bool has(){
                return entity_.has<Components_...>();
            }
            /// Returns true if entity has not been destroyed. False otherwise
            inline bool valid(){
                return entity_.valid();
            }

            template <typename ...Components_>
            std::tuple<Component<Components_>...> unpack() {
                return entity_.unpack<Components_...>();
            }

            std::tuple<Component<Components>...> unpack() {
                return entity_.unpack<Components...>();
            }

        protected:
            EntityAlias(){}

        private:
            static ComponentMask mask(){
                return componentMask<Components...>();
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
            ASSERT_IS_ENTITY(T);
            ASSERT_ENTITY_CORRECT_SIZE(T);
            Entity entity = create();
            T* entity_alias = new(&entity) T(std::forward<Args>(args)...);
            assert(entity.has(T::mask()) &&
                   "Entity should have all required components attached");
            return *entity_alias;
        }

        // Access a View of all entities with specified components
        template<typename ...Components>
        View<EntityAlias<Components...>> with(){
            ComponentMask mask = componentMask<Components...>();
            // Ensure they exists
            // TODO: make prettier
            std::make_tuple(&get_component_manager<Components>()...);
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
            ASSERT_IS_ENTITY(T);
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
            if (id != entity.id()) throw exceptions::InvalidEntityException();
            return entity;
        }

        inline size_t count() {
            return entity_versions_.size() - free_list_.size();
        }

    private:
        template<size_t N, typename...>
        struct with_t;

        template<size_t N, typename Lambda, typename... Components>
        struct with_t<N, Lambda, Components...> :
                with_t<N - 1, Lambda, typename details::function_traits<Lambda>::template arg_remove_ref<N - 1> ,Components...>{};

        template<typename Lambda, typename... Components>
        struct with_t<0, Lambda, Components...>{
            static void for_each(EntityManager& manager, Lambda lambda){
                auto view = manager.with<Components...>();
                auto it = view.begin();
                auto end = view.end();
                for(; it != end; ++it){
                    lambda(manager.get_component_fast<Components>(it.index())...);
                }
            }
        };

        template<typename Lambda>
        using with_ = with_t<details::function_traits<Lambda>::arity, Lambda>;

        template<typename C>
        static inline ComponentMask componentMask(){
            return ComponentMask((1 << Component<C>::family()));
        }
        //recursive function for componentMask creation
        template<typename C1, typename C2, typename ...Cs>
        static inline ComponentMask componentMask(){
            return componentMask<C1>() | componentMask<C2, Cs...>();
        }

        template<typename C, typename ...Args>
        ComponentManager<C>& create_component_manager(Args && ... args){
            ComponentManager<C>* ptr = new ComponentManager<C>(std::forward<EntityManager&>(*this), std::forward<Args>(args) ...);
            component_managers_[Component<C>::family()] = ptr;
            return *ptr;
        };

        template<typename C>
        inline ComponentManager<C>& get_component_manager_fast(){
            return *reinterpret_cast<ComponentManager<C>*>(component_managers_[Component<C>::family()]);
        }

        template<typename C>
        inline ComponentManager<C>& get_component_manager(){
            auto family = Component<C>::family();
            if(component_managers_.size() <= family){
                component_managers_.resize(family + 1, nullptr);
                return create_component_manager<C>();
            } else if(component_managers_[family] == nullptr){
                return create_component_manager<C>();
            }
            return *reinterpret_cast<ComponentManager<C>*>(component_managers_[family]);
        }

        template<typename C>
        inline C& get_component(Entity& entity){
            if(!has_component<C>(entity)) throw exceptions::MissingComponentException();
            return get_component_fast<C>(entity);
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
            if(has_component<C>(entity)) throw exceptions::RedundantComponentException();
            return set_component_fast<C>(entity, std::forward<Args>(args)...);
        }

        template<typename C>
        inline void remove_component(Entity& entity){
            if(!entity.valid()) throw exceptions::InvalidEntityException();
            get_component_manager<C>().remove(entity.id_.index_);
        }

        template<typename C>
        inline void remove_component_fast(Entity& entity){
            if(!entity.valid()) throw exceptions::InvalidEntityException();
            get_component_manager<C>().force_remove(entity.id_.index_);
        }

        /// Removes all components from a single entity
        inline void remove_all_components(Entity& entity){
            for (auto componentManager : component_managers_) {
                if(componentManager && has_component(entity, componentManager->mask())) {
                    componentManager->remove(entity.id_.index_);
                }
            }
        }

        inline void clear_mask(Entity &entity){
            component_masks_[entity.id_.index_].reset();
        }

        template<typename C, typename ...Args>
        inline C& set_component(Entity& entity, Args && ... args){
            if(entity.has<C>()) return get_component_fast<C>(entity) = C(args...);
            else return set_component_fast<C>(entity, std::forward<Args>(args)...);
        }

        template<typename C, typename ...Args>
        inline C& set_component_fast(Entity& entity, Args && ... args){
            C& component = get_component_manager<C>().create(entity.id_.index_, std::forward<Args>(args) ...);
            entity.mask() |= componentMask<C>();
            return component;
        }

        inline bool has_component(Entity& entity, ComponentMask component_mask){
            return (mask(entity) & component_mask) == component_mask;
        }

        inline bool has_component(index_t index, ComponentMask component_mask){
            return (mask(index) & component_mask) == component_mask;
        }

        template<typename ...Components>
        inline bool has_component(Entity& entity){
            return has_component(entity, componentMask<Components...>());
        }

        template<typename ...Components>
        inline bool has_component(index_t index){
            return has_component(index, componentMask<Components...>());
        }

        inline bool valid(Entity& entity){
            return !binary_search(free_list_.begin(), free_list_.end(), entity.id().index_) &&
            entity.id().index_ < entity_versions_.size() && entity == get(entity.id().index_);
        }

        inline void destroy(Entity& entity){
            index_t index = entity.id().index_;
            if(!valid(entity)) throw exceptions::InvalidEntityException();
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
        class BaseSystem {
        public:
            virtual ~BaseSystem(){}
            typedef std::size_t Family;

            virtual void update(float time) = 0;

        protected:
            static Family& family_counter(){ static Family counter = 0; return counter; }
            SystemManager* manager_;

            friend class SystemManager;
        }; //BaseSystem
    public:
        SystemManager(EntityManager& entities) : entities_(&entities){}

        #define ASSERT_SYSTEM(S)                                                            \
            static_assert(std::is_base_of<BaseSystem, S>::value,                            \
            "DirivedSystem must inherit System<DirivedSystem>.");

        template<typename S, typename ...Args>
        S& create(Args && ... args){
            ASSERT_SYSTEM(S);
            //If system exist
            if(exists<S>()) {
                throw exceptions::RedundantSystemException();
            }
            systems_.resize(S::family() + 1);
            S* system = new S(std::forward<Args>(args) ...);
            system->manager_ = this;
            systems_[S::family()] = system;
            return *system;
        }

        template<typename S>
        void destroy(){
            ASSERT_SYSTEM(S);
            if(!exists<S>()) {
                throw exceptions::MissingSystemException();
            }
            delete systems_[S::family()];
            systems_[S::family()] = nullptr;
        }

        void update(float time){
            for(auto system : systems_){
                if(system != nullptr) system->update(time);
            }
        }

        template<typename S>
        inline bool exists(){
            ASSERT_SYSTEM(S);
            return systems_.size() > S::family() && systems_[S::family()] != nullptr;
        }

    private:
        template<typename S>
        friend class System;

        std::vector<BaseSystem*> systems_;
        std::vector<BaseSystem::Family> order_;
        EntityManager* entities_;
    };

    template<typename S>
    class System : public SystemManager::BaseSystem{
    protected:
        EntityManager& entities(){
            return *manager_->entities_;
        }
    public:
        static Family family() {
            static Family family = family_counter()++;
            return family;
        }
    };

    template<typename ...Components>
    using EntityAlias = EntityManager::EntityAlias<Components...>;
    using Entity = EntityManager::Entity;
} // namespace ecs