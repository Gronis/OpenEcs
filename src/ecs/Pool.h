#ifndef ECS_POOL_H
#define ECS_POOL_H

#include "Utils.h"

namespace ecs{

namespace details {

///---------------------------------------------------------------------
/// A Pool is a memory pool.
/// See link for more info: http://en.wikipedia.org/wiki/Memory_pool
///---------------------------------------------------------------------
///
/// The BasePool is used to store void*. Use Pool<T> for a generic
/// Pool allocation class.
///
///---------------------------------------------------------------------
class BasePool: forbid_copies {
 public:
  explicit BasePool(size_t element_size, size_t chunk_size = ECS_DEFAULT_CHUNK_SIZE);
  virtual ~BasePool();

  index_t size() const { return size_; }
  index_t capacity() const { return capacity_; }
  size_t chunks() const { return chunks_.size(); }
  void ensure_min_size(std::size_t size);
  void ensure_min_capacity(size_t min_capacity);

  virtual void destroy(index_t index) = 0;

 protected:
  index_t size_;
  index_t capacity_;
  size_t element_size_;
  size_t chunk_size_;
  std::vector<char *> chunks_;
};

///---------------------------------------------------------------------
/// A Pool is a memory pool.
/// See link for more info: http://en.wikipedia.org/wiki/Memory_pool
///---------------------------------------------------------------------
///
/// The Pool is used to store * of any class. Destroying an object calls
/// destructor. The pool doesn't know where there is data allocated.
/// This must be done from outside.
///
///---------------------------------------------------------------------
template<typename T>
class Pool: public BasePool {
 public:
  Pool(size_t chunk_size);

  virtual void destroy(index_t index) override;

  T *get_ptr(index_t index);
  const T *get_ptr(index_t index) const;

  T &get(index_t index);
  const T &get(index_t index) const;

  T &operator[](size_t index);
  const T &operator[](size_t index) const;
};

} // namespace details

} // namespace ecs

#include "Pool.inl"

#endif //ECS_POOL_H
