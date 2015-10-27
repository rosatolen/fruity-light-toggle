#!/bin/bash

set -e

function helptext {
    echo "Usage: ./go <command>"
    echo ""
    echo "Available commands are:"
    echo "    fleet             Create and deploy a Fleet: All connected devices become Nodes except for 1 Gateway at oldest attached device"
    echo "    nodes             Create Nodes out of all connected devices"
    echo "    gateway           Create and deploy a Gateway to oldest attached device"
    echo "    term <tty.file>   Open terminal to specified file"
    echo "    compile           Clean and compile FruityMesh source"
    echo "    debug             Setup jlinkgdbserver and start gdb."
}

function deploy-nodes-to-all-local-devices {
    toggle-gateway-config false
    compile
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

function toggle-gateway-config {
    if [[ `sed '30q;d' src/modules/GatewayModule.cpp` != *"#define IS_GATEWAY_DEVICE"* ]]
    then
        echo -e "ERROR!"
        echo -e "'#define IS_GATEWAY_DEVICE' is not on line 30 in src/modules/GatewayModule.cpp"
        echo -e "I know this dependency is terrible. I'm sorry. Help automate it better."
        exit 1
    fi
    perl -pe "s/.*/#define IS_GATEWAY_DEVICE $1/ if $. == 30" < src/modules/GatewayModule.cpp > src/modules/temporary
    mv src/modules/temporary src/modules/GatewayModule.cpp
}

function create-gateway {
    toggle-gateway-config true
    compile
    # Oldest connected J-Link device will be created as a gateway
    echo 0 | $HOME/nrf/tools/jlink $HOME/nrf/projects/fruitymesh/deploy/single-fruitymesh-softdevice-deploy.jlink
}

function fleet {
    deploy-nodes-to-all-local-devices
    create-gateway
}

function debug {
    if [ -f $(which jlinkgdbserver) ]; then
        jlinkgdbserver -device nrf51822 -if swd -speed 4000 -noir -port 2331 &
    else
        echo 'Please include jlinkgdbserver on your path.'
        exit 1
    fi

    if [ "$(uname)" == "Darwin" ]; then
        $HOME/nrf/sdk/gcc_arm_embedded_4_9_mac/bin/arm-none-eabi-gdb -q
    elif [ "$(expr substr $(uname -s) 1 5)" == "Linux" ]; then
        $HOME/nrf/sdk/gcc_arm_embedded_4_9_linux/bin/arm-none-eabi-gdb -q
    fi

    killall jlinkgdbserver
}

case "$1" in
    fleet) fleet
    ;;
    nodes) deploy-nodes-to-all-local-devices
    ;;
    gateway) create-gateway
    ;;
    term) term "$2"
    ;;
    compile) compile
    ;;
    debug) debug
    ;;
    *) helptext
    ;;
esac
