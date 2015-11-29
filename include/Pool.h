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
/// Pool allocation class. The standard is to store one cache-line
/// (64 bytes) per chunk.
///
///---------------------------------------------------------------------
class BasePool: forbid_copies {
 public:
  inline explicit BasePool(size_t element_size, size_t chunk_size = ECS_DEFAULT_CHUNK_SIZE);
  inline virtual ~BasePool();

  inline index_t size() const { return size_; }
  inline index_t capacity() const { return capacity_; }
  inline size_t chunks() const { return chunks_.size(); }
  inline void ensure_min_size(std::size_t size);
  inline void ensure_min_capacity(size_t min_capacity);

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
/// This must be done from outside. The default chunk-size is 64 bytes *
/// the size of each object.
///
///---------------------------------------------------------------------
template<typename T>
class Pool: public BasePool {
 public:
  Pool(size_t chunk_size);

  inline virtual void destroy(index_t index) override;

  inline T *get_ptr(index_t index);
  inline const T *get_ptr(index_t index) const;

  inline T &get(index_t index);
  inline const T &get(index_t index) const;

  inline T &operator[](size_t index);
  inline const T &operator[](size_t index) const;
};

} // namespace details

} // namespace ecs

#include "Pool.inl"

#endif //ECS_POOL_H
