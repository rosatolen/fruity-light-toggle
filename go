#!/bin/bash

set -e

function helptext {
    echo "Usage: ./go <command>"
    echo ""
    echo "Available commands are:"
    echo "    deploy        Deploy latest built FruityMesh to attached device"
    echo "    term          Open terminal to attached device"
    echo "    compile       Clean and compile FruityMesh source"
    echo "    cdt           Compile, deploy, and open the terminal."
}

function deploy-to-local-device {
    $HOME/nrf/tools/jlink deploy/upload_softdevice.jlink
    $HOME/nrf/tools/jlink deploy/upload_fruitymesh.jlink
}

function term {
    minicom --device /dev/ttyACM0 --b 38400
}

function compile {
    make clean && make
}

function cdt {
    compile
    deploy-to-local-device
    term
}

case "$1" in
    deploy) deploy-to-local-device
    ;;
    term) term
    ;;
    compile) compile
    ;;
    cdt) cdt
    ;;
    *) helptext
    ;;
esac
