#include <WiFi.h>
#include <LCD_I2C.h>
#include <Wire.h>
#include "Adafruit_MQTT.h"
#include "Adafruit_MQTT_Client.h"
#include <Bonezegei_HCSR04.h>

// #include "Task_1.cpp"
// #include "Task_2.h"
// #include "Task_3.h"
// #include "Task_4.h"
// #include "Task_5.h"
// #include "Task_6.h"
// #include "Task_7.h"
// #include "Task_8.h"




// WiFi & MQTT Setup
const char* ssid = "real";  // Change to your WiFi SSID
const char* password = "suman saha";  // Change to your WiFi password
#define AIO_SERVER      "io.adafruit.com"
#define AIO_SERVERPORT  1883
#define AIO_USERNAME  "cirsuman"
#define AIO_KEY  "aio_LqMt04747vwruj0g49TMsQoZpj2N"


WiFiClient client;
Adafruit_MQTT_Client mqtt(&client, AIO_SERVER, AIO_SERVERPORT, AIO_USERNAME, AIO_KEY);

Adafruit_MQTT_Publish levelFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/level");
Adafruit_MQTT_Publish timeFeed = Adafruit_MQTT_Publish(&mqtt, AIO_USERNAME "/feeds/time");

byte dotBlink[8]{ 0b11111,0b11111,0b11111,0b11111,0b11111,0b11111,0b11111,0b11111};



LCD_I2C lcd(0x27, 16, 2);  

volatile uint8_t blinkingDotIndex = 0;
volatile bool dotBlinkState = false;


int thresholdDistance = 24;

//////////////////
//pins
#define BUZZ 2  // BUZZER CON

#define GPIO1 12  //need to connect to gnd to start the game
#define GPIO2 13  // need to connect to gnd to start the game

#define TRIG_PIN 18  // Ultrasonic Trigger
#define ECHO_PIN 17  // Ultrasonic Echo

#define BT1_PIN 32  // press it 1st
#define BT2_PIN 33  // press it next 

#define LDR_PIN 27  // LDR sensor

#define MIC_PIN 25  // Microphone 

#define TILT_PIN 35  // Tilt sensor

#define REED_PIN 23  // Reed switch 

#define NTC_PIN 26  // NTC thermistor 

#define BUTTON_PIN 4  // level 7 bypass for us

#define WIRE_PIN 15  // we need to break it to complete the level

Bonezegei_HCSR04 ultrasonic(TRIG_PIN, ECHO_PIN);

////////////////////////

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

void sendMQTTMessage(int levelNumber, int remainingTime);

class LevelDisplay {
public:
    uint8_t currentLevel = 1;
    uint8_t completedLevels = 0;
    
    LevelDisplay() {}

    void initDisplay() {
        lcd.createChar(0, dotBlink);  
        lcd.clear();
        lcd.setCursor(0, 0);
        lcd.print("BOMB ONE ");
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

    void updateBlinkingDot(){
      unsigned long currentTime=millis();
      if(currentTime - lastBlinkTime >=500){
        dotBlinkState = !dotBlinkState;
        lastBlinkTime = currentTime;
        displayDots();
      }
    }

    void displayTimer() {
        int minutes = remainingSeconds / 60;
        int seconds = remainingSeconds % 60;
        lcd.setCursor(11, 0);
        if (minutes < 10) lcd.print("0");
        lcd.print(minutes);
        lcd.print(":");
        if (seconds < 10) lcd.print("0");
        lcd.print(seconds);
    }

uint8_t levelCompletion[8] = {0, 0, 0, 0, 0, 0, 0, 0};

void updateLevel(uint8_t levelNumber) {
    if (levelNumber >= 1 && levelNumber <= 8) {
        uint8_t index = levelNumber - 1;

        if (levelCompletion[index] == 0) {
            levelCompletion[index] = 1;
            currentLevel = levelNumber + 1;  

            if (currentLevel > 8) {
                gameOver = true;
                displayGameResult();
                return;
            }
        }
    }

    lcd.clear();
    displayHeader();
    displayDots();
}

void displayDots() {
    lcd.setCursor(8, 1);  
    for (uint8_t i = 0; i < 8; i++) {
        if (levelCompletion[i] == 1) {
            lcd.write(0);  
        } else if (i == currentLevel - 1) {
            lcd.write(dotBlinkState ? 0 : 32);  
        } else {
            lcd.write(32);  // unachieved goal
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
            displayGameResult();
        }
    }
}

    void displayGameResult() {
        lcd.clear();
        lcd.setCursor(0, 0);
        if (completedLevels == 8) {
            lcd.print("CONGO!");
            lcd.setCursor(0, 1);
            lcd.print("YOU WON!");
        } else {
            lcd.print("GAME OVER!");
            lcd.setCursor(0, 1);
            lcd.print("Levels: ");
            lcd.print(completedLevels);
            lcd.print("/8");
        }
    }

    bool isGameOver() {
        return gameOver;
    }
};

LevelDisplay levelDisplay;

void setup() {
    Serial.begin(115200);
    lcd.begin();
    lcd.backlight();
    levelDisplay.initDisplay();

     pinMode(GPIO1, INPUT_PULLUP);
    pinMode(GPIO2, INPUT_PULLUP);
    
    WiFi.begin(ssid, password);
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }
    Serial.println("Connected to WiFi");
    Serial.println(WiFi.localIP().toString(false));
    reconnectMQTT();
  
