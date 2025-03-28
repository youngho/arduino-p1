# 아두이노 R4 네트워크 예제

이 프로젝트는 아두이노 R4의 네트워크 기능을 보여주는 예제입니다.

## 준비사항

1. 아두이노 R4 보드
2. WiFi 네트워크 접속 정보 (SSID와 비밀번호)
3. 아두이노 IDE

## 설치 방법

1. 아두이노 IDE를 설치합니다.
2. WiFiS3 라이브러리를 설치합니다:
   - 아두이노 IDE의 라이브러리 매니저를 엽니다
   - "WiFiS3"를 검색하여 설치합니다

## 사용 방법

1. `network_example.ino` 파일을 엽니다
2. WiFi 설정을 수정합니다:
   ```cpp
   char ssid[] = "YOUR_WIFI_SSID";      // WiFi 네트워크 이름
   char pass[] = "YOUR_WIFI_PASSWORD";   // WiFi 비밀번호
   ```
3. 코드를 아두이노 R4에 업로드합니다
4. 시리얼 모니터를 열어 연결 상태를 확인합니다

## 기능

- WiFi 네트워크 연결
- HTTP 클라이언트 기능
- 서버로부터 데이터 수신
