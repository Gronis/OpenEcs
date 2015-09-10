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
#include <stdexcept>
#include "common/thirdparty/catch.hpp"

#define ECS_ASSERT(Expr, Msg) if(!(Expr)) throw std::runtime_error(Msg);

#include "ecs/ecs.h"

using namespace ecs;

namespace {

int health_component_count;

struct Health: Property<short> {
  Health(short value) : Property<short>(value) {
    health_component_count++;
  }

  inline Health &operator=(const short &value) {
    health_component_count++;
    this->value = value;
    return *this;
  }

  ~Health() { health_component_count--; }
};


struct Mana: Property<float> {
  // Needed for assignment operator
  Mana(float value) {
    this->value = value;
  }
};

struct Weight: Property<int> { };

struct Height: Property<int> { };

struct Clothes { };

struct Shoes { };

struct Hat { };

struct Name: Property<std::string> { };

struct Velocity {
  float x, y;
};

struct Position {
  float x, y;
};

struct Wheels {
  int number;
};

struct Car: EntityAlias<Wheels> {

  Car(float x, float y) : Car() {
    drive(x, y);
  }

  Car() {
    add<Wheels>();
  }

  void drive(float x, float y) {
    set<Velocity>(x, y);
  }

  bool is_moving() {
    if (!has<Velocity>()) return false;
    Velocity &vel = get<Velocity>();
    return !(vel.x == 0 && vel.y == 0);
  }
};

struct Character: EntityAlias<Name, Height, Weight> { };

struct CountCarSystem: System {
  int count = 0;

  virtual void update(float time) {
    count = 0;
    entities().with([&](Wheels &wheels) {
      ++count;
    });
  }
};

struct RemoveDeadEntitiesSystem: System {
  virtual void update(float time) {
    for (auto entity : entities().with<Health>()) {
      if (entity.get<Health>() <= 0) {
        entity.destroy();
      }
    }
  }
};

}

