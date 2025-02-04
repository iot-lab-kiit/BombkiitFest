#include <TM1637.h>
#include <WiFi.h>

#define CLK 4                // TM1637 Clock 
#define DIO 5                // TM1637 Data 
#define BUTTON_1 12          // Button 1 Pin
#define BUTTON_2 13          // Button 2 Pin
#define LDR_PIN 34           // LDR 
#define VIBRATION_PIN 36     // Vibration sensor
#define HEAT_SENSOR_PIN 39   // Heat sensor 
#define BUZZER_PIN 14        // Buzzer 

// all task definations

TaskHandle_t TimerTaskHandle = NULL;
TaskHandle_t SensorTaskHandle = NULL;
TaskHandle_t ButtonTaskHandle = NULL;
TaskHandle_t WebServerTaskHandle = NULL;

TM1637 display(CLK, DIO);

// Global Variables
int gameTimer = 80;            // Timer for the bomb defusal
bool gameInProgress = false;   // status
int vibrationThreshold1 = 300; // Vibration threshold 
int vibrationThreshold2 = 600; // Vibration threshold bypass
int ldrThreshold = 800;        // LDR threshold
int heatThreshold = 400;       // Heat threshold 

const char* ssid = "IOT_DEVICES";         // Your WiFi SSID
const char* password = "iot_lab_devices"; // Your WiFi password

void setup() {
  Serial.begin(115200);
  pinMode(BUTTON_1, INPUT_PULLUP);
  pinMode(BUTTON_2, INPUT_PULLUP);
  pinMode(BUZZER_PIN, OUTPUT);
  
  display.setBrightness(0x0f);

  // Connect to Wi-Fi
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    Serial.println("Connecting to WiFi...");
  }
  Serial.println("Connected to WiFi!");

//  // Web server setup
//  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
//    String html = "<html><body><h2>Solve the Puzzle!</h2>";
//    html += "<form action='/puzzle' method='GET'><input type='text' name='answer' placeholder='Your answer'>";
//    html += "<input type='submit' value='Submit'></form></body></html>";
//    request->send(200, "text/html", html);
//  });
//
//  server.on("/puzzle", HTTP_GET, [](AsyncWebServerRequest *request){
//    String answer = request->getParam("answer")->value();
//    if (answer == "correct") {
//      request->send(200, "text/html", "<html><body><h2>Correct! You moved to the next round.</h2></body></html>");
//    } else {
//      request->send(200, "text/html", "<html><body><h2>Wrong answer. Try again!</h2></body></html>");
//    }
//  });
//
//  server.begin();

  xTaskCreatePinnedToCore(TimerTask, "TimerTask", 4096, NULL, 1, &TimerTaskHandle, 1);  // Core 1
  xTaskCreatePinnedToCore(SensorTask, "SensorTask", 4096, NULL, 1, &SensorTaskHandle, 0); // Core 0
  xTaskCreatePinnedToCore(ButtonTask, "ButtonTask", 4096, NULL, 1, &ButtonTaskHandle, 0); // Core 0
  xTaskCreatePinnedToCore(WebServerTask, "WebServerTask", 4096, NULL, 1, &WebServerTaskHandle, 1); // Core 1
}

void loop() {
}

//start the fake timer 
void TimerTask(void *parameter) {
  for (int i = 10; i > 0; i--) {
    //display.showNumberDec(i, false); 
    delay(1000);
  }

// wire connection part 
  while (!gameInProgress) {

    bool a = false;
    
    if (digitalRead(34) == LOW) { 
 a= true;
      if((digitalRead(36 == LOW) && (a))){

        
      gameInProgress = true;
      gameTimer = 60;  // Start countdown
      break;
    }
    }
  }

  while (gameInProgress && gameTimer > 0) {
   // display.showNumberDec(gameTimer, false); 
    delay(1000);  // Wait for 1 second
    gameTimer--;
  }

  if (gameTimer == 0) {
    tone(BUZZER_PIN, 2000, 1000); // Bomb exploded
    gameInProgress = false;
  }
}



void SensorTask(void *parameter) {
  while (gameInProgress)
  {
    if (analogRead(LDR_PIN) < ldrThreshold) {
      delay(4000);  
      tone(BUZZER_PIN, 1000, 500); 
    }

    int vibrationLevel = analogRead(VIBRATION_PIN);
    if (vibrationLevel > vibrationThreshold2) {
      tone(BUZZER_PIN, 1000, 500);  
      gameTimer += 10; 
    } else if (vibrationLevel > vibrationThreshold1) {
      tone(BUZZER_PIN, 500, 500); 
    }

    if (analogRead(HEAT_SENSOR_PIN) > heatThreshold) {
      tone(BUZZER_PIN, 1500, 500); 
    }

    delay(100); 
  }

  vTaskDelete(NULL); 
}

void ButtonTask(void *parameter) {
  while (gameInProgress) {
    if (digitalRead(BUTTON_1) == LOW) {
      delay(500);  
      if (digitalRead(BUTTON_2) == LOW) {
       
        break;
      }
    }

    delay(100); 
  }

  vTaskDelete(NULL); 
}


// the webserver

void WebServerTask(void *parameter) {
 
  vTaskDelete(NULL); 
}
