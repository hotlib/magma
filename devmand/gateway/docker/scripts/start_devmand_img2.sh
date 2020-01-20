#!/bin/bash 

DEVMANDDOCKERNET="devmanddevelnet"
DEVMANDIP="172.8.0.85"
DEVMANDSUBNET="172.8.0.0/16"

docker network inspect ${DEVMANDDOCKERNET}

if [ $? -eq 0 ]
then
    echo "docker network exists"
else
    docker network create --subnet=${DEVMANDSUBNET} ${DEVMANDDOCKERNET}
fi

docker run -h devmanddevel --net ${DEVMANDDOCKERNET} --ip ${DEVMANDIP} -it "fd1f7ede8fb3" /bin/bash -c "bash"
