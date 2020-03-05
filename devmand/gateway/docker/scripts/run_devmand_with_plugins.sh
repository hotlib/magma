#!/bin/bash
dirname="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

cd ${dirname}

docker run -d -h devmanddevel \
      --name dvmnd \
      -v "$(realpath ../../):/cache/devmand/repo:rw" \
      -v "$(realpath ~/cache_devmand_build):/cache/devmand/build:rw" \
      --entrypoint /bin/bash \
      "frinx/dvmnd-test" \
      -c 'mkdir -p /run/sshd && /usr/sbin/sshd && bash \
        -c "cp -r -n ../build_copy/* ../build && cd ../build && \
        (sleep 4 && ./devmand --logtostderr=1 --device_configuration_file=config-sample/dev_conf.yaml & \
        cd js_plugins && nodemon plugin-linux-ifc-reader.js)"'
