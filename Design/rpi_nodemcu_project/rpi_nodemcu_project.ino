/*** Import Libraries  ***/
// DHT sensor library for ESPx by beegee_tokyo
#include "DHTesp.h"

// EspMQTTClient by Patrck Lapointe
#include "EspMQTTClient.h"

// Adafruit SSD1306 by Adafruit
#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

// ArduinoJson by Benoit Blanchon
#include <ArduinoJson.h>


/***  Define PINs  ***/
#define LED_PIN  D0
#define RELAY2_PIN D4
#define USBLED_PIN RELAY2_PIN
#define DHT_PIN D3
#define CDS_PIN A0

/***  Define values  ***/
#define DHTTYPE DHT22
#define RELAY_ON LOW    // active low
#define RELAY_OFF HIGH

/*** LED & USBLED State ***/
enum LED_STATE {
  LED_OFF,  // 0
  LED_ON    // 1
};

enum USBLED_STATE {
  USBLED_OFF,       // 0
  USBLED_EVENT_ON,  // 1
  USBLED_USER_ON,   // 2
  USBLED_TOGGLE_ON  // 3
};

LED_STATE     led_state     = LED_OFF;
USBLED_STATE  usbled_state  = USBLED_OFF;


/*** WIFI ***/
// const char *WifiSSID = "wow";
// const char *WifiPassword = "secretpassword";
// const char *WifiSSID = "GHOST-2G";
// const char *WifiPassword = "ghostlinux";
// const char *WifiSSID = "Grace_2.4G";
// const char *WifiPassword = "freedom22";
const char *WifiSSID = "NTH413";
const char *WifiPassword = "cseenth413";


/*** MQTT broker ***/
// const char *mqtt_broker = "203.252.106.154";
const char *mqtt_broker = "sweetdream.iptime.org";
const char *mqtt_username = "iot";
const char *mqtt_password = "csee1414";
const char *mqtt_clientname = "client A";
const int mqtt_port = 1883;

EspMQTTClient mqtt_client(
  WifiSSID, 
  WifiPassword,
  mqtt_broker, 
  mqtt_username, 
  mqtt_password,
  mqtt_clientname,
  mqtt_port
);


/*** Publish topic ***/
const char *pub_dht = "iot/21900050/dht22";
const char *pub_cds = "iot/21900050/cds";

/*** Subscribe topic ***/
const char *sub_topic = "iot/21900050";


String usbled_instant_message;


/***  DHT22 object  ***/
DHTesp dht;


/*** OLED display ***/
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define OLED_RESET -1
#define SCREEN_ADDRESS 0X3C
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



/*** CDS Control values ***/
#define CDS_DARK_TH 500     // 이 값보다 낮으면 어둡다는 뜻이다
#define CDS_BRIGHT_TH 600   // 이 값보다 높으면 밝다는 뜻이다
// 밝을 수록 값이 높다


/*** dht22 and cds value ***/
int cds_value = 0;
int previous_cds_value = 0;           // for determining dark_event
float humidity = 0;
float temperature = 0;


/*** variables for non-blocking delay control ***/
/* 아래 시간은 시간 간격이 아니고 nodeMCU가 시작된 이후 지난 시간을 의미한다 */
long long next_dht22_read_and_publish_time = 0;       // dht22에서 값을 읽을 다음 최소 시간
long long next_cds_read_and_publish_time = 0;         // cds에서 읽은 값을 publish할 다음 최소 시간
long long next_oled_update_time = 0;                  // oled 화면을 업데이트 할 다음 최소 시간
long long next_debug_print_time = 0;                  // debug문구를 print할 다음 최소 시간
long long usbled_time_out_time = 0;                   // usbled가 꺼지는 다음 최소 시간

const long long event_time = 10 * 1000;               // usbled가 event로 인해 켜졌을 때 led가 켜지는 최대 시간 간격

/*** dark event 를 판별하는데 필요한 변수 ***/
bool dark_event = false;
bool has_been_bright = true;      // 처음엔 밝았었다는 가정으로 시작한다


