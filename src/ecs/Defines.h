#ifndef ECS_DEFINES_H
#define ECS_DEFINES_H

/// The cache line size for the processor. Usually 64 bytes
#ifndef ECS_CACHE_LINE_SIZE
#define ECS_CACHE_LINE_SIZE 64
#endif

/// This is how an assertion is done. Can be defined with something else if needed.
#ifndef ECS_ASSERT
#define ECS_ASSERT(Expr, Msg) assert(Expr && Msg)
#endif

/// The maximum number of component types the EntityManager can handle
#ifndef ECS_MAX_NUM_OF_COMPONENTS
#define ECS_MAX_NUM_OF_COMPONENTS 64
#endif

/// How many components each block of memory should contain
/// By default, this is divided into the same size as cache-line size
#ifndef ECS_DEFAULT_CHUNK_SIZE
#define ECS_DEFAULT_CHUNK_SIZE ECS_CACHE_LINE_SIZE
#endif

#define ECS_ASSERT_IS_CALLABLE(T)                                                           \
            static_assert(details::is_callable<T>::value,                                   \
            "Provide a function or lambda expression");                                     \

#define ECS_ASSERT_IS_ENTITY(T)                                                             \
            static_assert(std::is_base_of<details::BaseEntityAlias, T>::value ||            \
                      std::is_same<Entity, T>::value ,                                      \
            #T " does not inherit EntityInterface.");

#define ECS_ASSERT_ENTITY_CORRECT_SIZE(T)                                                   \
            static_assert(sizeof(details::BaseEntityAlias) == sizeof(T),                    \
            #T " should not include new variables, add them as Components instead.");       \

#define ECS_ASSERT_VALID_ENTITY(E)                                                          \
            ECS_ASSERT(is_valid(E), "Entity is no longer valid");                           \

#define ECS_ASSERT_IS_SYSTEM(S)                                                             \
            static_assert(std::is_base_of<System, S>::value,                                \
            "DirivedSystem must inherit System.");                                          \

#define ECS_ASSERT_MSG_ONLY_ONE_ARGS_PROPERTY_CONSTRUCTOR                                   \
            "Creating a property component should only take 1 argument. "                   \
            "If component should initilize more members, provide a "                        \
            "constructor to initilize property component correctly"                         \

namespace ecs{
/// Type used for entity index
using index_t = uint32_t;
/// Type used for entity version
using version_t = uint8_t;

namespace details{

/// ComponentMask is a mask defining what components and entity has.
using ComponentMask = std::bitset<ECS_MAX_NUM_OF_COMPONENTS>;

} // namespace details
} // namespace ecs

#endif //ECS_DEFINES_H
