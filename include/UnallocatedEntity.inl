#include "UnallocatedEntity.h"
#include "ComponentManager.h"
#include <cstring>

namespace ecs{

UnallocatedEntity::UnallocatedEntity(EntityManager& manager) : manager_(&manager), entity_(&manager, Id(index_t(-1),0)){ }

UnallocatedEntity::~UnallocatedEntity(){
  allocate();
}

UnallocatedEntity& UnallocatedEntity::operator=(const UnallocatedEntity &rhs){
  allocate();
  manager_ = rhs.manager_;
  entity_ = rhs.entity_;
  component_data = rhs.component_data;
  component_headers_ = rhs.component_headers_;
  return *this;
}

UnallocatedEntity::operator Entity &() {
  allocate();
  return entity_;
}

Id& UnallocatedEntity::id() {
  allocate();
  return entity_.id();
}

template<typename C>
C& UnallocatedEntity::get() {
  ECS_ASSERT(is_valid(), "Unallocated Entity invalid");
  ECS_ASSERT(has<C>(), "UnallocatedEntity does not have Component Attached");
  if(is_allocated()){
    return entity_.get<C>();
  }
  int index = 0;
  for (auto& componentHeader : component_headers_) {
    if(componentHeader.index == details::component_index<C>()){
      return *reinterpret_cast<C*>(&component_data[index]);
    }
    index += componentHeader.size;
  }
  //should not happen
  return *static_cast<C*>(nullptr);
}

template<typename C>
C const & UnallocatedEntity::get() const {
  ECS_ASSERT(is_valid(), "Unallocated Entity invalid");
  ECS_ASSERT(has<C>(), "UnallocatedEntity does not have Component Attached");
  if(is_allocated()){
    return entity_.get<C>();
  }else{
    int index = 0;
    for (auto& componentHeader : component_headers_) {
      if(componentHeader.index == details::component_index<C>()){
        return *reinterpret_cast<C*>(&component_data[index]);
      }
      index += componentHeader.size;
    }
  }
  //should not happen
  return *static_cast<C*>(nullptr);
}

template<typename C, typename ... Args>
C& UnallocatedEntity::set(Args &&... args){
  if(is_allocated()){
    return entity_.set<C>(std::forward<Args>(args)...);
  }
  if(has<C>()){
    return get<C>() = manager_->create_tmp_component<C>(std::forward<Args>(args)...);
  }else{
    return add<C>(std::forward<Args>(args)...);
  }
}

template<typename C, typename ... Args>
C& UnallocatedEntity::add(Args &&... args){
  if(is_allocated()){
    return entity_.add<C>(std::forward<Args>(args)...);
  }
  ECS_ASSERT(!has<C>(), "Unallocated Entity cannot assign already assigned component with add. Use set instead");
  ECS_ASSERT(is_valid(), "Unallocated Entity invalid");
  int index = 0;
  //Find component location in memory
  for (auto componentHeader : component_headers_) {
    index += componentHeader.size;
  }
  //Ensure that a component manager exists for C
  manager_->get_component_manager<C>();
  //Set component data
  auto component_index = details::component_index<C>();
  mask_.set(component_index);
  component_headers_.push_back(ComponentHeader{static_cast<unsigned int>(component_index), sizeof(C)});
  component_data.resize(index + sizeof(C));
  return details::create_component<C>(&component_data[index], std::forward<Args>(args)...);
}

template<typename C>
C & UnallocatedEntity::as() {
  allocate();
  return entity_.as<C>();
}

template<typename ...Components_>
EntityAlias<Components_...> & UnallocatedEntity::assume() {
  allocate();
  return entity_.assume<Components_...>();
}

template<typename C>
void UnallocatedEntity::remove() {
  if(is_allocated()){
    entity_.remove<C>();
  }else{
    int index = 0;
    auto component_index = details::component_index<C>();
    for (auto& componentHeader : component_headers_) {
      if(componentHeader.index == component_index){
        C& component = *reinterpret_cast<C*>(&component_data[index]);
        component.~C();
        break;
      }
      index += componentHeader.size;
    }
    mask_.reset(component_index);
  }
}

void UnallocatedEntity::remove_everything() {
  if(is_allocated()){
    entity_.remove_everything();
  }else{
    //TODO: call dtors on components
    component_headers_.clear();
    mask_.reset();
  }
}

void UnallocatedEntity::clear_mask() {
  if(is_allocated()){
    entity_.clear_mask();
  } else{
    component_headers_.clear();
    mask_.reset();
  }
}

void UnallocatedEntity::destroy() {
  if(is_allocated()){
    entity_.destroy();
  }else{
    manager_ = nullptr;
  }
}

template<typename... Components>
bool UnallocatedEntity::has() const {
  if(is_allocated()){
    return entity_.has<Components...>();
  }else{
    auto component_mask = details::component_mask<Components...>();
    return (component_mask & mask_) == component_mask;
  }
}

template<typename T>
bool UnallocatedEntity::is() const {
  if(is_allocated()){
    return entity_.is<T>();
  }else{
    ECS_ASSERT_IS_ENTITY(T);
    ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
    auto component_mask = T::static_mask();
    return (component_mask & mask_) == component_mask;
  }
}

bool UnallocatedEntity::is_valid() const {
  if(is_allocated()){
    return entity_.is_valid();
  }else{
    return manager_ != nullptr;
  }
}

inline bool operator==(const UnallocatedEntity &lhs, const UnallocatedEntity &rhs) {
  return &lhs == &rhs;
}

inline bool operator!=(const UnallocatedEntity &lhs, const UnallocatedEntity &rhs) {
  return &lhs != &rhs;
}

inline bool UnallocatedEntity::operator==(const Entity &rhs) const {
  return &entity_ == &rhs;
}

inline bool UnallocatedEntity::operator!=(const Entity &rhs) const {
  return &entity_ != &rhs;
}

bool UnallocatedEntity::is_allocated() const {
  return manager_ == nullptr;
}

void UnallocatedEntity::allocate() {
  if(!is_allocated()){
    entity_ = manager_->create_with_mask(mask_);
    if(component_headers_.size() > 0){
      auto index = entity_.id().index();
      manager_->mask(index) |= mask_;
      unsigned int offset = 0;
      //TODO: set mask
      for (auto componentHeader : component_headers_) {
        details::BaseManager& componentManager = manager_->get_component_manager(componentHeader.index);
        componentManager.ensure_min_size(index);
        //Copy data from tmp location to acctuial location in component manager
        std::memcpy(componentManager.get_void_ptr(index), &component_data[offset], componentHeader.size);
        offset+=componentHeader.size;
      }
    }
    manager_ = nullptr;
  }
}

} // namespace ecs