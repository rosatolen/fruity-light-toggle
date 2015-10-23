#!/bin/bash

NUMBER_OF_DEVICES=`expr $(echo -e "ShowEmuList\nexit\n" | $HOME/nrf/tools/jlink | grep 'J-Link\[' | wc -l) - 1`

FRUITY_REPO=$HOME/nrf/projects/fruitymesh

for i in $(seq 0 $NUMBER_OF_DEVICES); do
    echo $i | $HOME/nrf/tools/jlink $FRUITY_REPO/deploy/single-fruitymesh-softdevice-deploy.jlink
done
