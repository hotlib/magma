FROM debian:stretch AS %%PKG%%
ARG PKG_DIR=/cache/%%PKG%%
ARG PKG_REPO_DIR=/cache/%%PKG%%/repo
ARG PKG_BUILD_DIR=/cache/%%PKG%%/build
ARG PKG_INSTALL_DIR=/cache/%%PKG%%/install

RUN %%INSTALL%% git cmake ninja-build %%DEPS%%
RUN usermod --password $(echo root | openssl passwd -1 -stdin) root
RUN apt-get update && apt-get install openssh-server -y
RUN echo 'PermitRootLogin yes' >> /etc/ssh/sshd_config
# Increase OS limit for file descriptors
RUN echo 'ulimit -n 20000' >> ~/.bashrc

# TODO need to build this manually for some reason
WORKDIR /usr/src/gtest
RUN cmake CMakeLists.txt
RUN make
RUN cp *.a /usr/lib

COPY --from=%%DOCKER_REGISTRY%%/thirdparty /cache/install /usr
COPY --from=%%DOCKER_REGISTRY%%/firstparty /cache/install /usr
RUN cp -r /usr/include/prometheus/* /usr/include

# There is a bug in grpc lib naming so manually fix it.
RUN mkdir -p $PKG_INSTALL_DIR/lib/
RUN cp /usr/lib/libgrpc++.so* $PKG_INSTALL_DIR/lib/

WORKDIR $PKG_REPO_DIR

# Install nodejs for js_plugins
RUN curl -sL https://deb.nodesource.com/setup_12.x | bash -
RUN apt-get update && apt-get install nodejs -y
RUN npm install -g nodemon

WORKDIR $PKG_REPO_DIR

ENV PKG_REPO_DIR=$PKG_REPO_DIR
ENV PKG_BUILD_DIR=$PKG_BUILD_DIR
ENV PKG_INSTALL_DIR=$PKG_INSTALL_DIR
ARG PKG_BUILD_DIR_SUFFIX=_copy
ENV PKG_BUILD_DIR_COPY=$PKG_BUILD_DIR$PKG_BUILD_DIR_SUFFIX

RUN printf '#!/bin/bash \n\
\n\
make \n\
cd $PKG_BUILD_DIR/js_plugins \n\
npm install \n\
mkdir $PKG_BUILD_DIR_COPY \n\
cp -r $PKG_BUILD_DIR/devmand $PKG_BUILD_DIR_COPY/devmand \n\
cp -r $PKG_BUILD_DIR/js_plugins $PKG_BUILD_DIR_COPY/ \n\
cp -r $PKG_BUILD_DIR/config-sample $PKG_BUILD_DIR_COPY/ \n\
ls $PKG_BUILD_DIR_COPY \n\
' >> /start.sh
RUN chmod +x /start.sh
RUN cat /start.sh

# Execute devmand build and js_plugins package installation upon entry
CMD ["/start.sh"]
