You will need a patched version of the SDK in order to get the buttons and GPIOTE system to work. The problem is detailed [here](https://devzone.nordicsemi.com/question/47031/button-debouncer-nrf51822/) and [here](https://devzone.nordicsemi.com/question/40670/sdk81-and-sdk-90-app_gpiote-and-nrf_drv_gpiote-conflict/).

RUN THIS FROM HERE:
```
cd ../../sdk/nrf_sdk_9_0/
curl https://devzone.nordicsemi.com/attachment/a7a813d4112f2f2f4921a8e6a3a60b67 | sudo patch -p1
```

##Set up QA Environment
1. Install fleet
    a. Connect all the devices through a USB hub to your computer
    b. Open terminals to all the connected devices using the command `$./go term </dev/cu.file>`
    c. Make sure the device with a button is not the oldest attached device so that it cannot become a gateway with the install script. (simply plug it in last!)
    d. Run the command: `sudo ./go fleet`
    e. Press the button (3) to send the vote. You should see it acknowledged on the gateway node. (if you're not seeing the acknowledgement, power cycling the device)
