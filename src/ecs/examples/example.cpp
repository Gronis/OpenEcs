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

struct Health{
    Health(int value) : value(value){};
    int value;
};

struct Mana{
    Mana(int value) : value(value){};
    int value;
};

struct Name{
    Name(std::string value) : value(value){};
    std::string value;
};

struct Spellcaster : public EntityAlias<Name, Health, Mana>{
    Spellcaster() : Spellcaster("NoName", 0, 0){ };
    Spellcaster(std::string name, int health, int mana){
        set<Name>(name);
        set<Health>(health);
        set<Mana>(mana);
    }
    bool isOom(){
        return get<Mana>().value == 0;
    }

    bool isAlive(){
        return get<Health>().value > 0;
    }
    void castSpell(Spellcaster& target){
        --get<Mana>().value;
        --target.get<Health>().value;
    }
};

class RemoveCorpsesSystem : public System<RemoveCorpsesSystem>{
public:
    virtual void update(float time){
        for(auto entity : entities().with<Health>()){
            if(entity.get<Health>().value <= 0){
                entity.destroy();
            }
        }
    }
};

class CastSpellSystem : public System<CastSpellSystem>{
public:
    virtual void update(float time){
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
    virtual void update(float time){
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