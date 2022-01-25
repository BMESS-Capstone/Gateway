/**
  BLE Client for the pIRfusiX sensor system Version 6
  Note: uses NimBLE instead of orginal BLEDevice (less resource intensive)
  Note: gets rid of all Serial to increase performance speed
  Consider: It seems like NimBLE (and ESP32 Arduino BLE) can support up to 3 simulataneous connections

  Author: Khaled Elmalawany
*/

#include <NimBLEDevice.h>

//*****Shared with NANO 33 TODO: move to a common refererred .h file***
// Parameters only for Sensor
#define BATTERY_INTERVAL_MS 2000
#define BATTERY_PIN A0

// Parameters only for Gateway
#define ONBOARD_LED 2

// Shared parameters
#define TOTAL_POSSIBLE_LOCATIONS 4
#define LEFT_ARM 0
#define RIGHT_ARM 1
#define LEFT_LEG 2
#define RIGHT_LEG 3

#define CONNECT_UUID "6164e702-7565-11ec-90d6-0242ac120003"

#define SENSOR_CHAR_UUID "fec40b26-757a-11ec-90d6-0242ac120003"
#define BATTERY_CHAR_UUID "fec40dc4-757a-11ec-90d6-0242ac120003"
//********************************************************************

/* UUID's of the service, characteristic that we want to read and/or write */
// BLE Services
static const char serviceUUID[] = CONNECT_UUID;

// BLE Client
static BLEClient *pClient;

// Connection Characteristic
static const char sensorCharacteristicUUID[] = SENSOR_CHAR_UUID;
static const char batteryCharacteristicUUID[] = BATTERY_CHAR_UUID;

//Flags stating if should begin connecting and if the connection is up
boolean doConnect = false;
boolean connected = false;
boolean isConnectionComplete = false;
boolean moreThanOneSensor = false;
uint8_t connectionCounter = 0;
uint8_t iterationCounter = 0;
uint8_t deviceIndex = 0;
static uint8_t brockenDevicesCounter = 0;

//Advertised device of the peripheral device. Address will be found during scanning...
BLEAdvertisedDevice *myDevice = null;
static std::string myDevices[TOTAL_POSSIBLE_LOCATIONS];
static std::string brockenDevices[TOTAL_POSSIBLE_LOCATIONS];

//Characteristic that we want to read
BLERemoteCharacteristic *pRemoteSensorCharacteristic;
BLERemoteCharacteristic *pRemoteBatteryCharacteristic;

//Variable to store characteristics' value
float sensorValue;
int batteryValue;

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  if (isConnectionComplete) {
    if (pBLERemoteCharacteristic->getUUID().toString() == sensorCharacteristicUUID) {
      sensorValue = *(float *)pData;
      if (moreThanOneSensor)
        iterationCounter++;
    } else
      batteryValue = *(int *)pData;
  }
}

//Callback function for the BLE client (i.e. this device)
class MyClientCallback : public BLEClientCallbacks {
    void onConnect(BLEClient *pclient) {
      connected = true;
      digitalWrite(ONBOARD_LED, HIGH);
    }
    void onDisconnect(BLEClient *pclient) {
      connected = false;
      digitalWrite(ONBOARD_LED, LOW);
    }
};

//Scan for BLE servers and find the first one that advertises the service we are looking for.
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice *advertisedDevice) {
      std::__cxx11::string add = advertisedDevice->getAddress().toString();
      std::string *address = std::find(std::begin(myDevices), std::end(myDevices), add);
      std::string *brockenAddress = std::find(std::begin(brockenDevices), std::end(brockenDevices), add);
      if (address == std::end(myDevices) && brockenAddress == std::end(brockenDevices)) {
        if (advertisedDevice->haveServiceUUID() && advertisedDevice->getServiceUUID().toString() == serviceUUID) {
          if (connectionCounter > 0)
            moreThanOneSensor = true;
          BLEDevice::getScan()->stop();
          myDevice = advertisedDevice; /** Just save the reference now, no need to copy the object */
          doConnect = true;
        } // Found our server
      } // Check for duplicated address
    }// onResult
}; // MyAdvertisedDeviceCallbacks