/*** initial setup ***/
void setup() {
  Serial.begin(115200);         // setup Serial to baud rate 115200

  // dht22 setup
  dht.setup(DHT_PIN, DHTesp::DHT22);
  
  // Set PIN mode
  pinMode(LED_PIN, OUTPUT);     // LED
  pinMode(USBLED_PIN, OUTPUT);  // USBLED == RELAY2
  pinMode(CDS_PIN, INPUT);      // CDS
  
  // Display Setup
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS))
  {
    Serial.println(F("SSD1306 allocation failed!\n"));
    for(;;);  // don't proceed if oled cannot be connected
  }

  display.clearDisplay();               // clear buffer
  display.display();                    // display the cleared buffer
  display.setTextColor(SSD1306_WHITE);  // Draw white text
  display.cp437(true);                  // https://www.ascii-codes.com/

  mqtt_client.enableDebuggingMessages(true);        // enable debug message. Must be called before the first loop()
  // mqtt_client.setWifiReconnectionAttemptDelay(1);
  // mqtt_client.setMqttReconnectionAttemptDelay(1);

  // initialize LED/USBLED state
  digitalWrite(LED_PIN, LOW);
  digitalWrite(USBLED_PIN, RELAY_OFF);
}

void loop() {
  mqtt_client.loop();
  read_and_publish_cds_also_determine_dark_event_every_sec(1);
  read_and_publish_dht22_every_sec(10);
  update_oled_every_sec(10);
  
  usbled_event_handler();

  print_debug_every_sec(10);

  /* apply led and usbled state */
  digitalWrite(USBLED_PIN, usbled_state ? RELAY_ON : RELAY_OFF);
  digitalWrite(LED_PIN, led_state);
}


void update_oled_every_sec(long long interval_second) {
  if(millis() > next_oled_update_time) {    
    display.clearDisplay();

    display.setCursor(0,0);
    display.setTextSize(1); display.print("Temperature: ");
    display.setCursor(0,10);
    display.setTextSize(2); display.print(temperature); display.print(" "); 
    display.setTextSize(1); display.write(248); 
    display.setTextSize(2); display.print("C");
    display.setCursor(0,35);
    display.setTextSize(1); display.print("Humidity: ");
    display.setCursor(0,45);
    display.setTextSize(2); display.print(humidity, 1); display.print(" %");

    display.display();

    next_oled_update_time = millis() + interval_second * 1000;
  }

}


/***
 * handles usbled events
 * the events and corresponding state transformation is described in the FSM of the report
***/
void usbled_event_handler() {
  switch (usbled_state) {
    case USBLED_USER_ON:
      if      (usbled_instant_message == "usbledoff")             usbled_state = USBLED_OFF;
      else if (usbled_instant_message == "usbled")                usbled_state = USBLED_OFF;
      break;

    case USBLED_TOGGLE_ON:
      if      (usbled_instant_message == "usbledon")              usbled_state = USBLED_USER_ON;
      else if (usbled_instant_message == "usbled")                usbled_state = USBLED_OFF;
      else if (dark_event)                                        { usbled_state = USBLED_EVENT_ON;  usbled_time_out_time = millis() + event_time; }
      break;

    case USBLED_OFF:
      if      (usbled_instant_message == "usbledon")              usbled_state = USBLED_USER_ON;
      else if (usbled_instant_message == "usbled")                usbled_state = USBLED_TOGGLE_ON;
      else if (dark_event)                                        { usbled_state = USBLED_EVENT_ON;  usbled_time_out_time = millis() + event_time; }
      break;

    case USBLED_EVENT_ON:
      if      (usbled_instant_message == "usbledoff")             usbled_state = USBLED_OFF;
      else if (millis() > usbled_time_out_time)                   usbled_state = USBLED_OFF;
      break;  
  }

  // reset usbled instant message no null
  usbled_instant_message = "";
}



/***
 * dhtt센서에서 온도와 습도 값을 읽고 publish 한다
***/
void read_and_publish_dht22() {
  humidity = dht.getHumidity();
  temperature = dht.getTemperature();

  if(isnan(humidity) || isnan(temperature)) {
    Serial.println("Failed to read from DHT sensor!");
  }

  // publish only when connected
  if(mqtt_client.isConnected()) {
    StaticJsonDocument<100> dht22_doc;
    dht22_doc["dht22_h"] = humidity;
    dht22_doc["dht22_t"] = temperature;

    String output;
    serializeJson(dht22_doc, output);
    mqtt_client.publish(pub_dht, output);
  }
}


