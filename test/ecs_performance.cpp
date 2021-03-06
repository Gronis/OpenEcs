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

#include <iostream>
#include <chrono>

#include "common/thirdparty/catch.hpp"

#include "ecs.h"

using namespace ecs;

namespace {
struct Wheels {
  int value;
};

struct Door {
  int value;
};

struct Hat {
  int i;
};

struct Clothes {
  int i[16];
};

struct Car: EntityAlias<Wheels> {
};

// Used to make sure the comparator does not optimize away my for-loop
struct BaseFoo {
  virtual void bar() = 0;
  bool always_false = false;
};

struct Foo: BaseFoo {
  virtual void bar() override {

  }
};
}

class Timer {
 public:
  Timer() { restart(); }
  ~Timer() { std::cout << "Time elapsed: " << elapsed() << std::endl << std::endl; }

  void restart() { _start = std::chrono::system_clock::now(); }
  double elapsed() { return std::chrono::duration<double>(std::chrono::system_clock::now() - _start).count(); }
 private:
  std::chrono::time_point<std::chrono::system_clock> _start;
};

TEST_CASE("TestEntityCreation") {
  int count = 10000000;
  EntityManager em;

  std::cout << "Creating " << count << " entities using create()" << std::endl;
  Timer t;
  for (int i = 0; i < count; ++i) {
    em.create();
  }
  REQUIRE(em.count() == count);
}

TEST_CASE("TestEntityCreationMany") {
  int count = 10000000;
  EntityManager em;

  std::cout << "Creating " << count << " entities using create(int)" << std::endl;
  Timer t;
  auto new_entities = em.create(count);
  REQUIRE(em.count() == count);
}

TEST_CASE("TestEntityDestruction") {
  int count = 10000000;
  EntityManager em;
  auto entities = em.create(count);
  {
    std::cout << "Destroying " << count << " entities" << std::endl;
    Timer t;
    for (size_t i = 0; i < entities.size(); ++i) {
      entities[i].destroy();
    }
    REQUIRE(em.count() == 0);
  }
  {
    std::cout << "Recreating after destroying " << count << " entities" << std::endl;
    Timer t;
    auto entities = em.create(count);
  }
}

SCENARIO("TestEntityIteration") {
  const int count = 10000000;
  EntityManager entities;
  for (int i = 0; i < count; i++) {
    entities.create_with<Wheels, Door>();
  }

  WHEN("Iterating using normal for-loop") {
    std::cout << "Iterating over " << count << " using normal for loop" << std::endl;
    BaseFoo *foo = new Foo();
    {
      Timer t;
      for (int i = 0; i < count; ++i) {
        //we don't want to call dummy function.
        if (foo->always_false) {
          //useless function call to make sure the loop is not optimized away.
          foo->bar();
        }
      }
    }
  }

  WHEN("Iterating manually with iterator") {
    std::cout << "Iterating over " << count << " using iterator manually" << std::endl;
    Timer t;
    auto it = entities.with<Wheels>().begin();
    auto end = entities.with<Wheels>().end();
    for (; it != end; ++it) {
      (void) it;
    }
  }

  WHEN("Iterating using with") {
    std::cout << "Iterating over " << count << " using with for-loop without unpacking" << std::endl;
    Timer t;
    for (auto e : entities.with<Wheels>()) {
      (void) e;
    }
  }

  WHEN("Iterating using with") {
    std::cout << "Iterating over " << count << " using with for-loop unpacking one component" << std::endl;
    Timer t;
    for (auto e : entities.with<Wheels>()) {
      e.get<Wheels>();
    }
  }

  WHEN("Iterating using with") {
    std::cout << "Iterating over " << count << " using with lambda unpacking one component" << std::endl;
    Timer t;
    entities.with([](Wheels &wheels) {
      (void) wheels;
    });
  }

  WHEN("Iterating using with") {
    std::cout << "Iterating over " << count << " using with for-loop unpacking two components" << std::endl;
    Timer t;
    for (auto e : entities.with<Wheels, Door>()) {
      e.get<Wheels>();
      e.get<Door>();
    }
  }

  WHEN("Iterating using with") {
    std::cout << "Iterating over " << count << " using with lambda unpacking two components" << std::endl;
    Timer t;
    entities.with([](Wheels &wheels, Door &door) {
      (void) wheels;
      (void) door;
    });
  }

  WHEN("Iterating using fetch_every") {
    std::cout << "Iterating over " << count << " using fetch_every for-loop" << std::endl;
    Timer t;
    for (auto e : entities.fetch_every<Car>()) {
      (void) e;
    }
  }

  WHEN("Iterating using fetch_every") {
    std::cout << "Iterating over " << count << " using fetch_every lambda" << std::endl;
    Timer t;
    entities.fetch_every([](Car &car) {
      (void) car;
    });
  }
}

