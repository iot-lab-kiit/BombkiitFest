#include <WiFi.h>
#include "web.h"
#include <LCD_I2C.h>
#include <Wire.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Bonezegei_HCSR04.h>
#include <ArduinoJson.h>
#include <PubSubClient.h>
#include <math.h>

// WiFi & MQTT Setup
const char *ssid = "IOT_DEVICES";
const char *password = "iot_lab_devices";
// const char *ssid = "real";
// const char *password = "suman saha";


////////////////////////////////////////////////////
const char *mqttServer = "test.mosquitto.org";
const int mqttPort = 1883;
const char *mqttUser = "";
const char *mqttPassword = "";

#define MQTT_TOPIC "bomb_message_14123"
////////////////////////////////////////////////

WiFiClient espClient;
PubSubClient client(espClient);

byte dotBlink[8]{ 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111, 0b11111 };

LCD_I2C lcd(0x27, 16, 2);

volatile uint8_t blinkingDotIndex = 0;
volatile bool dotBlinkState = false;

int thresholdDistance = 7;

//////////////////
// pins
#define BUZZ 27     // BUZZER CON
#define LED_WIN 2   // pin to indicate win  GREEN
#define LED_STAT 4  // pin to indicate  RED

#define GPIO1 12  // need to connect to gnd to start the game
#define GPIO2 13  // need to connect to gnd to start the game

#define TRIG_PIN 18  // Ultrasonic Trigger
#define ECHO_PIN 17  // Ultrasonic Echo

#define BT1_PIN 32  // press it 1st
#define BT2_PIN 33  // press it next

//#define LDR_PIN 27  // LDR sensor

#define MIC_PIN 25  // Microphone

#define TILT_PIN 35  // Tilt sensor connected to gnd

#define REED_PIN 23  // Reed switch connected to gnd

#define NTC_PIN 14  // NTC thermistor

#define BUTTON_PIN 4  // level 7 bypass for us

#define WIRE_PIN 15  // we need to break it to complete the level

Bonezegei_HCSR04 ultrasonic(TRIG_PIN, ECHO_PIN);

////////////////////////
TaskHandle_t Task0Handle = NULL;

TaskHandle_t Task1Handle = NULL;
TaskHandle_t Task2Handle = NULL;
TaskHandle_t Task3Handle = NULL;
TaskHandle_t Task4Handle = NULL;
TaskHandle_t Task5Handle = NULL;
TaskHandle_t Task6Handle = NULL;
TaskHandle_t Task7Handle = NULL;
TaskHandle_t Task8Handle = NULL;

unsigned long lastBlinkTime = 0;
unsigned long lastSecondUpdate = 0;
extern int remainingSeconds = 600;
bool gameOver = false;

unsigned volatile int distance = 0;
unsigned volatile int heat = 0;

String lip;

void sendMQTTMessage(int bombno, int levelNumber, int remainingTime);

void beep(int times) {
  for (int i = 0; i < times; i++) {
    tone(BUZZ, 800, 300);
    vTaskDelay(1000 / portTICK_PERIOD_MS);
    // delay(1000);
  }
}

class LevelDisplay {
public:
  uint8_t currentLevel = 1;
  uint8_t completedLevels = 0;

  LevelDisplay() {}

  void initDisplay() {
    lcd.createChar(0, dotBlink);
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("BOMB ONE");
    delay(2000);
    displayHeader();
    displayDots();
  }

  void displayHeader() {
    lcd.setCursor(0, 0);
    lcd.print("LEVEL - ");
    lcd.print(currentLevel);
    lcd.setCursor(11, 0);
    displayTimer();
  }

  void updateBlinkingDot() {
    unsigned long currentTime = millis();
    if (currentTime - lastBlinkTime >= 300) {
      dotBlinkState = !dotBlinkState;
      lastBlinkTime = currentTime;
      displayDots();
      if (currentLevel == 2) {
        lcd.setCursor(0, 1);
        lcd.print(String(distance).substring(0, 1));
        lcd.setCursor(3, 1);
        lcd.print("cm");
      }

      if (currentLevel == 6) {
        if (heat != 1) {
          lcd.setCursor(0, 1);
          lcd.print("COLD");
        } else {
          lcd.setCursor(0, 1);
          lcd.print("HEAT");
        }
      }
    }
  }


