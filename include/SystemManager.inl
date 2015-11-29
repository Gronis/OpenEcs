#include "System.h"

namespace ecs{

SystemManager::~SystemManager() {
  for (auto system : systems_) {
    if (system != nullptr) delete system;
  }
  systems_.clear();
  order_.clear();
}

template<typename S, typename ...Args>
S& SystemManager::add(Args &&... args) {
  ECS_ASSERT_IS_SYSTEM(S);
  ECS_ASSERT(!exists<S>(), "System already exists");
  systems_.resize(details::system_index<S>() + 1);
  S *system = new S(std::forward<Args>(args) ...);
  system->manager_ = this;
  systems_[details::system_index<S>()] = system;
  order_.push_back(details::system_index<S>());
  return *system;
}

template<typename S>
void SystemManager::remove() {
  ECS_ASSERT_IS_SYSTEM(S);
  ECS_ASSERT(exists<S>(), "System does not exist");
  delete systems_[details::system_index<S>()];
  systems_[details::system_index<S>()] = nullptr;
  for (auto it = order_.begin(); it != order_.end(); ++it) {
    if (*it == details::system_index<S>()) {
      order_.erase(it);
    }
  }
}

void SystemManager::update(float time) {
  for (auto index : order_) {
    systems_[index]->update(time);
  }
}

template<typename S>
inline bool SystemManager::exists() {
  ECS_ASSERT_IS_SYSTEM(S);
  return systems_.size() > details::system_index<S>() && systems_[details::system_index<S>()] != nullptr;
}

} // namespace ecs