/***
 * argument로 초를 입력 받는다
 *
 * 입력 받은 초 마다 dht22 센서에서 온도와 습도 값을 읽고 publish하는 함수를 호출한다
***/
void read_and_publish_dht22_every_sec(long long interval_second) {
  // dht22 센서에서 값을 읽을까요?
  if(millis() > next_dht22_read_and_publish_time) {
    read_and_publish_dht22();
    next_dht22_read_and_publish_time = millis() + interval_second * 1000;     // 다음에 dht22를 읽고 publish할 최소 시간을 결정
  }
}



/***
 * 조도 값을 읽고 publish 한다
 * 또한 읽은 조도값을 기반으로 dark_event인지를 판별하여 dark_event 변수의 값을 업데이트 한다 
***/
void read_and_publish_cds_also_determine_dark_event() {
  cds_value = analogRead(CDS_PIN);

  StaticJsonDocument<100> cds_doc;
  cds_doc["cds_value"] = cds_value;

  // publish only when connected
  if(mqtt_client.isConnected()) {
    String output;
    serializeJson(cds_doc, output);
    mqtt_client.publish(pub_cds, output);
  }
  
  /** dark event가 발생했는지 판별하는 코드 **/
  dark_event = false;                 // reset past event
  if      (has_been_bright == true && cds_value < CDS_DARK_TH)      { dark_event = true; has_been_bright = false; }
  else if (has_been_bright == false && CDS_BRIGHT_TH < cds_value)   has_been_bright = true;
  if(dark_event) Serial.println("dark event!");
  previous_cds_value = cds_value;     // store previous cds value for checking dark event
}


/***
 * argument로 초를 입력 받는다
 *
 * 입력 받은 초 마다 조도 값을 읽고 publish하는 (dark event도 판별한다) 함수를 호출한다.
***/
void read_and_publish_cds_also_determine_dark_event_every_sec(long long interval_second) {
  // cds 센서에서 읽었던 값을 publish 할까요?
  if(millis() > next_cds_read_and_publish_time) {
    read_and_publish_cds_also_determine_dark_event();
    next_cds_read_and_publish_time = millis() + interval_second * 1000;   // 다음에 cds를 읽고 publish할 최소 시간을 결정
  }
}



/*** 
 * arugment로 초를 입력 받는다
 * 
 * debug 할 내용을 serial에 출력한다
 * debug 할 내용은 함수를 내부 값을 직접 수정하여 사용하면 된다
 ***/
void print_debug_every_sec(unsigned int interval_second) {
  if(millis() > next_debug_print_time) {
    bool wifi_state = mqtt_client.isWifiConnected();
    bool mqtt_state = mqtt_client.isMqttConnected();

    Serial.printf("Wifi: %d  MQTT %d \n", wifi_state, mqtt_state);
    Serial.printf("usbled_state: %d \n", usbled_state);

    next_debug_print_time = millis() + interval_second * 1000;
  }
}



/*** 
 * Funcion called once when Wifi and MQTT are connected 
 * `sub_topic`을 subscribe했을 때 받는 메세지를 처리하는 함수도 들어있다
***/
void onConnectionEstablished() {

  mqtt_client.publish(sub_topic, "sub_topic: Connection Established!");

  /* 
   받은 메시지 값이 ledon, ledoff, led면 led의 상태를 변경하고
   `cds`면 cds값을 읽고 publish하고 dark event를 감지하고
   `dht22`면 dht22센서에서 온습도 값을 읽고 publish한다
  */
  mqtt_client.subscribe(
    sub_topic, 
    [](const String& topic, const String & payload)
    {
      Serial.print(">> "); Serial.print(topic); Serial.print(" "); Serial.println(payload);

      if      (payload == "ledon")          led_state = LED_ON;
      else if (payload == "ledoff")         led_state = LED_OFF;
      else if (payload == "led")            led_state = digitalRead(LED_PIN)? LED_OFF: LED_ON;
      else if (payload == "cds")            read_and_publish_cds_also_determine_dark_event();
      else if (payload == "dht22")          read_and_publish_dht22();
      else                                  usbled_instant_message = payload;

    }
  );
}
