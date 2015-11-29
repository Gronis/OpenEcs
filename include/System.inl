#include "SystemManager.h"

namespace ecs{

EntityManager& System::entities(){
  return *manager_->entities_;
}

} //namespace ecs
