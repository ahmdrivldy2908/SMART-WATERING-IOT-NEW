#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <time.h>

// ================= UART =================
#define RXD2 16
#define TXD2 17

// ================= WIFI =================
#define WIFI_SSID "Abay"
#define WIFI_PASSWORD "123456789"

// ================= FIREBASE =================
#define API_KEY "AIzaSyDgXe75GNmv5bNRoBSJxfSFnh5G5D7_H8I"
#define DATABASE_URL "https://smart-watering-iot-a6306-default-rtdb.firebaseio.com/"

// ================= FIREBASE OBJECT =================
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

String lastCmd = "";

// ================= TIME =================
unsigned long getTimestamp() {
  time_t now;
  time(&now);
  return (unsigned long) now;
}

// ================= SETUP =================
void setup() {
  Serial.begin(115200);
  Serial2.begin(9600, SERIAL_8N1, RXD2, TXD2);

  // ===== WIFI =====
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
  }

  // ===== TIME (NTP) =====
  configTime(0, 0, "pool.ntp.org", "time.nist.gov");

  // ===== FIREBASE =====
  config.api_key = API_KEY;
  config.database_url = DATABASE_URL;

  Firebase.signUp(&config, &auth, "", "");
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);
}

// ================= LOOP =================
void loop() {

  // ===== TERIMA DATA DARI ESP A =====
  if (Serial2.available()) {
    String d = Serial2.readStringUntil('\n');

    int s, c1, c2, p;
    if (sscanf(d.c_str(), "S:%d,C1:%d,C2:%d,P:%d", &s, &c1, &c2, &p) == 4) {

      // ===== 1. UPDATE STATE (REALTIME UI) =====
      Firebase.RTDB.setInt(&fbdo, "/soil/server", s);
      Firebase.RTDB.setInt(&fbdo, "/soil/client1", c1);
      Firebase.RTDB.setInt(&fbdo, "/soil/client2", c2);
      Firebase.RTDB.setBool(&fbdo, "/pump/state", p);

      // ===== 2. LOG DATA (HISTORI UNTUK AI) =====
      FirebaseJson log;
      log.set("timestamp", getTimestamp());
      log.set("soil_server", s);
      log.set("soil_client1", c1);
      log.set("soil_client2", c2);
      log.set("pump", p);

      Firebase.RTDB.pushJSON(&fbdo, "/logs", &log);
    }
  }

  // ===== AMBIL COMMAND DARI FIREBASE (FLUTTER) =====
  String mode;
  bool manual;

  if (Firebase.RTDB.getString(&fbdo, "/pump/mode")) {
    mode = fbdo.stringData();
  }

  if (Firebase.RTDB.getBool(&fbdo, "/pump/manual_state")) {
    manual = fbdo.boolData();
  }

  String cmd = "CMD:" + mode + "," + String(manual);

  if (cmd != lastCmd) {
    Serial2.println(cmd);
    lastCmd = cmd;
  }

  delay(50);
}
