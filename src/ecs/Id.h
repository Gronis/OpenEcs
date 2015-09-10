#ifndef ECS_ID_H
#define ECS_ID_H

namespace ecs{

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

} // namespace ecs

#include "Id.inl"

#endif //ECS_ID_H
