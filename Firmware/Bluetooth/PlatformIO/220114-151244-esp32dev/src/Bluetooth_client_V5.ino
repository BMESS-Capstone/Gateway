/**
  BLE Client for the pIRfusiX sensor system Version 4
  Note: uses NimBLE instead of orginal BLEDevice (less resource intensive)

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

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  if (isConnectionComplete)
  {
    if (pBLERemoteCharacteristic->getUUID().toString() == sensorCharacteristicUUID)
    {
      sensorValue = *(float *)pData;
      Serial.println(sensorValue);
      if (moreThanOneSensor)
      {
        iterationCounter++;
      }
    }
    else
    {
      batteryValue = *(int *)pData;
      Serial.println(batteryValue);
    }
  }
}

//Callback function for the BLE client (i.e. this device)
class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
    connected = true;
    digitalWrite(ONBOARD_LED, HIGH);
  }

  void onDisconnect(BLEClient *pclient)
  {
    connected = false;
    digitalWrite(ONBOARD_LED, LOW);
    Serial.println("onDisconnect");
  }
};

/**
   Scan for BLE servers and find the first one that advertises the service we are looking for.
*/
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks
{
  /**
        Called for each advertising BLE server.
    */

  /*** Only a reference to the advertised device is passed now
      void onResult(BLEAdvertisedDevice advertisedDevice) { **/
  void onResult(BLEAdvertisedDevice *advertisedDevice)
  {
    // Serial.print("BLE Advertised Device found: ");
    // Serial.println(advertisedDevice->toString().c_str());

    std::string *address = std::find(std::begin(myDevices), std::end(myDevices), advertisedDevice->getAddress().toString());
    if (address == std::end(myDevices))
    {
      // We have found a device, let us now see if it contains the service we are looking for.
      /********************************************************************************
          if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      ********************************************************************************/
      if (advertisedDevice->haveServiceUUID() && advertisedDevice->getServiceUUID().toString() == serviceUUID)
      {
        if (connectionCounter > 0)
        {
          Serial.println("There is more than one sensor");
          moreThanOneSensor = true;
        }
        BLEDevice::getScan()->stop();
        /*******************************************************************
              myDevice = new BLEAdvertisedDevice(advertisedDevice);
        *******************************************************************/

        myDevice = advertisedDevice; /** Just save the reference now, no need to copy the object */
        doConnect = true;
      } // Found our server
    }   // onResult
  }
}; // MyAdvertisedDeviceCallbacks

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer(std::string device)
{
  Serial.print("Forming a connection to ");
  Serial.println(device.c_str());

  pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  Serial.println(" - Created client");

  // Connect to the remove BLE Server.
  if (!pClient->connect(device))
  {
    Serial.print("Failed to connect to device: ");
    Serial.println(device.c_str());
    return false;
  }
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID);
    return false;
  }
  Serial.println(" - Found our service");

  // Obtain a reference to the characteristics in the service of the remote BLE server.
  pRemoteSensorCharacteristic = pRemoteService->getCharacteristic(sensorCharacteristicUUID);
  if (pRemoteSensorCharacteristic == nullptr)
  {
    Serial.println("Failed to find our sensor characteristic UUID");
    return false;
  }
  pRemoteBatteryCharacteristic = pRemoteService->getCharacteristic(batteryCharacteristicUUID);
  if (pRemoteBatteryCharacteristic == nullptr)
  {
    Serial.println("Failed to find our battery characteristic UUID");
    return false;
  }
  Serial.println(" - Found our characteristics");

  // Read the value of the sensor characteristic.
  if (pRemoteSensorCharacteristic->canRead())
  {
    // Note timestamp from documentation: readValue(time_t *timestamp = nullptr);
    sensorValue = pRemoteSensorCharacteristic->readValue<float>();
    Serial.print("The sensor characteristic value was: ");
    Serial.println(sensorValue);

    if (!isConnectionComplete)
    {
      //TODO: Consider adding an algorithm to check if 2 sensors share the same location value
      myDevices[int(sensorValue)] = myDevice->getAddress().toString();
    }
  }

  // Read the value of the battery characteristic.
  if (pRemoteBatteryCharacteristic->canRead())
  {
    // TODO: Note timestamp from documentation: readValue(time_t *timestamp = nullptr);
    batteryValue = pRemoteBatteryCharacteristic->readValue<uint16_t>();
    Serial.print("The battery characteristic value was: ");
    Serial.println(batteryValue);
  }

  //Assign callback functions for the Characteristics
  if (pRemoteSensorCharacteristic->canNotify() || pRemoteBatteryCharacteristic->canNotify())
  {
    if (!pRemoteSensorCharacteristic->subscribe(true, notifyCallback))
    {
      /** Disconnect if subscribe failed */
      Serial.println("Sensor characteristic subscription failed");
      pClient->disconnect();
    }
    if (!pRemoteBatteryCharacteristic->subscribe(true, notifyCallback))
    {
      /** Disconnect if subscribe failed */
      Serial.println("Battery characteristic subscription failed");
      pClient->disconnect();
    }
  }
  return true;
}

void setup()
{
  Serial.begin(115200);

  Serial.println("Starting Arduino BLE Client application...");

  BLEDevice::init("pIRfusiX Gateway");

  // Retrieve a Scanner and set the callback we want to use to be informed when we
  // have detected a new device.  Specify that we want active scanning and start the
  // scan to run forever.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->start(0, false);

  pinMode(ONBOARD_LED, OUTPUT);
}

void loop()
{
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true)
  {
    if (connectToServer(myDevice->getAddress().toString()))
    {
      Serial.println("We are now connected to the BLE Server.");
    }
    else
    {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  // Assuming that after 1 scan without filling the myDevices array, then amount of sensors < TOTAL_POSSIBLE_LOCATIONS
  if (connectionCounter > TOTAL_POSSIBLE_LOCATIONS && !isConnectionComplete)
  {
    isConnectionComplete = true;
    BLEDevice::getScan()->stop();
    // Connect to the first available sensor
    for (deviceIndex = 0; deviceIndex < TOTAL_POSSIBLE_LOCATIONS; deviceIndex++)
    {
      if (myDevices[deviceIndex] != "")
      {
        break;
      }
    }
    // Written here to minimize memory usage due to scoping (i.e. instead of in the for loop)
    connectToServer(myDevices[deviceIndex]);
    connectionCounter++;
  }
  else if (connectionCounter <= TOTAL_POSSIBLE_LOCATIONS)
  {
    connectionCounter++;
  }

  if (connected && isConnectionComplete)
  {
    if (moreThanOneSensor && iterationCounter > 20)
    {
      iterationCounter = 0;
      pClient->disconnect();
      do
      {
        if (deviceIndex < TOTAL_POSSIBLE_LOCATIONS - 1)
          deviceIndex++;
        else
          deviceIndex = 0;
      } while (myDevices[deviceIndex] == "");
      connectToServer(myDevices[deviceIndex]);
    }
  }
  else
  {
    if (connectionCounter > TOTAL_POSSIBLE_LOCATIONS + 1)
    {
      ESP.restart();
    }
    pClient->disconnect();
    BLEDevice::getScan()->start(1, false); // this is just to start scan after disconnect
  }
  // delay(1000); // Delay a second between loops (does not affect callbacks - proably runs on the second core)
} // End of loop