  void displayTimer() {
    int minutes = remainingSeconds / 60;
    int seconds = remainingSeconds % 60;
    lcd.setCursor(11, 0);
    if (minutes < 10)
      lcd.print("0");
    lcd.print(minutes);
    lcd.print(":");
    if (seconds < 10)
      lcd.print("0");
    lcd.print(seconds);
    if (remainingSeconds < 6 && remainingSeconds > -1) {
      tone(BUZZ, ((500)+((6-remainingSeconds)*100)));
    }
  }

  uint8_t levelCompletion[8] = { 0, 0, 0, 0, 0, 0, 0, 0 };

  void updateLevel(uint8_t levelNumber) {
    if (levelNumber >= 1 && levelNumber <= 8) {
      uint8_t index = levelNumber - 1;

      if (levelCompletion[index] == 0) {
        levelCompletion[index] = 1;
        currentLevel = levelNumber + 1;

        if (currentLevel > 8) {
          //gameOver = true;
          //displayGameResult(0);
          return;
        }
      }
    }

    lcd.clear();
    displayHeader();
    displayDots();
  }

  void displayDots() {
    if (!(gameOver)) {
      lcd.setCursor(8, 1);
      for (uint8_t i = 0; i < 8; i++) {
        if (levelCompletion[i] == 1) {
          lcd.write(0);
        } else if (i == currentLevel - 1) {
          lcd.write(dotBlinkState ? 0 : 32);
        } else {
          lcd.write(32);
        }
      }
    }
  }
  void updateTimer() {
    unsigned long currentTime = millis();

    if (currentTime - lastSecondUpdate >= 1000) {
      remainingSeconds--;
      displayTimer();
      lastSecondUpdate = currentTime;

      if (remainingSeconds <= 0) {
        gameOver = true;
        displayGameResult(0);
      }
    }
  }

  void displayGameResult(int a) {
    remainingSeconds = 0;
    lcd.clear();
    lcd.setCursor(0, 0);
    if (a == 1) {
      lcd.print("Diffused");
      digitalWrite(LED_WIN, HIGH);
    }
    if (a == 0) {

      tone(BUZZ, 900);

      lcd.print("GAME OVER!");
      lcd.setCursor(0, 1);
      lcd.print("last completed:");
      lcd.print(currentLevel - 1);
      digitalWrite(LED_STAT, HIGH);
    }
  }

  bool isGameOver() {
    return gameOver;
  }
};

LevelDisplay levelDisplay;

void setup() {
  Serial.begin(115200);
  delay(10);
  lcd.begin();
  lcd.backlight();
  levelDisplay.initDisplay();
  pinMode(LED_WIN, OUTPUT);
  pinMode(LED_STAT, OUTPUT);
  digitalWrite(LED_WIN, LOW);
  digitalWrite(LED_STAT, HIGH);
  pinMode(NTC_PIN, INPUT);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  lip = WiFi.localIP().toString();
  Serial.println(lip);

  client.setServer(mqttServer, mqttPort);
  client.setCallback(mqttCallback);

  while (!client.connected()) {
    if (client.connect("ESP32SerialClient", mqttUser, mqttPassword)) {
      Serial.println("Connected to MQTT broker");
    } else {
      Serial.print("MQTT connection failed, rc=");
      Serial.print(client.state());
      Serial.println(" retrying in 2 seconds");
      delay(2000);
    }
  }
  client.subscribe(MQTT_TOPIC);
  Serial.print("Subscribed to topic: ");
  Serial.println(MQTT_TOPIC);

sendMQTTMessage(1, 0, remainingSeconds);// send the start time to the server 

  xTaskCreatePinnedToCore(Task1, "Level 1", 10000, NULL, 1, &Task1Handle, 0);
}

void loop() {
  client.loop();
  levelDisplay.updateTimer();
  levelDisplay.updateBlinkingDot();

  //delay(10);
}

// void reconnectMQTT() {
//     while (!mqtt.connected()) {
//         Serial.print("Attempting MQTT connection...");

