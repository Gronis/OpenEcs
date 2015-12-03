#include "Utils.h"

namespace ecs{

namespace details{

BaseEntityAlias::BaseEntityAlias(const Entity &entity) : entity_(entity) {  }
BaseEntityAlias::BaseEntityAlias() { }
BaseEntityAlias::BaseEntityAlias(const BaseEntityAlias &other) : entity_(other.entity_) { }

} // namespace details

template<typename ...Cs>
EntityAlias<Cs...>::EntityAlias(const Entity &entity) : details::BaseEntityAlias(entity) {}


template<typename ...Cs>
EntityAlias<Cs...>::operator Entity &() {
  return entity_;
}

template<typename ...Cs>
EntityAlias<Cs...>::operator Entity const &() const {
  return entity_;
}

template<typename ...Cs> template<typename T>
EntityAlias<Cs...>::operator const T &() const{
  return as<T>();
}

template<typename ...Cs> template<typename T>
EntityAlias<Cs...>::operator T &(){
  return as<T>();
}

template<typename ...Cs>
Id &EntityAlias<Cs...>::id() {
  return entity_.id();
}

template<typename ...Cs>
Id const &EntityAlias<Cs...>::id() const {
  return entity_.id();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() ->
typename std::enable_if<is_component<C>::value, C &>::type{
  return manager_->get_component_fast<C>(entity_);
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() const ->
typename std::enable_if<is_component<C>::value, C const &>::type{
  return manager_->get_component_fast<C>(entity_);
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() ->
typename std::enable_if<!is_component<C>::value, C &>::type{
  return entity_.get<C>();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() const ->
typename std::enable_if<!is_component<C>::value, C const &>::type{
  return entity_.get<C>();
}

template<typename ...Cs> template<typename C, typename ... Args>
inline auto EntityAlias<Cs...>::set(Args &&... args) ->
typename std::enable_if<is_component<C>::value, C &>::type{
  return manager_->set_component_fast<C>(entity_, std::forward<Args>(args)...);
}

template<typename ...Cs> template<typename C, typename ... Args>
inline auto EntityAlias<Cs...>::set(Args &&... args) ->
typename std::enable_if<!is_component<C>::value, C &>::type{
  return manager_->set_component<C>(entity_, std::forward<Args>(args)...);
}


template<typename ...Cs> template<typename C, typename ... Args>
inline C& EntityAlias<Cs...>::add(Args &&... args) {
  return entity_.add<C>(std::forward<Args>(args)...);
}

template<typename ...Cs> template<typename C>
C & EntityAlias<Cs...>::as() {
  return entity_.as<C>();
}

template<typename ...Cs> template<typename C>
C const & EntityAlias<Cs...>::as() const {
  return entity_.as<C>();
}

template<typename ...Cs> template<typename ...Components_>
EntityAlias<Components_...> &EntityAlias<Cs...>::assume() {
  return entity_.assume<Components_...>();
}

template<typename ...Cs> template<typename ...Components>
EntityAlias<Components...> const &EntityAlias<Cs...>::assume() const {
  return entity_.assume<Components...>();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::remove() ->
typename std::enable_if<!is_component<C>::value, void>::type {
  entity_.remove<C>();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::remove() ->
typename std::enable_if<is_component<C>::value, void>::type {
  manager_->remove_component_fast<C>(entity_);
}

template<typename ...Cs>
void EntityAlias<Cs...>::remove_everything() {
  entity_.remove_everything();
}

template<typename ...Cs>
void EntityAlias<Cs...>::clear_mask() {
  entity_.clear_mask();
}

template<typename ...Cs>
void EntityAlias<Cs...>::destroy() {
  entity_.destroy();
}

template<typename ...Cs> template<typename... Components>
bool EntityAlias<Cs...>::has() {
  return entity_.has<Components...>();
}

template<typename ...Cs> template<typename... Components>
bool EntityAlias<Cs...>::has() const {
  return entity_.has<Components...>();
}

template<typename ...Cs> template<typename T>
bool EntityAlias<Cs...>::is() {
  return entity_.is<T>();
}
template<typename ...Cs> template<typename T>
bool EntityAlias<Cs...>::is() const {
  return entity_.is<T>();
}

template<typename ...Cs>
bool EntityAlias<Cs...>::is_valid() {
  return entity_.is_valid();
}

template<typename ...Cs>
bool EntityAlias<Cs...>::is_valid() const {
  return entity_.is_valid();
}

template<typename ...Cs>
EntityAlias<Cs...>::EntityAlias() {

}

template<typename ...Cs>
details::ComponentMask EntityAlias<Cs...>::static_mask(){
  return details::component_mask<Cs...>();
}

template<typename ... Cs>
inline bool EntityAlias<Cs...>::operator==(const Entity &rhs) const {
  return entity_ == rhs;
}

template<typename ... Cs>
inline bool EntityAlias<Cs...>::operator!=(const Entity &rhs) const {
  return entity_ != rhs;
}


} // namespace ecs