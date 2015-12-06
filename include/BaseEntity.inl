#include "BaseEntity.h"

namespace ecs {

namespace details {

BaseEntity::BaseEntity(const Entity &entity) : entity_(entity) { }
BaseEntity::BaseEntity() { }
BaseEntity::BaseEntity(EntityManager * manager) : manager_(manager){ }
BaseEntity::BaseEntity(const BaseEntity &other) : entity_(other.entity_) { }

} // namespace details

} // namespace ecs