//         if (mqtt.connect(MQTT_CLIENT)) {
//             Serial.println("Connected to MQTT Broker");
//             // mqtt.subscribe("bomb_diffusal_game/level");
//         } else {
//             Serial.print("Failed, rc=");
//             Serial.print(mqtt.state());
//             Serial.println(" try again in 5 seconds");
//             delay(5000);
//         }
//     }
// }

void mqttCallback(char *topic, byte *payload, unsigned int length) {
  Serial.print("Received on topic: ");
  Serial.println(topic);
}

// void callback(char *topic, byte *payload, unsigned int length) {
//   Serial.print("Message arrived on topic: ");
//   Serial.print(topic);
//   Serial.print(" with message: ");

//   StaticJsonDocument<256> doc;
//   DeserializationError error = deserializeJson(doc, payload, length);

//   if (error) {
//     Serial.print("deserializeJson() failed: ");
//     Serial.println(error.c_str());
//     return;
//   }

//   if (doc.containsKey("level") && doc.containsKey("time") && doc.containsKey("bombno")) {
//     int bombno = doc["bombno"];
//     int level = doc["level"];
//     int time = doc["time"];

//     Serial.print("bombno: ");
//     Serial.println(bombno);
//     Serial.print("Level: ");
//     Serial.println(level);
//     Serial.print("Time: ");
//     Serial.println(time);

//   } else {
//     Serial.println("Missing 'level', 'time', or 'extra' keys in JSON");
//   }
// }

void sendMQTTMessage(int bombno, int levelNumber, int remainingTime) {
  StaticJsonDocument<256> doc;
  doc["bomb_id"] = bombno;
  doc["level"] = levelNumber;
  doc["time"] = remainingTime;
  doc["ip"] = lip;

  char payload[256];
  size_t n = serializeJson(doc, payload);

  boolean publishSuccess = client.publish(MQTT_TOPIC, payload, n);
  if (publishSuccess) {
    Serial.print("Published message: ");
    Serial.println(payload);
  } else {
    Serial.println("Failed to publish message");
  }
}

