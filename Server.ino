#include <Arduino.h>
#include <ESP32Servo.h>
#include <BLEDevice.h>
#include <BLEServer.h>
#include <BLEUtils.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

// ================= PIN =================
#define SOIL_PIN   32

// ===== L298N PUMP =====
#define PUMP_IN3 14
#define PUMP_IN4 15
#define PUMP_EN  26

// ===== SERVO =====
#define SERVO_PIN  19

// ================= UART =================
#define RXD2 16
#define TXD2 17

// ================= THRESHOLD =================
#define SOIL_DRY_PERCENT 35

// ================= KALIBRASI =================
int soilDryValue = 3400;
int soilWetValue = 1400;

// ================= SERVO =================
Servo servoMotor;

// ================= STATUS =================
bool pompaON = false;
bool serverDry = false;
bool client1Dry = false;
bool client2Dry = false;

// ================= NILAI SOIL =================
int soilServer  = -1;
int soilClient1 = -1;
int soilClient2 = -1;

// ================= MODE =================
String pumpMode = "auto";
bool manualPump = false;

// ================= LCD =================
LiquidCrystal_I2C lcd(0x27, 16, 2);
unsigned long lastLcdMillis = 0;
int lcdMode = 0;

// ================= BLE UUID =================
#define SERVICE_UUID        "8aa0bbce-d7ec-46fc-b10d-18a79dfed8e1"
#define CHARACTERISTIC_UUID "600e00d9-412d-469c-b877-bea33b8fc6ff"

// ================= BLE CALLBACK =================
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer*) {
    BLEDevice::startAdvertising();
  }
  void onDisconnect(BLEServer*) {
    BLEDevice::startAdvertising();
  }
};

class MyCharCallbacks : public BLECharacteristicCallbacks {
  void onWrite(BLECharacteristic* c) {
    String d = c->getValue().c_str();

    if (d.startsWith("C1:")) {
      soilClient1 = d.substring(3).toInt();
      client1Dry = soilClient1 < SOIL_DRY_PERCENT;
    }

    if (d.startsWith("C2:")) {
      soilClient2 = d.substring(3).toInt();
      client2Dry = soilClient2 < SOIL_DRY_PERCENT;
    }
  }
};

// ================= SPRAY FSM =================
enum SprayState { IDLE, SERVO_MOVE, SPRAY };
SprayState sprayState = IDLE;

unsigned long sprayMillis = 0;
int targetAngle = 60;

#define SERVO_MOVE_TIME 800
#define SPRAY_TIME 3000

// ================= MANUAL LOOP =================
int manualIndex = 0;
unsigned long manualMillis = 0;
#define MANUAL_INTERVAL 4000
int manualAngles[3] = {60, 0, 90};

// ================= SOIL =================
int bacaSoil() {
  int raw = analogRead(SOIL_PIN);
  int persen = map(raw, soilDryValue, soilWetValue, 0, 100);
  return constrain(persen, 0, 100);
}

// ================= POMPA (L298N) =================
void pompaHidup() {
  if (!pompaON) {
    digitalWrite(PUMP_IN3, HIGH);
    digitalWrite(PUMP_IN4, LOW);
    digitalWrite(PUMP_EN, HIGH);
    pompaON = true;
  }
}

void pompaMati() {
  if (pompaON) {
    digitalWrite(PUMP_IN3, LOW);
    digitalWrite(PUMP_IN4, LOW);
    digitalWrite(PUMP_EN, LOW);
    pompaON = false;
  }
}

// ================= REQUEST SPRAY =================
void requestSpray(int angle) {
  if (sprayState != IDLE) return;

  targetAngle = angle;
  servoMotor.write(angle);
  pompaMati();

  sprayMillis = millis();
  sprayState = SERVO_MOVE;
}

// ================= HANDLE SPRAY =================
void handleSpray() {
  unsigned long now = millis();

  if (sprayState == SERVO_MOVE && now - sprayMillis >= SERVO_MOVE_TIME) {
    pompaHidup();
    sprayMillis = now;
    sprayState = SPRAY;
  }

  if (sprayState == SPRAY && now - sprayMillis >= SPRAY_TIME) {
    pompaMati();
    sprayState = IDLE;
  }
}

// ================= LCD =================
void updateLCD() {
  if (millis() - lastLcdMillis >= 5000) {
    lastLcdMillis = millis();
    lcdMode = (lcdMode + 1) % 3;
    lcd.clear();
  }

  lcd.setCursor(0, 0);
  if (lcdMode == 0) {
    lcd.print("Server");
    lcd.setCursor(0, 1);
    lcd.print(soilServer); lcd.print("%");
  } else if (lcdMode == 1) {
    lcd.print("Client 1");
    lcd.setCursor(0, 1);
    lcd.print(soilClient1); lcd.print("%");
  } else {
    lcd.print("Client 2");
    lcd.setCursor(0, 1);
    lcd.print(soilClient2); lcd.print("%");
  }
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  pinMode(PUMP_IN3, OUTPUT);
  pinMode(PUMP_IN4, OUTPUT);
  pinMode(PUMP_EN, OUTPUT);
  pompaMati();

  servoMotor.attach(SERVO_PIN);
  servoMotor.write(60);

  Wire.begin(21, 22);
  lcd.init();
  lcd.backlight();

  BLEDevice::init("ESP32_SERVER");
  BLEServer* server = BLEDevice::createServer();
  server->setCallbacks(new MyServerCallbacks());

  BLEService* service = server->createService(SERVICE_UUID);
  BLECharacteristic* ch = service->createCharacteristic(
    CHARACTERISTIC_UUID,
    BLECharacteristic::PROPERTY_WRITE |
    BLECharacteristic::PROPERTY_WRITE_NR
  );
  ch->setCallbacks(new MyCharCallbacks());

  service->start();
  BLEDevice::getAdvertising()->addServiceUUID(SERVICE_UUID);
  BLEDevice::startAdvertising();
}

// ================= LOOP =================
void loop() {

  soilServer = bacaSoil();
  serverDry = soilServer < SOIL_DRY_PERCENT;

  if (Serial2.available()) {
    String cmd = Serial2.readStringUntil('\n');
    if (cmd.startsWith("CMD:")) {
      int c = cmd.indexOf(',');
      pumpMode = cmd.substring(4, c);
      manualPump = cmd.substring(c + 1).toInt();
    }
  }

  if (pumpMode == "auto") {
    if (serverDry)  requestSpray(60);
    if (client1Dry) requestSpray(0);
    if (client2Dry) requestSpray(90);
  }

  if (pumpMode == "manual" && manualPump) {
    if (millis() - manualMillis > MANUAL_INTERVAL) {
      manualMillis = millis();
      requestSpray(manualAngles[manualIndex]);
      manualIndex = (manualIndex + 1) % 3;
    }
  }

  handleSpray();
  updateLCD();

  String tx =
    "S:" + String(soilServer) +
    ",C1:" + String(soilClient1) +
    ",C2:" + String(soilClient2) +
    ",P:" + String(pompaON);

  Serial2.println(tx);
  delay(20);
}
