#include "Arduino_LED_Matrix.h"
#include "WiFiS3.h"

// arduino_secrets.h.example 를 arduino_secrets.h 로 복사 후 SSID/비밀번호 입력
#include "./arduino_secrets.h"

ArduinoLEDMatrix matrix;

// API 서버 (score-archery-api 헬스 체크)
const char SERVER[] = "158.179.161.203";
const uint16_t SERVER_PORT = 8080;
const char HEALTH_PATH[] = "/score/api/health";
const unsigned long HEALTH_CHECK_INTERVAL_MS = 10000;  // 10초마다
const unsigned long HEART_DISPLAY_MS = 2000;           // 성공 시 하트 2초 표시

// 하트 비트맵 (8x12, 메트릭스에 헬스 OK 표시)
byte Heart[8][12] = {
  { 0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0 },
  { 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0 },
  { 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0 },
  { 0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0 }
};

byte Empty[8][12] = {0};

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
unsigned long lastHealthCheck = 0;
unsigned long heartShowUntil = 0;
WiFiClient client;

// 메트릭스에 하트 표시 (헬스 OK)
void displayHeart() {
  matrix.renderBitmap(Heart, 8, 12);
}

// GET /api/health 호출, 200 + "ok" 이면 true
bool doHealthCheck() {
  client.stop();
  client.setTimeout(5000);
  if (!client.connect(SERVER, SERVER_PORT)) {
    Serial.println("Health: connect failed");
    return false;
  }
  client.print("GET ");
  client.print(HEALTH_PATH);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.println(SERVER);
  client.println("Connection: close");
  client.println();

  unsigned long start = millis();
  bool got200 = false;
  bool gotOk = false;
  while (client.connected() && (millis() - start < 6000)) {
    while (client.available()) {
      String line = client.readStringUntil('\n');
      if (line.startsWith("HTTP/")) {
        if (line.indexOf("200") >= 0) got200 = true;
      }
      if (line.indexOf("ok") >= 0 || line.indexOf("OK") >= 0) gotOk = true;
    }
  }
  client.stop();
  bool ok = got200 && gotOk;
  Serial.println(ok ? "Health: OK" : "Health: fail");
  return ok;
}

void setup() {
  Serial.begin(9600);
  matrix.begin();

  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module not found");
  } else {
    int status = WL_IDLE_STATUS;
    for (int tryCount = 0; tryCount < 3 && status != WL_CONNECTED; tryCount++) {
      Serial.print("WiFi connecting to ");
      Serial.println(ssid);
      status = WiFi.begin(ssid, pass);
      delay(10000);
    }
    if (status == WL_CONNECTED) {
      Serial.print("WiFi OK, IP: ");
      Serial.println(WiFi.localIP());
    } else {
      Serial.println("WiFi failed.");
    }
  }
}

void loop() {
  unsigned long now = millis();

  // 헬스 체크 주기 실행
  if (WiFi.status() == WL_CONNECTED && (now - lastHealthCheck >= HEALTH_CHECK_INTERVAL_MS)) {
    lastHealthCheck = now;
    if (doHealthCheck()) {
      heartShowUntil = now + HEART_DISPLAY_MS;
    }
  }

  // 하트 표시 중이면 메트릭스에 하트, 아니면 빈 화면
  if (now < heartShowUntil) {
    displayHeart();
  } else {
    matrix.renderBitmap(Empty, 8, 12);
  }
}