////////////////////////////////////////////
// void Task0(void *pvParameters) {
//   while (1) {
//     client.loop();
//     vTaskDelay(1 / portTICK_PERIOD_MS);
//   }
// }
void Task1(void *pvParameters) {
  Serial.println("Entered task 1");
  beep(1);
  pinMode(GPIO1, INPUT_PULLUP);
  pinMode(GPIO2, INPUT_PULLUP);

  unsigned long timerStart = millis();
  bool levelCompleted = false;

  while (remainingSeconds > 0 && !levelCompleted) {
    if (digitalRead(GPIO1) == LOW && digitalRead(GPIO2) == LOW) {
      levelDisplay.updateLevel(1);
      levelCompleted = true;
      sendMQTTMessage(1, (int)1, remainingSeconds);
    }



    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  xTaskCreatePinnedToCore(Task2, "Level 2", 10000, NULL, 1, &Task2Handle, 1);

  vTaskDelete(NULL);
}

////////////////////////////////////////////////////
void Task2(void *pvParameters) {
  Serial.println("Entered task 2");
  beep(2);

  bool levelCompleted = false;
  unsigned long distanceStartTime = 0;
  bool targetDistanceReached = false;

  while (remainingSeconds > 0 && !levelCompleted) {

    distance = ultrasonic.getDistance();


    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");

    unsigned long currentMillis = millis();

    if (distance == thresholdDistance) {
      if (!targetDistanceReached) {
        distanceStartTime = currentMillis;
        targetDistanceReached = true;
        Serial.println("Distance REACHED");
      }

      if (targetDistanceReached && currentMillis - distanceStartTime >= 5000)  // 5 seconds
      {
        Serial.println("Distance met for 5 seconds!");
        levelCompleted = true;
        levelDisplay.updateLevel(2);
        sendMQTTMessage(1, 2, remainingSeconds);
      }
    } else {
      targetDistanceReached = false;
      Serial.println("Distance NOT Met");
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  xTaskCreatePinnedToCore(Task3, "Level 3", 10000, NULL, 1, &Task3Handle, 0);

  vTaskDelete(NULL);
}

///////////////////////////////////////////////////////////

void Task3(void *pvParameters) {
  Serial.println("Entered task 3");
  beep(3);
  pinMode(MIC_PIN, INPUT);
  int clapCount = 0;
  bool levelCompleted = false;

  while (remainingSeconds > 0 && !levelCompleted) {
    if (digitalRead(MIC_PIN) == HIGH) {
      clapCount++;
      delay(500);

      if (clapCount >= 5) {
        levelCompleted = true;
        levelDisplay.updateLevel(3);
        sendMQTTMessage(1, (int)3, remainingSeconds);
      }
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  xTaskCreatePinnedToCore(Task4, "Level 4", 10000, NULL, 1, &Task4Handle, 1);

  vTaskDelete(NULL);
}

/////////////////////////////////////////////////////////////

void Task4(void *pvParameters) {
  Serial.println("Entered task 4");
  beep(4);
  pinMode(TILT_PIN, INPUT_PULLUP);
  bool levelCompleted = false;

  while (remainingSeconds > 0 && !levelCompleted) {
    int val = digitalRead(TILT_PIN);
    Serial.println(val);
    if (val != 0) {
      levelCompleted = true;
      levelDisplay.updateLevel(4);
      sendMQTTMessage(1, (int)4, remainingSeconds);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }
  xTaskCreatePinnedToCore(Task5, "Level 5", 10000, NULL, 1, &Task5Handle, 0);

  vTaskDelete(NULL);
}
//////////////////////////////////////////////////////////////////////////////

void Task5(void *pvParameters) {
  Serial.println("Entered task 5");
  beep(5);
  pinMode(REED_PIN, INPUT_PULLUP);
  bool levelCompleted = false;

  while (remainingSeconds > 0 && !levelCompleted) {
    if (digitalRead(REED_PIN) == LOW) {
      levelCompleted = true;
      levelDisplay.updateLevel(5);
      sendMQTTMessage(1, (int)5, remainingSeconds);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
    //delay(100);
  }
  xTaskCreatePinnedToCore(Task6, "Level 6", 10000, NULL, 1, &Task6Handle, 0);

  vTaskDelete(NULL);
}

////////////////////////////////////////////////////////////////////////

void Task6(void *pvParameters) {
  Serial.println("Entered task 6");
  beep(6);
  pinMode(NTC_PIN, INPUT_PULLUP);

  bool levelCompleted = false;

  while (remainingSeconds > 0 && !levelCompleted) {
    heat = digitalRead(NTC_PIN);
    Serial.println(heat);
    if (heat != 0) {

      Serial.println("Heat detected!");
      levelCompleted = true;
      levelDisplay.updateLevel(6);


      sendMQTTMessage(1, 6, remainingSeconds);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  xTaskCreatePinnedToCore(Task7, "Level 7", 10000, NULL, 1, &Task7Handle, 0);

  vTaskDelete(NULL);  // Delete the task when finished
}

/////////////////////////////////////////////

void Task7(void *pvParameters) {
  Serial.println("Entered task 7");
  beep(7);
  setupServer();
  bool levelCompleted = false;


  while (remainingSeconds > 0 && !levelCompleted) {
    if (isSolved == 1)  // declared in web.h
    {
      levelCompleted = true;
      levelDisplay.updateLevel(7);
      sendMQTTMessage(1, (int)7, remainingSeconds);
    }

    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  xTaskCreatePinnedToCore(Task8, "Level 8", 10000, NULL, 1, &Task8Handle, 1);

  vTaskDelete(NULL);
}

//////////////////////////////////////////////////////////
void Task8(void *pvParameters) {
  Serial.println("Entered task 8");
  beep(8);
  pinMode(WIRE_PIN, INPUT_PULLUP);
  vTaskDelay(1000 / portTICK_PERIOD_MS);
  int val = 1;
  bool levelCompleted = false;

  while (remainingSeconds > 0 && !levelCompleted) {
    val = digitalRead(WIRE_PIN);
    Serial.println(val);
    if (val == 0) {
      levelCompleted = true;
      levelDisplay.updateLevel(8);
      digitalWrite(LED_STAT, LOW);
      digitalWrite(LED_WIN, HIGH);
      sendMQTTMessage(1, 8, remainingSeconds);
      levelDisplay.displayGameResult(1);
    }
    vTaskDelay(100 / portTICK_PERIOD_MS);
  }

  vTaskDelete(NULL);
}

/////////////////////////////////////////////////////////////////////
