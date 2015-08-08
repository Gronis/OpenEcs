#OpenEcs - A fast, typesafe, C++11, configurable, header only, Entity Component System

##What is OpenEcs?
Open Ecs is an Entity Component System that uses metaprogramming, cache coherency, and other useful tricks to maximize 
performance and configurability. It is written in c++11 without further dependencies.

NOTE: OpenEcs is still in beta and usage with the library might change. If you need a complete ECS library for a serious
project, I suggest looking further. I want more stuff like custom component allocators and perhaps a compile time 
configurable EntityManager and SystemManager. Some way to handle events might be useful. to include.
Let me know what you think and what is missing, I hope you enjoy using OpenEcs.

##Why OpenEcs?
I authored OpenEcs after using other ECS libraries. The main reason for this was that I wanted to write my own, and it 
wasn't supposed to become its own library. As I progressed, I thought it might be worth releasing it to the public as it 
slightly became a library with a different approach than the others.

##Installation
Just [Download](https://github.com/Gronis/OpenEcs/raw/master/src/ecs/ecs.h) the header and include it into your project.
Make sure to enable c++11 when compiling. (-std=c++11 or -std=c++0x)

##Standard Feature's:
The first thing you need is somewhere to store the entities. This is called the EntityManager and is created like this:
```cpp
using namespace ecs;
EntityManager entities;
```

Once the EntityManager is created. Adding entitites can be done like this

```cpp
EntityManager entities;

//Create one entity;
Entity entity = entities.create(); 

//Create 100 entities
vector<Entity> new_entities = entities.create(100);
```

###Adding Components to entities
Adding components to entities is easy. Bur first we must define a Component. A Component can be any class or struct.

```cpp

struct Health{
    Health(int value) : value(value)
    int value;  
};

EntityManager entities;

//Create one entity;
Entity entity = entities.create(); 

//Add health component with a value of 10 to the entity
entity.add<Health>(10);

//The set function can be used even if health was added before.
entity.set<Health>(20);

//Add only works when it doesn't already exists.
entity.add<Health>(10);// <- Assert failure, component already added
```

### Accessing Components from entities
Accessing components is just as easy

```cpp
//Add health component with a value of 10 to the entity
entity.set<Health>(10);

//Access the Health Component
Health& health = entity.get<Health>();
health.value = 3;

//NOTE, you should use reference, not value.
//Otherwise, the components will be copied and
//any change made to it will not be registered
//on the acctual component

//NOTE: Do not use (unless you know what you are doing)
Health health = entity.get<Health>();
health.value = 3; // <- does not apply any change

```
Accessing a component which does not exist on a specified entity, will trigger a runtime assertion. To check if an entitiy has a specified component, use the "has" function.

```cpp
Entity entity = entities.create();
bool has_health = entity.has<Health>(); //Returns false

entity.add<Health>(10);
has_health = entity.has<Health>(); //Returns true

```

### Removing Entities and Components

Destroying an entity can be done using the destroy method
```cpp
Entity entity = entities.create();
entity.destroy();
//or
entities.destroy(entity);
```

Removing Components is done by using the remove function
```cpp
Entity entity = entities.create();
entity.add<Health>(0);
entity.remove<Health>();
```
Destroying an entity removes all components and calls their destructors. it also opens that memory slot for another entity to take its place.

When an entity is destroyed. It is no longer valid, and any action used with it should not work
```cpp
entity.destroy();
//Does not work
entity.add<Health>(0); // <- triggers runtime assertion, Entity invalid

//Check if entity is valid
bool valid = entity.valid();
```

To track if an entity is valid. OpenEcs accociates each entity with a version when accessed from the EntityManager. Whenever an entity is destoyed, the version for that Entity changes, and all entities with the old versions are invalid, as they no longer exists.

### Iterating through the EntityManager
To access components with certain components. There is a "with" function that looks like this

```cpp
EntityManager entities;

//Iterate through the entities, grabbing each 
//component with Health and Mana component
for(Entity entity : entities.with<Health, Mana>()){
    entity.get<Health>(); //Do things with health
    entity.get<Mana>(); //Do things with mana
}

//There is also a functional style iteration that works 
//with a provided lambda.
entities.with([](Health& health, Mana& mana){
    health.value = 2; //Do stuff
});

//If you need the entity, include it as well
entities.with([](Health& health, Mana& mana, Entity entity){
    health.value = 2; //Do stuff
    entity.remove<Health>();
});

//NOTE, use reference, not values as parameters.
//(with the exception of Entity)
//Otherwise, the components will be copied and
//any change made to it will not be registered
//on the acctual component

```

###Systems
Systems define our behavior. The SystemManager provided by OpenEcs is very simple and is just a wrapper around an interface with an update function, together with the entities.

First we define our SystemManager

```cpp
EntityManager entities;
//We must provide what entities we like to work with
SystemManager systems(entities);
```

Then we create a system class.
Any new system class must inherit the System class like this:

```cpp
//     Provide the type as template argument here
//                  | _________________________
//                  |                          |
//                  v                          v
class RemoveCorpsesSystem : public System<RemoveCorpsesSystem>{
public:
    void update(float time) override {
        // Get the entity manager using entities() function
        for(auto entity : entities().with<Health>()){
            if(entity.get<Health>().value <= 0){
                //Destroy the entity
                entity.destroy();
            }
        }
    }
};

EntityManager entities;
SystemManager systems(entities);

systems.create<RemoveCorpsesSystem>( /*Provide any constructor arguments*/ );

//Now update all systems at once
float deltaTime = 1;
systems.update(deltaTime);

```

The systems are updated in the same order as they are added.

###Error handling
Any runtime errors should be handled by static or runtime assertions.

##Extra feature's
Aside from the normal usage of bitmasks for Component recognition and storing components in a vector for cache 
coherency, OpenEcs has some extra features.

###EntityAlias that enables object oriented interaction.

As an object oriented programmer, the ability to encapsulate the implementation as much as possible is desired. By using 
EntityAlias, an Entity can be wrapped inside these, witch then define all functions needed. then, the systems can 
interact with these EntityAliases instead of the entity. This takes no extra performance, and creates an abstraction 
later between actual ECS implementation and the desired action.

Defining an EntityAlias
```cpp
class Actor : public EntityAlias<Health, Name /* Any required component*/ >{
public:
    //here we can hide the implementation details of how the components
    //are used by the entity, and provide ordinary functions instead.
    void kill(){
        get<Health>().value = 0;
    }

    bool attack(Entity target){
        if(!has<AttackAction>()){
            add<AttackAction>(target);
            return true;
        }
        return false;
    }
};
```

NOTE: An EntityAlias class cannot define new member variables. It is only a wrapper between the
entity and the actual components. If new data needs to be associated with the entity, introduce 
new components instead

The EntityAlias assumes that the entity has all components provided by the inheritance.
We can get every entity that can be associated as an Actor (all entities with Health and Name in this case) by using the
"fetch_every" function, or a from a single Entity using the "as" function.

```cpp
EntityManager entities;

Entity entity = entity.create();
entity.add<Health>(0);
entity.add<Name>("");

// Access all entities that has all required components that an Actor has
for(Actor actor : entities.fetch_every<Actor>()){
    actor.kill(); //For example
}
//or with lambda
entities.fetch_every([](Actor& actor){
    actor.kill();
});

//Access one entity as EntityAlias
Actor actor = entity.as<Actor>();
```

The sweetness with using the "fetch_every" function is that it basically generates the same code as this:

```cpp
EntityManager entities;

for(auto entity : entities.with<Health, Name>()){
    entity.get<Health>().value = 0; // actor.kill();
}
```

This is very useful, as we get another abstraction layer without giving up performance.

###Creating entities by using EntityAlias
By defining a constructor for the EntityAlias. We can use the create function from the EntityManager
like this:

```cpp
class Actor : public EntityAlias<Health, Name>{
//                                  ^      ^
//                                   \_____|
public://                                  |
    Actor(int health, std::string name){// |
        add<Health>(health);//             |
        add<Name>(name);//                 |
        //Make sure to add any required components.
        //If any required component is not added,
        //this will cause a runtime assertion failure

        //it is also important to use "add" instead of "set"
        //as "set" assumes that all required components are
        //already set for this entity.
    }
};
```

Once we have the Entity AliasConstructor, we can create
an entity using the "create" function.

```
EntityManager entities;

//Create an actor with 10 health and named Evil Dude
Actor actor = entities.create<Actor>(10, "Evil Dude");

```
This results in a useful factory method for any EntityAlias. Another good thing is that what an Entity is, is defined by
its components, and enables entities to be several things at the same time, without using hierarchy (Which is the idea 
behind Entity Component Systems).

###Override operators
It would be useful if the components override some basic operators for cleaner code.
```cpp
//Using == operator of component class
bool health_equals_mana = 
    entity.get<Health>() == entity.get<Mana>();

//If no operators are defined, we must write it like this
bool health_equals_mana = 
    entity.get<Health>().value == entity.get<Mana>().value;
```
However, defining operators for each component can be very anoying and time consume. In most cases (like the example above) almost unneccecary. If the component only has one property. In the case of Health in this example. There is a class called Property<T> which overrides useful operators. The Component class may extend this class to inherit some basic operators.

```cpp
struct Health : Property<int>{
  //Define constructor like this
  Health(int value) : Property<int>(value){};  
  //Or like this
  Health(int value){
    this->value = value;
  }; 
};
```

Now we should have some useful operator implementations for the health component which enables cleaner code

```cpp
// operator >
if(entity.get<Health>() > 10){
    
}
// without using operator >
if(entity.get<Health>().value > 10){
    
}
```
NOTE: Remember that any class can be a component, and that it is optional to use Property<T>, as overriding operators is not always desired. For those who does not what to use overridden operators, leave out the Property<T> class.

###Performance

On my Apple MBP Retina 13" 2014 computer, this is some performance tests using 10 000 000 entities with some components 
attached:

| Operation                                             |      Time      |
|-------------------------------------------------------|---------------|
| create()                                              |     0.31s     |
| create(10M)                                           |     0.19s     |
| destroy()                                             |     0.54s     |
| iterating using with for-loop (without unpacking)     |     0.00986s  |
| iterating using with for-loop (unpack one component)  |     0.01579s  |
| iterating using with lambda (unpack one component)    |     0.01609s  |
| iterating using with for-loop (unpack two components) |     0.01847s  |
| iterating using with lambda (unpack two components)   |     0.01886s  |
| iterating using fetch_every for-loop                  |     0.00909s  |
| iterating using fetch_every lambda                    |     0.00916s  |

To improve performance iterate by using auto when iterating with a for loop

```cpp  
for(auto entity : entities.with<Health, Name>()){
//   ^
//   \ 
//    ` The type here will not become Entity, it will become EntityAlias<Health, Name>.
//      It will only use the normal Entity class if you tell it to.
}

