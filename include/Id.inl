namespace ecs{

Id::Id() { }

Id::Id(index_t index, version_t version) :
    index_(index),
    version_(version)
{ }

bool operator==(const Id& lhs, const Id &rhs) {
  return lhs.index() == rhs.index() && lhs.version() == rhs.version();
}

bool operator!=(const Id& lhs, const Id &rhs) {
  return lhs.index() != rhs.index() || lhs.version() != rhs.version();
}

} // namespace ecs