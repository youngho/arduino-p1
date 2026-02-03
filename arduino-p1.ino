#include "RTC.h"
#include "Arduino_LED_Matrix.h"
#include "WiFiS3.h"

// arduino_secrets.h.example 를 arduino_secrets.h 로 복사 후 SSID/비밀번호 입력
#include "./arduino_secrets.h"

ArduinoLEDMatrix matrix;

// 표시 시간대: KST (GMT+9)
const int TIMEZONE_OFFSET_HOURS = 9;

// API 서버 (score-archery-api 헬스 체크)
const char SERVER[] = "158.179.161.203";
const uint16_t SERVER_PORT = 8080;
const char HEALTH_PATH[] = "/api/health";
const unsigned long HEALTH_CHECK_INTERVAL_MS = 10000;  // 10초마다
const unsigned long HEART_DISPLAY_MS = 2000;         // 성공 시 하트 2초 표시

byte Time[8][12] = {
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 },
  { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0}
};

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

byte  Digits  [5][30]{                                                                 
{ 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 0, 1, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 1, 0, 0, 1, 0, 0, 0, 0, 1, 1, 0, 1, 1, 0, 1 },
{ 1, 0, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
{ 1, 0, 1, 0, 0, 1, 1, 0, 0, 0, 0, 1, 0, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1, 1, 0, 1, 0, 0, 1 },
{ 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 1 },
};                                   


int currentSecond;
boolean secondsON_OFF = 1;
int hours, minutes, seconds, year, dayofMon;
String dayofWeek, month;

char ssid[] = SECRET_SSID;
char pass[] = SECRET_PASS;
unsigned long lastHealthCheck = 0;
unsigned long heartShowUntil = 0;   // 이 시간까지 하트 표시
WiFiClient client;

void displayDigit(int d, int s_x, int s_y){
  for (int i=0;i<3;i++)
    for (int j=0;j<5;j++)
      Time[i+s_x][11-j-s_y] = Digits[j][i+d*3];   
    
  matrix.renderBitmap(Time, 8, 12);
}


DayOfWeek convertDOW(String dow){
  if (dow == String("Mon")) return DayOfWeek::MONDAY;
  if (dow == String("Tue")) return DayOfWeek::TUESDAY;
  if (dow == String("Wed")) return DayOfWeek::WEDNESDAY;
  if (dow == String("Thu")) return DayOfWeek::THURSDAY;
  if (dow == String("Fri")) return DayOfWeek::FRIDAY;
  if (dow == String("Sat")) return DayOfWeek::SATURDAY;
  if (dow == String("Sun")) return DayOfWeek::SUNDAY;
}

Month convertMonth(String m){
 if (m == String("Jan")) return Month::JANUARY;
  if (m == String("Feb")) return Month::FEBRUARY;
  if (m == String("Mar")) return Month::MARCH;
  if (m == String("Apr")) return Month::APRIL;
  if (m == String("May")) return Month::MAY;
  if (m == String("Jun")) return Month::JUNE;
  if (m == String("Jul")) return Month::JULY;
  if (m == String("Aug")) return Month::AUGUST;
  if (m == String("Sep")) return Month::SEPTEMBER;
  if (m == String("Oct")) return Month::OCTOBER;
  if (m == String("Nov")) return Month::NOVEMBER;
  if (m == String("Dec")) return Month::DECEMBER;
}

void getCurTime(String timeSTR,String* d_w,int* d_mn, String* mn,int* h,int* m,int* s,int* y){
  
  *d_w = timeSTR.substring(0,3);
  *mn = timeSTR.substring(4,7);
  *d_mn = timeSTR.substring(8,11).toInt();
  *h = timeSTR.substring(11,13).toInt();
  *m = timeSTR.substring(14,16).toInt();
  *s = timeSTR.substring(17,19).toInt();
  *y = timeSTR.substring(20,24).toInt();

}

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
      if (line.indexOf("ok") >= 0) gotOk = true;
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
  RTC.begin();    
  // __TIMESTAMP__ 는 컴파일 시점(로컬) → KST로 간주하고 UTC로 변환해 RTC에 저장
  String timeStamp = __TIMESTAMP__;
  getCurTime(timeStamp,&dayofWeek,&dayofMon,&month,&hours,&minutes,&seconds,&year);
  int utcHour = (hours - TIMEZONE_OFFSET_HOURS + 24) % 24;
  int rtcDay = dayofMon;
  if (hours < TIMEZONE_OFFSET_HOURS) {
    rtcDay = (dayofMon > 1) ? dayofMon - 1 : 1;
  }
  RTCTime startTime(rtcDay, convertMonth(month), year, utcHour, minutes, seconds,
                    convertDOW(dayofWeek), SaveLight::SAVING_TIME_ACTIVE); 
  RTC.setTime(startTime);

  // WiFi (UNO R4 WiFi) - 실패해도 시계는 동작
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
      Serial.println("WiFi failed, clock only.");
    }
  }
}

void loop(){
  unsigned long now = millis();

  // 헬스 체크 주기 실행
  if (WiFi.status() == WL_CONNECTED && (now - lastHealthCheck >= HEALTH_CHECK_INTERVAL_MS)) {
    lastHealthCheck = now;
    if (doHealthCheck()) {
      heartShowUntil = now + HEART_DISPLAY_MS;
    }
  }

  // 하트 표시 중이면 메트릭스에 하트만 표시
  if (now < heartShowUntil) {
    displayHeart();
    if (now + 100 >= heartShowUntil) {
      currentSecond = -1;  // 다음 루프에서 시계 다시 그리기
    }
    return;
  }

  // 시계 표시 (RTC=UTC → GMT+9(KST)로 표시)
  RTCTime currentTime;
  RTC.getTime(currentTime);
  int kstHour = (currentTime.getHour() + TIMEZONE_OFFSET_HOURS) % 24;
  int kstMin  = currentTime.getMinutes();
  int kstSec  = currentTime.getSeconds();

  if (kstSec != currentSecond){
    secondsON_OFF ?  secondsON_OFF = 0 : secondsON_OFF = 1;
    displayDigit(kstHour / 10, 0, 0);
    displayDigit(kstHour % 10, 4, 0);
    displayDigit(kstMin / 10, 1, 6);
    displayDigit(kstMin % 10, 5, 6);
    Time[0][2]=secondsON_OFF;
    Time[0][4]=secondsON_OFF;
    currentSecond = kstSec;
    matrix.renderBitmap(Time, 8, 12);
    Serial.println(secondsON_OFF);
  }
}