```
Why should I do this? Because when you use the function get<Health> on the Entity object,
We do not know that the entity has the Health component, and we do a runtime check each loop.
If you are using EntityAlias<Health,Name>, the compiler assumes that the entity has Health
and Name components, and bypasses this check when accessing the component. We already do this
runtime check when we iterate through the list. Why do it again?

```cpp  
for(auto entity : entities.with<Health, Name>()){
    entity.get<Health>(); // <- No runtime check. Fast
    entity.get<Mana>(); // <- Runtime check required. 
}

```

Or just use the lambda version of "with" or "fetch_every" function with your own EntityAliases, and you
are good to go!

```cpp  
entities.with([](Health& health, Name& name){ });

```

##A complete example
This examples illustrates two spellcasters: Alice and Bob, and their fight to the death. Prints the winner and remaining
health and mana.

```cpp
#include <iostream>
#include "ecs/ecs.h"

using namespace ecs;

//Using property
struct Health: Property<int>{
    Health(int value) : Property<int>(value){};
};

struct Mana : Property<int>{
    Mana(int value) : Property<int>(value){};
};

struct Name : Property<std::string>{
    Name(std::string value) : Property<std::string>(value){};
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
        return get<Health>() > 0;
    }
    void castSpell(Spellcaster& target){
        if(!isOom()){
            --get<Mana>();
            --target.get<Health>();
        }
    }
};

class RemoveCorpsesSystem : public System<RemoveCorpsesSystem>{
public:
    void update(float time) override {
        //Method 1
        //Any health entity
        entities().with([](Health& health, Entity entity){
           if(health <= 0){
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
            std::cout << name << " won!" << std::endl;
            std::cout << "Health: " << health << std::endl;
            std::cout << "Mana:   " << mana << std::endl;
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
```

Output:
```
Bob won!
Health: 4
Mana:   1337
```

##License
Copyright (C) 2015  Robin Grönberg

This program is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

This program is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with this program.  If not, see <http://www.gnu.org/licenses/>.