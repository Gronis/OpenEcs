//
// Created by Robin Grönberg on 05/12/15.
//

#ifndef OPENECS_BASEENTITY_H
#define OPENECS_BASEENTITY_H

#include "Entity.h"
#include "EntityManager.h"


namespace ecs {

namespace details {

class BaseEntity {
 public:
  inline BaseEntity(const Entity &entity);
 protected:
  inline BaseEntity();
  inline BaseEntity(EntityManager* manager);
  inline BaseEntity(const BaseEntity &other);
  inline BaseEntity& operator=(const BaseEntity& other) { entity_ = other.entity_; return *this; }

  inline EntityManager &entities() { return *manager_; }
  inline Entity &entity() { return entity_; }
  inline EntityManager const& entities() const { return *manager_; }
  inline Entity const& entity() const { return entity_; }
 private:
  union {
    EntityManager *manager_;
    Entity entity_;
  };
}; //BaseEntity

} // namespace details

} // namespace ecs

#include "BaseEntity.inl"

#endif //OPENECS_BASEENTITY_H