SCENARIO("Testing ecs framework, unittests") {
  GIVEN("An Entity Manager") {
    EntityManager entities;
    health_component_count = 0;

    //Testing entity functions
    GIVEN("1 Entitiy without components") {
      Entity entity = entities.create();

      WHEN("Adding 4 Components to the entity") {
        entity.add<Health>(5);
        entity.add<Mana>(10);
        entity.add<Height>(15);
        entity.add<Weight>(20);
        THEN("They have the components attached") {
          bool has_health = entity.has<Health>();
          bool has_health_and_mana = entity.has<Health, Mana>();
          bool has_health_mana_and_weight = entity.has<Health, Mana, Weight>();
          bool has_health_mana_weight_and_height = entity.has<Health, Mana, Weight, Height>();
          REQUIRE(has_health);
          REQUIRE(has_health_and_mana);
          REQUIRE(has_health_mana_and_weight);
          REQUIRE(has_health_mana_weight_and_height);
        }
        THEN("Accessing added components should work") {
          entity.get<Health>();
          entity.get<Mana>();
          entity.get<Weight>();
          entity.get<Health>();
        }
        THEN("Unpack data from added components should work") {
          REQUIRE(entity.get<Health>().value == 5);
          REQUIRE(entity.get<Mana>().value == 10);
          REQUIRE(entity.get<Height>().value == 15);
          REQUIRE(entity.get<Weight>().value == 20);
        }
        THEN("Accessing other components should not work") {
          REQUIRE_THROWS(entity.get<Clothes>());
          REQUIRE_THROWS(entity.get<Hat>());
          REQUIRE_THROWS(entity.get<Shoes>());
        }
        THEN("Only one Health component should exist") {
          REQUIRE(health_component_count == 1);
        }
        AND_WHEN("When removing the Health component") {
          entity.remove<Health>();
          THEN("No Health component should exist") {
            REQUIRE(health_component_count == 0);
          }
          THEN("Entity should not have that component attached") {
            REQUIRE(!entity.has<Health>());
          }
        }
        AND_WHEN("When removing the Health component twice") {
          entity.remove<Health>();
          THEN("It should not work") {
            REQUIRE_THROWS(entity.remove<Health>());
          }
        }
        AND_WHEN("When removing the Health component twice") {
          entity.remove<Health>();
          THEN("It should not work") {
            REQUIRE_THROWS(entity.remove<Health>());
          }
        }
        AND_WHEN("Destroying the entity") {
          entity.destroy();
          THEN("No Health component should exist") {
            REQUIRE(health_component_count == 0);
          }
        }
        AND_WHEN("Accessing Component as ref and change value") {
          Health &health = entity.get<Health>();
          health.value = 123;
          THEN("Health should be that value") {
            REQUIRE(entity.get<Health>().value == 123);
          }
        }
        AND_WHEN("Accessing Component as value and change value") {
          Health health = entity.get<Health>();
          health.value = 123;
          THEN("Health should not be that value") {
            REQUIRE(entity.get<Health>().value != 123);
          }
        }
      }
      WHEN("Adding the same component twice with \"add\" method") {
        entity.add<Health>(1);
        THEN("Should not work") {
          REQUIRE_THROWS(entity.add<Health>(2));
        }
      }
      WHEN("Adding the same component twice with \"set\" method") {
        entity.set<Health>(1);
        THEN("Should work") {
          entity.set<Health>(2);
        }
      }
      WHEN("Entity remains without components") {
        THEN("It should be valid") {
          REQUIRE(entity.is_valid());
        }
      }
      WHEN("Adding some components to Entity") {
        THEN("It should still be valid") {
          REQUIRE(entity.is_valid());
        }
      }
      WHEN("Destroying entity once") {
        entity.destroy();
        THEN("It should be invalid") {
          REQUIRE(!entity.is_valid());
        }
      }
      WHEN("Destroying entity twice") {
        entity.destroy();
        THEN("It should not work") {
          REQUIRE_THROWS(entity.destroy());
        }
      }
      WHEN("Destroying entity once and replace it with a new entity") {
        entity.destroy();
        Entity entity2 = entities.create();
        THEN("Old entity and new entity should have same id but old should be invalid") {
          REQUIRE(entity.id().index() == entity2.id().index());
          REQUIRE(!entity.is_valid());
          REQUIRE(entity2.is_valid());
        }
      }
      WHEN("Adding Health and Mana with different values to entity") {
        entity.add<Health>(10);
        entity.add<Mana>(20);
        THEN("Health and Mana should be different") {
          REQUIRE(entity.get<Health>() != entity.get<Mana>());
          REQUIRE(entity.get<Mana>() != entity.get<Health>());
        }
      }
      WHEN("Adding Health and Mana with same value to entity") {
        entity.add<Health>(10);
        entity.add<Mana>(10);
        THEN("Health and Mana should be same") {
          REQUIRE(entity.get<Health>() == entity.get<Mana>());
          REQUIRE(entity.get<Mana>() == entity.get<Health>());
        }
      }
    }

    WHEN("Adding 100 entities") {
      auto new_entities = entities.create(100);
      THEN("Number of entities should be 100") {
        REQUIRE(entities.count() == 100);
        REQUIRE(entities.count() == new_entities.size());
      }
      THEN("Iterating through the list should be the same") {
        for (size_t i = 0; i < new_entities.size(); ++i) {
          REQUIRE(new_entities[i] == entities[i]);
        }
      }
      WHEN("Removing all added entities") {
        for (size_t i = 0; i < new_entities.size(); ++i) {
          new_entities[i].destroy();
        }
        THEN("EntityManager should be empty") {
          REQUIRE(entities.count() == 0);
        }
      }
    }

    // Testing EntityManager::View functions
    GIVEN("4 entities with no components attached") {
      Entity e1 = entities.create();
      Entity e2 = entities.create();
      Entity e3 = entities.create();
      Entity e4 = entities.create();
      WHEN("Adding Health with same value to all but one entities") {
        e1.add<Health>(12);
        e2.add<Health>(12);
        e3.add<Health>(12);
        e4.add<Health>(100);
        THEN("All entities should have Health") {
          REQUIRE(entities.with<Health>().count() == entities.count());
        }
        THEN("They should have same amout of getHealth (Not the one with different getHealth ofc)") {
          REQUIRE(e1.get<Health>() == e2.get<Health>());
        }
        AND_WHEN("Adding Mana to 2 of them") {
          e1.add<Mana>(0);
          e2.add<Mana>(0);
          THEN("2 entities should have mana and getHealth") {
            size_t count = entities.with<Mana, Health>().count();
            REQUIRE(count == 2);
          }
          THEN("Entities with Health and Mana == entities with Mana and Health (order independent)") {
            size_t count1 = entities.with<Mana, Health>().count();
            size_t count2 = entities.with<Health, Mana>().count();
            REQUIRE(count1 == count2);
          }
        }
        AND_WHEN("Accessing Health component as references") {
          short &health = e1.get<Health>();
          THEN("Changing the value should affect the actual component") {
            health += 1;
            REQUIRE(health == (int) e1.get<Health>());
          }
        }
        AND_WHEN("Accessing Health component as values") {
          int health = e1.get<Health>();
          THEN("Changing the value should not affect the actual component") {
            health += 1;
            REQUIRE(health != (int) e1.get<Health>());
          }
        }
        AND_WHEN("Accessing e1s' Health as pointer, and setting this pointer to e4s' different Health") {
          Health &health = e1.get<Health>();
          health = e4.get<Health>();
          THEN("Health of e1 should change") {
            REQUIRE(e4.get<Health>().value == e1.get<Health>().value);
          }
        }
        THEN("Iterating over entities with health as type Entity should compile") {
          for (Entity e : entities.with<Health>()) { (void) e; }
        }
        THEN("Iterating over entities with health as type EntityAlias<Health> should compile") {
          for (EntityAlias<Health> e : entities.with<Health>()) { (void) e; }
        }
        THEN("Iterating over entities with health as type EntityAlias<Health> should compile") {
          for (auto e : entities.with<Health>()) {
            typedef std::is_same<decltype(e), EntityAlias<Health>> e_is_entity_alias;
            REQUIRE(e_is_entity_alias::value);
          }
        }
      }
      WHEN("Adding Mana to 2 of them") {
        e1.add<Mana>(0);
        e2.add<Mana>(0);
        THEN("2 entities should have mana") {
          REQUIRE(entities.with<Mana>().count() == 2);
        }
      }
    }
    // Testing component operators
    GIVEN("An Entity with 2 Health and 10 Mana") {
      Entity entity = entities.create();
      entity.add<Health>(2);
      entity.add<Mana>(10);
      WHEN("Adding 2 Health with +=") {
        entity.get<Health>() += 2;
        THEN("Health should be 4") {
          REQUIRE(entity.get<Health>() == 4);
        }
        THEN("Health should be > 1") {
          REQUIRE(entity.get<Health>() > 1);
        }
      }
      WHEN("Multiplying Health with 2 using *=") {
        entity.get<Health>() *= 2;
        THEN("Health should be 4") {
          REQUIRE(entity.get<Health>() == 4);
        }
      }
      WHEN("Multiplying Health with 2 using [health = health * 2]") {
        entity.get<Health>() = entity.get<Health>() * 2;
        THEN("Health should be 4") {
          REQUIRE(entity.get<Health>() == 4);
        }
      }
      WHEN("Multiplying Health with 2 to an int variable") {
        int health = entity.get<Health>() * 2;
        THEN("Health should still be 2 and variable should be 4") {
          REQUIRE(entity.get<Health>() == 2);
          REQUIRE(health == 4);
        }
      }
      WHEN("Adding 2 Health with [health = health + 2]") {
        entity.get<Health>() = entity.get<Health>() + 2;
        THEN("Health should be 4") {
          REQUIRE(entity.get<Health>() == 4);
        }
      }
      WHEN("Adding 2 Health to an int variable") {
        int health = entity.get<Health>() + 2;
        THEN("Health should still be 0 and variable should be 2") {
          REQUIRE(entity.get<Health>() == 2);
          REQUIRE(health == 4);
        }
      }
      WHEN("Multiplying Health with 2 using *=") {
        entity.get<Health>() *= 2;
        THEN("Health should be 4") {
          REQUIRE(entity.get<Health>() == 4);
        }
      }
      WHEN("Subtracting 2 Health with -=") {
        entity.get<Health>() -= 2;
        THEN("Health should be 0") {
          REQUIRE(entity.get<Health>() == 0);
        }
        THEN("Health should be < -1") {
          REQUIRE(entity.get<Health>() < 1);
        }
      }
      WHEN("Dividing Health with 2 using /=") {
        entity.get<Health>() /= 2;
        THEN("Health should be 1") {
          REQUIRE(entity.get<Health>() == 1);
        }
      }
      WHEN("Dividing Health with 2 using [health = health * 2]") {
        entity.get<Health>() = entity.get<Health>() / 2;
        THEN("Health should be 1") {
          REQUIRE(entity.get<Health>() == 1);
        }
      }
      WHEN("Dividing Health with 2 to an int variable") {
        int health = entity.get<Health>() / 2;
        THEN("Health should still be 2 and variable should be 1") {
          REQUIRE(entity.get<Health>() == 2);
          REQUIRE(health == 1);
        }
      }
      AND_WHEN("Dividing Health with 2 using /=") {
        entity.get<Health>() /= 2;
        THEN("Health should be 1") {
          REQUIRE(entity.get<Health>() == 1);
        }
      }
      WHEN("Subtracting 2 Health with [health = health - 2]") {
        entity.get<Health>() = entity.get<Health>() - 2;
        THEN("Health should be 0") {
          REQUIRE(entity.get<Health>() == 0);
        }
      }
      WHEN("Subtracting 2 Health to an int variable") {
        int health = entity.get<Health>() - 2;
        THEN("Health should still be 0 and variable should be -2") {
          REQUIRE(entity.get<Health>() == 2);
          REQUIRE(health == 0);
        }
      }
      WHEN("Setting Health to a value") {
        entity.get<Health>() = 3;
        THEN("Health should be that value") {
          REQUIRE(entity.get<Health>() == 3);
        }
      }
      WHEN("Adding health with ++e.get<Health>()") {
        int health = ++entity.get<Health>();
        THEN("returned value should be new value") {
          REQUIRE(health == 3);
        }
      }
      WHEN("Adding health with e.get<Health>()++") {
        int health = entity.get<Health>()++;
        THEN("Returned value should be old value") {
          REQUIRE(health == 2);
        }
      }
      WHEN("Subtracting health with --e.get<Health>()") {
        int health = --entity.get<Health>();
        THEN("returned value should be new value") {
          REQUIRE(health == 1);
        }
      }
      WHEN("Subtracting health with e.get<Health>()--") {
        int health = entity.get<Health>()--;
        THEN("Returned value should be old value") {
          REQUIRE(health == 2);
        }
      }
      WHEN("Setting its Health to the same level as Mana") {
        entity.get<Health>() = entity.get<Mana>();
        THEN("Entity should have 10 Health") {
          REQUIRE(entity.get<Health>() == 10);
        }
      }
      WHEN("Comparing its Health with its Mana") {
        bool c = entity.get<Health>() == entity.get<Mana>();
        THEN("This should be false") {
          REQUIRE(!c);
        }
      }
    }
    GIVEN("An entity with wheels and health and mana of 1") {
      Entity entity = entities.create();
      entity.add<Wheels>();
      entity.add<Health>(1);
      entity.add<Mana>(1);

      WHEN("Accessing Entity as Car and start driving") {
        Car &car = entity.as<Car>();
        car.drive(1, 1);
        THEN("Car should be driving") {
          REQUIRE(entity.has<Velocity>());
          REQUIRE(entity.get<Velocity>().x == 1);
          REQUIRE(entity.get<Velocity>().y == 1);
        }
      }
      WHEN("Assumeing Entity has Wheels, should work") {
        entity.assume<Wheels>().get<Wheels>();
      }
      // TODO: Find a way to test assert
      /*
      WHEN("Assumeing Entity has Hat, should not work"){
          REQUIRE_THROWS(entity.assume<Hat>());
      }
      */
      WHEN("Creating 2 new entities and fetching every Car") {
        entities.create();
        entities.create();
        auto cars = entities.fetch_every<Car>();
        THEN("Number of cars should be 1, when using count method") {
          REQUIRE(cars.count() == 1);
        }
        THEN("Number of cars should be 1, when counting with for each loop") {
          int count = 0;
          for (Car car : cars) {
            (void) car;
            ++count;
          }
          REQUIRE(count == 1);
        }
        THEN("Number of cars should be 1, when counting with lambda, pass by value") {
          int count = 0;
          entities.fetch_every([&](Car car) {
            ++count;
          });
          REQUIRE(count == 1);
        }
        THEN("Number of cars should be 1 when counting with lambda, pass by reference") {
          int count = 0;
          entities.fetch_every([&](Car &car) {
            ++count;
          });
          REQUIRE(count == 1);
        }
        THEN("Number of entities with Wheels should be one and that entity should have 1 health and mana") {
          int count = 0;
          entities.with([&](Wheels &wheels, Health &health, Mana &mana) {
            ++count;
            REQUIRE(health == 1);
            REQUIRE(mana == 1);
          });
          REQUIRE(count == 1);
        }
        THEN("Unpacking Wheels and entity should work") {
          int count = 0;
          entities.with([&](Wheels &wheels, Entity entity) {
            ++count;
            REQUIRE(&entity.get<Wheels>() == &wheels);
          });
          REQUIRE(count == 1);
        }
        THEN("Getting Health and Mana using get should result in using different get functions") {
          int count = 0;
          for (auto e : entities.with<Wheels, Health>()) {
            ++count;
            REQUIRE(e.get<Health>() == 1);
            REQUIRE(e.get<Mana>() == 1);
            e.remove<Wheels>();
          }
          REQUIRE(count == 1);
        }
        THEN("Unpacking Wheels, Health and Mana components. Accessing components should work") {
          auto tuple = entity.unpack<Wheels, Health, Mana>();
          auto &health = std::get<1>(tuple);
          auto &mana = std::get<2>(tuple);
          REQUIRE(health == 1);
          REQUIRE(mana == 1);
          AND_WHEN("Adding 1 to Mana") {
            mana.value += 1;
            THEN("Mana should be 2") {
              mana = entity.get<Mana>();
              REQUIRE(mana.value == 2);
            }
          }
        }
        AND_WHEN("A change is made to some of the components as ref") {
          entities.with([](Mana &mana) {
            mana = 10;
          });
          THEN("Change should be made") {
            REQUIRE(entity.get<Mana>() == 10);
          }
        }
        AND_WHEN("A change is made to some of the components as copied value") {
          entities.with([](Mana mana) {
            mana = 10;
          });
          THEN("Change should NOT be made") {
            REQUIRE(entity.get<Mana>() != 10);
          }
        }
      }
    }
    WHEN("Creating a car using the EntityManager with speed 10,10") {
      auto car = entities.create<Car>(10, 10);
      THEN("Speed should be 10") {
        REQUIRE(car.get<Velocity>().x == 10);
        REQUIRE(car.get<Velocity>().y == 10);
      }
    }
    WHEN("Creating a car using the EntityManager without setting speed") {
      Car car = entities.create<Car>();
      THEN("Car should be a car") {
        REQUIRE(car.is<Car>());
      }
      THEN("Car should not be moving") {
        REQUIRE(!car.is_moving());
      }
      AND_WHEN("Car starts driving") {
        car.drive(1, 1);
        THEN("Car should be driving") {
          REQUIRE(car.is_moving());
        }
      }
      AND_WHEN("Removing Wheels") {
        car.remove<Wheels>();
        THEN("Car is no longer a car") {
          REQUIRE(!car.is<Car>());
        }
      }
    }
    WHEN("Creating a Character using standard constructor with name: TestCharacter, Height: 180, Weight: 80") {
      Character character = entities.create<Character>("TestCharacter", 180, 80);
      THEN("Name, height and weight should be correct") {
        REQUIRE(character.get<Name>() == "TestCharacter");
        REQUIRE(character.get<Height>() == 180);
        REQUIRE(character.get<Weight>() == 80);
      }
    }

    WHEN("Creating 1 entity with Health and mana") {
      Entity entity = entities.create_with<Health, Mana>(10, 1);
      THEN("They should have health and mana") {
        REQUIRE(entity.has<Health>());
        REQUIRE(entity.has<Mana>());
        REQUIRE(entity.get<Health>() == 10);
        REQUIRE(entity.get<Mana>() == 1);
      }
    }

    WHEN("Creating 1 entity with Health and mana") {
      Entity entity = entities.create_with<Health, Mana>();
      THEN("They should have health and mana") {
        REQUIRE(entity.has<Health>());
        REQUIRE(entity.has<Mana>());
        REQUIRE(entity.get<Health>() == 0);
        REQUIRE(entity.get<Mana>() == 0);
      }
    }

    // Testing so that entities are created at appropriate locations
    WHEN("Creating some entities with different components attached") {
      Entity e1 = entities.create();
      Entity e2 = entities.create_with<Health, Mana>();
      Entity e3 = entities.create();
      Entity e4 = entities.create_with<Health>(10);
      Entity e5 = entities.create_with<Health, Mana>(1, 10);
      THEN("They should be placed in their appropriate locations") {
        REQUIRE(e1.id().index() == 0);
        REQUIRE(e2.id().index() == ECS_CACHE_LINE_SIZE);
        REQUIRE(e3.id().index() == 1);
        REQUIRE(e4.id().index() == ECS_CACHE_LINE_SIZE * 2);
        REQUIRE(e5.id().index() == 1 + ECS_CACHE_LINE_SIZE);
      }
    }

    WHEN("Adding 1 entity, 1 car, then 1 entity in sequence") {
      Entity e1 = entities.create();
      Car c1 = entities.create<Car>();
      Entity e2 = entities.create();
      THEN("They should be assigned to indexes e1 = [0], car = [64] e2 = [1]") {
        REQUIRE(e1.id().index() == 0);
        REQUIRE(e2.id().index() == 1);
        REQUIRE(c1.id().index() == ECS_CACHE_LINE_SIZE);
      }
    }

    WHEN("Adding 1 entity, 1 car, then 64 entities, then 1 car in sequence") {
      Entity e1 = entities.create();
      Car c1 = entities.create<Car>();
      std::vector<Entity> es = entities.create(64);
      Car c2 = entities.create<Car>();
      THEN("They should be assigned to indexes e0 = [0], car = [64] e64 = [128] ()") {
        REQUIRE(e1.id().index() == 0);
        REQUIRE(c1.id().index() == ECS_CACHE_LINE_SIZE);
        REQUIRE(c2.id().index() == ECS_CACHE_LINE_SIZE + 1);
        //REQUIRE(es.back().id().index() == ECS_CACHE_LINE_SIZE * 2);
      }
    }

    WHEN("Adding 1000 entities with, Health, and 1000 with Mana") {
      for (int i = 0; i < 1000; i++) {
        entities.create_with<Health>();
        entities.create_with<Mana>();
        if (i == 800) {
          (void) i;
        }
      }
      THEN("They enitities with Health and Mana respectivly should be 1000") {
        REQUIRE(entities.with<Health>().count() == 1000);
        REQUIRE(entities.with<Mana>().count() == 1000);
      }

    }

    //Testing Systems
    GIVEN("A SystemManager") {
      SystemManager systems(entities);
      WHEN("Adding count car system and remove dead entities system") {
        systems.add<CountCarSystem>();
        systems.add<RemoveDeadEntitiesSystem>();
        THEN("Systems should exists") {
          REQUIRE(systems.exists<CountCarSystem>());
          REQUIRE(systems.exists<RemoveDeadEntitiesSystem>());
        }
        AND_WHEN("Removing one of them") {
          systems.remove<CountCarSystem>();
          THEN("That system should not exist anymore") {
            REQUIRE(!systems.exists<CountCarSystem>());
          }
        }
        AND_WHEN("Adding one dead entity, and calling update") {
          Entity e = entities.create();
          e.add<Health>(-1);
          systems.update(0);
          THEN("Entity should be removed.") {
            REQUIRE(!e.is_valid());
            REQUIRE(entities.count() == 0);
          }
        }
      }
    }
  }
}


