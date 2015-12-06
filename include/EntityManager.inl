#include "Entity.h"
#include "UnallocatedEntity.h"

namespace ecs{

namespace details{

template<size_t N, typename Lambda, typename... Args>
struct with_t<N, Lambda, Args...>:
    with_t<N - 1, Lambda,
           typename details::function_traits<Lambda>::template arg_remove_ref<N - 1>, Args...> {
};

template<typename Lambda, typename... Args>
struct with_t<0, Lambda, Args...> {
  static inline void for_each(EntityManager &manager, Lambda lambda) {
    typedef details::function_traits <Lambda> function;
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

} // namespace details

EntityManager::EntityManager(size_t chunk_size) {
  entity_versions_.reserve(chunk_size);
  component_masks_.reserve(chunk_size);
}


EntityManager::~EntityManager()  {
  for (details::BaseManager *manager : component_managers_) {
    if (manager) delete manager;
  }
  component_managers_.clear();
  component_masks_.clear();
  entity_versions_.clear();
  next_free_indexes_.clear();
  component_mask_to_index_accessor_.clear();
  index_to_component_mask.clear();
}

UnallocatedEntity EntityManager::create() {
  return UnallocatedEntity(*this);
}


std::vector<Entity> EntityManager::create(const size_t num_of_entities)  {
  return create_with_mask(details::ComponentMask(0), num_of_entities);
}

template<typename T>
inline void EntityManager::create(const size_t num_of_entities, T lambda){
  ECS_ASSERT_IS_CALLABLE(T);
  using EntityAlias_ = typename details::function_traits<T>::template arg_remove_ref<0>;
  ECS_ASSERT_IS_ENTITY(EntityAlias_);
  for(Entity& entity : create_with_mask(EntityAlias_::static_mask(), num_of_entities)){
    //We cannot use as<EntityAlias> since we dont have attached any components
    lambda(*reinterpret_cast<EntityAlias_*>(&entity));
    ECS_ASSERT(entity.has(EntityAlias_::static_mask()), "Entity are missing certain components.");
  }
}

/// If EntityAlias is constructable with Args...
template<typename T, typename ...Args>
auto EntityManager::create(Args && ... args) ->
typename std::enable_if<std::is_constructible<T, Args...>::value, T>::type {
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  auto mask = T::static_mask();
  Entity entity = create_with_mask(mask);
  T *entity_alias = new(&entity) T(std::forward<Args>(args)...);
  ECS_ASSERT(entity.has(mask),
             "Every required component must be added when creating an Entity Alias");
  return *entity_alias;
}

/// If EntityAlias is not constructable with Args...
/// Attempt to create with underlying EntityAlias
template<typename T, typename ...Args>
auto EntityManager::create(Args && ... args) ->
typename std::enable_if<!std::is_constructible<T, Args...>::value, T>::type {
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  typedef typename T::Type Type;
  auto mask = T::static_mask();
  Entity entity = create_with_mask(mask);
  Type *entity_alias = new(&entity) Type();
  entity_alias->init(std::forward<Args>(args)...);
  ECS_ASSERT(entity.has(mask),
             "Every required component must be added when creating an Entity Alias");
  return *reinterpret_cast<T *>(entity_alias);
}

template<typename ...Components, typename ...Args>
auto EntityManager::create_with(Args && ... args ) ->
typename std::conditional<(sizeof...(Components) > 0), EntityAlias<Components...>, EntityAlias<Args...>>::type{
  using Type = typename std::conditional<(sizeof...(Components) > 0), EntityAlias<Components...>, EntityAlias<Args...>>::type;
  Entity entity = create_with_mask(Type::static_mask());
  Type *entity_alias = new(&entity) Type();
  entity_alias->init(std::forward<Args>(args)...);
  return *entity_alias;
}

template<typename ...Components>
EntityAlias<Components...> EntityManager::create_with() {
  using Type = EntityAlias<Components...>;
  Entity entity = create_with_mask(details::component_mask<Components...>());
  Type *entity_alias = new(&entity) Type();
  entity_alias->init();
  return *entity_alias;
}

Entity EntityManager::create_with_mask(details::ComponentMask mask)  {
  ++count_;
  index_t index = find_new_entity_index(mask);
  size_t slots_required = index + 1;
  if (entity_versions_.size() < slots_required) {
    entity_versions_.resize(slots_required);
    component_masks_.resize(slots_required, details::ComponentMask(0));
  }
  return get_entity(index);
}

std::vector<Entity> EntityManager::create_with_mask(details::ComponentMask mask, const size_t num_of_entities) {
  std::vector<Entity> new_entities;
  size_t entities_left = num_of_entities;
  new_entities.reserve(entities_left);
  auto mask_as_ulong = mask.to_ulong();
  IndexAccessor &index_accessor = component_mask_to_index_accessor_[mask_as_ulong];
  //See if we can use old indexes for destroyed entities via free list
  while (!index_accessor.free_list.empty() && entities_left--) {
    new_entities.push_back(get_entity(index_accessor.free_list.back()));
    index_accessor.free_list.pop_back();
  }
  index_t block_index = 0;
  index_t current = ECS_CACHE_LINE_SIZE; // <- if empty, create new block instantly
  size_t slots_required;
  // Are there any unfilled blocks?
  if (!index_accessor.block_index.empty()) {
    block_index = index_accessor.block_index[index_accessor.block_index.size() - 1];
    current = next_free_indexes_[block_index];
    slots_required = block_count_ * ECS_CACHE_LINE_SIZE + current + entities_left;
  } else {
    slots_required = block_count_ * ECS_CACHE_LINE_SIZE + entities_left;
  }
  entity_versions_.resize(slots_required);
  component_masks_.resize(slots_required, details::ComponentMask(0));

  // Insert until no entity is left or no block remain
  while (entities_left) {
    for (; current < ECS_CACHE_LINE_SIZE && entities_left; ++current) {
      new_entities.push_back(get_entity(current + ECS_CACHE_LINE_SIZE * block_index));
      entities_left--;
    }
    // Add more blocks if there are entities left
    if (entities_left) {
      block_index = block_count_;
      create_new_block(index_accessor, mask_as_ulong, 0);
      ++block_count_;
      current = 0;
    }
  }
  count_ += num_of_entities;
  return new_entities;
}

template<typename ...Components>
View<EntityAlias<Components...>> EntityManager::with()  {
  details::ComponentMask mask = details::component_mask<Components...>();
  return View<EntityAlias<Components...>>(this, mask);
}

template<typename T>
void EntityManager::with(T lambda)  {
  ECS_ASSERT_IS_CALLABLE(T);
  details::with_<T>::for_each(*this, lambda);
}


template<typename T>
View<T> EntityManager::fetch_every()  {
  ECS_ASSERT_IS_ENTITY(T);
  return View<T>(this, T::static_mask());
}


template<typename T>
void EntityManager::fetch_every(T lambda)  {
  ECS_ASSERT_IS_CALLABLE(T);
  typedef details::function_traits<T> function;
  static_assert(function::arg_count == 1, "Lambda or function must only have one argument");
  typedef typename function::template arg_remove_ref<0> entity_interface_t;
  for (entity_interface_t entityInterface : fetch_every<entity_interface_t>()) {
    lambda(entityInterface);
  }
}


Entity EntityManager::operator[](index_t index) {
  return get_entity(index);
}


Entity EntityManager::operator[](Id id)  {
  Entity entity = get_entity(id);
  ECS_ASSERT(id == entity.id(), "Id is no longer valid (Entity was destroyed)");
  return entity;
}

size_t EntityManager::count(){
  return count_;
}

index_t EntityManager::find_new_entity_index(details::ComponentMask mask) {
  auto mask_as_ulong = mask.to_ulong();
  IndexAccessor &index_accessor = component_mask_to_index_accessor_[mask_as_ulong];
  //See if we can use old indexes for destroyed entities via free list
  if (!index_accessor.free_list.empty()) {
    auto index = index_accessor.free_list.back();
    index_accessor.free_list.pop_back();
    return index;
  }
  // EntityManager has created similar entities already
  if (!index_accessor.block_index.empty()) {
    //No free_indexes in free list (removed entities), find a new index
    //at last block, if that block has free slots
    auto &block_index = index_accessor.block_index[index_accessor.block_index.size() - 1];
    auto &current = next_free_indexes_[block_index];
    // If block has empty slot, use it
    if (ECS_CACHE_LINE_SIZE > current) {
      return (current++) + ECS_CACHE_LINE_SIZE * block_index;
    }
  }
  create_new_block(index_accessor, mask_as_ulong, 1);
  return (block_count_++) * ECS_CACHE_LINE_SIZE;
}

void EntityManager::create_new_block(EntityManager::IndexAccessor &index_accessor,
                                     unsigned long mask_as_ulong,
                                     index_t next_free_index)  {
  index_accessor.block_index.push_back(block_count_);
  next_free_indexes_.resize(block_count_ + 1);
  index_to_component_mask.resize(block_count_ + 1);
  next_free_indexes_[block_count_] = next_free_index;
  index_to_component_mask[block_count_] = mask_as_ulong;
}

template<typename C, typename ...Args>
details::ComponentManager<C> &EntityManager::create_component_manager(Args && ... args)  {
  details::ComponentManager<C> *ptr = new details::ComponentManager<C>(std::forward<EntityManager &>(*this),
                                                                       std::forward<Args>(args) ...);
  component_managers_[details::component_index<C>()] = ptr;
  return *ptr;
}

template<typename C>
details::ComponentManager<C> &EntityManager::get_component_manager_fast() {
  return *reinterpret_cast<details::ComponentManager<C> *>(component_managers_[details::component_index<C>()]);
}

template<typename C>
details::ComponentManager<C> const &EntityManager::get_component_manager_fast() const {
  return *reinterpret_cast<details::ComponentManager<C> *>(component_managers_[details::component_index<C>()]);
}


template<typename C>
details::ComponentManager<C> &EntityManager::get_component_manager()  {
  auto index = details::component_index<C>();
  if (component_managers_.size() <= index) {
    component_managers_.resize(index + 1, nullptr);
    return create_component_manager<C>();
  } else if (component_managers_[index] == nullptr) {
    return create_component_manager<C>();
  }
  return *reinterpret_cast<details::ComponentManager<C> *>(component_managers_[index]);
}


template<typename C>
details::ComponentManager<C> const &EntityManager::get_component_manager() const  {
  auto index = details::component_index<C>();
  ECS_ASSERT(component_managers_.size() > index, component_managers_[index] == nullptr &&
      "Component manager not created");
  return *reinterpret_cast<details::ComponentManager<C> *>(component_managers_[index]);
}

details::BaseManager &EntityManager::get_component_manager(size_t component_index){
  ECS_ASSERT(component_managers_.size() > component_index, "ComponentManager not created with that component index.");
  return *component_managers_[component_index];
}
details::BaseManager const &EntityManager::get_component_manager(size_t component_index) const{
  ECS_ASSERT(component_managers_.size() > component_index, "ComponentManager not created with that component index.");
  return *component_managers_[component_index];
}

template<typename C>
C &EntityManager::get_component(Entity &entity) {
  ECS_ASSERT(has_component<C>(entity), "Entity doesn't have this component attached");
  return get_component_manager<C>().get(entity.id_.index_);
}

template<typename C>
C const &EntityManager::get_component(Entity const &entity) const {
  ECS_ASSERT(has_component<C>(entity), "Entity doesn't have this component attached");
  return get_component_manager<C>().get(entity.id_.index_);
}

template<typename C>
C &EntityManager::get_component_fast(index_t index)  {
  return get_component_manager_fast<C>().get(index);
}

template<typename C>
C const &EntityManager::get_component_fast(index_t index) const {
  return get_component_manager_fast<C>().get(index);
}


template<typename C>
C &EntityManager::get_component_fast(Entity &entity)  {
  return get_component_manager_fast<C>().get(entity.id_.index_);
}

template<typename C>
C const &EntityManager::get_component_fast(Entity const &entity) const  {
  return get_component_manager_fast<C>().get(entity.id_.index_);
}

template<typename C, typename ...Args>
C &EntityManager::create_component(Entity &entity, Args && ... args) {
  ECS_ASSERT_VALID_ENTITY(entity);
  ECS_ASSERT(!has_component<C>(entity), "Entity already has this component attached");
  C &component = get_component_manager<C>().create(entity.id_.index_, std::forward<Args>(args) ...);
  entity.mask().set(details::component_index<C>());
  return component;
}

template<typename C>
void EntityManager::remove_component(Entity &entity)  {
  ECS_ASSERT_VALID_ENTITY(entity);
  ECS_ASSERT(has_component<C>(entity), "Entity doesn't have component attached");
  get_component_manager<C>().remove(entity.id_.index_);
}

template<typename C>
void EntityManager::remove_component_fast(Entity &entity) {
  ECS_ASSERT_VALID_ENTITY(entity);
  ECS_ASSERT(has_component<C>(entity), "Entity doesn't have component attached");
  get_component_manager_fast<C>().remove(entity.id_.index_);
}

void EntityManager::remove_all_components(Entity &entity)  {
  ECS_ASSERT_VALID_ENTITY(entity);
  for (auto componentManager : component_managers_) {
    if (componentManager && has_component(entity, componentManager->mask())) {
      componentManager->remove(entity.id_.index_);
    }
  }
}

void EntityManager::clear_mask(Entity &entity) {
  ECS_ASSERT_VALID_ENTITY(entity);
  component_masks_[entity.id_.index_].reset();
}

template<typename C, typename ...Args>
C &EntityManager::set_component(Entity &entity, Args && ... args) {
  ECS_ASSERT_VALID_ENTITY(entity);
  if (entity.has<C>()) {
    return get_component_fast<C>(entity) = create_tmp_component<C>(std::forward<Args>(args)...);
  }
  else return create_component<C>(entity, std::forward<Args>(args)...);
}

template<typename C, typename ...Args>
C &EntityManager::set_component_fast(Entity &entity, Args && ... args) {
  ECS_ASSERT_VALID_ENTITY(entity);
  ECS_ASSERT(entity.has<C>(), "Entity does not have component attached");
  return get_component_fast<C>(entity) = create_tmp_component<C>(std::forward<Args>(args)...);
}

bool EntityManager::has_component(Entity &entity, details::ComponentMask component_mask) {
  ECS_ASSERT_VALID_ENTITY(entity);
  return (mask(entity) & component_mask) == component_mask;
}

bool EntityManager::has_component(Entity const &entity, details::ComponentMask const &component_mask) const  {
  ECS_ASSERT_VALID_ENTITY(entity);
  return (mask(entity) & component_mask) == component_mask;
}

template<typename ...Components>
bool EntityManager::has_component(Entity &entity) {
  return has_component(entity, details::component_mask<Components...>());
}

template<typename ...Components>
bool EntityManager::has_component(Entity const &entity) const  {
  return has_component(entity, details::component_mask<Components...>());
}


bool EntityManager::is_valid(Entity &entity)  {
  return entity.id_.index_ < entity_versions_.size() &&
      entity.id_.version_ == entity_versions_[entity.id_.index_];
}

bool EntityManager::is_valid(Entity const &entity) const  {
  return entity.id_.index_ < entity_versions_.size() &&
      entity.id_.version_ == entity_versions_[entity.id_.index_];
}

void EntityManager::destroy(Entity &entity) {
  index_t index = entity.id().index_;
  remove_all_components(entity);
  ++entity_versions_[index];
  auto &mask_as_ulong = index_to_component_mask[index / ECS_CACHE_LINE_SIZE];
  component_mask_to_index_accessor_[mask_as_ulong].free_list.push_back(index);
  --count_;
}

details::ComponentMask &EntityManager::mask(Entity &entity)  {
  return mask(entity.id_.index_);
}

details::ComponentMask const &EntityManager::mask(Entity const &entity) const {
  return mask(entity.id_.index_);
}

details::ComponentMask &EntityManager::mask(index_t index) {
  return component_masks_[index];
}

details::ComponentMask const &EntityManager::mask(index_t index) const {
  return component_masks_[index];
}

Entity EntityManager::get_entity(Id id) {
  return Entity(this, id);
}

Entity EntityManager::get_entity(index_t index) {
  return get_entity(Id(index, entity_versions_[index]));
}

size_t EntityManager::capacity() const  {
  return entity_versions_.capacity();
}

} // namespace ecs