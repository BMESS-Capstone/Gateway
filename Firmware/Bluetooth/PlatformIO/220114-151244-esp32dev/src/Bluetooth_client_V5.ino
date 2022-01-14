/**
  BLE Client for the pIRfusiX sensor system Version 4
  Note: uses NimBLE instead of orginal BLEDevice (less resource intensive)

  Author: Khaled Elmalawany
*/

#include <NimBLEDevice.h>

//*****Shared with NANO 33 TODO: move to a common refererred .h file***
#define BUFFER_SIZE 40

#define TOTAL_POSSIBLE_LOCATIONS 4
#define LEFT_ARM 0
#define RIGHT_ARM 1
#define LEFT_LEG 2
#define RIGHT_LEG 3

#define CONNECT_UUID "6164e702-7565-11ec-90d6-0242ac120003"

#define SENSOR_CHAR_UUID "fec40b26-757a-11ec-90d6-0242ac120003"
#define BATTERY_CHAR_UUID "fec40dc4-757a-11ec-90d6-0242ac120003"
//********************************************************************

#define ONBOARD_LED 2

/* UUID's of the service, characteristic that we want to read and/or write */
// BLE Services
static char serviceUUID[] = CONNECT_UUID;

// Connection Characteristic
static char sensorCharacteristicUUID[] = SENSOR_CHAR_UUID;
static char batteryCharacteristicUUID[] = BATTERY_CHAR_UUID;

//Flags stating if should begin connecting and if the connection is up
static boolean doConnect = false;
static boolean connected = false;

//Advertised device of the peripheral device. Address will be found during scanning...
static BLEAdvertisedDevice *myDevice[TOTAL_POSSIBLE_LOCATIONS];

//Characteristic that we want to read
static BLERemoteCharacteristic *pRemoteSensorCharacteristic;
static BLERemoteCharacteristic *pRemoteBatteryCharacteristic;

//Variable to store characteristic value
float value;

static void notifyCallback(
    BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify)
{
  Serial.print("Notify callback for characteristic ");
  Serial.print(pBLERemoteCharacteristic->getUUID().toString().c_str());
  Serial.print(" of data length ");
  Serial.println(length);
  Serial.print("data: ");
  Serial.println((char *)pData);
}

//Callback function for the BLE client (i.e. this device)
class MyClientCallback : public BLEClientCallbacks
{
  void onConnect(BLEClient *pclient)
  {
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

    // We have found a device, let us now see if it contains the service we are looking for.
    /********************************************************************************
          if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      ********************************************************************************/
    if (advertisedDevice->haveServiceUUID() && advertisedDevice->getServiceUUID().toString() == serviceUUID)
    {

      BLEDevice::getScan()->stop();
      /*******************************************************************
              myDevice = new BLEAdvertisedDevice(advertisedDevice);
        *******************************************************************/
      myDevice[0] = advertisedDevice; /** Just save the reference now, no need to copy the object */
      doConnect = true;

    } // Found our server
  }   // onResult
};    // MyAdvertisedDeviceCallbacks

//Connect to the BLE Server that has the name, Service, and Characteristics
bool connectToServer()
{
  Serial.print("Forming a connection to ");
  Serial.println(myDevice[0]->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  pClient->setClientCallbacks(new MyClientCallback());
  Serial.println(" - Created client");

  // Connect to the remove BLE Server.
  pClient->connect(myDevice[0]);
  Serial.println(" - Connected to server");

  // Obtain a reference to the service we are after in the remote BLE server.
  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (pRemoteService == nullptr)
  {
    Serial.print("Failed to find our service UUID: ");
    Serial.println(serviceUUID);
    return (false);
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
    value = atof(pRemoteSensorCharacteristic->readValue().c_str());
    Serial.print("The characteristic value was: ");
    Serial.println(value);
  }

  // Read the value of the battery characteristic.
  if (pRemoteBatteryCharacteristic->canRead())
  {
    // Note timestamp from documentation: readValue(time_t *timestamp = nullptr);
    value = atof(pRemoteBatteryCharacteristic->readValue().c_str());
    Serial.print("The characteristic value was: ");
    Serial.println(value);
  }

  //Assign callback functions for the Characteristics
  if (pRemoteSensorCharacteristic->canNotify() || pRemoteBatteryCharacteristic->canNotify())
  {
    Serial.println("Characteristic(s) can notify");
    if (!pRemoteSensorCharacteristic->subscribe(true, notifyCallback))
    {
      /** Disconnect if subscribe failed */
      Serial.println("Sensor characteristic subscription failed");
      pClient->disconnect();
      return false;
    }
    if (!pRemoteBatteryCharacteristic->subscribe(true, notifyCallback))
    {
      /** Disconnect if subscribe failed */
      Serial.println("Battery characteristic subscription failed");
      pClient->disconnect();
      return false;
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
  // scan to run for 15 seconds.
  BLEScan *pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(1349);
  pBLEScan->setWindow(449);
  pBLEScan->start(0);

  pinMode(ONBOARD_LED, OUTPUT);
}

void loop()
{
  
  // If the flag "doConnect" is true then we have scanned for and found the desired
  // BLE Server with which we wish to connect.  Now we connect to it.  Once we are
  // connected we set the connected flag to be true.
  if (doConnect == true)
  {
    if (connectToServer())
    {
      Serial.println("We are now connected to the BLE Server.");
    }
    else
    {
      Serial.println("We have failed to connect to the server; there is nothin more we will do.");
    }
    doConnect = false;
  }

  if (connected)
  {
    // Set the characteristic's value to be the array of bytes that is actually a string.
    /*** Note: write / read value now returns true if successful, false otherwise - try again or disconnect ***/
  }
  else
  {
    BLEDevice::getScan()->start(0); // this is just eample to start scan after disconnect, most likely there is better way to do it in arduino
  }

  delay(1000); // Delay a second between loops.
} // End of loop
