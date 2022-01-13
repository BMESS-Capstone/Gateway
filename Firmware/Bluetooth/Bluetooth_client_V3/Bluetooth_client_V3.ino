/**
 BLE Client for the pIRfusiX sensor system Version 3

 Author: Khaled Elmalawany
 */

#include "BLEDevice.h"

#define ONBOARD_LED  2

// BLE Server Name
#define bleServerName "pIRfusiX sensor"

/* UUID's of the service, characteristic that we want to read and/or write */
// BLE Services
static BLEUUID connectServiceUUID("175f0000-73f7-11ec-90d6-0242ac120003");

// Connection Characteristic
static BLEUUID connectCharacteristicUUID("175f50003-73f7-11ec-90d6-0242ac120003");

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

//Address of the peripheral device. Address will be found during scanning...
static BLEAddress *pServerAddress;

//Characteristic that we want to read
static BLERemoteCharacteristic *connectCharacteristic;

//Variable to store characteristic value
int connectChar;

static void notifyCallback(
  BLERemoteCharacteristic* pBLERemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
    Serial.print("Notify callback for characteristic ");
    Serial.print(pBLERemoteCharacteristic -> getUUID().toString().c_str());
    Serial.print(" of data length ");
    Serial.println(length);
    Serial.print("data: ");
    Serial.println((char*)pData);
}

//Callback function for the BLE client (i.e. this device)
class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient* pclient) {
    digitalWrite(ONBOARD_LED, HIGH);
  }

  void onDisconnect(BLEClient* pclient) {
    connected = false;
    digitalWrite(ONBOARD_LED, LOW);
    Serial.println("onDisconnect");
  }
};

//Callback function that gets called, when another device's advertisement has been received during scan
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice advertisedDevice) {
    if (advertisedDevice.getName() == bleServerName) { //Check if the name of the advertiser matches
      advertisedDevice.getScan()->stop(); //Scan can be stopped, we found what we are looking for
      pServerAddress = new BLEAddress(advertisedDevice.getAddress()); //Address of advertiser is the one we need
      doConnect = true; //Set indicator, stating that we are ready to connect
      Serial.println("Device found. Connecting!");
    }
  }
};

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(BLEAddress pAddress) {
  BLEClient* pClient = BLEDevice::createClient();
  pClient -> setClientCallbacks(new MyClientCallback());
  Serial.println(" - Created client");
 
  // Connect to the remove BLE Server.
  pClient->connect(pAddress);
  Serial.println(" - Connected to server");
 
  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService* pRemoteService = pClient->getService(connectServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(bmeServiceUUID.toString().c_str());
    return (false);
  }
  Serial.println(" - Found our service");
 
  // Obtain a reference to the characteristics in the service of the remote BLE server.
  connectCharacteristic = pRemoteService->getCharacteristic(connectCharacteristicUUID);
  if (connectCharacteristicUUID == nullptr) {
    Serial.print("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if(pRemoteCharacteristic -> canRead()) {
    std::string value = pRemoteCharacteristic -> readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }
 
  //Assign callback functions for the Characteristics
  connectCharacteristic->registerForNotify(connectNotifyCallback);
  return true;
}

void setup() {
  Serial.begin(115200);

  Serial.println("Starting Arduino BLE Client application...");
  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 15 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(15);

  pinMode(ONBOARD_LED,OUTPUT);
}

void loop() {
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are 
  // connected we set the connected flag to be true.
  if (doConnect == true) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // If we are connected to a peer BLE Server, update the characteristic each time we are reached
  // with the current time since boot.
  if (connected) {
    String newValue = "Time since boot: " + String(millis()/1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");
    
    // Set the characteristic's value to be the array of bytes that is actually a string.
    // pRemoteCharacteristic -> writeValue(newValue.c_str(), newValue.length());
  } else if(doScan){
    BLEDevice::getScan() -> start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }
  
  delay(1000); // Delay a second between loops.

}