    xTaskCreatePinnedToCore(Task1, "Level 1", 10000, NULL, 1, &Task1Handle, 0);
    xTaskCreatePinnedToCore(Task2, "Level 2", 10000, NULL, 1, &Task2Handle, 1);
    xTaskCreatePinnedToCore(Task3, "Level 3", 10000, NULL, 1, &Task3Handle, 0);
    xTaskCreatePinnedToCore(Task4, "Level 4", 10000, NULL, 1, &Task4Handle, 1);
    xTaskCreatePinnedToCore(Task5, "Level 5", 10000, NULL, 1, &Task5Handle, 0);
    xTaskCreatePinnedToCore(Task6, "Level 6", 10000, NULL, 1, &Task6Handle, 1);
    xTaskCreatePinnedToCore(Task7, "Level 7", 10000, NULL, 1, &Task7Handle, 0);
    xTaskCreatePinnedToCore(Task8, "Level 8", 10000, NULL, 1, &Task8Handle, 1);
}

void loop() {
 if (!client.connected()) {
        reconnectMQTT();
    }
    //mqtt.loop();
    levelDisplay.updateTimer();
    levelDisplay.updateBlinkingDot();

    if (levelDisplay.isGameOver()) {
        Serial.println("Game Over!");        
        delay(2000);  
        ESP.restart(); 

    delay(100);
   }
}
void reconnectMQTT() {
    while (!mqtt.connected()) {
        Serial.print("Attempting MQTT connection...");
        
        if (mqtt.connect()) {
            Serial.println("Connected to MQTT Broker");
            
            // mqtt.subscribe("bomb_diffusal_game/level");
        } else {
            Serial.print("Failed, rc=");
            Serial.print(mqtt.connected()?"Connected":"Not Connected");
            delay(5000);  // Retry after 5 seconds
        }
    }
}


void mqttCallback(char* topic, byte* payload, unsigned int length) {
    Serial.print("Message arrived on topic: ");
    Serial.print(topic);
    Serial.print(" with message: ");
    
    for (int i = 0; i < length; i++) {
        Serial.print((char)payload[i]);
    }
    
    Serial.println();
}


void sendMQTTMessage(int levelNumber, int remainingTime) {
    char message[50];
    snprintf(message, sizeof(message), "{\"level\": %d, \"time\": %d}", levelNumber, remainingTime);
    
    if (mqtt.connected()) {
        if (mqtt.publish("bomb_diffusal_game/level", message)) {
            Serial.print("Sent MQTT Message: ");
            Serial.println(message);  
        } else {
            Serial.println("Failed to send MQTT message");
        }
    } else {
        Serial.println("MQTT not connected");
    }
}




////////////////////////////////////////////
void Task1(void *pvParameters) {
   

    unsigned long timerStart = millis();
    bool levelCompleted = false;

    while (remainingSeconds > 0 && !levelCompleted) {
        if (digitalRead(GPIO1) == LOW && digitalRead(GPIO2) == LOW) {
            remainingSeconds = 600;  
            levelDisplay.updateLevel(1);  
            levelCompleted = true;
            sendMQTTMessage((int)1, remainingSeconds);
        }

        if (millis() - timerStart > 2000 && millis() % 1000 < 500) {
            tone(BUZZ, 1000);  
        } else {
            noTone(BUZZ);  
        }

        delay(100);  
    }

    vTaskDelete(NULL);  
}

////////////////////////////////////////////////////
void Task2(void *pvParameters) {
    pinMode(LDR_PIN, INPUT);  
    int distance;
    bool levelCompleted = false;
    bool bt1Pressed = false, bt2Pressed = false;
    bool ldrBlocked = false;
        unsigned long previousMillisLDR = 0;
    const unsigned long ldrBlockTime = 3000;  // 3 seconds
    const int thresholdDistance = 30;  // Threshold for distance in cm


    while (remainingSeconds > 0 && !levelCompleted) {

        distance = ultrasonic.getDistance();

        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" cm");

       int ldrValue = analogRead(LDR_PIN);
    unsigned long currentMillis = millis();

    if (distance >= thresholdDistance) {
        if (!ldrBlocked) {
            previousMillisLDR = currentMillis;  
            ldrBlocked = true;
        }

        if (ldrBlocked && currentMillis - previousMillisLDR >= ldrBlockTime) {
            // LDR is blocked for 3 seconds, now wait for button presses at any time
            if (!digitalRead(BT1_PIN) && !bt1Pressed) {  
                bt1Pressed = true;
                digitalWrite(BT1_PIN, HIGH); 
            }

        if (bt1Pressed && !digitalRead(BT2_PIN) && !bt2Pressed) {  
                bt2Pressed = true;
                digitalWrite(BT2_PIN, HIGH);  
                levelCompleted = true;
                levelDisplay.updateLevel(2);  
                sendMQTTMessage(2, remainingSeconds);  
            }
        }
    }

        delay(100);  
    }

    vTaskDelete(NULL); 
}

