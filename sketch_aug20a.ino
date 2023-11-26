#include <Arduino.h>
#include <regex>
#include <LiquidCrystal_I2C.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <ESP32Ping.h>
#include <ESPAsyncWebServer.h>
#include "webpage.h"
#include <DHT.h>
#include <ArduinoJson.h>
#include <EEPROM.h>
//#include <analogWrite.h>

#define DHTPIN 15
#define DHTTYPE DHT22


unsigned long motorStartTime = 0;
const unsigned long motorDuration = 5000;  // 执行时间，单位为毫秒

DHT dht(DHTPIN, DHTTYPE);

const int EEPROM_SIZE = 64;  // EEPROM的大小，根据需要更改
int MAX_API_DATA_LENGTH = 64;


AsyncWebServer server(80);
IPAddress localIP;

String inputData = "";
String notificationAPI = "";

const int motor1A = 26; // 连接到L298N的引脚
const int motor1B = 27;
const int motor2A = 25;
const int motor2B = 33;
const int en1 = 2;
const int en2 = 4;

const int pwmFreq = 30000;
const int pwmResolution = 8;

const char* apiKey = "z81Tzmwb14JThgmf2ijfpmkCVvdN4ZMQ";
const char* endpoint = "https://dataservice.accuweather.com/forecasts/v1/hourly/12hour/228037";//"http://192.168.50.90:8090/228037.json";

LiquidCrystal_I2C lcd(0x27,20,4);
const int threshold = 3500; // 阈值
const unsigned long requiredDuration = 3000; // 三秒
unsigned long startTime = 0;
unsigned long currentTime = 0;
const unsigned long timeout = 10000;
bool counting = false;

unsigned long rainstartTime = 0;
String ipv4Address;
bool belowThreshold = false;

const unsigned long pingInterval = 5000; // 每5秒ping一次
unsigned long lastPingTime = 0;
bool isCAMOnline(const char* ip);

const char* ssid = "WiFi@FoonYew";
const char* pwd = "";

void setup() {
    //esp_task_wdt_disable();
    Serial.begin(115200);
    Serial2.begin(115200);
    EEPROM.begin(EEPROM_SIZE);
    notificationAPI = readData();
    dht.begin();
    lcd.init();
    lcd.backlight();
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.print("Starting...");
    pinMode(32, INPUT);
    pinMode(13, OUTPUT);
    pinMode(12, OUTPUT);
    pinMode(14, OUTPUT);
    pinMode(35, INPUT);
    pinMode(motor1A, OUTPUT);
    pinMode(motor1B, OUTPUT);
    pinMode(motor2A, OUTPUT);
    pinMode(motor2B, OUTPUT);
    pinMode(en1, OUTPUT);
    pinMode(en2, OUTPUT);
    pinMode(5, OUTPUT);
    pinMode(18, OUTPUT);
    digitalWrite(18, LOW);
    Serial.println(analogRead(32));
    
    //ledcSetup(0, pwmFreq, pwmResolution);
    //ledcSetup(1, pwmFreq, pwmResolution);

  // 将PWM通道与引脚关联
    //ledcAttachPin(en1, 0);
    //ledcAttachPin(en2, 1);

}
    
    // 初始化其他设置


bool camera = false;
bool campwr = false;
bool wifi = false;
bool serveron = false;
bool rainsensor = true;
bool motortrue = false;
bool reverseMotor = false;


bool isValidIPv4(String str) {
    int parts[4];
    int count = sscanf(str.c_str(), "%d.%d.%d.%d", &parts[0], &parts[1], &parts[2], &parts[3]);
    
    if (count != 4) {
        return false;
    }
    
    for (int i = 0; i < 4; i++) {
        if (parts[i] < 0 || parts[i] > 255) {
            return false;
        }
    }
    
    return true;
}

