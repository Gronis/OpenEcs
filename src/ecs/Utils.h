#ifndef ECS_UTILS_H
#define ECS_UTILS_H

namespace ecs{

/// Forward declarations
class Entity;

namespace details{

///--------------------------------------------------------------------
/// function_traits is used to determine function properties
/// such as parameter types (arguments) and return type.
///--------------------------------------------------------------------
template<typename T>
struct function_traits
    : public function_traits<decltype(&T::operator())> {
};
// For generic types, directly use the result of the signature of its 'operator()'

template<typename ClassType, typename ReturnType, typename... Args>
struct function_traits<ReturnType(ClassType::*)(Args...) const>
  // we specialize for pointers to member function
{
  enum { arg_count = sizeof...(Args) };

  typedef ReturnType return_type;

  template<size_t i>
  struct arg_t {
    typedef typename std::tuple_element<i, std::tuple<Args...>>::type type;
  };
  template<size_t i>
  using arg = typename arg_t<i>::type;

  template<size_t i>
  struct arg_remove_ref_t {
    typedef typename std::remove_reference<arg<i>>::type type;
  };
  template<size_t i>
  using arg_remove_ref = typename arg_remove_ref_t<i>::type;

  typedef std::tuple<Args...> args;
};


///---------------------------------------------------------------------
/// Determine if type is part of a list of types
///---------------------------------------------------------------------
template<typename T, typename... Ts>
struct is_type;

template<typename T, typename... Ts>
struct is_type<T, T, Ts...>: std::true_type { };

template<typename T>
struct is_type<T, T>: std::true_type { };

template<typename T, typename Tail>
struct is_type<T, Tail>: std::false_type { };

template<typename T, typename Tail, typename... Ts>
struct is_type<T, Tail, Ts...>: is_type<T, Ts...>::type { };


///---------------------------------------------------------------------
/// Check if a class has implemented operator()
///
/// Mostly used for checking if provided type is a lambda expression
///---------------------------------------------------------------------
///
template<typename T>
struct is_callable {
  template<typename U>
  static char test(decltype(&U::operator()));

  template<typename U>
  static int test(...);

  enum { value = sizeof(test<T>(0)) == sizeof(char) };
};


///---------------------------------------------------------------------
/// Any class that should not be able to copy itself inherit this class
///---------------------------------------------------------------------
///
class forbid_copies {
 protected:
  forbid_copies() = default;
  ~forbid_copies() = default;
  forbid_copies(const forbid_copies &) = delete;
  forbid_copies &operator=(const forbid_copies &) = delete;
};

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

} // namespace details

} // namespace ecs

#endif //ECS_UTILS_H
