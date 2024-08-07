# RaspberryPi
:fire: 졸업작품 라즈베리 파이 구현 :fire:





**0. 라즈베리파이4**


(1) 폴더 생성 : mkdir "폴더 이름"

(2) C 파일 생성 : nano "파일".c
- 파일 저장 : cltrl+s 후 cltrl+x 또는 cltrl+x 후 y + Enter

(3) C 파일 컴파일 -> gcc 이용
  - gcc install : sudo apt-get install gcc
  - gcc compile : gcc -o "실행 파일 명" "파일".c 
    - 컴파일 후 실행 파일이 생성됨

- 컴파일 옵션
  - mysql : -I/usr/include/mysql -L/usr/lib -Imysqlclient
  - wiringPi : -lwriringPi -lwiringPiDev
  - 스레드 : -lpthread

(4) 파일 실행 : ./"실행 파일명"

(5) 기타
- #include <mysql.h> 오류 시 : sudo apt-get install default-libmysqlclient-dev
- 라즈베리파이 시간 설정
  - $date로 타임존 확인
  - $sudo raspi-config
  - 'Raspberry Pi Software Configuration Tool'에서 'Localisation Options' + Enter
  - 'Change Timezone' + Enter
  - 지역 'Asia' + Enter
  - TimeZone 'Seoul' + Enter 후 Finish
또는
  - $sudo nano /etc/timezone
  - 'Asia/Seoul' 기록 후 저장

- 라즈베리파이 핫스팟 사용 설정
  - $sudo raspi-config
  - 'Change Wi-fi Country'에서 지역을 'US'로 변경



**1. LED**


(1) 구성
- LED 12개(array로 구성)
- 저항 110 ohm
- 점퍼선


(2) PIN 구성

|라즈베리파이|LED|
|:---:|:---:|
|40번(GPIO21)|1|
|38번(GPIO20)|2|
|36번(GPIO16)|3|
|32번(GPIO12)|4|
|26번(GPIO7)|5|
|24번(GPIO8)|6|
|37번(GPIO26)|7|
|35번(GPIO19)|8|
|33번(GPIO13)|9|
|31번(GPIO6)|10|


(3) 동작
- 예약되었을 경우, LED ON
- 예약되지 않았을 경우, LED OFF
- 정류장 하차 1분 전일 경우, LED 1분간 깜빡임


**2. LCD**


(1) 구성
- 라즈베리파이 4
- 가변 저항 10K ohm
- 저항 560M ohm
- Text LCD 1602
- 점퍼선



(2) PIN 구성

|*라즈베리파이*|*LCD*|
|:---:|:---:|
|6번(GND)|ground|
|2번(+5V)|5V|
|26번(GPIO7)|4번(RS)|
|24번(GPIO8)|6번(E)|
|11번(GPIO17)|11번(D4)|
|12번(GPIO18)|12번(D5)|
|13번(GPIO27)|13번(D6)|
|15번(GPIO22)|14번(D7)|


(3) 동작
- 예약되었을 경우, "X시 X분에 예약된 좌석입니다."
- 예약된 시간이 넘었을 경우, "현재 예약된 좌석입니다."
- 예약되지 않았을 경우, "빈 좌석입니다."
- 정류장 하차 1분 전일 경우, 1분동안 깜빡이면서 "하차 예정입니다."


**3. 자이로 센서**



(1) 구성
- 라즈베리파이 4
- MPU 6050
- 점퍼선


(2) PIN 구성
|*라즈베리파이*|*Sensor*|
|:---:|:---:|
|5번(GPIO3(SCL1))|SCL|
|3번번(GPIO2(SDA1))|SDA|
|6번(GND)|GND|
|1번(3.3V)|VCC|


(3) 동작
- Ax 값이 -1일 경우 빈좌석임을 나타낸다. 
- Ax 값이 0일 경우 좌석 이용 중임을 나타낸다.