void loop() {
    //String ipv4Address
    //Serial.println(ESP.getFreeHeap());
    //Serial.println(motortrue);

    digitalWrite(12, LOW);
    if (!camera && Serial2.available()) {
        String receivedString = Serial2.readStringUntil('\n');
        receivedString.trim();

        Serial.println(receivedString);
        
        // 创建一个正则表达式模式，匹配IPv4地址
        std::regex ipv4Pattern("\\b(?:\\d{1,3}\\.){3}\\d{1,3}\\b");

        // 将String转换为std::string，以便在正则表达式中使用
        std::string str = receivedString.c_str();

        // 在字符串中查找匹配的IPv4地址
        std::smatch matches;
        if (std::regex_search(str, matches, ipv4Pattern)) {
            ipv4Address = matches[0].str().c_str();
            if (isValidIPv4(ipv4Address)) {
                Serial.println("Found and valid IPv4 address: " + ipv4Address);
                lcd.clear();
                lcd.setCursor(0,0);
                lcd.print("Camera Ready!CmrIP:");
                lcd.setCursor(0,1);
                lcd.print(ipv4Address);
                camera = true;
                campwr = true;
                analogWrite(5, 220);
                delay(500);
                analogWrite(5, 0);
                delay(1000);
            }
        } else {
          if (receivedString.indexOf("clock") != -1) {
            counting = true;
            startTime = millis();
          }
    
          // 检测到"triggered"或"Ready"停止计时
          if (receivedString.indexOf("triggered") != -1 || receivedString.indexOf("Ready") != -1 || camera == true) {
            counting = false;
            currentTime = 0;
          }

          if (receivedString.indexOf("triggered") != -1) {
            lcd.clear();
            lcd.setCursor(0, 0);
            lcd.print("Insufficient pwr to start the cam");
            //digitalWrite(12, HIGH);
            //delay(2000);
            //lcd.clear();
            camera = true;
            counting = false;
            campwr = false;
            //Serial2.println("reset");
            //lcd.print("Starting...");
            //digitalWrite(12, LOW);
            //return;
          }
  

  // 如果计时达到10秒，执行操作
          //if (counting == true) {
          //  currentTime = millis() - startTime;
           // if (currentTime >= timeout) {
           //   Serial.println("Cam did't start correctly");
           //   lcd.clear();
           //   lcd.print("Camera no respond");
           //   delay(2000);
           //   reset();
           //   return;
          //  }
          }
        
        
    } else {
        if (!camera) {
          return;
        }
      }

  if (camera == true && wifi == false) {
    // 连接到WiFi网络
    WiFi.begin(ssid, pwd);

    // 等待连接
    while (WiFi.status() != WL_CONNECTED) {
        delay(1000);
        Serial.println("Connecting to WiFi...");
    }

    // 打印本地IP地址
    Serial.print("Connected to WiFi. Local IP address: ");
    Serial.println(WiFi.localIP());
    localIP = WiFi.localIP();
    wifi = true;
    if (!serveron) {
      beginserver();
    }
    lcd.setCursor(0,2);
    lcd.print("WiFi Connected");
    lcd.setCursor(0,3);
    lcd.print("IP:" + localIP.toString());
  }

    if (camera == true && rainsensor == true) {
      int rainValue = analogRead(32);
      //Serial.println("Rain Value: " + String(rainValue));
      if (rainValue < threshold) {
        if (!belowThreshold) {
            // 当模拟值低于阈值时，开始计时
            belowThreshold = true;
            rainstartTime = millis();
        } else {
            // 检查是否持续低于阈值达到所需时间
            if (millis() - rainstartTime >= requiredDuration) {
                Serial.println("Analog value below threshold for 3 seconds.");
                digitalWrite(13, HIGH);
                //noInterrupts();  // 禁用中断
                motortrue = true;
                //interrupts();  // 启用中断
                if (wifi == true) {
                  HTTPClient http; 
                  http.begin("http://xdroid.net/api/message?k=" + String(notificationAPI) + "&t=%E9%99%8D%E9%9B%A8%E8%AD%A6%E6%8A%A5&c=from+%E6%99%BE%E8%A1%A3%E6%9E%B6&u=http://" + localIP.toString());
                  Serial.println("http://xdroid.net/api/message?k=" + String(notificationAPI) + "&t=%E9%99%8D%E9%9B%A8%E8%AD%A6%E6%8A%A5&c=from+%E6%99%BE%E8%A1%A3%E6%9E%B6&u=http://" + localIP.toString());
                  int httpCode = http.GET();
                    if (httpCode > 0) {
                    String payload = http.getString();
                    Serial.println(httpCode);
                    Serial.println(payload);
                    }
                  }
                // 在这里执行你希望在满足条件后执行的操作
                delay(3000);
            }
        }
    } else {
        // 当模拟值高于阈值时，重置状态
        belowThreshold = false;
        digitalWrite(13, LOW);
        noInterrupts();  // 禁用中断
        motortrue = false;
        interrupts();  // 启用中断
        //Serial.println("motortrue is now: " + String(motortrue));
    }
    }

  if (wifi) {
    if (digitalRead(35)) {
      Serial.print("yaya");
      getWeather();
      rainsensor = true;
    }
}
    if (motortrue == true) {
      if (motorStartTime == 0) {
            motorStartTime = millis();
        }

        // 检查电机运行时间是否超过 5 秒
        if (millis() - motorStartTime < motorDuration) {
            digitalWrite(14, HIGH);
            analogWrite(en1, 80);
            analogWrite(en2, 90);
            digitalWrite(motor1A, HIGH);
            digitalWrite(motor1B, LOW);
            digitalWrite(motor2A, HIGH);
            digitalWrite(motor2B, LOW);
            Serial.println("motortrue Started");
        } else {
            // 关闭电机
            digitalWrite(14, LOW);
            analogWrite(en1, 0);
            analogWrite(en2, 0);
            digitalWrite(motor1A, LOW);
            digitalWrite(motor1B, LOW);
            digitalWrite(motor2A, LOW);
            digitalWrite(motor2B, LOW);

            // 重置计时器
            motorStartTime = 0;
            
            // 将 motortrue 设置为 false，以便下次触发时可以重新开启
            //noInterrupts();
            motortrue = false;
            rainsensor = false;
            //interrupts();
        }
    } else {
      if (reverseMotor == true) {
            if (motorStartTime == 0) {
            motorStartTime = millis();
        }

        // 检查电机运行时间是否超过 5 秒
        if (millis() - motorStartTime < motorDuration) {
            digitalWrite(14, HIGH);
            analogWrite(en1, 80);
            analogWrite(en2, 90);
            digitalWrite(motor1A, LOW);
            digitalWrite(motor1B, HIGH);
            digitalWrite(motor2A, LOW);
            digitalWrite(motor2B, HIGH);
            Serial.println("motortrue Started");
        } else {
            // 关闭电机
            digitalWrite(14, LOW);
            analogWrite(en1, 0);
            analogWrite(en2, 0);
            digitalWrite(motor1A, LOW);
            digitalWrite(motor1B, LOW);
            digitalWrite(motor2A, LOW);
            digitalWrite(motor2B, LOW);

            // 重置计时器
            motorStartTime = 0;
            
            // 将 motortrue 设置为 false，以便下次触发时可以重新开启
            //noInterrupts();
            reverseMotor = false;
            rainsensor = false;
            //interrupts();
        }
      } else {
            digitalWrite(14, LOW);
            analogWrite(en1, 0);
            analogWrite(en2, 0);
            digitalWrite(motor1A, LOW);
            digitalWrite(motor1B, LOW);
            digitalWrite(motor2A, LOW);
            digitalWrite(motor2B, LOW);

    }
    }
    

    // 其他代码和延时
    if(wifi && camera) {
      delay(5000);
      mainPage();
    }
}

