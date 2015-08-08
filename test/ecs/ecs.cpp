
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
#include <time.h>
#include "common/thirdparty/catch.hpp"

#include "ecs/ecs.h"

using namespace ecs;

namespace {

int health_component_count;

struct Health : Property<short> {
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


struct Mana : Property<float> {
    Mana(float value) {
        this->value = value;
    }
};

struct Weight : Property<int> {
    Weight(int value) : Property<int>(value) { }
};

struct Height : Property<int> {
    Height(int value) : Property<int>(value) { }
};

struct Clothes {
};

struct Shoes {
};

struct Hat {
};

struct Name : Property<std::string> {
    Name(std::string value) : Property<std::string>(value) { }
};

struct Velocity {
    Velocity(float x, float y) : x(x), y(y) { }
    float x, y;
};

struct Wheels {
    int number;
};

struct Car : EntityAlias<Wheels> {

    Car(float x, float y) : Car(){
        drive(x,y);
    }

    Car(){
        add<Wheels>();
    }

    void drive(float x, float y) {
        set<Velocity>(x, y);
    }

    bool is_moving(){
        if(!has<Velocity>()) return false;
        Velocity& vel = get<Velocity>();
        return !(vel.x == 0 && vel.y == 0);
    }
};

struct CountCarSystem : System<CountCarSystem>{
    int count = 0;

