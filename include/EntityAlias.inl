#include "Utils.h"
#include "EntityAlias.h"

namespace ecs{

template<typename ...Cs>
EntityAlias<Cs...>::EntityAlias() { }

template<typename ...Cs>
EntityAlias<Cs...>::EntityAlias(const Entity &entity) : details::BaseEntity(entity) {}


template<typename ...Cs>
EntityAlias<Cs...>::operator Entity &() {
  return entity();
}

template<typename ...Cs>
EntityAlias<Cs...>::operator Entity const &() const {
  return entity();
}

template<typename ...Cs>
Id &EntityAlias<Cs...>::id() {
  return entity().id();
}

template<typename ...Cs>
Id const &EntityAlias<Cs...>::id() const {
  return entity().id();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() ->
typename std::enable_if<is_component<C>::value, C &>::type{
  return entities().template get_component_fast<C>(entity());
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() const ->
typename std::enable_if<is_component<C>::value, C const &>::type{
  return entities().template get_component_fast<C>(entity());
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() ->
typename std::enable_if<!is_component<C>::value, C &>::type{
  return entity().template get<C>();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() const ->
typename std::enable_if<!is_component<C>::value, C const &>::type{
  return entity().template get<C>();
}

template<typename ...Cs> template<typename C, typename ... Args>
inline auto EntityAlias<Cs...>::set(Args &&... args) ->
typename std::enable_if<is_component<C>::value, C &>::type{
  return entities().template set_component_fast<C>(entity(), std::forward<Args>(args)...);
}

template<typename ...Cs> template<typename C, typename ... Args>
inline auto EntityAlias<Cs...>::set(Args &&... args) ->
typename std::enable_if<!is_component<C>::value, C &>::type{
  return entities().template set_component<C>(entity(), std::forward<Args>(args)...);
}


template<typename ...Cs> template<typename C, typename ... Args>
inline C& EntityAlias<Cs...>::add(Args &&... args) {
  return entity().template add<C>(std::forward<Args>(args)...);
}

template<typename ...Cs> template<typename C>
C & EntityAlias<Cs...>::as() {
  return entity().template as<C>();
}

template<typename ...Cs> template<typename C>
C const & EntityAlias<Cs...>::as() const {
  return entity().template as<C>();
}

template<typename ...Cs> template<typename ...Components_>
EntityAlias<Components_...> &EntityAlias<Cs...>::assume() {
  return entity().template assume<Components_...>();
}

template<typename ...Cs> template<typename ...Components>
EntityAlias<Components...> const &EntityAlias<Cs...>::assume() const {
  return entity().template assume<Components...>();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::remove() ->
typename std::enable_if<!is_component<C>::value, void>::type {
  entity().template remove<C>();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::remove() ->
typename std::enable_if<is_component<C>::value, void>::type {
  entities().template remove_component_fast<C>(entity());
}

template<typename ...Cs>
void EntityAlias<Cs...>::remove_everything() {
  entity().remove_everything();
}

template<typename ...Cs>
void EntityAlias<Cs...>::clear_mask() {
  entity().clear_mask();
}

template<typename ...Cs>
void EntityAlias<Cs...>::destroy() {
  entity().destroy();
}

template<typename ...Cs> template<typename... Components>
bool EntityAlias<Cs...>::has() {
  return entity().template has<Components...>();
}

template<typename ...Cs> template<typename... Components>
bool EntityAlias<Cs...>::has() const {
  return entity().template has<Components...>();
}

template<typename ...Cs> template<typename T>
bool EntityAlias<Cs...>::is() {
  return entity().template is<T>();
}
template<typename ...Cs> template<typename T>
bool EntityAlias<Cs...>::is() const {
  return entity().template is<T>();
}

template<typename ...Cs>
bool EntityAlias<Cs...>::is_valid() {
  return entity().is_valid();
}

template<typename ...Cs>
bool EntityAlias<Cs...>::is_valid() const {
  return entity().is_valid();
}

template<typename ...Cs>
details::ComponentMask EntityAlias<Cs...>::static_mask(){
  return details::component_mask<Cs...>();
}

template<typename ... Cs>
inline bool EntityAlias<Cs...>::operator==(const Entity &rhs) const {
  return entity() == rhs;
}

template<typename ... Cs>
inline bool EntityAlias<Cs...>::operator!=(const Entity &rhs) const {
  return entity() != rhs;
}


} // namespace ecs