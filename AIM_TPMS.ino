#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEAdvertisedDevice.h>
#include <CAN.h>

#define TX_GPIO_NUM  5
#define RX_GPIO_NUM  4

unsigned long previousMillis = 0;
int scanTime = 1; // In seconds
BLEScan* pBLEScan;
BLEUtils utils;

static BLEUUID serviceUUID("0000180d-0000-1000-8000-00805f9b34fb");  // Heart Rate Service
static BLEUUID charUUID("00002a37-0000-1000-8000-00805f9b34fb");  // Heart Rate Measurement Characteristic
static String frontPressureGauge = "38:8d:00:00:8f:00";  // Front pressure gauge address
static String rearPressureGauge = "38:8d:00:00:33:3b";  // Rear pressure gauge address

int heartRate = 0;
int fPress = 0;
int fTemp = 0;
int fBatt = 0;
int rPress = 0;
int rTemp = 0;
int rBatt = 0;

static boolean doConnect = false;
static boolean connected = false;
static BLERemoteCharacteristic *pRemoteCharacteristic;
static BLEAdvertisedDevice *myDevice;

char convertCharToHex(char ch) {
  if (ch >= '0' && ch <= '9') return ch - '0';
  if (ch >= 'a' && ch <= 'f') return ch - 'a' + 10;
  return 0;
}

String findCommonPrefix(String addr1, String addr2) {
  int minLength = min(addr1.length(), addr2.length());
  int i = 0;
  while (i < minLength && addr1[i] == addr2[i]) {
    i++;
  }
  return addr1.substring(0, i);
}

static String commonPrefix = findCommonPrefix(frontPressureGauge, rearPressureGauge);

static void notifyCallback(BLERemoteCharacteristic *pBLERemoteCharacteristic, uint8_t *pData, size_t length, bool isNotify) {
  heartRate = (pData[1]);
}

class MyClientCallback : public BLEClientCallbacks {
  void onConnect(BLEClient *pclient) {}
  void onDisconnect(BLEClient *pclient) {
    connected = false;
    Serial.println("onDisconnect");
    delay(500);
    ESP.restart();
  }
};

bool connectToServer() {
  Serial.print("Forming a connection to ");
  Serial.println(myDevice->getAddress().toString().c_str());

  BLEClient *pClient = BLEDevice::createClient();
  Serial.println(" - Created client");
  pClient->setClientCallbacks(new MyClientCallback());

  if (!pClient->connect(myDevice)) {
    Serial.println(" - Failed to connect to server");
    return false;
  }
  Serial.println(" - Connected to server");
  pClient->setMTU(517);

  BLERemoteService *pRemoteService = pClient->getService(serviceUUID);
  if (!pRemoteService) {
    Serial.println("Failed to find our service UUID");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our service");

  pRemoteCharacteristic = pRemoteService->getCharacteristic(charUUID);
  if (!pRemoteCharacteristic) {
    Serial.println("Failed to find our characteristic UUID");
    pClient->disconnect();
    return false;
  }
  Serial.println(" - Found our characteristic");

  if (pRemoteCharacteristic->canRead()) {
    Serial.print("The characteristic value was: ");
    Serial.println(pRemoteCharacteristic->readValue().c_str());
  }

  if (pRemoteCharacteristic->canNotify()) {
    pRemoteCharacteristic->registerForNotify(notifyCallback);
  }

  connected = true;
  return true;
}

class MyAdvertisedDeviceCallbacks: public BLEAdvertisedDeviceCallbacks {
    void onResult(BLEAdvertisedDevice advertisedDevice) {
      String addr = advertisedDevice.getAddress().toString().c_str();

      if (addr.startsWith(commonPrefix)) {
        char *pHex = utils.buildHexData(nullptr, (uint8_t*)advertisedDevice.getManufacturerData().c_str(), advertisedDevice.getManufacturerData().length());
        String sHex = (String) pHex;
        byte nib[16];
        sHex.getBytes(nib, 15);
        for (int i = 0; i < 14; i++) {
              nib[i] = convertCharToHex(nib[i]);
          }

        if (addr.equals(frontPressureGauge)) {
          fPress = ((nib[7]*256+nib[8]*16+nib[9])-145);
          fTemp = nib[4]*16+nib[5];
          fBatt = (nib[2]*16+nib[3]);
        }

        else if (addr.equals(rearPressureGauge)) {
          rPress = ((nib[7]*256+nib[8]*16+nib[9])-145);
          rTemp = nib[4]*16+nib[5];
          rBatt = (nib[2]*16+nib[3]);
      }
    }
    
    if (advertisedDevice.haveServiceUUID() && advertisedDevice.isAdvertisingService(serviceUUID)) {
      BLEDevice::getScan()->stop();
      myDevice = new BLEAdvertisedDevice(advertisedDevice);
      doConnect = true;
    }
    }
};

void setup() {
  Serial.begin(115200);
  delay(500);
    CAN.setPins (RX_GPIO_NUM, TX_GPIO_NUM);
  if (!CAN.begin(1000E3)) {
    Serial.println("Starting CAN failed!");
    while (1);
  }
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.println("Scanning...");

  BLEDevice::init("");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);
  pBLEScan->setInterval(100);
  pBLEScan->setWindow(99);
}

void loop() {
  pBLEScan->start(scanTime, false);
  pBLEScan->clearResults();
  if (doConnect) {
    if (connectToServer()) {
      Serial.println("We are now connected to the BLE Server.");
    } else {
      Serial.println("We have failed to connect to the server.");
    }
    doConnect = false;
  }

  unsigned long currentMillis = millis();
  if (currentMillis - previousMillis >= 300) {
    previousMillis = currentMillis;
    digitalWrite(LED_BUILTIN, LOW);
    Serial.print("currentMillis: "); Serial.println(currentMillis);
    Serial.print("HR : "); Serial.println(heartRate);
    Serial.print("fPress : "); Serial.println(fPress);
    Serial.print("fTemp : "); Serial.println(fTemp);
    Serial.print("fBatt : "); Serial.println(fBatt);
    Serial.print("rPress : "); Serial.println(rPress);
    Serial.print("rTemp : "); Serial.println(rTemp);
    Serial.print("rBatt : "); Serial.println(rBatt);

      /*
      0x5FE and 0xFF are the CANBUS ID for user data on the AIM ECU protocol. Ref: https://support.aimshop.com/pdf/racing-ecus/AiM/CAN.pdf
      The protocol expects 4 channels per id, denoted by 2 bytes per channel
      */

    CAN.beginExtendedPacket(0x5FF);
    CAN.write(fTemp);
    CAN.write(0);
    CAN.write(rTemp);
    CAN.write(0);
    CAN.write(heartRate);
    CAN.write(0);
    CAN.write(0);
    CAN.write(0);
    CAN.endPacket();

    CAN.beginExtendedPacket(0x5FE);
    CAN.write(fPress & 0xFF);
    CAN.write(fPress >> 8);
    CAN.write(rPress & 0xFF);
    CAN.write(rPress >> 8);
    CAN.write(fBatt);
    CAN.write(0);
    CAN.write(rBatt);
    CAN.write(0);
    CAN.endPacket();
  }
    digitalWrite(LED_BUILTIN, HIGH);
}