    virtual void update(float time){
        count = 0;
        entities().with([&](Wheels& wheels){
            ++count;
        });
    }
};

struct RemoveDeadEntitiesSystem : System<RemoveDeadEntitiesSystem>{
    virtual void update(float time){
        for(auto entity : entities().with<Health>()){
            if(entity.get<Health>() <= 0){
                entity.destroy();
            }
        }
    }
};

}

SCENARIO("Testing ecs framework, unittests"){
    GIVEN("An Entity Manager"){
        EntityManager entities;
        health_component_count = 0;
        
        //Testing entity functions
        GIVEN("1 Entitiy without components"){
            Entity entity = entities.create();
            
            WHEN("Adding 4 Components to the entity"){
                entity.add<Health>(5);
                entity.add<Mana>(10);
                entity.add<Height>(15);
                entity.add<Weight>(20);
                THEN("They have the components attached"){
                    bool has_health = entity.has<Health>();
                    bool has_health_and_mana = entity.has<Health, Mana>();
                    bool has_health_mana_and_weight = entity.has<Health, Mana, Weight>();
                    bool has_health_mana_weight_and_height = entity.has<Health,Mana, Weight, Height>();
                    REQUIRE(has_health);
                    REQUIRE(has_health_and_mana);
                    REQUIRE(has_health_mana_and_weight);
                    REQUIRE(has_health_mana_weight_and_height);
                }
                THEN("Accessing added components should work"){
                    entity.get<Health>();
                    entity.get<Mana>();
                    entity.get<Weight>();
                    entity.get<Health>();
                }
                THEN("Unpack data from added components should work"){
                    REQUIRE(entity.get<Health>().value == 5);
                    REQUIRE(entity.get<Mana>().value == 10);
                    REQUIRE(entity.get<Height>().value == 15);
                    REQUIRE(entity.get<Weight>().value == 20);
                }
                // TODO: Find a way to test assert
                /*
                THEN("Accessing other components should not work"){
                    REQUIRE_THROWS(entity.get<Clothes>());
                    REQUIRE_THROWS(entity.get<Hat>());
                    REQUIRE_THROWS(entity.get<Shoes>());
                }
                 */
                THEN("Only one Health component should exist"){
                    REQUIRE(health_component_count == 1);
                }
                AND_WHEN("When removing the Health component"){
                    entity.remove<Health>();
                    THEN("No Health component should exist"){
                        REQUIRE(health_component_count == 0);
                    }
                    THEN("Entity should not have that component attached"){
                        REQUIRE(!entity.has<Health>());
                    }
                }
                // TODO: Find a way to test assert
                /*
                AND_WHEN("When removing the Health component twice"){
                    entity.remove<Health>();
                    THEN("It should not work"){
                        REQUIRE_THROWS(entity.remove<Health>());
                    }
                }
                AND_WHEN("When removing the Health component twice"){
                    entity.remove<Health>();
                    THEN("It should not work"){
                        REQUIRE_THROWS(entity.remove<Health>());
                    }
                }
                */
                AND_WHEN("Destroying the entity"){
                    entity.destroy();
                    THEN("No Health component should exist"){
                        REQUIRE(health_component_count == 0);
                    }
                }
                AND_WHEN("Accessing Component as ref and change value"){
                    Health& health = entity.get<Health>();
                    health.value = 123;
                    THEN("Health should be that value"){
                        REQUIRE(entity.get<Health>().value == 123);
                    }
                }
                AND_WHEN("Accessing Component as value and change value"){
                    Health health = entity.get<Health>();
                    health.value = 123;
                    THEN("Health should not be that value"){
                        REQUIRE(entity.get<Health>().value != 123);
                    }
                }
            }
            // TODO: Find a way to test assert
            /*
            WHEN("Adding the same component twice with \"add\" method"){
                entity.add<Health>(1);
                THEN("Should not work"){
                    REQUIRE_THROWS(entity.add<Health>(2));
                }
            }
            */
            WHEN("Adding the same component twice with \"set\" method"){
                entity.set<Health>(1);
                THEN("Should work"){
                    entity.set<Health>(2);
                }
            }
            WHEN("Entity remains without components"){
                THEN("It should be valid"){
                    REQUIRE(entity.valid());
                }
            }
            WHEN("Adding some components to Entity"){
                THEN("It should still be valid"){
                    REQUIRE(entity.valid());
                }
            }
            WHEN("Destroying entity once"){
                entity.destroy();
                THEN("It should be invalid"){
                    REQUIRE(!entity.valid());
                }
            }
            // TODO: Find a way to test assert
            /*
            WHEN("Destroying entity twice"){
                entity.destroy();
                THEN("It should not work"){
                    REQUIRE_THROWS(entity.destroy());
                }
            }
             */
            WHEN("Destroying entity once and replace it with a new entity"){
                entity.destroy();
                Entity entity2 = entities.create();
                THEN("Old entity and new entity should have same id but old should be invalid"){
                    REQUIRE(entity.id().index() == entity2.id().index());
                    REQUIRE(!entity.valid());
                    REQUIRE(entity2.valid());
                }
            }
            WHEN("Adding Health and Mana with different values to entity"){
                entity.add<Health>(10);
                entity.add<Mana>(20);
                THEN("Health and Mana should be different"){
                    REQUIRE(entity.get<Health>() != entity.get<Mana>());
                    REQUIRE(entity.get<Mana>() != entity.get<Health>());
                }
            }
            WHEN("Adding Health and Mana with same value to entity"){
                entity.add<Health>(10);
                entity.add<Mana>(10);
                THEN("Health and Mana should be same"){
                    REQUIRE(entity.get<Health>() == entity.get<Mana>());
                    REQUIRE(entity.get<Mana>() == entity.get<Health>());
                }
            }
        }

        WHEN("Adding 100 entities"){
            auto new_entities = entities.create(100);
            THEN("Number of entities should be 100"){
                REQUIRE(entities.count() == 100);
                REQUIRE(entities.count() == new_entities.size());
            }
            THEN("Iterating through the list should be the same"){
                for (size_t i = 0; i < new_entities.size(); ++i) {
                    REQUIRE(new_entities[i] == entities[i]);
                }
            }
            WHEN("Removing all added entities"){
                for (size_t i = 0; i < new_entities.size(); ++i) {
                    new_entities[i].destroy();
                }
                THEN("EntityManager should be empty"){
                    REQUIRE(entities.count() == 0);
                }
            }
        }
        
        // Testing EntityManager::View functions
        GIVEN("4 entities with no components attached"){
            Entity e1 = entities.create();
            Entity e2 = entities.create();
            Entity e3 = entities.create();
            Entity e4 = entities.create();
            WHEN("Adding Health with same value to all but one entities"){
                e1.add<Health>(12);
                e2.add<Health>(12);
                e3.add<Health>(12);
                e4.add<Health>(100);
                THEN("All entities should have Health"){
                    REQUIRE(entities.with<Health>().count() == entities.count());
                }
                THEN("They should have same amout of getHealth (Not the one with different getHealth ofc)"){
                    REQUIRE(e1.get<Health>() == e2.get<Health>());
                }
                AND_WHEN("Adding Mana to 2 of them"){
                    e1.add<Mana>(0);
                    e2.add<Mana>(0);
                    THEN("2 entities should have mana and getHealth"){
                        size_t count = entities.with<Mana,Health>().count();
                        REQUIRE(count == 2);
                    }
                    THEN("Entities with Health and Mana == entities with Mana and Health (order independent)"){
                        size_t count1 = entities.with<Mana, Health>().count();
                        size_t count2 = entities.with<Health, Mana>().count();
                        REQUIRE(count1 == count2);
                    }
                }
                AND_WHEN("Accessing Health component as references"){
                    short& health = e1.get<Health>();
                    THEN("Changing the value should affect the actual component"){
                        health += 1;
                        REQUIRE(health == (int)e1.get<Health>());
                    }
                }
                AND_WHEN("Accessing Health component as values"){
                    int health = e1.get<Health>();
                    THEN("Changing the value should not affect the actual component"){
                        health += 1;
                        REQUIRE(health != (int)e1.get<Health>());
                    }
                }
                AND_WHEN("Accessing e1s' Health as pointer, and setting this pointer to e4s' different Health"){
                    Health& health = e1.get<Health>();
                    health = e4.get<Health>();
                    THEN("Health of e1 should change"){
                        REQUIRE(e4.get<Health>().value == e1.get<Health>().value);
                    }
                }
                THEN("Iterating over entities with health as type Entity should compile"){
                    for(Entity e : entities.with<Health>()){}
                }
                THEN("Iterating over entities with health as type EntityAlias<Health> should compile"){
                    for(EntityAlias<Health> e : entities.with<Health>()){}
                }
                THEN("Iterating over entities with health as type EntityAlias<Health> should compile"){
                    for(auto e : entities.with<Health>()){
                        typedef std::is_same<decltype(e), EntityAlias<Health>> e_is_entity_alias;
                        REQUIRE(e_is_entity_alias::value);
                    }
                }
            }
            WHEN("Adding Mana to 2 of them"){
                e1.add<Mana>(0);
                e2.add<Mana>(0);
                THEN("2 entities should have mana"){
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
            WHEN("Setting its Health to the same level as Mana"){
                entity.get<Health>() = entity.get<Mana>();
                THEN("Entity should have 10 Health"){
                    REQUIRE(entity.get<Health>() == 10);
                }
            }
            WHEN("Comparing its Health with its Mana"){
                bool c = entity.get<Health>() == entity.get<Mana>();
                THEN("This should be false"){
                    REQUIRE(!c);
                }
            }
        }
        GIVEN("An entity with wheels and health and mana of 1"){
            Entity entity = entities.create();
            entity.add<Wheels>();
            entity.add<Health>(1);
            entity.add<Mana>(1);

            WHEN("Accessing Entity as Car and start driving"){
                Car& car = entity.as<Car>();
                car.drive(1,1);
                THEN("Car should be driving"){
                    REQUIRE(entity.has<Velocity>());
                    REQUIRE(entity.get<Velocity>().x == 1);
                    REQUIRE(entity.get<Velocity>().y == 1);
                }
            }
            WHEN("Assumeing Entity has Wheels, should work"){
                entity.assume<Wheels>().get<Wheels>();
            }
            // TODO: Find a way to test assert
            /*
            WHEN("Assumeing Entity has Hat, should not work"){
                REQUIRE_THROWS(entity.assume<Hat>());
            }
            */
            WHEN("Creating 2 new entities and fetching every Car"){
                entities.create();
                entities.create();
                auto cars = entities.fetch_every<Car>();
                THEN("Number of cars should be 1, when using count method"){
                    REQUIRE(cars.count() == 1);
                }
                THEN("Number of cars should be 1, when counting with for each loop"){
                    int count = 0;
                    for (Car car : cars) {
                        ++count;
                    }
                    REQUIRE(count == 1);
                }
                THEN("Number of cars should be 1, when counting with lambda, pass by value"){
                    int count = 0;
                    entities.fetch_every([&](Car car) {
                        ++count;
                    });
                    REQUIRE(count == 1);
                }
                THEN("Number of cars should be 1 when counting with lambda, pass by reference"){
                    int count = 0;
                    entities.fetch_every([&](Car &car) {
                        ++count;
                    });
                    REQUIRE(count == 1);
                }
                THEN("Number of entities with Wheels should be one and that entity should have 1 health and mana"){
                    int count = 0;
                    entities.with([&](Wheels& wheels, Health& health, Mana& mana){
                        ++count;
                        REQUIRE(health == 1);
                        REQUIRE(mana == 1);
                    });
                    REQUIRE(count == 1);
                }
                THEN("Unpacking Wheels and entity should work"){
                    int count = 0;
                    entities.with([&](Wheels& wheels, Entity entity){
                        ++count;
                        REQUIRE(&entity.get<Wheels>() == &wheels);
                    });
                    REQUIRE(count == 1);
                }
                THEN("Getting Health and Mana using get should result in using different get functions"){
                    int count = 0;
                    for (auto e : entities.with<Wheels, Health>()) {
                        ++count;
                        REQUIRE(e.get<Health>() == 1);
                        REQUIRE(e.get<Mana>() == 1);
                        e.remove<Wheels>();
                    }
                    REQUIRE(count == 1);
                }
                THEN("Unpacking Wheels, Health and Mana components. Accessing components should work"){
                    auto tuple = entity.unpack<Wheels, Health, Mana>();
                    auto& health = std::get<1>(tuple);
                    auto& mana = std::get<2>(tuple);
                    REQUIRE(health == 1);
                    REQUIRE(mana == 1);
                    AND_WHEN("Adding 1 to Mana"){
                        mana.value += 1;
                        THEN("Mana should be 2"){
                            mana = entity.get<Mana>();
                            REQUIRE(mana.value == 2);
                        }
                    }
                }
                AND_WHEN("A change is made to some of the components as ref"){
                    entities.with([](Mana& mana){
                        mana = 10;
                    });
                    THEN("Change should be made"){
                        REQUIRE(entity.get<Mana>() == 10);
                    }
                }
                AND_WHEN("A change is made to some of the components as copied value"){
                    entities.with([](Mana mana){
                        mana = 10;
                    });
                    THEN("Change should NOT be made"){
                        REQUIRE(entity.get<Mana>() != 10);
                    }
                }
            }
        }
        WHEN("Creating a car using the EntityManager with speed 10,10"){
            auto car = entities.create<Car>(10, 10);
            THEN("Speed should be 10"){
                REQUIRE(car.get<Velocity>().x == 10);
                REQUIRE(car.get<Velocity>().y == 10);
            }
        }
        WHEN("Creating a car using the EntityManager without setting speed"){
            Car car = entities.create<Car>();
            THEN("Car should not be moving"){
                REQUIRE(!car.is_moving());
            }
            AND_WHEN("Car starts driving"){
                car.drive(1,1);
                THEN("Car should be driving"){
                    REQUIRE(car.is_moving());
                }
            }
        }
        GIVEN("A SystemManager"){
            SystemManager systems(entities);
            WHEN("Adding count car system and remove dead entities system") {
                systems.create<CountCarSystem>();
                systems.create<RemoveDeadEntitiesSystem>();
                THEN("Systems should exists"){
                    REQUIRE(systems.exists<CountCarSystem>());
                    REQUIRE(systems.exists<RemoveDeadEntitiesSystem>());
                }
                AND_WHEN("Removing one of them"){
                    systems.destroy<CountCarSystem>();
                    THEN("That system should not exist anymore"){
                        REQUIRE(!systems.exists<CountCarSystem>());
                    }
                }
                AND_WHEN("Adding one dead entity, and calling update"){
                    Entity e = entities.create();
                    e.add<Health>(-1);
                    systems.update(0);
                    THEN("Entity should be removed."){
                        REQUIRE(!e.valid());
                        REQUIRE(entities.count() == 0);
                    }
                }
            }
        }
    }
}