void Detection() {
  unsigned long currentTime = millis();

  if (currentTime - lastPingTime >= pingInterval) {
    lastPingTime = currentTime;
    const char* targetIP = ipv4Address.c_str();
    bool pingResult = isCAMOnline(targetIP);
  if (pingResult == false) {
  digitalWrite(14, HIGH);
  Serial.println("Restarting cam in 3s");
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print("Cam not responding");
  lcd.setCursor(0,1);
  lcd.print("Restarting cam...");
  delay(3000);
  digitalWrite(12, HIGH);
  Serial2.println("reset");
  camera = false;
  campwr = false;
  lcd.clear();
  lcd.print("Starting...");
  return;

  } else {
  digitalWrite(14, LOW);
  }
  delay(5000);
  return;
}
}

bool isCAMOnline(const char* ip) {
  if (Ping.ping(ip)) {
    Serial.println("Ping 成功！");
    return true;
  } else {
    Serial.println("Ping 失败！");
    return false;
  }
}

void reset() {
digitalWrite(18, HIGH);
delay(1000);
digitalWrite(18,LOW);
}

void beginserver() {
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
        // 使用Processor对象注入全局变量到HTML页面
        String html = String(INDEX_HTML);
        html.replace("{globalVariable}", String(ipv4Address));
        
        request->send(200, "text/html", html);
    });

  server.on("/submit", HTTP_POST, [](AsyncWebServerRequest *request){
    // 处理表单提交
    if (request->hasParam("data", true)) {
        inputData = request->getParam("data", true)->value();
        Serial.println("Received data: " + String(inputData));
        verifyAPI();
        
        // 发送带有数据的 JSON 响应
        String jsonResponse = "{\"success\":true, \"message\":\"Data received.\"}";
        request->send(200, "application/json", jsonResponse);
    } else {
        request->send(400, "text/plain", "Bad Request");
    }
});

    server.on("/update", HTTP_GET, [](AsyncWebServerRequest *request){
        String humidity = DHThum();
        String temperature = DHTtemp();
        // 构建JSON响应
        String jsonResponse = "{\"humidity\":" + String(humidity) + ",\"temperature\":" + String(temperature) + "}";
        request->send(200, "application/json", jsonResponse);
    });

  server.on("/motor", HTTP_GET, [](AsyncWebServerRequest *request){
    if (request->hasParam("value")) {
      String value = request->getParam("value")->value();
      if (value == "true") {
        Serial.println("hi");
        rainsensor = false;
        motortrue = true;
      } else {
        if (value == "false") {
          rainsensor = false;
          motortrue = false;
        }
      }
    }
    request->send(200, "text/plain", "OK"); // 返回响应
   });


    server.on("/reset", HTTP_POST, [](AsyncWebServerRequest *request){
    request->send(200, "text/plain", "Resetting...");  // 等待 1 秒，确保响应被发送
    reset();  // 重启 ESP32
    });

    server.on("/setRainSensor", HTTP_GET, [](AsyncWebServerRequest *request) {
    noInterrupts();  // 禁用中断
    rainsensor = true;  // 设置 rainsensor 为 true
    interrupts();  // 启用中断
    request->send(200, "text/plain", "OK");  // 返回响应
  });

  server.on("/reverseMotor", HTTP_GET, [](AsyncWebServerRequest *request) {
    //noInterrupts();  // 禁用中断
    rainsensor = false;
    reverseMotor = true;  // 调用反转电机的函数
    //interrupts();  // 启用中断
    request->send(200, "text/plain", "OK");  // 返回响应
  });



  // 开启Web服务器
  serveron = true;
  server.begin();
  
}

