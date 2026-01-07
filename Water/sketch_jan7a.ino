#define BLYNK_TEMPLATE_ID "TMPL6hF8nEFm7" 
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "kyOo8itDymWdC0IIM9HFzxy8ao7QnNyM" 

#include <WiFi.h>
#include <WiFiClient.h>
#include <BlynkSimpleEsp32.h> 
#include <OneWire.h>
#include <DallasTemperature.h>

#define BLYNK_PRINT Serial

// Cấu hình WiFi
char ssid[] = "My Phone"; 
char pass[] = "88888888"; 

BlynkTimer timer;

// --- Chân cắm cảm biến ---
const int PH_WATER_PIN = 32;       
const int EC_WATER_PIN = 33;       
#define ONE_WIRE_BUS 5  // DS18B20 nối GPIO 4

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

// --- Chân cắm Relay ---
const int WATER_FAN_PIN = 14;  
const int WATER_PUMP_PIN = 27; 

// --- 6 LED trạng thái ---
const int LED_PH_R = 18; const int LED_PH_Y = 19; const int LED_PH_G = 21;
const int LED_EC_R = 22; const int LED_EC_Y = 23; const int LED_EC_G = 25;

// --- Chân ảo (Virtual Pins) trên Blynk ---
#define VPIN_WATER_TEMP V20
#define VPIN_WATER_PH V21
#define VPIN_WATER_EC V22
#define VPIN_WATER_FAN_STATUS V23
#define VPIN_WATER_PUMP_STATUS V24
#define VPIN_CONTROL_FAN V25
#define VPIN_CONTROL_PUMP V26

// --- Ngưỡng điều khiển ---
const float WATER_TEMP_HIGH = 28.0; 
const int PH_LOW_THRESHOLD = 1500; // Giá trị Analog thô để kích hoạt cảnh báo/bơm
const int EC_LOW_THRESHOLD = 1500;

bool isManualFan = false;  
bool isManualPump = false; 

// ==========================================================
// ĐIỀU KHIỂN TỪ APP BLYNK
// ==========================================================

BLYNK_WRITE(VPIN_CONTROL_FAN) {
    isManualFan = param.asInt(); 
    digitalWrite(WATER_FAN_PIN, isManualFan);
    Serial.print("Quạt Nước: "); Serial.println(isManualFan ? "BẬT" : "TẮT");
}

BLYNK_WRITE(VPIN_CONTROL_PUMP) {
    isManualPump = param.asInt(); 
    digitalWrite(WATER_PUMP_PIN, isManualPump);
    Serial.print("Bơm Nước: "); Serial.println(isManualPump ? "BẬT" : "TẮT");
}

// ==========================================================
// HÀM XỬ LÝ LED VÀ LOGIC
// ==========================================================

void updateLEDGroup(int value, int pinR, int pinY, int pinG, int low, int high) {
    if (value < low) { 
        digitalWrite(pinR, HIGH); digitalWrite(pinY, LOW); digitalWrite(pinG, LOW);
    } 
    else if (value >= low && value < high) { 
        digitalWrite(pinR, LOW); digitalWrite(pinY, HIGH); digitalWrite(pinG, LOW);
    } 
    else { 
        digitalWrite(pinR, LOW); digitalWrite(pinY, LOW); digitalWrite(pinG, HIGH);
    }
}

void runWaterLogic() {
    Serial.println("\n--- ĐANG CẬP NHẬT DỮ LIỆU ---");

    // 1. Xử lý Nhiệt độ (DS18B20)
    sensors.requestTemperatures(); 
    float wTemp = sensors.getTempCByIndex(0);
    
    if (wTemp != DEVICE_DISCONNECTED_C) {
        Serial.print("> Nhiệt độ: "); Serial.print(wTemp); Serial.println(" °C");
        Blynk.virtualWrite(VPIN_WATER_TEMP, wTemp);
        
        if (!isManualFan) {
            digitalWrite(WATER_FAN_PIN, (wTemp > WATER_TEMP_HIGH) ? HIGH : LOW);
        }
    } else {
        Serial.println("> Lỗi: Không tìm thấy DS18B20!");
    }

    // 2. Đọc biến trở pH (Chân 32)
    int ph_raw = analogRead(PH_WATER_PIN);
    float ph_display = ph_raw; // Chuyển sang thang 0-14 pH
    
    // LOG pH ra Serial Monitor
    Serial.print("> pH Analog: "); Serial.print(ph_raw); 
    Serial.print(" | pH quy đổi: "); Serial.println(ph_display);

    // 3. Đọc biến trở EC (Chân 33)
    int ec_raw = analogRead(EC_WATER_PIN);
    float ec_display = ec_raw; // Chuyển sang thang 0-2000 µS/cm
    
    // LOG EC ra Serial Monitor
    Serial.print("> EC Analog: "); Serial.print(ec_raw); 
    Serial.print(" | EC quy đổi: "); Serial.println(ec_display);

    // 4. Gửi dữ liệu lên Blynk
    Blynk.virtualWrite(VPIN_WATER_PH, ph_display);
    Blynk.virtualWrite(VPIN_WATER_EC, ec_display);

    // 5. Cập nhật hệ thống LED vật lý
    updateLEDGroup(ph_raw, LED_PH_R, LED_PH_Y, LED_PH_G, 1000, 2800);
    updateLEDGroup(ec_raw, LED_EC_R, LED_EC_Y, LED_EC_G, 1500, 3200);

    // 6. Tự động bật Bơm nếu nồng độ thấp (Nếu không ở chế độ tay)
    if (!isManualPump) {
        if (ph_raw < PH_LOW_THRESHOLD || ec_raw < EC_LOW_THRESHOLD) {
            digitalWrite(WATER_PUMP_PIN, HIGH);
        } else {
            digitalWrite(WATER_PUMP_PIN, LOW);
        }
    }
    
    // 7. Phản hồi trạng thái thiết bị
    Blynk.virtualWrite(VPIN_WATER_FAN_STATUS, digitalRead(WATER_FAN_PIN));
    Blynk.virtualWrite(VPIN_WATER_PUMP_STATUS, digitalRead(WATER_PUMP_PIN));
    
    Serial.print("> Quạt: "); Serial.print(digitalRead(WATER_FAN_PIN) ? "ON" : "OFF");
    Serial.print(" | Bơm: "); Serial.println(digitalRead(WATER_PUMP_PIN) ? "ON" : "OFF");
    Serial.println("-----------------------------");
}

BLYNK_CONNECTED() {
    Blynk.syncAll(); 
}

void setup() {
    Serial.begin(115200);
    
    // Cấu hình chân ra cho Relay và LED
    int outPins[] = {WATER_FAN_PIN, WATER_PUMP_PIN, 18, 19, 21, 22, 23, 25};
    for(int i=0; i<8; i++) {
        pinMode(outPins[i], OUTPUT);
        digitalWrite(outPins[i], LOW);
    }
    
    sensors.begin();
    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);
    
    // Chạy hàm logic mỗi 2 giây
    timer.setInterval(2000L, runWaterLogic); 
}

void loop() {
    Blynk.run();
    timer.run();
}