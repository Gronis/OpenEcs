namespace ecs{


template<typename T>
Iterator<T>::Iterator(EntityManager *manager, details::ComponentMask mask, bool begin) :
    manager_(manager),
    mask_(mask),
    cursor_(0){
  // Must be pool size because of potential holes
  size_ = manager_->entity_versions_.size();
  if (!begin) cursor_ = size_;
  find_next();
}

template<typename T>
Iterator<T>::Iterator(const Iterator &it) : Iterator(it.manager_, it.cursor_) { };

template<typename T>
index_t Iterator<T>::index() const {
  return cursor_;
}

template<typename T>
inline void Iterator<T>::find_next() {
  while (cursor_ < size_ && (manager_->component_masks_[cursor_] & mask_) != mask_) {
    ++cursor_;
  }
}

template<typename T>
T Iterator<T>::entity() {
  return manager_->get_entity(index()).template as<typename Iterator<T>::T_no_ref>();
}

template<typename T>
const T Iterator<T>::entity() const  {
  return manager_->get_entity(index()).template as<typename Iterator<T>::T_no_ref>();
}

template<typename T>
Iterator<T> &Iterator<T>::operator++() {
  ++cursor_;
  find_next();
  return *this;
}

template<typename T>
bool operator==(Iterator<T> const &lhs, Iterator<T> const &rhs) {
  return lhs.index() == rhs.index();
}

template<typename T>
bool operator!=(Iterator<T> const &lhs, Iterator<T> const &rhs) {
  return !(lhs == rhs);
}

template<typename T>
inline T operator*(Iterator<T> &lhs) {
  return lhs.entity();
}

template<typename T>
inline T const &operator*(Iterator<T> const &lhs) {
  return lhs.entity();
}


} // namespace ecs