String DHThum() {
  float humidity = dht.readHumidity();
  return String(humidity);
}

String DHTtemp() {
  float temperature = dht.readTemperature();
  return String(temperature);
}

void getWeather() {
  Serial.println("geting...");
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Getting info...");
  digitalWrite(14,  HIGH);
  HTTPClient http;
  String url = String(endpoint) + "?apikey=" + String(apiKey);
  http.begin(url);

  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    // Process the JSON payload
    analyzeWeatherData(payload);
  } else {
    Serial.println("Failed to connect to AccuWeather");
    digitalWrite(14, LOW);
  }
  http.end();
  return;
}

void analyzeWeatherData(String data) {
  digitalWrite(14, LOW);
  const size_t capacity = JSON_ARRAY_SIZE(12) + 12 * JSON_OBJECT_SIZE(10) + 500;
  DynamicJsonDocument doc(capacity);

  deserializeJson(doc, data);

  JsonArray hourlyData = doc.as<JsonArray>();

  bool willRainInNextThreeHours = false;

  // Check if there is any precipitation possibility in the next three hours
  for (int i = 0; i < 3 && i < hourlyData.size(); ++i) {
    bool hasPrecipitation = hourlyData[i]["HasPrecipitation"];
    if (hasPrecipitation) {
      willRainInNextThreeHours = true;
      break;
    }
  }

  if (!willRainInNextThreeHours) {
    Serial.println("Twelve hours without precipitation, suitable for drying clothes.");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print("   ---Suitable---   ");
    lcd.setCursor(0, 1);
    lcd.print("For drying clothes");
    lcd.setCursor(0, 2);
    //lcd.print("within 12 hours");
    analogWrite(5, 220);
    delay(500);
    analogWrite(5, 0);

    // 计算降水前的剩余小时数
    int beforePrecipCount = 0;
  
  for (JsonObject forecast : hourlyData) {
    if (forecast["HasPrecipitation"] == true) {
      // 如果遇到 HasPrecipitation 为 true，终止循环
      break;
    }

    // 统计 HasPrecipitation 为 true 之前的对象数量
    beforePrecipCount++;
  }

  int remainingHours = beforePrecipCount / 2;

    lcd.setCursor(0, 3);
 //   lcd.print("Remaining hours: " + String(remainingHours));
    delay(5000);
    mainPage();
    return;

  } else {
    // Find the hours with precipitation
    Serial.println("Not suitable for drying clothes. Precipitation expected at:");
    lcd.clear();
    lcd.setCursor(0, 0);
    lcd.print(" ---Not Suitable--- ");
    lcd.setCursor(0, 1);
    lcd.print("For drying clothes");
    lcd.setCursor(0, 2);
    lcd.print(" -Rain expected at-");
    analogWrite(5, 220);
    delay(100);
    analogWrite(5, 0);
    delay(100);
    analogWrite(5, 220);
    delay(100);
    analogWrite(5, 0);
    delay(100);
    analogWrite(5, 220);
    delay(100);
    analogWrite(5, 0);
    delay(100);
    analogWrite(5, 220);
    delay(100);
    analogWrite(5, 0);
    delay(100);
    analogWrite(5, 220);
    delay(100);
    analogWrite(5, 0);

    for (int i = 0; i < hourlyData.size(); ++i) {
      bool hasPrecipitation = hourlyData[i]["HasPrecipitation"];
      if (hasPrecipitation) {
        String dateTime = hourlyData[i]["DateTime"];
        Serial.println(" - " + dateTime);
        String timePart = dateTime.substring(11, 16);
        lcd.setCursor(0, 3);
        lcd.print("       " + String(timePart));
        delay(5000);
        mainPage();
        return;
      }
    }
  }
}

