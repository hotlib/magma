// Copyright (c) 2016-present, Facebook, Inc.
// All rights reserved.
//
// This source code is licensed under the BSD-style license found in the
// LICENSE file in the root directory of this source tree. An additional grant
// of patent rights can be found in the PATENTS file in the same directory.

#include <devmand/devices/cli/translation/BindingReaderRegistry.h>

namespace devmand {
namespace devices {
namespace cli {

using namespace folly;
using namespace std;
using namespace ydk;

void BindingReaderRegistryBuilder::addReader(
    Path path,
    shared_ptr<BindingReader> reader) {
  domBuilder.add(path, make_shared<BindingReaderAdapter>(reader, context));
}

Future<dynamic> BindingReaderAdapter::read(
    const Path& path,
    const DeviceAccess& device) const {
  return bindingReader->read(path, device)
      .thenValue([path, &context = this->context](auto entity) {
        dynamic asDynamic = context.getCodec().toDom(path, *entity);
        // the object is prefixed with its name inside dynamic, extract the
        // object
        return asDynamic.items().begin()->second;
      });
}

BindingReaderAdapter::BindingReaderAdapter(
    shared_ptr<BindingReader> _bindingReader,
    BindingContext& _context)
    : bindingReader(_bindingReader), context(_context) {}

} // namespace cli
} // namespace devices
} // namespace devmand
