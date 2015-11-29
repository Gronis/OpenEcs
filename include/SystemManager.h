//
// Created by Robin Grönberg on 29/11/15.
//

#ifndef OPENECS_SYSTEM_MANAGER_H
#define OPENECS_SYSTEM_MANAGER_H

namespace ecs {

class EntityManager;
class System;

///---------------------------------------------------------------------
/// A SystemManager is responsible for managing Systems.
///---------------------------------------------------------------------
///
/// A SystemManager is associated with an EntityManager and any
/// number of systems. Calling update calls update on every system.
/// Each system can access the EntityManager and perform operations
/// on the entities.
///
///---------------------------------------------------------------------
class SystemManager: details::forbid_copies {
 public:

  inline SystemManager(EntityManager &entities) : entities_(&entities) { }

  inline ~SystemManager();

  /// Adds a System to this SystemManager.
  template<typename S, typename ...Args>
  inline S &add(Args &&... args);

  /// Removes a System from this System Manager
  template<typename S>
  inline void remove();

  /// Update all attached systems. They are updated in the order they are added
  inline void update(float time);

  /// Check if a system is attached.
  template<typename S>
  inline bool exists();

 private:
  std::vector<System *> systems_;
  std::vector<size_t> order_;
  EntityManager *entities_;

  friend class System;
};

} // namespace ecs

#include "SystemManager.inl"

#endif //OPENECS_SYSTEM_MANAGER_H