void verifyAPI() {
  //notificationAPI = inputData;
  //Serial.println(notificationAPI);
    HTTPClient http;

  http.begin("https://xdroid.net/api/message?k=" + String(inputData) + "&t=API%E9%AA%8C%E8%AF%81%E9%80%9A%E8%BF%87%EF%BC%81&c=API%E9%80%9A%E8%BF%87%E9%AA%8C%E8%AF%81%EF%BC%8C%E4%B9%8B%E5%90%8E%E7%9A%84%E9%80%9A%E7%9F%A5%E5%B0%86%E4%BC%9A%E5%8F%91%E9%80%81%E5%88%B0%E8%BF%99%E9%83%A8%E8%AE%BE%E5%A4%87&u=http://" + localIP.toString());
  int httpCode = http.GET();
  if (httpCode > 0) {
    String payload = http.getString();
    Serial.println(httpCode);
    Serial.println(payload);
    if (httpCode == 200){
        //HTTPClient http;
        verifyPass();
        //http.end();
        //return;
      } else {
        Serial.println("is not valid");
        // 这里可以添加处理无效payload的逻辑
      }
    }
  
  
  http.end();
  return;
}

// 从EEPROM读取字符串数据
String readData() {
  char buffer[EEPROM_SIZE + 1];  // 为字符串留一个额外的空间，以存储终止符
  for (int i = 0; i < EEPROM_SIZE; ++i) {
    buffer[i] = EEPROM.read(i);
  }
  buffer[EEPROM_SIZE] = '\0';  // 添加字符串终止符
  Serial.println("ReadData" + String(buffer));
  return String(buffer);
}

// 将字符串写入EEPROM
void writeData(const String& data) {
  for (int i = 0; i < data.length(); ++i) {
    EEPROM.write(i, data[i]);
  }
  EEPROM.commit();
  Serial.println("Data written");
}

void verifyPass() {
  notificationAPI = inputData;
  MAX_API_DATA_LENGTH = notificationAPI.length();
  clearUnusedEEPROM();
  writeData(notificationAPI);
  return;
}

void clearUnusedEEPROM() {
  for (int i = MAX_API_DATA_LENGTH; i < EEPROM_SIZE; ++i) {
    EEPROM.write(i, 0);  // 将未使用的部分写入零值
  }
  EEPROM.commit();
  Serial.println(MAX_API_DATA_LENGTH);
}

void mainPage() {
  float humidity = dht.readHumidity();
  float temperature = dht.readTemperature();
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(" ---Drying Rack--- ");
  lcd.setCursor(0,1);
  lcd.print("Humidity: " + String(humidity) + "%");
  lcd.setCursor(0,2);
  lcd.print("Temp: " + String(temperature) + "°C");
  lcd.setCursor(0,3);
  lcd.print("IP: " + localIP.toString());
  //delay(1000);
  return;

}

void motor(String value) {
  if (value == "true") {
    motortrue = true;
  } else {
    if (value == "false") {
      motortrue = false;
    }
  }
}