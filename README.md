#OpenEcs - A fast, typesafe, C++11, configurable, header only, Entity Component System

##What is OpenEcs?
Open Ecs is an Entity Component System that uses metaprogramming, cache coherency, and other useful tricks to maximize 
performance and configurability. It is written in c++11 without further dependencies.

NOTE: OpenEcs is still in beta and usage with the library might change.

##Why OpenEcs?
I authored OpenEcs after using other ECS libraries. The main reason for this was that I wanted to write my own, and it 
wasn't supposed to become its own library. As I progressed, I thought it might be worth releasing it to the public as it 
slightly became a library with a different approach than the others.

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
	Health(int value) : this->value(value)
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
//entity.add<Health>(10); <- Error, component already added
```

### Accessing Components from entities
Accessing components is just as easy

```cpp
//Add health component with a value of 10 to the entity
entity.add<Health>(10);

//Access the Health Component
Component<Health> health = entity.get<Health>();
//Access members of Health by using -> 
health->value = 3;

//Or just use a reference
Health& health = entity.get<Health>();
health.value = 3;

//NOTE, you can use reference, but do not use value
//otherwise, the components will be copies and
//any change made to it will not be registered
//on the acctual component

//NOTE: Do not use (unless you know what you are doing)
Health health = entity.get<Health>();
health.value = 3; // <- does not apply any change

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
entity.remove<Health>();
//or
entity.get<Health>().remove();
```

### Iterating throught the EntityManager
To access components with certain components. There is a "with" function that looks like this

```cpp
EntityManager entities;

//Iterate through the entities, grabing each 
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

//NOTE, use reference, not values as parameters.
//otherwise, the components will be copies and
//any change made to it will not be registered
//on the acctual component

```

###Systems
First we define our SystemManager

```cpp
EntityManager entities;
//We must provide what entities we like to work with
SystemManager systems(entities);
```

Then we create a system class.
Any new system class must inherit the System class like this:

```cpp
class RemoveCorpsesSystem : public System<RemoveCorpsesSystem>{
public:
	virtual void update(float time){
		//Get EntityManager
		EntityManager& entities = entities();

		for(auto entity : entities.with<Health>()){
			if(entity.get<Health>()->value <= 0){
				//Destroy the entitiy
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


##Extra feature's
Aside from the normal usage of bitmasks for Component recognition and storing components in a vector for cache 
coherency, OpenEcs has some extra features.

###EntityAlias that enables object oriented interaction.

As an object oriented programmer, the ability to encapsulate the implementation as much as possible is desired. By using 
EntityAlias, an Entity can be wrapped inside these, witch then define all functions needed. then, the systems can 
interact with these EntityAliases instead of the entity. This takes no extra performance, and creates an abstraction later 
between actual ECS implementation and the desired action.

Defining an EntityAlias
```cpp
class Actor : public EntityAlias<Health, Name /* Any required component*/ >{
public:
	//here we can hide the implementation details of how the components
	//are used by the entity, and provide ordinary functions instead.
	void kill(){
		get<Health>()->value = 0;
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
entity and the actual components. If new data needs to be associated with the entity, intruduce 
new components instead

The EntityAlias assumes that the entity has all components provided by the inheritance.
We can get every entity that can be accociated as an Actor (all entities with Health and Name in this case) by using the "fetch_every" function, or a from a single Entity using the "as" function.

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
	entity.get<Health>()->value = 0;
}
```

This is very useful, as we get another abstraction layer without giving up performance.

###Creating entities by using EntityAlias
By defining a constructor for the EntityAlias. We can use the create function from the EntityManager
like this:

```cpp
class Actor : public EntityAlias<Health, Name>{
public:
	Actor(int health, std::string name){
		set<Health>(health);
		set<Name>(name);
		//Make sure you set all required components here.
		//Anything else is forbidden
	}
};

EntityManager entities;
//Create an actor with 10 health and named Evil Dude
entities.create<Actor>(10, "Evil Dude");

```


###Performance

On my Apple MBP Retina 13" 2014 computer, this is some performance tests using 10 000 000 entities with some components attached:

| Operation											 	| 	   Time 	 |
|-------------------------------------------------------|---------------|
| create()                        					 	|     0.31s     |
| create(10M) 										 	|     0.19s     |
| destroy()   										 	|     0.54s     |
| iterating using with for-loop (without unpacking)	 	|     0.00986s  |
| iterating using with for-loop (unpack one component) 	|     0.01579s  |
| iterating using with lambda (unpack one component) 	|     0.01609s  |
| iterating using with for-loop (unpack two components) |     0.01847s  |
| iterating using with lambda (unpack two components)	|     0.01886s  |
| iterating using fetch_every for-loop			 	 	|     0.00909s  |
| iterating using fetch_every lambda				 	|     0.00916s  |

To improve performance iterate by using auto when iterating with a for loop

```cpp  
for(auto entity : entities.with<Health, Name>()){
//   ^
//   | 
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
	entity.get<Mana>(); // <- Runtime check required. Slow(er)
}

```

Or just use the lambda version of "with" or "fetch_every" function with your own EntityAliases, and you
are good to go!

```cpp  
entities.with([](Health& health, Name& name){ });

```

###Configurable memory Allocation 

An EntityManager can define what components can be assigned to Entities and how different Components are stored.

###Define different EntityManagers to fit you needs

Each EntityManager can have different Components. Therefore the ability to use one EM for game entities, and one for 
example, particles, in particle effects is possible. Also, the size of the bitmasks depends only on the components used
for that EM witch reduces memory usage.

###Component Dependencies

Let's say Velocity depends on position. The ability to tell the entity manager that a specific Component depends on 
another Component might be useful to ensure a valid gamestate. 