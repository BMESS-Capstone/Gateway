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
static char serviceUUID[] = CONNECT_UUID;

// BLE Client
static BLEClient *pClient;

// Connection Characteristic
static char sensorCharacteristicUUID[] = SENSOR_CHAR_UUID;
static char batteryCharacteristicUUID[] = BATTERY_CHAR_UUID;

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;
static boolean isConnectionComplete = false;
static boolean moreThanOneSensor = false;
static uint8_t connectionCounter;
static uint8_t iterationCounter;
static uint8_t deviceIndex;

//Advertised device of the peripheral device. Address will be found during scanning...
static BLEAdvertisedDevice *myDevice;
static std::string myDevices[TOTAL_POSSIBLE_LOCATIONS];

//Characteristic that we want to read
static BLERemoteCharacteristic *pRemoteSensorCharacteristic;
static BLERemoteCharacteristic *pRemoteBatteryCharacteristic;

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
      std::string *address = std::find(std::begin(myDevices), std::end(myDevices), advertisedDevice->getAddress().toString());
      if (address == std::end(myDevices)) {
        if (advertisedDevice->haveServiceUUID() && advertisedDevice->getServiceUUID().toString() == serviceUUID) {
          if (connectionCounter > 0)
            moreThanOneSensor = true;
          BLEDevice::getScan()->stop();
          myDevice = advertisedDevice; /** Just save the reference now, no need to copy the object */
          doConnect = true;
        } // Found our server
      }
    }// onResult
}; // MyAdvertisedDeviceCallbacks

//Connect to the BLE Server that has the name, Service, and Characteristics
void connectToServer(std::string device) {
  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());

  // Connect to the remote BLE Server.
  pClient->connect(device);

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);

  // Obtain a reference to the characteristics in the service of the remote BLE server.
  pRemoteSensorCharacteristic = pRemoteService->getCharacteristic(sensorCharacteristicUUID);
  pRemoteBatteryCharacteristic = pRemoteService->getCharacteristic(batteryCharacteristicUUID);

  // Read the value of the sensor characteristic.
  sensorValue = pRemoteSensorCharacteristic->readValue<float>();

  if (!isConnectionComplete) {
    if (myDevices[int(sensorValue)] == "")
      myDevices[int(sensorValue)] = myDevice->getAddress().toString();
    else {
      Serial.println("2 sensors have the same location value");
      while (1) {
        digitalWrite(ONBOARD_LED, HIGH);
        delay(300);
        digitalWrite(ONBOARD_LED, LOW);
        delay(300);
      }
    }
  }

  // Read the value of the battery characteristic.
  batteryValue = pRemoteBatteryCharacteristic->readValue<uint16_t>();

  //Assign callback functions for the Characteristics
  pRemoteSensorCharacteristic->subscribe(true, notifyCallback);
  pRemoteBatteryCharacteristic->subscribe(true, notifyCallback);
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
    connectToServer(myDevice->getAddress().toString());
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
      do {
        if (deviceIndex < TOTAL_POSSIBLE_LOCATIONS - 1)
          deviceIndex++;
        else
          deviceIndex = 0;
      } while (myDevices[deviceIndex] == "");
      connectToServer(myDevices[deviceIndex]);
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
