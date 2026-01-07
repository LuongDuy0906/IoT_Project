#define BLYNK_TEMPLATE_ID "TMPL6hF8nEFm7" 
#define BLYNK_TEMPLATE_NAME "Quickstart Template"
#define BLYNK_AUTH_TOKEN "kyOo8itDymWdC0IIM9HFzxy8ao7QnNyM" 

#include <WiFi.h>
#include <WiFiClient.h>
#include <DHT.h> 
#include <BlynkSimpleEsp32.h> 

#define BLYNK_PRINT Serial1

char ssid[] = "My Phone"; 
char pass[] = "88888888"; 

BlynkTimer timer;

// ==========================================================
// 2. KHAI BÁO CHÂN GPIO
// ==========================================================

// --- Cảm biến Analog ---
const int MOISTURE_PIN = 35; 
const int PH_PIN = 32;       
const int EC_PIN = 33;       
const int NPK_PIN = 34;      `      

// --- Cảm biến Digital ---
const int LDR_DO_PIN = 27; 
#define DHT_PIN 16 
#define DHT_TYPE DHT11
DHT dht(DHT_PIN, DHT_TYPE);

// --- Thiết bị đầu ra (Relay) ---
const int PUMP_PIN = 14; 
const int FAN_PIN = 12; 
const int STATUS_LED_PIN = 15; 

// --- 9 ĐÈN LED TRẠNG THÁI ---
// Nhóm pH
const int LED_PH_R = 18; const int LED_PH_Y = 19; const int LED_PH_G = 21;
// Nhóm EC
const int LED_EC_R = 4;  const int LED_EC_Y = 2;  const int LED_EC_G = 5;
// Nhóm NPK
const int LED_NPK_R = 22; const int LED_NPK_Y = 23; const int LED_NPK_G = 13;

// --- Virtual Pins Blynk ---
#define VPIN_TEMP V1
#define VPIN_HUMIDITY V2
#define VPIN_MOISTURE V3
#define VPIN_PH V5
#define VPIN_EC V6
#define VPIN_NPK V7
#define VPIN_STATUS_PUMP V8 
#define VPIN_STATUS_FAN V9
#define VPIN_STATUS_LIGHT V10 
#define VPIN_MANUAL_LIGHT V11 
#define VPIN_MANUAL_FAN V12 

const float TEMP_HIGH_THRESHOLD = 30.0; 
const int MOISTURE_DRY_THRESHOLD = 3000; 
const int MOISTURE_WET_THRESHOLD = 2000; 
const int LDR_DARK_STATE = HIGH; 

bool manualLightOverride = false; 
bool manualFanOverride = false;

// ==========================================================
// 3. HÀM ĐIỀU KHIỂN ĐÈN TRẠNG THÁI (3 MỨC)
// ==========================================================

void updateLEDGroup(int value, int pinR, int pinY, int pinG, int low, int high) {
    if (value < low) { // THẤP
        digitalWrite(pinR, HIGH); digitalWrite(pinY, LOW); digitalWrite(pinG, LOW);
    } 
    else if (value >= low && value < high) { // TRUNG BÌNH
        digitalWrite(pinR, LOW); digitalWrite(pinY, HIGH); digitalWrite(pinG, LOW);
    } 
    else { // TỐT
        digitalWrite(pinR, LOW); digitalWrite(pinY, LOW); digitalWrite(pinG, HIGH);
    }
}

// ==========================================================
// 4. HÀM LOGIC ĐIỀU KHIỂN TỰ ĐỘNG
// ==========================================================

void runSmartFarmLogic() {
    float temp = dht.readTemperature();
    int moisture = analogRead(MOISTURE_PIN);
    int light_digital = digitalRead(LDR_DO_PIN);

    // Đọc giá trị raw để cập nhật LED
    int ph_val = analogRead(PH_PIN);
    int ec_val = analogRead(EC_PIN);
    int npk_val = analogRead(NPK_PIN);

    // Cập nhật 9 đèn LED (Ngưỡng có thể điều chỉnh lại cho phù hợp)
    updateLEDGroup(ph_val,  LED_PH_R,  LED_PH_Y,  LED_PH_G,  1000, 2800);
    updateLEDGroup(ec_val,  LED_EC_R,  LED_EC_Y,  LED_EC_G,  1500, 3200);
    updateLEDGroup(npk_val, LED_NPK_R, LED_NPK_Y, LED_NPK_G, 800,  2500);

    if (isnan(temp)) return; 

    // Logic Quạt
    if (!manualFanOverride) {
        digitalWrite(FAN_PIN, (temp > TEMP_HIGH_THRESHOLD) ? HIGH : LOW);
        Blynk.virtualWrite(VPIN_STATUS_FAN, digitalRead(FAN_PIN));
    }

    // Logic Bơm
    if (moisture > MOISTURE_DRY_THRESHOLD) digitalWrite(PUMP_PIN, HIGH);
    else if (moisture < MOISTURE_WET_THRESHOLD) digitalWrite(PUMP_PIN, LOW);
    Blynk.virtualWrite(VPIN_STATUS_PUMP, digitalRead(PUMP_PIN));

    // Logic Đèn chiếu sáng
    if (!manualLightOverride) {
        digitalWrite(STATUS_LED_PIN, (light_digital == LDR_DARK_STATE) ? HIGH : LOW);
        Blynk.virtualWrite(VPIN_STATUS_LIGHT, digitalRead(STATUS_LED_PIN));
    }
}

// ==========================================================
// 5. HÀM GỬI DỮ LIỆU CẢM BIẾN
// ==========================================================

void sendSensorDataToBlynk() {
    Serial.println("\n--- MONITORING (AIR & SOIL) ---");

    float temp = dht.readTemperature();
    float hum = dht.readHumidity();
    if (!isnan(temp)) {
        Blynk.virtualWrite(VPIN_TEMP, temp);
        Blynk.virtualWrite(VPIN_HUMIDITY, hum);
        Serial.print("Nhiệt độ khí: "); Serial.print(temp); Serial.println(" *C");
    }

    int ph_raw = analogRead(PH_PIN);
    Blynk.virtualWrite(VPIN_PH, ph_raw);
    Serial.print("pH Raw: "); Serial.println(ph_raw);

    int ec_raw = analogRead(EC_PIN);
    Blynk.virtualWrite(VPIN_EC, ec_raw);
    Serial.print("EC Raw: "); Serial.println(ec_raw);

    int npk_raw = analogRead(NPK_PIN);
    Blynk.virtualWrite(VPIN_NPK, npk_raw);
    Serial.print("NPK Raw: "); Serial.println(npk_raw);

    int m_raw = analogRead(MOISTURE_PIN);
    Blynk.virtualWrite(VPIN_MOISTURE, m_raw); 
    Serial.print("Độ ẩm đất Raw: "); Serial.println(m_raw);

    Serial.println("--------------------------------");
}

BLYNK_WRITE(VPIN_MANUAL_LIGHT) {
    manualLightOverride = (param.asInt() == HIGH); 
    digitalWrite(STATUS_LED_PIN, param.asInt());
}

BLYNK_WRITE(VPIN_MANUAL_FAN) {
    manualFanOverride = (param.asInt() == HIGH); 
    digitalWrite(FAN_PIN, param.asInt());
}

BLYNK_CONNECTED() {
    Blynk.syncAll();
}

void setup() {
    Serial.begin(115200);

    // Cấu hình chân Output cho Relay và 9 LED
    int outputPins[] = {PUMP_PIN, FAN_PIN, STATUS_LED_PIN, 18, 19, 21, 4, 2, 5, 22, 23, 13};
    for(int i=0; i < 12; i++) {
        pinMode(outputPins[i], OUTPUT);
        digitalWrite(outputPins[i], LOW);
    }
    
    pinMode(LDR_DO_PIN, INPUT);
    dht.begin();

    Blynk.begin(BLYNK_AUTH_TOKEN, ssid, pass);

    timer.setInterval(1000L, runSmartFarmLogic); 
    timer.setInterval(3000L, sendSensorDataToBlynk); 
}

void loop() {
    Blynk.run();
    timer.run();
}