You will need a patched version of the SDK in order to get the buttons and GPIOTE system to work. The problem is detailed [here](https://devzone.nordicsemi.com/question/40670/sdk81-and-sdk-90-app_gpiote-and-nrf_drv_gpiote-conflict/).

RUN THIS FROM HERE:
```
cd ../../sdk/nrf_sdk_9_0/
curl https://devzone.nordicsemi.com/attachment/a7a813d4112f2f2f4921a8e6a3a60b67 | sudo patch -p1
```
