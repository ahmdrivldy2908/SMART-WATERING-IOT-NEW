#include <Arduino.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <BLEClient.h>
#include <BLEAdvertisedDevice.h>

#define SERVICE_UUID        "8aa0bbce-d7ec-46fc-b10d-18a79dfed8e1"
#define CHARACTERISTIC_UUID "600e00d9-412d-469c-b877-bea33b8fc6ff"
#define SOIL_PIN 32

int soilDryValue = 3300;
int soilWetValue = 1400;

static BLEAddress* pServerAddress;
static bool doConnect = false;
static bool connected = false;

BLEClient* pClient;
BLERemoteCharacteristic* pRemoteCharacteristic;

int soilPercent = 0;
TaskHandle_t SoilTaskHandle = NULL;
TaskHandle_t BLETaskHandle = NULL;

void BacaSoil(void *parameter) {
  
  pinMode(SOIL_PIN, INPUT);

  for (;;) {

     int raw = analogRead(SOIL_PIN);
     Serial.println(raw);
     int persen = map(raw, soilDryValue, soilWetValue, 0, 100);
     
     soilPercent = constrain(persen, 0, 100);

     Serial.print("🌱 Soil Moisture: ");
     Serial.print(soilPercent);
     Serial.println(" %");

    vTaskDelay(pdMS_TO_TICKS(1000)); // sampling 1 detik
  }
}

void KirimBLE(void *parameter) {
  for (;;) {

    if (doConnect && !connected) {
      connectToServer(*pServerAddress);
      doConnect = false;
    }

    if (connected) {
      String data = "C2:" + String(soilPercent);
    
      pRemoteCharacteristic->writeValue(
        (uint8_t*)data.c_str(),
        data.length(),
        false
      );

      Serial.println("📤 BLE Task Send: " + data);
    }

    vTaskDelay(pdMS_TO_TICKS(2000)); // kirim tiap 2 detik
  }
}



bool connectToServer(BLEAddress pAddress) {
  pClient = BLEDevice::createClient();
  if (!pClient->connect(pAddress)) return false;

  BLERemoteService* pService = pClient->getService(SERVICE_UUID);
  if (!pService) return false;

  pRemoteCharacteristic = pService->getCharacteristic(CHARACTERISTIC_UUID);
  if (!pRemoteCharacteristic) return false;

  connected = true;
  Serial.println("🟢 Client 2 Connected");
  return true;
}

class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
  void onResult(BLEAdvertisedDevice device) {
    if (device.haveServiceUUID() &&
        device.isAdvertisingService(BLEUUID(SERVICE_UUID))) {
      BLEDevice::getScan()->stop();
      pServerAddress = new BLEAddress(device.getAddress());
      doConnect = true;
    }
  }
};

void setup() {
  Serial.begin(115200);
  BLEDevice::init("ESP32_CLIENT_2");

  BLEScan* pScan = BLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pScan->setActiveScan(true);
  pScan->start(5, false);

  xTaskCreatePinnedToCore(
    BacaSoil,          // Task function
    "SoilTask",        // Task name
    4096,              // Stack size
    NULL,              // Parameter
    1,                 // Priority
    &SoilTaskHandle,   // Task handle
    1                  // Core 1
  );

  xTaskCreatePinnedToCore(
    KirimBLE,
    "BLETask",
    6144,
    NULL,
    1,
    &BLETaskHandle,
    0
  );

}

void loop() {
   
}