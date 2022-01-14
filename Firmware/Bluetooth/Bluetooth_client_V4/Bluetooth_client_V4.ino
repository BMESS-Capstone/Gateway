/**
  BLE Client for the pIRfusiX sensor system Version 4
  Note: uses NimBLE instead of orginal BLEDevice (less resource intensive)
  Uses 41% of program storage space & 9% of dynamic memory

  Author: Khaled Elmalawany
*/

#include <NimBLEDevice.h>

#define ONBOARD_LED 2

// BLE Server Name
#define bleServerName "pIRfusiX sensor"

/* UUID's of the service, characteristic that we want to read and/or write */
// BLE Services
static BLEUUID connectServiceUUID("7e161f79-2ce9-46d1-917a-8e20f6b1f675");

// Connection Characteristic
static BLEUUID connectCharacteristicUUID("cc58110b-d173-4a9f-b0d5-1b0dd006c357");

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;
static boolean doScan = false;

//Advertised device of the peripheral device. Address will be found during scanning...
static BLEAdvertisedDevice *myDevice;

//Characteristic that we want to read
static BLERemoteCharacteristic *pRemoteCharacteristic;

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

/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    /**
        Called for each advertising BLE server.
    */

    /*** Only a reference to the advertised device is passed now
      void onResult(BLEAdvertisedDevice advertisedDevice) { **/
    void onResult(BLEAdvertisedDevice* advertisedDevice) {
      // Serial.print("BLE Advertised Device found: ");
      // Serial.println(advertisedDevice->toString().c_str());

      // We have found a device, let us now see if it contains the service we are looking for.
      /********************************************************************************
          if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      ********************************************************************************/
      if (advertisedDevice->haveServiceUUID() && advertisedDevice->isAdvertisingService(connectServiceUUID)) {

        BLEDevice::getScan()->stop();
        /*******************************************************************
              myDevice = new BLEAdvertisedDevice(advertisedDevice);
        *******************************************************************/
        myDevice = advertisedDevice; /** Just save the reference now, no need to copy the object */
        doConnect = true;
        doScan = true;

      } // Found our server
    } // onResult
}; // MyAdvertisedDeviceCallbacks

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient* pClient = BLEDevice::createClient();
  pClient -> setClientCallbacks(new MyClientCallback());
  Serial.println(" - Created client");

  // Connect to the remove BLE Server.
  pClient->connect(myDevice);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(connectServiceUUID);
  if (pRemoteService == nullptr) {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(connectServiceUUID.toString().c_str());
    return (false);
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristics in the service of the remote BLE server.
  pRemoteCharacteristic = pRemoteService->getCharacteristic(connectCharacteristicUUID);
  if (pRemoteCharacteristic == nullptr) {
    Serial.println("Failed to find our characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristic");

  // Read the value of the characteristic.
  if (pRemoteCharacteristic -> canRead()) {
    std::string value = pRemoteCharacteristic -> readValue();
    Serial.print("The characteristic value was: ");
    Serial.println(value.c_str());
  }

  //Assign callback functions for the Characteristics
  if (pRemoteCharacteristic->canNotify()) {
    //if(!pChr->registerForNotify(notifyCB)) {
    if (!pRemoteCharacteristic->subscribe(true, notifyCallback)) {
      /** Disconnect if subscribe failed */
      pClient->disconnect();
      return false;
    }
  } else if (pRemoteCharacteristic->canIndicate()) {
    /** Send false as first argument to subscribe to indications instead of notifications */
    //if(!pChr->registerForNotify(notifyCB, false)) {
    if (!pRemoteCharacteristic->subscribe(false, notifyCallback)) {
      /** Disconnect if subscribe failed */
      pClient->disconnect();
      return false;
    }
  }
  return true;
}

void setup() {
  Serial.begin(115200);

  Serial.println("Starting Arduino BLE Client application...");
  while (Serial.available() == 0) {}

  BLEDevice::init("");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run for 15 seconds.
  BLEScan* pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(15);

  pinMode(ONBOARD_LED, OUTPUT);
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
    String newValue = "Time since boot: " + String(millis() / 1000);
    Serial.println("Setting new characteristic value to \"" + newValue + "\"");

    // Set the characteristic's value to be the array of bytes that is actually a string.
    /*** Note: write / read value now returns true if successful, false otherwise - try again or disconnect ***/
    pRemoteCharacteristic->writeValue(newValue.c_str(), newValue.length());
  } else if (doScan) {
    BLEDevice::getScan()->start(0);  // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(1000); // Delay a second between loops.
} // End of loop
