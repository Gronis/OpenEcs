namespace ecs{

namespace details{

BasePool::BasePool(size_t element_size, size_t chunk_size) :
    size_(0),
    capacity_(0),
    element_size_(element_size),
    chunk_size_(chunk_size) {
}

BasePool::~BasePool() {
  for (char *ptr : chunks_) {
    delete[] ptr;
  }
}
void BasePool::ensure_min_size(std::size_t size) {
  if (size >= size_) {
    if (size >= capacity_) ensure_min_capacity(size);
    size_ = size;
  }
}
void BasePool::ensure_min_capacity(size_t min_capacity) {
  while (min_capacity >= capacity_) {
    char *chunk = new char[element_size_ * chunk_size_];
    chunks_.push_back(chunk);
    capacity_ += chunk_size_;
  }
}

template<typename T>
Pool<T>::Pool(size_t chunk_size) : BasePool(sizeof(T), chunk_size) { }

template<typename T>
void Pool<T>::destroy(index_t index) {
  ECS_ASSERT(index < size_, "Pool has not allocated memory for this index.");
  T *ptr = get_ptr(index);
  ptr->~T();
}

template<typename T>
inline T* Pool<T>::get_ptr(index_t index) {
  ECS_ASSERT(index < capacity_, "Pool has not allocated memory for this index.");
  return reinterpret_cast<T *>(chunks_[index / chunk_size_] + (index % chunk_size_) * element_size_);
}

template<typename T>
inline const T* Pool<T>::get_ptr(index_t index) const {
  ECS_ASSERT(index < this->capacity_, "Pool has not allocated memory for this index.");
  return reinterpret_cast<T *>(chunks_[index / chunk_size_] + (index % chunk_size_) * element_size_);
}

template<typename T>
inline T & Pool<T>::get(index_t index) {
  return *get_ptr(index);
}

template<typename T>
inline const T & Pool<T>::get(index_t index) const {
  return *get_ptr(index);
}

template<typename T>
inline T & Pool<T>::operator[](size_t index) {
  return get(index);
}

template<typename T>
inline const T & Pool<T>::operator[](size_t index) const {
  return get(index);
}

} // namespace details

} // namespace ecs