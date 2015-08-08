/// --------------------------------------------------------------------------
/// OpenEcs - A fast, typesafe, C++11, configurable, header only, ECS
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
#include "ecs/ecs.h"

using namespace ecs;

//Not using Property
struct Health{
    Health(int value) : value(value){};
    int value;
};
//Using property
struct Mana : Property<int>{
    Mana(int value){
        this->value = value;
    };
    int value;
};

struct Name{
    Name(std::string value) : value(value){};
    std::string value;
};

struct Spellcaster : public EntityAlias<Name, Health, Mana>{
    Spellcaster(std::string name = "NoName" , int health = 0, int mana = 0){
        add<Name>(name);
        add<Health>(health);
        add<Mana>(mana);
    }
    bool isOom(){
        return get<Mana>() == 0;// <- override == operator
    }

    bool isAlive(){
        return get<Health>().value > 0;
    }
    void castSpell(Spellcaster& target){
        if(!isOom()){
            --get<Mana>();
            --target.get<Health>().value;
        }
    }
};

class RemoveCorpsesSystem : public System<RemoveCorpsesSystem>{
public:
    void update(float time) override {
        //Method 1
        //Any health entity
        entities().with([](Health& health, Entity entity){
           if(health.value <= 0){
               entity.destroy();
           }
        });
        //Method 2
        //Any spellcaster that is dead
        entities().fetch_every([](Spellcaster& spellcaster){
            if(!spellcaster.isAlive()){
                spellcaster.destroy();
            }
        });
    }
};

class CastSpellSystem : public System<CastSpellSystem>{
public:
    void update(float time) override {
        entities().fetch_every([&] (Spellcaster& spellcaster1){
            entities().fetch_every([&] (Spellcaster& spellcaster2){
                if(spellcaster1 != spellcaster2){
                    spellcaster1.castSpell(spellcaster2);
                }
            });
        });
    }
};

class GiveManaSystem : public System<GiveManaSystem>{
public:
    void update(float time) override {
        entities().fetch_every([] (Spellcaster& spellcaster){
            if(spellcaster.isOom()) spellcaster.set<Mana>(1337);
        });
    }
};

class Game{
public:
    Game() : systems(entities) {}
    void run(){
        systems.create<CastSpellSystem>();
        systems.create<GiveManaSystem>();
        systems.create<RemoveCorpsesSystem>();
        entities.create<Spellcaster>("Alice", 8, 12);
        entities.create<Spellcaster>("Bob", 12, 8);
        while(entities.count() > 1) systems.update(1);
        entities.with([] (Name& name, Health& health, Mana& mana){
            std::cout << name.value << " won!" << std::endl;
            std::cout << "Health: " << health.value << std::endl;
            std::cout << "Mana:   " << mana.value << std::endl;
        });
    }

private:
    EntityManager entities;
    SystemManager systems;
};

int main(){
    Game game;
    game.run();
}