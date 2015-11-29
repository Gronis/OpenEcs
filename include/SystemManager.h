//
// Created by Robin Grönberg on 29/11/15.
//

#ifndef OPENECS_SYSTEM_H
#define OPENECS_SYSTEM_H

namespace ecs {

class SystemManager: details::forbid_copies {
 private:
 public:
  class System {
   public:
    virtual ~System() { }
    virtual void update(float time) = 0;
   protected:
    EntityManager &entities() {
      return *manager_->entities_;
    }
   private:
    friend class SystemManager;
    SystemManager *manager_;
  };

  SystemManager(EntityManager &entities) : entities_(&entities) { }

  ~SystemManager() {
    for (auto system : systems_) {
      if (system != nullptr) delete system;
    }
    systems_.clear();
    order_.clear();
  }

  template<typename S, typename ...Args>
  S &add(Args &&... args) {
    ECS_ASSERT_IS_SYSTEM(S);
    ECS_ASSERT(!exists<S>(), "System already exists");
    systems_.resize(system_index<S>() + 1);
    S *system = new S(std::forward<Args>(args) ...);
    system->manager_ = this;
    systems_[system_index<S>()] = system;
    order_.push_back(system_index<S>());
    return *system;
  }

  template<typename S>
  void remove() {
    ECS_ASSERT_IS_SYSTEM(S);
    ECS_ASSERT(exists<S>(), "System does not exist");
    delete systems_[system_index<S>()];
    systems_[system_index<S>()] = nullptr;
    for (auto it = order_.begin(); it != order_.end(); ++it) {
      if (*it == system_index<S>()) {
        order_.erase(it);
      }
    }
  }

  void update(float time) {
    for (auto index : order_) {
      systems_[index]->update(time);
    }
  }

  template<typename S>
  inline bool exists() {
    ECS_ASSERT_IS_SYSTEM(S);
    return systems_.size() > system_index<S>() && systems_[system_index<S>()] != nullptr;
  }

 private:
  template<typename C>
  static size_t system_index() {
    static size_t index = system_counter()++;
    return index;
  }

  static size_t &system_counter() {
    static size_t counter = 0;
    return counter;
  }

  std::vector<System *> systems_;
  std::vector<size_t> order_;
  EntityManager *entities_;
};

using System = SystemManager::System;
} // namespace ecs

#include "System.inl"

#endif //OPENECS_SYSTEM_H
