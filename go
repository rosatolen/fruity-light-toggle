#!/bin/bash

set -e

function helptext {
    echo "Usage: ./go <command>"
    echo ""
    echo "Available commands are:"
    echo "    deploy            Deploy latest built FruityMesh to all attached devices"
    echo "    term <tty.file>   Open specified terminal to attached device"
    echo "    compile           Clean and compile FruityMesh source"
    echo "    gateway           Create and deploy a Gateway to oldest attached device"
}

function deploy-to-all-local-devices {
    $HOME/nrf/projects/fruitymesh/deploy/deploy-to-all.sh
}

function term {
    if [ -z "$1" ]
    then
        echo "No tty file supplied."
        exit 1
    fi
    minicom --device $1 --b 38400
}

function compile {
    make clean && make
}

function gateway {
    if [[ `sed '30q;d' src/modules/GatewayModule.cpp` != *"#define IS_GATEWAY_DEVICE"* ]]
    then
        echo -e "ERROR!"
        echo -e "'#define IS_GATEWAY_DEVICE' is not on line 30 in src/modules/GatewayModule.cpp"
        echo -e "I know this dependency is terrible. I'm sorry. Help automate it better."
        exit 1
    fi
    perl -pe "s/.*/#define IS_GATEWAY_DEVICE true/ if $. == 30" < src/modules/GatewayModule.cpp > src/modules/temporary
    mv src/modules/temporary src/modules/GatewayModule.cpp
    compile
    # Oldest connected J-Link device will be created as a gateway
    echo 0 | $HOME/nrf/tools/jlink $HOME/nrf/projects/fruitymesh/deploy/single-fruitymesh-softdevice-deploy.jlink
}

case "$1" in
    deploy) deploy-to-all-local-devices
    ;;
    term) term "$2"
    ;;
    compile) compile
    ;;
    gateway) gateway
    ;;
    *) helptext
    ;;
esac
