This is the QCon Platform for wireless bluetooth communication. It builds on the following repositories:

* [FruityMesh](https://github.com/mwaylabs/fruitymesh)
* [FruityGate](https://github.com/microcosm/fruitygate-nrf)

FruityMesh provides the Bluetooth Low Energy mesh infrastructure and FruityGate contains the node.js server with which we based our mesh-to-internet solution.

This repository has the code to configure all nRF devices used on the mesh.

In order to deploy, please use the following resources to set up your environment.
* Use the [wiki](https://github.com/mwaylabs/fruitymesh/wiki/Quick-Start) to flash a vanilla version of FruityMesh onto your Nordic dongle. This will make sure that you have the softdevice.hex and the jlink software installed properly.
* [FruityFactory](https://github.com/FruityLoopers/fruityfactory) works on OS X with Docker
* [Fruity Ubuntu VM](https://github.com/ihassin/fruitymesh-ubuntu-vm) works with ansible and vagrant
* [MwayLabs Dev Environment](https://github.com/mwaylabs/fruitymesh-devenv) works on Windows and sets up an environment with eclipse

Lastly, make sure the path to the Nordic SoftDevice on line 2 of fruitymesh/deploy/single-fruitymesh-softdevice-deploy.jlink is correct for your setup.

To check that you have your environment running properly, you can run the go script in this repository with:
`$./go compile`

If successful, 'go compile' will create the FruityMesh.hex file in your ./\_build folder.

The go script will also flash the compiled code onto all the nRF devices attached to your machine. In order to see your options, run:
`$./go`

We have compiled some learnings into the DEV\_README.md file.
