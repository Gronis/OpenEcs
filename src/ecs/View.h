#ifndef ECS_VIEW_H
#define ECS_VIEW_H

namespace ecs{

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

} // namespace ecs

#include "View.inl"

#endif //OPENECS_VIEW_H
