# Designing IoT server Project2
본 프로젝트는 기숙사 세탁기 예약 서비스를 제공함으로써 기숙사 거주 학생들에게 편리한 세탁 경험을 제공하는 것을 목표로 합니다.

<a href= "https://github.com/AlwaysEden/IoT-Server-Design/blob/main/Doc/IoT%E1%84%89%E1%85%A5%E1%84%87%E1%85%A5%E1%84%89%E1%85%A5%E1%86%AF%E1%84%80%E1%85%A8%20Report.pdf">최종보고서 링크

## 환경설정
### 1. ESP8266개발환경
(1) 
- ESP8266 보드매니저
아두이노IDE->파일->환경설정->추가적인 보드매니저 URLs에 <br>
http://arduino.esp8266.com/stable/package_esp8266com_index.json 붙여넣기

- ESP32 보드매니저
Board Manager->esp32 다운 <br>

(2) 관련파일 <br>
툴->보드:”Arduino/GenuinoUno”-> 보드매니저-> esp검색-> esp8266 by ESP8266 Community 설치

(3) 보드 선택 <br>
툴->보드 -> NodeMCU 1.0(ESP-12E Module) 선택

(4) 포트 <br>
툴-> 포트-> 사용할 포트 선택

### 2. Arduino 라이브러리
```
#include "EspMQTTClient.h"
#include <ArduinoJson.h>
#include <WiFiManager.h>
```

### 3. User사용법
home assistatn application을 통해서 사용할 수 있음.

### 4. Developer사용법
1. Project2_WashingMechine.ino를 esp8266을 올려놓고, esp32-cam에는 cameraWebserver.ino를 올려놓아야합니다.
2. home assistant에는 해당 yaml파일 모두를 정상적으로 올려놔야합니다.

### 5. 디렉토리 구조
```
.
├── Doc/
│   ├── README.md
│   └── project2_report.doc
└── Design/
    ├── Project2_WashingMechine/
    │   └── Project2_WashingMechine.ino
    └── yaml/
        └── configuration.yaml
        └── automation.yaml
```