namespace ecs {

namespace details {

///--------------------------------------------------------------------
/// Helper functions
///--------------------------------------------------------------------

size_t &component_counter() {
  static size_t counter = 0;
  return counter;
}

size_t inc_component_counter()  {
  size_t index = component_counter()++;
  ECS_ASSERT(index < ECS_MAX_NUM_OF_COMPONENTS, "maximum number of components exceeded.");
  return index;
}

template<typename C>
size_t component_index() {
  static size_t index = inc_component_counter();
  return index;
}

//C1 should not be Entity
template<typename C>
inline auto component_mask() -> typename
std::enable_if<!std::is_same<C, Entity>::value, ComponentMask>::type {
  ComponentMask mask = ComponentMask((1UL << component_index<C>()));
  return mask;
}

//When C1 is Entity, ignore
template<typename C>
inline auto component_mask() -> typename
std::enable_if<std::is_same<C, Entity>::value, ComponentMask>::type {
  return ComponentMask(0);
}

//recursive function for component_mask creation
template<typename C1, typename C2, typename ...Cs>
inline auto component_mask() -> typename
std::enable_if<!std::is_same<C1, Entity>::value, ComponentMask>::type {
  ComponentMask mask = component_mask<C1>() | component_mask<C2, Cs...>();
  return mask;
}


////////////////////////////////////////////////////////////////////////////////

BasePool::BasePool(size_t element_size, size_t chunk_size) :
    size_(0),
    capacity_(0),
    element_size_(element_size),
    chunk_size_(chunk_size) {
}

BasePool::~BasePool() {
  for (char *ptr : chunks_) {
    delete[] ptr;
  }
}
void BasePool::ensure_min_size(std::size_t size) {
  if (size >= size_) {
    if (size >= capacity_) ensure_min_capacity(size);
    size_ = size;
  }
}
void BasePool::ensure_min_capacity(size_t min_capacity) {
  while (min_capacity >= capacity_) {
    char *chunk = new char[element_size_ * chunk_size_];
    chunks_.push_back(chunk);
    capacity_ += chunk_size_;
  }
}


//////////////////////////////////////////////////////////////////////////////

template<typename T>
Pool<T>::Pool(size_t chunk_size) : BasePool(sizeof(T), chunk_size) { }

template<typename T>
void Pool<T>::destroy(index_t index) {
  ECS_ASSERT(index < size_, "Pool has not allocated memory for this index.");
  T *ptr = get_ptr(index);
  ptr->~T();
}

template<typename T>
inline T* Pool<T>::get_ptr(index_t index) {
  ECS_ASSERT(index < capacity_, "Pool has not allocated memory for this index.");
  return reinterpret_cast<T *>(chunks_[index / chunk_size_] + (index % chunk_size_) * element_size_);
}

template<typename T>
inline const T* Pool<T>::get_ptr(index_t index) const {
  ECS_ASSERT(index < this->capacity_, "Pool has not allocated memory for this index.");
  return reinterpret_cast<T *>(chunks_[index / chunk_size_] + (index % chunk_size_) * element_size_);
}

template<typename T>
inline T & Pool<T>::get(index_t index) {
  return *get_ptr(index);
}

template<typename T>
inline const T & Pool<T>::get(index_t index) const {
  return *get_ptr(index);
}

template<typename T>
inline T & Pool<T>::operator[](size_t index) {
  return get(index);
}

template<typename T>
inline const T & Pool<T>::operator[](size_t index) const {
  return get(index);
}

///////////////////////////////////////////////////////////////////////////////////////////////

template<typename C>
ComponentManager<C>::ComponentManager(EntityManager &manager, size_t chunk_size)  :
    manager_(manager),
    pool_(chunk_size)
{ }

// Creating a component that has a defined ctor
template<typename C, typename ...Args>
auto create_component(void* ptr, Args && ... args) ->
typename std::enable_if<std::is_constructible<C, Args...>::value, void>::type {
  new(ptr) C(std::forward<Args>(args)...);
}

// Creating a component that doesn't have ctor, and is not a property -> create using uniform initialization
template<typename C, typename ...Args>
auto create_component(void* ptr, Args && ... args) ->
typename std::enable_if<!std::is_constructible<C, Args...>::value &&
        !std::is_base_of<details::BaseProperty, C>::value, void>::type {
  new(ptr) C{std::forward<Args>(args)...};
}

// Creating a component that doesn't have ctor, and is a property -> create using underlying Property ctor
template<typename C, typename ...Args>
auto create_component(void* ptr, Args && ... args) ->
typename std::enable_if<
    !std::is_constructible<C, Args...>::value &&
        std::is_base_of<details::BaseProperty, C>::value, void>::type {
  static_assert(sizeof...(Args) <= 1, ECS_ASSERT_MSG_ONLY_ONE_ARGS_PROPERTY_CONSTRUCTOR);
  new(ptr) typename C::ValueType(std::forward<Args>(args)...);
}

template<typename C> template<typename ...Args>
C& ComponentManager<C>::create(index_t index, Args &&... args) {
  pool_.ensure_min_size(index + 1);
  create_component<C>(get_ptr(index), std::forward<Args>(args)...);
  return get(index);
}

template<typename C>
void ComponentManager<C>::remove(index_t index) {
  pool_.destroy(index);
  manager_.mask(index).reset(component_index<C>());
}

template<typename C>
C &ComponentManager<C>::operator[](index_t index){
  return get(index);
}

template<typename C>
C &ComponentManager<C>::get(index_t index) {
  return *get_ptr(index);
}

template<typename C>
C const &ComponentManager<C>::get(index_t index) const {
  return *get_ptr(index);
}

template<typename C>
C *ComponentManager<C>::get_ptr(index_t index)  {
  return pool_.get_ptr(index);
}

template<typename C>
C const *ComponentManager<C>::get_ptr(index_t index) const {
  return pool_.get_ptr(index);
}

template<typename C>
ComponentMask ComponentManager<C>::mask() {
  return component_mask<C>();
}


////////////////////////////////////////////////////////////////////

BaseEntityAlias::BaseEntityAlias(Entity &entity) : entity_(entity) { }
BaseEntityAlias::BaseEntityAlias() { }
BaseEntityAlias::BaseEntityAlias(const BaseEntityAlias &other) : entity_(other.entity_) { }

} // namespace details


/////////////////////////////////////////////////////////////////////////

template<typename T, typename E>
inline bool operator==(Property<T> const &lhs, const E &rhs) {
  return lhs.value == rhs;
}

template<typename T, typename E>
inline bool operator!=(Property<T> const &lhs, const E &rhs) {
  return lhs.value != rhs;
}

template<typename T, typename E>
inline bool operator>=(Property<T> const &lhs, const E &rhs) {
  return lhs.value >= rhs;
}

template<typename T, typename E>
inline bool operator>(Property<T> const &lhs, const E &rhs) {
  return lhs.value > rhs;
}

template<typename T, typename E>
inline bool operator<=(Property<T> const &lhs, const E &rhs) {
  return lhs.value <= rhs;
}

template<typename T, typename E>
inline bool operator<(Property<T> const &lhs, const E &rhs) {
  return lhs.value < rhs;
}

template<typename T, typename E>
inline T &operator+=(Property<T> &lhs, const E &rhs) {
  return lhs.value += rhs;
}

template<typename T, typename E>
inline T &operator-=(Property<T> &lhs, const E &rhs) {
  return lhs.value -= rhs;
}

template<typename T, typename E>
inline T &operator*=(Property<T> &lhs, const E &rhs) {
  return lhs.value *= rhs;
}

template<typename T, typename E>
inline T &operator/=(Property<T> &lhs, const E &rhs) {
  return lhs.value /= rhs;
}

template<typename T, typename E>
inline T &operator%=(Property<T> &lhs, const E &rhs) {
  return lhs.value %= rhs;
}

template<typename T, typename E>
inline T &operator&=(Property<T> &lhs, const E &rhs) {
  return lhs.value &= rhs;
}

template<typename T, typename E>
inline T &operator|=(Property<T> &lhs, const E &rhs) {
  return lhs.value |= rhs;
}

template<typename T, typename E>
inline T &operator^=(Property<T> &lhs, const E &rhs) {
  return lhs.value ^= rhs;
}

template<typename T, typename E>
inline T operator+(Property<T> &lhs, const E &rhs) {
  return lhs.value + rhs;
}

template<typename T, typename E>
inline T operator-(Property<T> &lhs, const E &rhs) {
  return lhs.value - rhs;
}

template<typename T, typename E>
inline T operator*(Property<T> &lhs, const E &rhs) {
  return lhs.value * rhs;
}

template<typename T, typename E>
inline T operator/(Property<T> &lhs, const E &rhs) {
  return lhs.value / rhs;
}

template<typename T, typename E>
inline T operator%(Property<T> &lhs, const E &rhs) {
  return lhs.value % rhs;
}

template<typename T>
inline T &operator++(Property<T> &lhs) {
  ++lhs.value;
  return lhs.value;
}

template<typename T>
inline T operator++(Property<T> &lhs, int) {
  T copy = lhs;
  ++lhs;
  return copy;
}

template<typename T>
inline T& operator--(Property<T> &lhs) {
  --lhs.value;
  return lhs.value;
}

template<typename T>
inline T operator--(Property<T> &lhs, int) {
  T copy = lhs;
  --lhs;
  return copy;
}

template<typename T, typename E>
inline T operator&(Property<T> &lhs, const E &rhs) {
  return lhs.value & rhs;
}

template<typename T, typename E>
inline T operator|(Property<T> &lhs, const E &rhs) {
  return lhs.value | rhs;
}

template<typename T, typename E>
inline T operator^(Property<T> &lhs, const E &rhs) {
  return lhs.value ^ rhs;
}

template<typename T, typename E>
inline T operator~(Property<T> &lhs) {
  return ~lhs.value;
}

template<typename T, typename E>
inline T operator>>(Property<T> &lhs, const E &rhs) {
  return lhs.value >> rhs;
}

template<typename T, typename E>
inline T operator<<(Property<T> &lhs, const E &rhs) {
  return lhs.value << rhs;
}



/////////////////////////////////////////////////////////////////////////////////

Id::Id() { }

Id::Id(index_t index, version_t version) :
    index_(index),
    version_(version)
{ }

bool operator==(const Id& lhs, const Id &rhs) {
  return lhs.index() == rhs.index() && lhs.version() == rhs.version();
}

bool operator!=(const Id& lhs, const Id &rhs) {
  return lhs.index() != rhs.index() || lhs.version() != rhs.version();
}

//////////////////////////////////////////////////////////////////////////////////

Entity::Entity(const Entity &other) : Entity(other.manager_, other.id_) { }

Entity::Entity(EntityManager *manager, Id id) :
    manager_(manager),
    id_(id)
{ }

Entity &Entity::operator=(const Entity &rhs) {
  manager_ = rhs.manager_;
  id_ = rhs.id_;
  return *this;
}

template<typename C>
C &Entity::get() {
  return manager_->get_component<C>(*this);
}

template<typename C>
C const &Entity::get() const {
  return manager_->get_component<C>(*this);
}

template<typename C, typename ... Args>
C &Entity::set(Args && ... args){
  return manager_->set_component<C>(*this, std::forward<Args>(args) ...);
}

template<typename C, typename ... Args>
C &Entity::add(Args && ... args){
  return manager_->create_component<C>(*this, std::forward<Args>(args) ...);
}

template<typename T>
inline T & Entity::as(){
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  ECS_ASSERT(has(T::mask()), "Entity doesn't have required components for this EntityAlias");
  return reinterpret_cast<T &>(*this);
}

template<typename T>
inline T const & Entity::as() const{
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  ECS_ASSERT(has(T::mask()), "Entity doesn't have required components for this EntityAlias");
  return reinterpret_cast<T const &>(*this);
}

/// Assume that this entity has provided Components
/// Use for faster component access calls
template<typename ...Components>
inline EntityAlias<Components...> & Entity::assume() {
  return as<EntityAlias<Components...>>();
}

template<typename ...Components>
inline EntityAlias<Components...> const & Entity::assume() const {
  return as<EntityAlias<Components...>>();
}

template<typename C>
void Entity::remove()  {
  manager_->remove_component<C>(*this);
}

void Entity::remove_everything() {
  manager_->remove_all_components(*this);
}

void Entity::clear_mask() {
  manager_->clear_mask(*this);
}

void Entity::destroy() {
  manager_->destroy(*this);
}

template<typename... Components>
bool Entity::has() {
  return manager_->has_component<Components ...>(*this);
}

template<typename... Components>
bool Entity::has() const {
  return manager_->has_component<Components ...>(*this);
}

template<typename T>
bool Entity::is() {
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  return has(T::mask());
}

template<typename T>
bool Entity::is() const {
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  return has(T::mask());
}

bool Entity::is_valid() {
  return manager_->is_valid(*this);
}

bool Entity::is_valid() const {
  return manager_->is_valid(*this);
}

template<typename ...Components>
std::tuple<Components &...>  Entity::unpack() {
  return std::forward_as_tuple(get<Components>()...);
}

template<typename ...Components>
std::tuple<Components const &...> Entity::unpack() const{
  return std::forward_as_tuple(get<Components>()...);
}

bool Entity::has(details::ComponentMask &check_mask)  {
  return manager_->has_component(*this, check_mask);
}

bool Entity::has(details::ComponentMask const &check_mask) const  {
  return manager_->has_component(*this, check_mask);
}

details::ComponentMask &Entity::mask()  {
  return manager_->mask(*this);
}
details::ComponentMask const &Entity::mask() const {
  return manager_->mask(*this);
}

inline bool operator==(const Entity &lhs, const Entity &rhs) {
  return lhs.id() == rhs.id();
}

inline bool operator!=(const Entity &lhs, const Entity &rhs) {
  return lhs.id() != rhs.id();
}


///////////////////////////////////////////////////////////////////////////////////////////////

template<typename T>
Iterator<T>::Iterator(EntityManager *manager, details::ComponentMask mask, bool begin) :
  manager_(manager),
  mask_(mask),
  cursor_(0){
  // Must be pool size because of potential holes
  size_ = manager_->entity_versions_.size();
  if (!begin) cursor_ = size_;
  find_next();
}

template<typename T>
Iterator<T>::Iterator(const Iterator &it) : Iterator(it.manager_, it.cursor_) { };

template<typename T>
index_t Iterator<T>::index() const {
  return cursor_;
}

template<typename T>
inline void Iterator<T>::find_next() {
  while (cursor_ < size_ && (manager_->component_masks_[cursor_] & mask_) != mask_) {
    ++cursor_;
  }
}

template<typename T>
T Iterator<T>::entity() {
  return manager_->get_entity(index()).template as<typename Iterator<T>::T_no_ref>();
}

template<typename T>
const T Iterator<T>::entity() const  {
  return manager_->get_entity(index()).template as<typename Iterator<T>::T_no_ref>();
}

template<typename T>
Iterator<T> &Iterator<T>::operator++() {
  ++cursor_;
  find_next();
  return *this;
}

template<typename T>
bool operator==(Iterator<T> const &lhs, Iterator<T> const &rhs) {
  return lhs.index() == rhs.index();
}

template<typename T>
bool operator!=(Iterator<T> const &lhs, Iterator<T> const &rhs) {
  return !(lhs == rhs);
}

template<typename T>
inline T operator*(Iterator<T> &lhs) {
  return lhs.entity();
}

template<typename T>
inline T const &operator*(Iterator<T> const &lhs) {
  return lhs.entity();
}

///////////////////////////////////////////////////////////////////////////////

template<typename T>
View<T>::View(EntityManager *manager, details::ComponentMask mask)  :
    manager_(manager),
    mask_(mask) { }


template<typename T>
typename View<T>::iterator View<T>::begin() {
  return iterator(manager_, mask_, true);
}

template<typename T>
typename View<T>::iterator View<T>::end() {
  return iterator(manager_, mask_, false);
}

template<typename T>
typename View<T>::const_iterator View<T>::begin() const {
  return const_iterator(manager_, mask_, true);
}

template<typename T>
typename View<T>::const_iterator View<T>::end() const {
  return const_iterator(manager_, mask_, false);
}

template<typename T>
inline index_t View<T>::count() {
  index_t count = 0;
  for (auto it = begin(); it != end(); ++it) {
    ++count;
  }
  return count;
}

template<typename T> template<typename ...Components>
View<T>&& View<T>::with() {
  mask_ |= details::component_mask<Components...>();
  return *this;
}

///////////////////////////////////////////////////////////////////////////////////////////
template<typename ...Cs>
EntityAlias<Cs...>::EntityAlias(Entity &entity) : details::BaseEntityAlias(entity) { }


template<typename ...Cs>
EntityAlias<Cs...>::operator Entity &() {
  return entity_;
}

template<typename ...Cs>
EntityAlias<Cs...>::operator Entity const &() const {
  return entity_;
}

template<typename ...Cs>
Id &EntityAlias<Cs...>::id() {
  return entity_.id();
}

template<typename ...Cs>
Id const &EntityAlias<Cs...>::id() const {
  return entity_.id();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() ->
typename std::enable_if<is_component<C>::value, C &>::type{
  return manager_->get_component_fast<C>(entity_);
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() const ->
typename std::enable_if<is_component<C>::value, C const &>::type{
  return manager_->get_component_fast<C>(entity_);
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() ->
typename std::enable_if<!is_component<C>::value, C &>::type{
  return entity_.get<C>();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::get() const ->
typename std::enable_if<!is_component<C>::value, C const &>::type{
  return entity_.get<C>();
}

template<typename ...Cs> template<typename C, typename ... Args>
inline auto EntityAlias<Cs...>::set(Args &&... args) ->
typename std::enable_if<is_component<C>::value, C &>::type{
  return manager_->set_component_fast<C>(entity_, std::forward<Args>(args)...);
}

template<typename ...Cs> template<typename C, typename ... Args>
inline auto EntityAlias<Cs...>::set(Args &&... args) ->
typename std::enable_if<!is_component<C>::value, C &>::type{
  return manager_->set_component<C>(entity_, std::forward<Args>(args)...);
}


template<typename ...Cs> template<typename C, typename ... Args>
inline C& EntityAlias<Cs...>::add(Args &&... args) {
  return entity_.add<C>(std::forward<Args>(args)...);
}

template<typename ...Cs> template<typename C>
C & EntityAlias<Cs...>::as() {
  return entity_.as<C>();
}

template<typename ...Cs> template<typename C>
C const & EntityAlias<Cs...>::as() const {
  return entity_.as<C>();
}

template<typename ...Cs> template<typename ...Components_>
EntityAlias<Components_...> &EntityAlias<Cs...>::assume() {
  return entity_.assume<Components_...>();
}

template<typename ...Cs> template<typename ...Components>
EntityAlias<Components...> const &EntityAlias<Cs...>::assume() const {
  return entity_.assume<Components...>();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::remove() ->
typename std::enable_if<!is_component<C>::value, void>::type {
  entity_.remove<C>();
}

template<typename ...Cs> template<typename C>
inline auto EntityAlias<Cs...>::remove() ->
typename std::enable_if<is_component<C>::value, void>::type {
  manager_->remove_component_fast<C>(entity_);
}

template<typename ...Cs>
void EntityAlias<Cs...>::remove_everything() {
  entity_.remove_everything();
}

template<typename ...Cs>
void EntityAlias<Cs...>::clear_mask() {
  entity_.clear_mask();
}

template<typename ...Cs>
void EntityAlias<Cs...>::destroy() {
  entity_.destroy();
}

template<typename ...Cs> template<typename... Components>
bool EntityAlias<Cs...>::has() {
  return entity_.has<Components...>();
}

template<typename ...Cs> template<typename... Components>
bool EntityAlias<Cs...>::has() const {
  return entity_.has<Components...>();
}

template<typename ...Cs> template<typename T>
bool EntityAlias<Cs...>::is() {
  return entity_.is<T>();
}
template<typename ...Cs> template<typename T>
bool EntityAlias<Cs...>::is() const {
  return entity_.is<T>();
}

template<typename ...Cs>
bool EntityAlias<Cs...>::is_valid() {
  return entity_.is_valid();
}

template<typename ...Cs>
bool EntityAlias<Cs...>::is_valid() const {
  return entity_.is_valid();
}

template<typename ...Cs> template<typename... Components>
std::tuple<Components &...> EntityAlias<Cs...>::unpack(){
  return entity_.unpack<Components...>();
}

template<typename ...Cs> template<typename... Components>
std::tuple<Components const &...> EntityAlias<Cs...>::unpack() const{
  return entity_.unpack<Components...>();
}

template<typename ...Cs>
std::tuple<Cs &...> EntityAlias<Cs...>::unpack(){
  return entity_.unpack<Cs...>();
}

template<typename ...Cs>
std::tuple<Cs const &...> EntityAlias<Cs...>::unpack() const{
  return entity_.unpack<Cs...>();
}

template<typename ...Cs>
EntityAlias<Cs...>::EntityAlias() {

}

template<typename ...Cs>
details::ComponentMask EntityAlias<Cs...>::mask(){
  return details::component_mask<Cs...>();
}

template<typename ... Cs>
inline bool operator==(const EntityAlias<Cs...> &lhs, const Entity &rhs) {
  return lhs.entity_ == rhs;
}

template<typename ... Cs>
inline bool operator!=(const EntityAlias<Cs...> &lhs, const Entity &rhs) {
  return lhs.entity_ != rhs;
}


////////////////////////////////////////////////////////////////////////////////////////////////


EntityManager::EntityManager(size_t chunk_size) {
  entity_versions_.reserve(chunk_size);
  component_masks_.reserve(chunk_size);
}


EntityManager::~EntityManager()  {
  for (details::BaseManager *manager : component_managers_) {
    if (manager) delete manager;
  }
  component_managers_.clear();
  component_masks_.clear();
  entity_versions_.clear();
  next_free_indexes_.clear();
  component_mask_to_index_accessor_.clear();
  index_to_component_mask.clear();
}

Entity EntityManager::create() {
  return create_with_mask(details::ComponentMask(0));
}


std::vector<Entity> EntityManager::create(const size_t num_of_entities)  {
  return create_with_mask(details::ComponentMask(0), num_of_entities);
}

/// If EntityAlias is constructable with Args...
template<typename T, typename ...Args>
auto EntityManager::create(Args && ... args) ->
typename std::enable_if<std::is_constructible<T, Args...>::value, T>::type {
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  typedef typename T::Type Type;
  auto mask = T::mask();
  Entity entity = create_with_mask(mask);
  T *entity_alias = new(&entity) T(std::forward<Args>(args)...);
  ECS_ASSERT(entity.has(mask),
             "Every required component must be added when creating an Entity Alias");
  return *entity_alias;
}

/// If EntityAlias is not constructable with Args...
/// Attempt to create with underlying EntityAlias
template<typename T, typename ...Args>
auto EntityManager::create(Args && ... args) ->
typename std::enable_if<!std::is_constructible<T, Args...>::value, T>::type {
  ECS_ASSERT_IS_ENTITY(T);
  ECS_ASSERT_ENTITY_CORRECT_SIZE(T);
  typedef typename T::Type Type;
  auto mask = T::mask();
  Entity entity = create_with_mask(mask);
  Type *entity_alias = new(&entity) Type();
  entity_alias->init(std::forward<Args>(args)...);
  ECS_ASSERT(entity.has(mask),
             "Every required component must be added when creating an Entity Alias");
  return *reinterpret_cast<T *>(entity_alias);
}

template<typename ...Components, typename ...Args>
EntityAlias<Components...> EntityManager::create_with(Args && ... args ) {
  using Type =  EntityAlias<Components...>;
  Entity entity = create_with_mask(details::component_mask<Components...>());
  Type *entity_alias = new(&entity) Type();
  entity_alias->init(std::forward<Args>(args)...);
  return *entity_alias;
}

template<typename ...Components>
EntityAlias<Components...> EntityManager::create_with() {
  using Type = EntityAlias<Components...>;
  Entity entity = create_with_mask(details::component_mask<Components...>());
  Type *entity_alias = new(&entity) Type();
  entity_alias->init();
  return *entity_alias;
}

Entity EntityManager::create_with_mask(details::ComponentMask mask)  {
  ++count_;
  index_t index = find_new_entity_index(mask);
  size_t slots_required = index + 1;
  if (entity_versions_.size() < slots_required) {
    entity_versions_.resize(slots_required);
    component_masks_.resize(slots_required, details::ComponentMask(0));
  }
  return get_entity(index);
}

std::vector<Entity> EntityManager::create_with_mask(details::ComponentMask mask, const size_t num_of_entities) {
  std::vector<Entity> new_entities;
  index_t index;
  size_t entities_left = num_of_entities;
  new_entities.reserve(entities_left);
  auto mask_as_ulong = mask.to_ulong();
  IndexAccessor &index_accessor = component_mask_to_index_accessor_[mask_as_ulong];
  //See if we can use old indexes for destroyed entities via free list
  while (!index_accessor.free_list.empty() && entities_left--) {
    new_entities.push_back(get_entity(index_accessor.free_list.back()));
    index_accessor.free_list.pop_back();
  }
  index_t block_index = 0;
  index_t current = ECS_CACHE_LINE_SIZE; // <- if empty, create new block instantly
  size_t slots_required;
  // Are there any unfilled blocks?
  if (!index_accessor.block_index.empty()) {
    block_index = index_accessor.block_index[index_accessor.block_index.size() - 1];
    current = next_free_indexes_[block_index];
    slots_required = block_count_ * ECS_CACHE_LINE_SIZE + current + entities_left;
  } else {
    slots_required = block_count_ * ECS_CACHE_LINE_SIZE + entities_left;
  }
  entity_versions_.resize(slots_required);
  component_masks_.resize(slots_required, details::ComponentMask(0));

  // Insert until no entity is left or no block remain
  while (entities_left) {
    for (; current < ECS_CACHE_LINE_SIZE && entities_left; ++current) {
      new_entities.push_back(get_entity(current + ECS_CACHE_LINE_SIZE * block_index));
      entities_left--;
    }
    // Add more blocks if there are entities left
    if (entities_left) {
      block_index = block_count_;
      create_new_block(index_accessor, mask_as_ulong, 0);
      ++block_count_;
      current = 0;
    }
  }
  count_ += num_of_entities;
  return new_entities;
}

template<typename ...Components>
View<EntityAlias<Components...>> EntityManager::with()  {
  details::ComponentMask mask = details::component_mask<Components...>();
  return View<EntityAlias<Components...>>(this, mask);
}

template<typename T>
void EntityManager::with(T lambda)  {
  ECS_ASSERT_IS_CALLABLE(T);
  with_<T>::for_each(*this, lambda);
}


template<typename T>
View<T> EntityManager::fetch_every()  {
  ECS_ASSERT_IS_ENTITY(T);
  return View<T>(this, T::mask());
}


template<typename T>
void EntityManager::fetch_every(T lambda)  {
  ECS_ASSERT_IS_CALLABLE(T);
  typedef details::function_traits<T> function;
  static_assert(function::arg_count == 1, "Lambda or function must only have one argument");
  typedef typename function::template arg_remove_ref<0> entity_interface_t;
  for (entity_interface_t entityInterface : fetch_every<entity_interface_t>()) {
    lambda(entityInterface);
  }
}


Entity EntityManager::operator[](index_t index) {
  return get_entity(index);
}


Entity EntityManager::operator[](Id id)  {
  Entity entity = get_entity(id);
  ECS_ASSERT(id == entity.id(), "Id is no longer valid (Entity was destroyed)");
  return entity;
}

size_t EntityManager::count(){
  return count_;
}

index_t EntityManager::find_new_entity_index(details::ComponentMask mask) {
  auto mask_as_ulong = mask.to_ulong();
  IndexAccessor &index_accessor = component_mask_to_index_accessor_[mask_as_ulong];
  //See if we can use old indexes for destroyed entities via free list
  if (!index_accessor.free_list.empty()) {
    auto index = index_accessor.free_list.back();
    index_accessor.free_list.pop_back();
    return index;
  }
  // EntityManager has created similar entities already
  if (!index_accessor.block_index.empty()) {
    //No free_indexes in free list (removed entities), find a new index
    //at last block, if that block has free slots
    auto &block_index = index_accessor.block_index[index_accessor.block_index.size() - 1];
    auto &current = next_free_indexes_[block_index];
    // If block has empty slot, use it
    if (ECS_CACHE_LINE_SIZE > current) {
      return (current++) + ECS_CACHE_LINE_SIZE * block_index;
    }
  }
  create_new_block(index_accessor, mask_as_ulong, 1);
  return (block_count_++) * ECS_CACHE_LINE_SIZE;
}

void EntityManager::create_new_block(EntityManager::IndexAccessor &index_accessor,
                                     unsigned long mask_as_ulong,
                                     index_t next_free_index)  {
  index_accessor.block_index.push_back(block_count_);
  next_free_indexes_.resize(block_count_ + 1);
  index_to_component_mask.resize(block_count_ + 1);
  next_free_indexes_[block_count_] = next_free_index;
  index_to_component_mask[block_count_] = mask_as_ulong;
}

template<typename C, typename ...Args>
details::ComponentManager<C> &EntityManager::create_component_manager(Args && ... args)  {
  details::ComponentManager<C> *ptr = new details::ComponentManager<C>(std::forward<EntityManager &>(*this),
                                                                       std::forward<Args>(args) ...);
  component_managers_[details::component_index<C>()] = ptr;
  return *ptr;
}

template<typename C>
details::ComponentManager<C> &EntityManager::get_component_manager_fast() {
  return *reinterpret_cast<details::ComponentManager<C> *>(component_managers_[details::component_index<C>()]);
}

template<typename C>
details::ComponentManager<C> const &EntityManager::get_component_manager_fast() const {
  return *reinterpret_cast<details::ComponentManager<C> *>(component_managers_[details::component_index<C>()]);
}


template<typename C>
details::ComponentManager<C> &EntityManager::get_component_manager()  {
  auto index = details::component_index<C>();
  if (component_managers_.size() <= index) {
    component_managers_.resize(index + 1, nullptr);
    return create_component_manager<C>();
  } else if (component_managers_[index] == nullptr) {
    return create_component_manager<C>();
  }
  return *reinterpret_cast<details::ComponentManager<C> *>(component_managers_[index]);
}


template<typename C>
details::ComponentManager<C> const &EntityManager::get_component_manager() const  {
  auto index = details::component_index<C>();
  ECS_ASSERT(component_managers_.size() > index, component_managers_[index] == nullptr &&
      "Component manager not created");
  return *reinterpret_cast<details::ComponentManager<C> *>(component_managers_[index]);
}

template<typename C>
C &EntityManager::get_component(Entity &entity) {
  ECS_ASSERT(has_component<C>(entity), "Entity doesn't have this component attached");
  return get_component_manager<C>().get(entity.id_.index_);
}

template<typename C>
C const &EntityManager::get_component(Entity const &entity) const {
  ECS_ASSERT(has_component<C>(entity), "Entity doesn't have this component attached");
  return get_component_manager<C>().get(entity.id_.index_);
}

template<typename C>
C &EntityManager::get_component_fast(index_t index)  {
  return get_component_manager_fast<C>().get(index);
}

template<typename C>
C const &EntityManager::get_component_fast(index_t index) const {
  return get_component_manager_fast<C>().get(index);
}


template<typename C>
C &EntityManager::get_component_fast(Entity &entity)  {
  return get_component_manager_fast<C>().get(entity.id_.index_);
}

template<typename C>
C const &EntityManager::get_component_fast(Entity const &entity) const  {
  return get_component_manager_fast<C>().get(entity.id_.index_);
}

template<typename C, typename ...Args>
C &EntityManager::create_component(Entity &entity, Args && ... args) {
  ECS_ASSERT_VALID_ENTITY(entity);
  ECS_ASSERT(!has_component<C>(entity), "Entity already has this component attached");
  C &component = get_component_manager<C>().create(entity.id_.index_, std::forward<Args>(args) ...);
  entity.mask().set(details::component_index<C>());
  return component;
}

template<typename C>
void EntityManager::remove_component(Entity &entity)  {
  ECS_ASSERT_VALID_ENTITY(entity);
  ECS_ASSERT(has_component<C>(entity), "Entity doesn't have component attached");
  get_component_manager<C>().remove(entity.id_.index_);
}

template<typename C>
void EntityManager::remove_component_fast(Entity &entity) {
  ECS_ASSERT_VALID_ENTITY(entity);
  ECS_ASSERT(has_component<C>(entity), "Entity doesn't have component attached");
  get_component_manager_fast<C>().remove(entity.id_.index_);
}

void EntityManager::remove_all_components(Entity &entity)  {
  ECS_ASSERT_VALID_ENTITY(entity);
  for (auto componentManager : component_managers_) {
    if (componentManager && has_component(entity, componentManager->mask())) {
      componentManager->remove(entity.id_.index_);
    }
  }
}

void EntityManager::clear_mask(Entity &entity) {
  ECS_ASSERT_VALID_ENTITY(entity);
  component_masks_[entity.id_.index_].reset();
}

template<typename C, typename ...Args>
C &EntityManager::set_component(Entity &entity, Args && ... args) {
  ECS_ASSERT_VALID_ENTITY(entity);
  if (entity.has<C>()) {
  return get_component_fast<C>(entity) = create_tmp_component<C>(std::forward<Args>(args)...);
  }
  else return create_component<C>(entity, std::forward<Args>(args)...);
}

template<typename C, typename ...Args>
C &EntityManager::set_component_fast(Entity &entity, Args && ... args) {
  ECS_ASSERT_VALID_ENTITY(entity);
  ECS_ASSERT(entity.has<C>(), "Entity does not have component attached");
  return get_component_fast<C>(entity) = create_tmp_component<C>(std::forward<Args>(args)...);
}

bool EntityManager::has_component(Entity &entity, details::ComponentMask component_mask) {
  ECS_ASSERT_VALID_ENTITY(entity);
  return (mask(entity) & component_mask) == component_mask;
}

bool EntityManager::has_component(Entity const &entity, details::ComponentMask const &component_mask) const  {
  ECS_ASSERT_VALID_ENTITY(entity);
  return (mask(entity) & component_mask) == component_mask;
}

bool EntityManager::has_component(index_t index, details::ComponentMask &component_mask) {
  return (mask(index) & component_mask) == component_mask;
}

template<typename ...Components>
bool EntityManager::has_component(Entity &entity) {
  return has_component(entity, details::component_mask<Components...>());
}

template<typename ...Components>
bool EntityManager::has_component(Entity const &entity) const  {
  return has_component(entity, details::component_mask<Components...>());
}


bool EntityManager::is_valid(Entity &entity)  {
  return entity.id_.index_ < entity_versions_.size() &&
      entity.id_.version_ == entity_versions_[entity.id_.index_];
}

bool EntityManager::is_valid(Entity const &entity) const  {
  return entity.id_.index_ < entity_versions_.size() &&
      entity.id_.version_ == entity_versions_[entity.id_.index_];
}

void EntityManager::destroy(Entity &entity) {
  index_t index = entity.id().index_;
  remove_all_components(entity);
  ++entity_versions_[index];
  auto &mask_as_ulong = index_to_component_mask[index / ECS_CACHE_LINE_SIZE];
  component_mask_to_index_accessor_[mask_as_ulong].free_list.push_back(index);
  --count_;
}

details::ComponentMask &EntityManager::mask(Entity &entity)  {
  return mask(entity.id_.index_);
}

details::ComponentMask const &EntityManager::mask(Entity const &entity) const {
  return mask(entity.id_.index_);
}

details::ComponentMask &EntityManager::mask(index_t index) {
  return component_masks_[index];
}

details::ComponentMask const &EntityManager::mask(index_t index) const {
  return component_masks_[index];
}

Entity EntityManager::get_entity(Id id) {
  return Entity(this, id);
}

Entity EntityManager::get_entity(index_t index) {
  return get_entity(Id(index, entity_versions_[index]));
}

size_t EntityManager::capacity() const  {
  return entity_versions_.capacity();
}

} // namespace ecs