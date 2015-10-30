CHIP INFO
---
We are using the nRF51422 and nRF51822. The chips are the same except for what kind of softdevice software they use.
32/16kB RAM
256/128kB Flash WLCSP
2.4GHz
32-bit ARM Cortex MO CPU core
---

You will need a patched version of the SDK in order to get the buttons and GPIOTE system to work. The problem is detailed [here](https://devzone.nordicsemi.com/question/47031/button-debouncer-nrf51822/) and [here](https://devzone.nordicsemi.com/question/40670/sdk81-and-sdk-90-app_gpiote-and-nrf_drv_gpiote-conflict/).

RUN THIS FROM HERE:
```
cd ../../sdk/nrf_sdk_9_0/
curl https://devzone.nordicsemi.com/attachment/a7a813d4112f2f2f4921a8e6a3a60b67 | sudo patch -p1
```

## Set up QA Environment
1. Install fleet
    a. Connect all the devices through a USB hub to your computer
    b. Open terminals to all the connected devices using the command `$./go term </dev/cu.file>`
    c. Make sure the device with a button is not the oldest attached device so that it cannot become a gateway with the install script. (simply plug it in last!)
    d. Run the command: `sudo ./go fleet`
    e. Press the button (3) to send the vote. You should see it acknowledged on the gateway node. (if you're not seeing the acknowledgement, power cycling the device)

## WHAT ARE ALL THESE BLINKING LIGHTS?!?!?!?!?!
### Nodes are *Ready to Run*
A gateway is ready to go when it is consistently blinking purple.
A node is ready to go when it is consistently blinking blue.

### Nodes are in an *Error State* if...
1. The light is not on. This means the application has crashed.
2. The light is stuck in a solid color and not blinking. This means the application has crashed.
3. The light is blinking red. This means the node was not able to connect to the mesh.

### How do I return the node to ready state?
1. Turn off and on the node.
2. While the node is turning on, the lights show its state. A node can be connected to up to 4 other nodes. It will flash red 3 times if it has not been able to connect with any other nodes. If it is a gateway node, it will flash purple during set up as well as red.
3. Every time the node connects to another, it will flash green once and then begin consistently blinking blue or purple if it is a gateway.

## How do I get the timer handler right?
Inside the TimerEventHandler, do the following logs:
logt("<yourtag>", "appTimer = %d\n", appTimer);
logt("<yourtag>", "minutes = %d\n", appTimer/60000 % 1000); // find the minutes
logt("<yourtag>", "seconds = %d\n", appTimer/1000 % 60);  // find the seconds

## Extern C
Know what extern c is used for and why, when you import libraries from the NordicSDK, you get linker failures when you forget to put the libraries in extern c.
