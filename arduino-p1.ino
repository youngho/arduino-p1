/*
 * Arduino P1: API 헬스 체크 → LED 매트릭스에 하트(성공) / X(실패) 표시
 * 로직 구성: 설정 → 표시 → 네트워크 → setup/loop
 */

// ---- includes ----
#include "./arduino_secrets.h" // arduino_secrets.h.example 복사 후 SSID/비밀번호 입력
#include "Arduino_LED_Matrix.h"
#include "WiFiS3.h"

ArduinoLEDMatrix matrix;
WiFiSSLClient client;

// ---- 설정 (API·주기·표시 시간) ----
const char SERVER[] = "pinksoft.io";
const uint16_t SERVER_PORT = 8080;
const char HEALTH_PATH[] = "/score/api/health";
const unsigned long HEALTH_CHECK_INTERVAL_MS = 10000; // 10초마다
const unsigned long HEART_DISPLAY_MS = 2000;          // 성공 시 하트 2초
const unsigned long FAIL_DISPLAY_MS = 2000;           // 실패 시 X 2초

// ---- 비트맵 (8x12) ----
byte Heart[8][12] = {
    {0, 0, 0, 1, 1, 0, 0, 1, 1, 0, 0, 0}, {0, 0, 1, 1, 1, 1, 0, 1, 1, 1, 1, 0},
    {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0}, {0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0},
    {0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0}, {0, 0, 0, 1, 1, 1, 1, 1, 0, 0, 0, 0},
    {0, 0, 0, 0, 1, 1, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 0, 0, 0, 0, 0, 0}};

byte Cross[8][12] = {
    {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0}, {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0},
    {0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0}, {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0},
    {0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 0, 0}, {0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 0, 0},
    {0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0}, {0, 0, 1, 0, 0, 0, 0, 0, 0, 1, 0, 0}};

byte Empty[8][12] = {0};

// ---- 상태 (헬스 체크·표시) ----
char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
unsigned long lastHealthCheck = 0; // 마지막 헬스 체크 시각
unsigned long resultShowUntil = 0; // 하트/X 표시 종료 시각
bool lastHealthOk = false;         // 마지막 헬스 결과 (true=하트, false=X)

// ---- 표시 로직 ----
void displayHeart() { matrix.renderBitmap(Heart, 8, 12); }

void displayCross() { matrix.renderBitmap(Cross, 8, 12); }

void displayEmpty() { matrix.renderBitmap(Empty, 8, 12); }

/** 현재 시각 기준으로 매트릭스에 표시할 내용 갱신 */
void updateDisplay(unsigned long now) {
  if (now < resultShowUntil) {
    if (lastHealthOk) {
      displayHeart();
    } else {
      displayCross();
    }
  } else {
    displayEmpty();
  }
}

// ---- 네트워크 로직 ----
/** WiFi 연결 시도 (최대 3회), 성공 시 true */
bool connectWiFi() {
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi module not found");
    return false;
  }
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
    return true;
  }
  Serial.println("WiFi failed.");
  return false;
}

// ---- 고유 시리얼 번호 (UID) ----
/** 보드의 고유 시리얼 번호(Unique ID) 문자열 반환 */
String getDeviceUid() {
#if defined(ARDUINO_ARCH_RENESAS) || defined(ARDUINO_UNOR4_WIFI) ||            \
    defined(ARDUINO_ARCH_RENESAS_UNO) || defined(RENESAS_RA4M1)
  bsp_unique_id_t const *p_uid = R_BSP_UniqueIdGet();
  char uidBuf[33];
  snprintf(uidBuf, sizeof(uidBuf),
           "%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X",
           p_uid->unique_id_bytes[0], p_uid->unique_id_bytes[1],
           p_uid->unique_id_bytes[2], p_uid->unique_id_bytes[3],
           p_uid->unique_id_bytes[4], p_uid->unique_id_bytes[5],
           p_uid->unique_id_bytes[6], p_uid->unique_id_bytes[7],
           p_uid->unique_id_bytes[8], p_uid->unique_id_bytes[9],
           p_uid->unique_id_bytes[10], p_uid->unique_id_bytes[11],
           p_uid->unique_id_bytes[12], p_uid->unique_id_bytes[13],
           p_uid->unique_id_bytes[14], p_uid->unique_id_bytes[15]);
  return String(uidBuf);
#elif defined(ESP32)
  uint64_t chipid = ESP.getEfuseMac();
  char chipBuf[13];
  snprintf(chipBuf, sizeof(chipBuf), "%04X%08X", (uint16_t)(chipid >> 32),
           (uint32_t)chipid);
  return String(chipBuf);
#else
  byte mac[6];
  WiFi.macAddress(mac);
  char macBuf[18];
  snprintf(macBuf, sizeof(macBuf), "%02X%02X%02X%02X%02X%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);
  return String(macBuf);
#endif
}

/** 보드의 고유 시리얼 번호(Unique ID)를 시리얼 모니터에 출력 */
void printSerialNumber() {
  Serial.print("Arduino Serial Number (UID): ");
  Serial.println(getDeviceUid());
}

/** GET /score/api/health?uid={UID} 호출, 200 + "ok" 이면 true */
bool doHealthCheck() {
  client.stop();
  client.setTimeout(5000);
  if (!client.connect(SERVER, SERVER_PORT)) {
    Serial.println("Health: connect failed");
    return false;
  }

  String pathWithUid = String(HEALTH_PATH) + "?uid=" + getDeviceUid();

  client.print("GET ");
  client.print(pathWithUid);
  client.println(" HTTP/1.1");
  client.print("Host: ");
  client.print(SERVER);
  client.print(":");
  client.println(SERVER_PORT);
  client.println("Accept: */*");
  client.println("Connection: close");
  client.println();

  unsigned long start = millis();
  bool got200 = false;
  bool gotOk = false;
  while (client.connected() && (millis() - start < 6000)) {
    while (client.available()) {
      String line = client.readStringUntil('\n');
      if (line.startsWith("HTTP/") && line.indexOf("200") >= 0)
        got200 = true;
      if (line.indexOf("ok") >= 0 || line.indexOf("OK") >= 0)
        gotOk = true;
    }
  }
  client.stop();
  bool ok = got200 && gotOk;
  Serial.println(ok ? "Health: OK" : "Health: fail (unauthorized or error)");
  return ok;
}

// ---- setup / loop ----
void setup() {
  Serial.begin(9600);
  delay(1500); // USB 시리얼 포트 연결 및 모니터 인식 대기
  matrix.begin();
  printSerialNumber();
  connectWiFi();
}

void loop() {
  unsigned long now = millis();
  runScheduledHealthCheck(now);
  updateDisplay(millis());
}
