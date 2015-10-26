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

## WHAT ARE ALL THESE BLINKING LIGHTS?!?!?!?!?!

A node reports both whether it is a gateway node and also the status of its connections to other nodes using different colored flashing lights.

A node can be connected to up to 4 other nodes. If the node cannot see any other nodes at all, it **flashes red** 3 times. 

To report the status of its connections  it cycles through each of the 4 possible connection slots and flashes the LED to indicate the status of that connection.
a **green flash** means "I see another node, I'm in the process of connecting". A **blue flash** means "I'm connected to another node".

If the node is a gateway node if will **flash purple** once after cycling through the 4 connection slots. If it isn't then the led will remain dark for a beat.

### Examples
If the node flashes "blue-dark-green-purple" it is connected to one node (in the first connection slot), in the process 
of connecting to a node (in the 3rd connection slot), and is a gateway node.
 
If the node flashes "red-red-red-dark" it cannot see any other nodes, and is not a gateway node.