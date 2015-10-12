#!/bin/bash

function helptext {
    echo "Usage: ./go <command>"
    echo ""
    echo "Available commands are:"
    echo "    deploy        Deploy latest built FruityMesh to attached device"
    echo "    term          Open terminal to attached device"
    echo "    compile       Clean and compile FruityMesh source"
}

function deploy-to-local-device {
    /home/deploy/nrf/tools/jlink deploy/upload_softdevice.jlink
    /home/deploy/nrf/tools/jlink deploy/upload_fruitymesh.jlink
}

function term {
    minicom --device /dev/ttyACM0 --b 38400
}

function compile {
    make clean && make
}

case "$1" in
    deploy) deploy-to-local-device
    ;;
    term) term
    ;;
    compile) compile
    ;;
    *) helptext
    ;;
esac