#include "EntityAlias.h"
#include "EntityManager.h"

namespace ecs{

Entity::Entity(const Entity &other) : Entity(other.manager_, other.id_) { }

Entity::Entity(EntityManager *manager, Id id) :
    manager_(manager),
    id_(id)
{ }

Entity &Entity::operator=(const Entity &rhs) {
  manager_ = rhs.manager_;
  id_ = rhs.id_;
  return *this;
}

template<typename C>
C &Entity::get() {
  return manager_->get_component<C>(*this);
}

template<typename C>
C const &Entity::get() const {
  return manager_->get_component<C>(*this);
}

template<typename C, typename ... Args>
C &Entity::set(Args && ... args){
  return manager_->set_component<C>(*this, std::forward<Args>(args) ...);
}

template<typename C, typename ... Args>
C &Entity::add(Args && ... args){
  return manager_->create_component<C>(*this, std::forward<Args>(args) ...);
}

template<typename T>
inline T & Entity::as(){
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  ECS_ASSERT(has(T::static_mask()), "Entity doesn't have required components for this EntityAlias");
  return reinterpret_cast<T &>(*this);
}

template<typename T>
inline T const & Entity::as() const{
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  ECS_ASSERT(has(T::static_mask()), "Entity doesn't have required components for this EntityAlias");
  return reinterpret_cast<T const &>(*this);
}

/// Assume that this entity has provided Components
/// Use for faster component access calls
template<typename ...Components>
inline EntityAlias<Components...> & Entity::assume() {
  return as<EntityAlias<Components...>>();
}

template<typename ...Components>
inline EntityAlias<Components...> const & Entity::assume() const {
  return as<EntityAlias<Components...>>();
}

template<typename C>
void Entity::remove()  {
  manager_->remove_component<C>(*this);
}

void Entity::remove_everything() {
  manager_->remove_all_components(*this);
}

void Entity::clear_mask() {
  manager_->clear_mask(*this);
}

void Entity::destroy() {
  manager_->destroy(*this);
}

template<typename... Components>
bool Entity::has() {
  return manager_->has_component<Components ...>(*this);
}

template<typename... Components>
bool Entity::has() const {
  return manager_->has_component<Components ...>(*this);
}

template<typename T>
bool Entity::is() {
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  return has(T::static_mask());
}

template<typename T>
bool Entity::is() const {
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  return has(T::static_mask());
}

bool Entity::is_valid() {
  return manager_->is_valid(*this);
}

bool Entity::is_valid() const {
  return manager_->is_valid(*this);
}

template<typename ...Components>
std::tuple<Components &...>  Entity::unpack() {
  return std::forward_as_tuple(get<Components>()...);
}

template<typename ...Components>
std::tuple<Components const &...> Entity::unpack() const{
  return std::forward_as_tuple(get<Components>()...);
}

bool Entity::has(details::ComponentMask &check_mask)  {
  return manager_->has_component(*this, check_mask);
}

bool Entity::has(details::ComponentMask const &check_mask) const  {
  return manager_->has_component(*this, check_mask);
}

details::ComponentMask &Entity::mask()  {
  return manager_->mask(*this);
}
details::ComponentMask const &Entity::mask() const {
  return manager_->mask(*this);
}

inline bool operator==(const Entity &lhs, const Entity &rhs) {
  return lhs.id() == rhs.id();
}

inline bool operator!=(const Entity &lhs, const Entity &rhs) {
  return lhs.id() != rhs.id();
}

details::ComponentMask Entity::static_mask(){
  return details::ComponentMask(0);
}

} // namespace ecs