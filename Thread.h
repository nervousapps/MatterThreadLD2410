// Copyright 2024 Espressif Systems (Shanghai) PTE LTD
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at

//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

/*
   OpenThread.begin(false) will not automatically start a node in a Thread Network
   A Leader node is the first device, that has a complete dataset, to start Thread
   A complete dataset is easily achieved by using the OpenThread CLI command "dataset init new"

   In order to allow other node to join the network,
   all of them shall use the same network master key
   The network master key is a 16-byte key that is used to secure the network

   Using the same channel will make the process faster

*/

#include "OThreadCLI.h"
#include "OThreadCLI_Util.h"
#include "config.h"

otInstance *aInstance = NULL;

void setupThread() {
  OThreadCLI.begin(false);  // No AutoStart - fresh start
  Serial.println();
  Serial.println("Setting up OpenThread Node as Router/Child");
  Serial.println("Make sure the Leader Node is already running");
  aInstance = esp_openthread_get_instance();

  OThreadCLI.println("dataset clear");
  OThreadCLI.println(CLI_NETWORK_KEY);
  OThreadCLI.println(CLI_NETWORK_CHANEL);
  OThreadCLI.println("dataset commit active");
  OThreadCLI.println("ifconfig up");
  OThreadCLI.println("thread start");
}

void waitThread() {
  while(otGetStringDeviceRole() != "Child" && otGetStringDeviceRole() != "Router") {
  // while(otGetDeviceRole() != OT_ROLE_CHILD || otGetDeviceRole() != OT_ROLE_ROUTER) {
    Serial.println("=============================================");
    Serial.print("Thread Node State: ");
    Serial.println(otGetStringDeviceRole());
    delay(1000);
  }
  // Native OpenThread API calls:
  // wait until the node become Child or Router
  if(otGetStringDeviceRole() == "Child" || otGetStringDeviceRole() == "Router") {
  // if (otGetDeviceRole() == OT_ROLE_CHILD || otGetDeviceRole() == OT_ROLE_ROUTER) {
    // Network Name
    const char *networkName = otThreadGetNetworkName(aInstance);
    Serial.printf("Network Name: %s\r\n", networkName);
    // Channel
    uint8_t channel = otLinkGetChannel(aInstance);
    Serial.printf("Channel: %d\r\n", channel);
    // PAN ID
    uint16_t panId = otLinkGetPanId(aInstance);
    Serial.printf("PanID: 0x%04x\r\n", panId);
    // Extended PAN ID
    const otExtendedPanId *extPanId = otThreadGetExtendedPanId(aInstance);
    Serial.printf("Extended PAN ID: ");
    for (int i = 0; i < OT_EXT_PAN_ID_SIZE; i++) {
      Serial.printf("%02x", extPanId->m8[i]);
    }
    Serial.println();
    // Network Key
    otNetworkKey networkKey;
    otThreadGetNetworkKey(aInstance, &networkKey);
    Serial.printf("Network Key: ");
    for (int i = 0; i < OT_NETWORK_KEY_SIZE; i++) {
      Serial.printf("%02x", networkKey.m8[i]);
    }
    Serial.println();
    // IP Addresses
    char buf[OT_IP6_ADDRESS_STRING_SIZE];
    const otNetifAddress *address = otIp6GetUnicastAddresses(aInstance);
    while (address != NULL) {
      otIp6AddressToString(&address->mAddress, buf, sizeof(buf));
      Serial.printf("IP Address: %s\r\n", buf);
      address = address->mNext;
    }
    // Multicast IP Addresses
    const otNetifMulticastAddress *mAddress = otIp6GetMulticastAddresses(aInstance);
    while (mAddress != NULL) {
      otIp6AddressToString(&mAddress->mAddress, buf, sizeof(buf));
      printf("Multicast IP Address: %s\n", buf);
      mAddress = mAddress->mNext;
    }
  }
}
