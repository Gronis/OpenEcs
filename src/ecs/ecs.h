/// --------------------------------------------------------------------------
/// Copyright (C) 2015  Robin Grönberg
///
/// This program is free software: you can redistribute it and/or modify
/// it under the terms of the GNU General Public License as published by
/// the Free Software Foundation, either version 3 of the License, or
/// (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program.  If not, see <http://www.gnu.org/licenses/>.

#pragma once
#ifndef ECS_SINGLE_HEADER
#define ECS_SINGLE_HEADER

#include <bitset>
#include <vector>
#include <map>
#include <string>
#include <functional>
#include <cassert>
#include <iostream>


#include "Defines.h"
#include "Pool.h"
#include "ComponentManager.h"
#include "Property.h"
#include "Id.h"
#include "Entity.h"
#include "EntityAlias.h"
#include "Iterator.h"
#include "View.h"
#include "EntityManager.h"

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

#endif //Header guard