#pragma once

#include <devmand/devices/cli/ModelRegistry.h>
#include <ydk/entity_data_node_walker.hpp>
#include <ydk/types.hpp>

namespace devmand {
namespace devices {
namespace cli {

using namespace std;
using namespace ydk;
using namespace devmand::devices::cli;

class BaseReader {
 public:
  virtual shared_ptr<Entity> read() = 0;
};

template <typename R>
class Reader : public BaseReader {
 public:
  virtual void read(shared_ptr<R> data) = 0;

  shared_ptr<Entity> read() override {
    shared_ptr<R> ptr = makeEmptyEntity();
    read(ptr);
    return static_pointer_cast<Entity>(ptr);
  }

  shared_ptr<R> makeEmptyEntity() {
    return make_shared<R>();
  }
};

class ReaderEncapsulation {
 private:
  shared_ptr<BaseReader> reader;

  Path path;
  map<Path, string> additionalConditions;

 public:
  ReaderEncapsulation(
      shared_ptr<BaseReader> _reader,
      Path _path,
      map<Path, string> _additionalConditions = map<Path, string>())
      : reader(_reader),
        path(_path),
        additionalConditions(_additionalConditions) {}

  shared_ptr<Entity> read() const {
    return reader->read();
  }
};

class ReaderRegistry {
 public:
  explicit ReaderRegistry(Bundle& _bundle) : bundle(_bundle){};
  // TODO disable copying and moving

  RootDataNode readAll() const {
    map<string, shared_ptr<DataNode>> roots;
    for (const auto& root : rootReaders) {
      shared_ptr<Entity> entity = root.read();

      DataNode& node =
          get_data_node_from_entity(*entity, bundle.rootSchemaNode());
      roots[entity->get_segment_path()] = shared_ptr<DataNode>(&node);
    }

    return RootDataNode(roots);
  }

  void checkReaderConsistency() const {}

  template <typename R>
  void add(Path path, shared_ptr<Reader<R>> reader) {
    rootReaders.emplace_back(static_pointer_cast<BaseReader>(reader), path);
  }

 private:
  vector<ReaderEncapsulation> rootReaders;
  Bundle& bundle;
};

} // namespace cli
} // namespace devices
} // namespace devmand
