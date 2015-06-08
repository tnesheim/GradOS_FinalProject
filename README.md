# GradOS_FinalProject
Final Project Code for Grad OS involving the implementation of RIOT OS on various BLE sensor nodes.

The modified RIOT OS base code can be found in the with_riot/ directory.

The important files that were modified within RIOT OS are the following:
sys/transceivers/transceivers.c
sys/include/transceivers.h

The following driver files were added to facilitate the BLE Slave Transceiver:
drivers/nrf51822BLE/nrf51822BLE.c
drivers/include/nrf51822BLE.h

The example application code along with the BLE Slave Transceiver code can be found here:
nodes/riot_nodes
nodes/ble_nodes