///////////////////////////////////////////////////////////


void Task3(void *pvParameters) {
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
                sendMQTTMessage((int)3, remainingSeconds);
            }
        }
        delay(100);
    }

    vTaskDelete(NULL);
} 

/////////////////////////////////////////////////////////////

void Task4(void *pvParameters) {
    pinMode(TILT_PIN, INPUT);
    int tiltValue;
    bool levelCompleted = false;

    while (remainingSeconds > 0 && !levelCompleted) {
        tiltValue = analogRead(TILT_PIN);
        
        if (tiltValue > 1000) {  
            levelCompleted = true;
            levelDisplay.updateLevel(4);
            sendMQTTMessage((int)4, remainingSeconds);
        }

        delay(100); 
    }

    vTaskDelete(NULL);
}
//////////////////////////////////////////////////////////////////////////////


void Task5(void *pvParameters) {
    pinMode(REED_PIN, INPUT_PULLUP);
    bool levelCompleted = false;

    while (remainingSeconds > 0 && !levelCompleted) {
        if (digitalRead(REED_PIN) == LOW) {  
            levelCompleted = true;
            levelDisplay.updateLevel(5);
            sendMQTTMessage((int)5, remainingSeconds);
        }

        delay(100);  
    }

    vTaskDelete(NULL);
}

////////////////////////////////////////////////////////////////////////

float temperature; 

void Task6(void *pvParameters) {
    pinMode(NTC_PIN, INPUT);
    bool levelCompleted = false;

    while (remainingSeconds > 0 && !levelCompleted) {
        int sensorValue = analogRead(NTC_PIN);
        temperature = (float)sensorValue * (3.3 / 4095.0);  // Convert analog reading to voltage
        temperature = (1 / (log(1 / (3.3 / temperature - 1)) / 3950 + 1 / 298.15) - 273.15);  // Convert to Celsius

        if (temperature >= 30.0) {  
            levelCompleted = true;
            levelDisplay.updateLevel(6);
            sendMQTTMessage((int)6, remainingSeconds);
        }

        delay(500);  
    }

    vTaskDelete(NULL);
}

/////////////////////////////////////////////
#include <ESPAsyncWebServer.h>


AsyncWebServer server(80);

void Task7(void *pvParameters) {
    pinMode(BUTTON_PIN, INPUT_PULLUP);
    bool levelCompleted = false;

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
        String html = "<html><body><h1>Level 7: Click the Button to Continue</h1>";
        html += "<form method='POST' action='/complete'><button>Complete Level</button></form></body></html>";
        request->send(200, "text/html", html);
    });

    server.on("/complete", HTTP_POST, [](AsyncWebServerRequest *request){
        //levelCompleted = true;
        levelDisplay.updateLevel(7);
        sendMQTTMessage((int)7, remainingSeconds);
        request->send(200, "text/html", "Level Completed! <a href='/'>Back</a>");
    });

    server.begin();

  while (remainingSeconds > 0 && !levelCompleted) {
        if (digitalRead(BUTTON_PIN) == LOW) {
            levelCompleted = true;
            levelDisplay.updateLevel(7);
            sendMQTTMessage((int)7, remainingSeconds);
        }
        delay(100);
  }

    vTaskDelete(NULL);
}

//////////////////////////////////////////////////////////
void Task8(void *pvParameters) {
    pinMode(WIRE_PIN, INPUT_PULLUP);  
    bool levelCompleted = false;
    unsigned long previousMillis = 0;
    const unsigned long interval = 100; 

    while (remainingSeconds > 0 && !levelCompleted) {
        unsigned long currentMillis = millis();

        if (currentMillis - previousMillis >= interval) {
            previousMillis = currentMillis;

            if (digitalRead(WIRE_PIN) == LOW) {  
                levelCompleted = true;
                levelDisplay.updateLevel(8);
                sendMQTTMessage(8, remainingSeconds);  
                levelDisplay.displayGameResult(); 
            }
        }
    }

    vTaskDelete(NULL);  
}

/////////////////////////////////////////////////////////////////////