//Connect to the BLE Server that has the name, Service, and Characteristics
boolean connectToServer(std::string device) {
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remote BLE Server.
  if (!pClient->connect(device))
    return false;

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr) {
    pClient->disconnect();
    while (connected)
      delay(1);
    return false;
  }

  // Obtain a reference to the characteristics in the service of the remote BLE server.
  pRemoteSensorCharacteristic = pRemoteService->getCharacteristic(sensorCharacteristicUUID);
  if (pRemoteSensorCharacteristic == nullptr) {
    pClient->disconnect();
    while (connected)
      delay(1);
    return false;
  }
  pRemoteBatteryCharacteristic = pRemoteService->getCharacteristic(batteryCharacteristicUUID);
  if (pRemoteBatteryCharacteristic == nullptr) {
    pClient->disconnect();
    while (connected)
      delay(1);
    return false;
  }

  if (pRemoteSensorCharacteristic->canRead() && pRemoteBatteryCharacteristic->canRead()) {
    // Read the values of the characteristics.
    sensorValue = pRemoteSensorCharacteristic->readValue<float>();
    batteryValue = pRemoteBatteryCharacteristic->readValue<uint16_t>();
  } else {
    pClient->disconnect();
    while (connected)
      delay(1);
    return false;
  }

  //Assign callback functions for the Characteristics
  if (!pRemoteSensorCharacteristic->canNotify() || !pRemoteBatteryCharacteristic-> canNotify()) {
    pClient->disconnect();
    while (connected)
      delay(1);
    return false;
  }

  if (!isConnectionComplete) {
    if (myDevices[int(sensorValue)] == "")
      myDevices[int(sensorValue)] = myDevice->getAddress().toString();
    else {
      Serial.println("2 sensors have the same location value");
      while (1) {
        digitalWrite(ONBOARD_LED, HIGH);
        delay(500);
        digitalWrite(ONBOARD_LED, LOW);
        delay(500);
      }
    }
  }

  //Subscribe and assign callbacks
  pRemoteSensorCharacteristic->subscribe(true, notifyCallback);
  pRemoteBatteryCharacteristic->subscribe(true, notifyCallback);

  return true;
}

void setup()
{
  Serial.begin(115200);

  BLEDevice::init("pIRfusiX Gateway");

  // Retrieve a Scanner and set the callback
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(0, false);

  pinMode(ONBOARD_LED, OUTPUT);
}

void loop() {
  if (doConnect) {
    if (!connectToServer(myDevice->getAddress().toString())) {
      brockenDevices[brockenDevicesCounter] = myDevice->getAddress().toString();
      brockenDevicesCounter++;
      //Visual of error
      for (int i = 0; i < 5; i++) {
        digitalWrite(ONBOARD_LED, HIGH);
        delay(200);
        digitalWrite(ONBOARD_LED, LOW);
        delay(200);
      }
    }
    //Algorithm to ensure brocken sensor does not affect moreThanOneSensor
    int tempCounter = 0;
    for (int i = 0; i < TOTAL_POSSIBLE_LOCATIONS; i++) {
      if (myDevices[i] != "")
        tempCounter++;
    }
    if (tempCounter == 1)
      moreThanOneSensor = 0;
    doConnect = false;
  }

  // Assuming that after 1 scan without filling the myDevices array, then amount of sensors < TOTAL_POSSIBLE_LOCATIONS
  if (connectionCounter > TOTAL_POSSIBLE_LOCATIONS && !isConnectionComplete)
  {
    isConnectionComplete = true;
    BLEDevice::getScan()->stop();
    // Connect to the first available sensor
    for (deviceIndex = 0; deviceIndex < TOTAL_POSSIBLE_LOCATIONS; deviceIndex++) {
      if (myDevices[deviceIndex] != "")
        break;
    }
    // Written here to minimize memory usage due to scoping (i.e. instead of in the for loop)
    connectToServer(myDevices[deviceIndex]);
    connectionCounter++;
  } else if (connectionCounter <= TOTAL_POSSIBLE_LOCATIONS) {
    connectionCounter++;
  }

  if (connected && isConnectionComplete) {
    if (moreThanOneSensor && iterationCounter > 20) {
      iterationCounter = 0;
      pClient->disconnect();
      while (connected)
        delay(1);
LOOP:
      do {
        if (deviceIndex < TOTAL_POSSIBLE_LOCATIONS - 1)
          deviceIndex++;
        else
          deviceIndex = 0;
      } while (myDevices[deviceIndex] == "");
      if (!connectToServer(myDevices[deviceIndex]))
        goto LOOP;
    }
  } else {
    if (connectionCounter > TOTAL_POSSIBLE_LOCATIONS + 1) {
      ESP.restart();
    }
    pClient->disconnect();
    while (connected)
      delay(1);
    BLEDevice::getScan()->start(1, false); // this is just to start scan after disconnect
  }
} // End of loop
