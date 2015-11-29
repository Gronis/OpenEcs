//
// Created by Robin Grönberg on 29/11/15.
//

#ifndef OPENECS_SYSTEM_H
#define OPENECS_SYSTEM_H

namespace ecs {

class EntityManager;
class SystemManager;

///---------------------------------------------------------------------
/// A system is responsible for some kind of behavior for entities
/// with certain components
///---------------------------------------------------------------------
///
/// A system implements behavior of entities with required components
/// The update method is called every frame/update from the
/// SystemManager.
///
///---------------------------------------------------------------------
class System {
 public:
  virtual ~System() { }
  virtual void update(float time) = 0;
 protected:
  inline EntityManager &entities();
 private:
  friend class SystemManager;
  SystemManager *manager_;
};

} //namespace ecs

#include "System.inl"

#endif //OPENECS_SYSTEM_H
