#include <iostream>
#include "common/thirdparty/catch.hpp"

#include "ecs/ecs.h"

using namespace ecs;

namespace {
    struct Wheels {
        int i;
    };

    struct Door {
        int i;
    };

    struct Car : EntityAlias<Wheels> {
    };
};

double time_elapsed(clock_t begin, clock_t end){
    return double(end - begin) / CLOCKS_PER_SEC;
}

class Timer {
public:
    Timer(){ restart(); }
    ~Timer(){ std::cout << "Time elapsed: " << elapsed() << std::endl << std::endl;}

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
    int count = 10000000;
    EntityManager entities;
    for (int i = 0; i < count; i++) {
        auto e = entities.create();
        e.add<Wheels>();
        e.add<Door>();
    }

    WHEN("Iterating manually with iterator"){
        std::cout << "Iterating over " << count << " using iterator manually" << std::endl;
        Timer t;
        auto it = entities.with<Wheels>().begin();
        auto end = entities.with<Wheels>().end();
        for (; it != end; ++it) {
            (void)it;
        }
    }

    WHEN("Iterating using with") {
        std::cout << "Iterating over " << count << " using with for-loop without unpacking" << std::endl;
        Timer t;
        for (auto e : entities.with<Wheels>()) {
            (void)e;
        }
    }

    WHEN("Iterating using with") {
        std::cout << "Iterating over " << count << " using with for-loop unpacking one component" << std::endl;
        Timer t;
        for (auto e : entities.with<Wheels>()) {
            auto wheels = e.get<Wheels>();
        }
    }

    WHEN("Iterating using with") {
        std::cout << "Iterating over " << count << " using with lambda unpacking one component" << std::endl;
        Timer t;
        entities.with([](Wheels& wheels){
            (void)wheels;
        } );
    }

    WHEN("Iterating using with") {
        std::cout << "Iterating over " << count << " using with for-loop unpacking two components" << std::endl;
        Timer t;
        for (auto e : entities.with<Wheels, Door>()) {
            auto wheels = e.get<Wheels>();
            auto door = e.get<Door>();
        }
    }

    WHEN("Iterating using with") {
        std::cout << "Iterating over " << count << " using with lambda unpacking two components" << std::endl;
        Timer t;
        entities.with([](Wheels& wheels, Door& door){
            (void)wheels;
            (void)door;
        } );
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
        entities.fetch_every([](Car& car){
            (void)car;
        } );
    }
}