SCENARIO("TestEntityIterationForContinousMemory hard code") {
  const int count = 10000000;
  Wheels* wheels = new Wheels[count];

  WHEN("Iterating over entities with doors (hard code)") {
    std::cout << "Iterating over " << count << " using with Doors continous in memory (hard code)" << std::endl;
    BaseFoo *foo = new Foo();
    {
      Timer t;
      for (int i = 0; i < count; ++i) {
        if(foo->always_false){
          foo->bar();
        }
        (void)wheels[i].value;
      }
    }
  }
}

SCENARIO("TestEntityIterationForSplittedMemory hard code") {
  const int count = 10000000;
  Wheels* wheels = new Wheels[count * 20];

  WHEN("Iterating over entities with doors (hard code)") {
    std::cout << "Iterating over " << count << " using with Doors split in memory (hard code)" << std::endl;
    BaseFoo *foo = new Foo();
    {
      Timer t;
      for (int i = 0; i < count * 20; i+=20) {
        if(foo->always_false){
          foo->bar();
        }
        (void)wheels[i].value;
      }
    }
  }
}

SCENARIO("TestEntityIterationForContinousMemory") {
  int count = 10000000;
  EntityManager entities;

  for (int i = 0; i < count / 16; ++i) {
    for (int j = 0; j < 15; ++j) {
      entities.create_with<Wheels>();
    }
    entities.create_with<Clothes>();
  }
  REQUIRE(entities.with<Clothes>().count() == count / 16);

  WHEN("Iterating over entities with doors") {
    std::cout << "Iterating over " << count << " using with Doors continous in memory" << std::endl;
    {
      Timer t;
      entities.with([](Clothes &clothes) {  clothes.i[0] = 0; });
    }
  }
}

SCENARIO("TestEntityIterationForContinousMemory Unallocated") {
  int count = 10000000;
  EntityManager entities;
  for (int i = 0; i < count / 16; ++i) {
    for (int j = 0; j < 15; ++j) {
      entities.create().add<Wheels>();
    }
    entities.create().add<Clothes>();
  }
  REQUIRE(entities.with<Clothes>().count() == count / 16);

  WHEN("Iterating over entities with doors") {
    std::cout << "Iterating over " << count << " using with Doors, continous in memory (unallocated)" << std::endl;
    {
      Timer t;
      entities.with([](Clothes &clothes) {  clothes.i[0] = 0; });
    }
  }
}

SCENARIO("TestEntityIterationForSplittedMemory") {
  int count = 10000000;
  EntityManager entities;
  for (int i = 0; i < count / 16; ++i) {
    for (int j = 0; j < 15; ++j) {
      Entity entity = entities.create();
      entity.add<Wheels>();
    }
    Entity entity = entities.create();
    entity.add<Clothes>();
  }
  REQUIRE(entities.with<Clothes>().count() == count / 16);

  WHEN("Iterating over entities with doors") {
    std::cout << "Iterating over " << count << " using with Doors, split in memory" << std::endl;
    {
      Timer t;
      entities.with([](Clothes &clothes) { clothes.i[0] = 0; });
    }
  }
}

SCENARIO("TestEntityCreationUnallocatedEntity") {
  int count = 10000000;
  EntityManager entities;
  std::cout << "Creating " << count << " with with Doors, Unallocated Entity" << std::endl;
  {
    Timer t;
    for (int i = 0; i < count; ++i) {
      entities.create().add<Door>();
    }
  }
}

SCENARIO("TestEntityCreationEntity") {
  int count = 10000000;
  EntityManager entities;
  std::cout << "Creating " << count << " with with Doors, Entity" << std::endl;
  {
    Timer t;
    for (int i = 0; i < count; ++i) {
      Entity e = entities.create();
      e.add<Door>();
    }
  }
}

SCENARIO("TestEntityCreation_with_Entity") {
  int count = 10000000;
  EntityManager entities;
  std::cout << "Creating " << count << " with with Doors, Entity create_with" << std::endl;
  {
    Timer t;
    for (int i = 0; i < count; ++i) {
      entities.create_with<Door>();
    }
  }
}