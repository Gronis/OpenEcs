#ifndef ECS_ID_H
#define ECS_ID_H

namespace ecs{

///---------------------------------------------------------------------
/// Id is used for Entity to identify entities. It consists of an index
/// and a version. The index describes where the entity is located in
/// memory. The version is used to separate entities if they get the
/// same index.
///---------------------------------------------------------------------
class Id {
 public:
  inline Id();
  inline Id(index_t index, version_t version);

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

inline bool operator==(const Id& lhs, const Id &rhs);
inline bool operator!=(const Id& lhs, const Id &rhs);

} // namespace ecs

#include "Id.inl"

#endif //ECS_ID_H
