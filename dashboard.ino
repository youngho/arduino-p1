#include <WiFiS3.h>

// WiFi 네트워크 설정
char ssid[] = "YOUR_WIFI_SSID";      // WiFi 네트워크 이름
char pass[] = "YOUR_WIFI_PASSWORD";   // WiFi 비밀번호
int status = WL_IDLE_STATUS;          // WiFi 상태

// HTTP 요청 설정
char server[] = "example.com";        // 테스트용 서버
WiFiClient client;

void setup() {
  Serial.begin(9600);
  
  // WiFi 모듈 초기화
  if (WiFi.status() == WL_NO_MODULE) {
    Serial.println("WiFi 모듈이 연결되지 않았습니다!");
    while (true); // 실행 중지
  }

  // WiFi 연결
  while (status != WL_CONNECTED) {
    Serial.print("WiFi 연결 시도 중...");
    status = WiFi.begin(ssid, pass);
    delay(10000);
  }
  
  Serial.println("\nWiFi 연결 성공!");
  Serial.print("IP 주소: ");
  Serial.println(WiFi.localIP());
}

void loop() {
  Serial.println("\n서버에 연결 시도 중...");
  
  if (client.connect(server, 80)) {
    Serial.println("서버 연결 성공!");
    
    // HTTP GET 요청 보내기
    client.println("GET / HTTP/1.1");
    client.println("Host: example.com");
    client.println("Connection: close");
    
    // 서버로부터 응답 받기
    while (client.connected()) {
      if (client.available()) {
        char c = client.read();
        Serial.print(c);
      }
    }
    
    client.stop();
  } else {
    Serial.println("서버 연결 실패!");
  }
  
  delay(5000); // 5초 대기
} 