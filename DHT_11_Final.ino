#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>
#include <DHT.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

//——— Hardware Definitions —————————————————————————————
#define DHT_SENSOR_PIN 4
#define DHT_SENSOR_TYPE DHT11
DHT dht(DHT_SENSOR_PIN, DHT_SENSOR_TYPE);

LiquidCrystal_I2C lcd(0x27, 16, 2); // I2C addr, cols, rows

//——— Wi-Fi & Firebase Credentials ————————————————————————
#define WIFI_SSID     "MAX"
#define WIFI_PASSWORD "12345678"

#define API_KEY       "AIzaSyDiXngppGpEtZThuRJGnzfgIY4Q9213-qI"
#define DATABASE_URL  "https://esp32-6afd8-default-rtdb.firebaseio.com"

#define USER_EMAIL    "abc@gmail.com"
#define USER_PASSWORD "12345678"

//——— Firebase Objects ————————————————————————————————
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

unsigned long lastMillis = 0;

//——— Token Helper Functions ——————————————————————————
const char* getTokenType(TokenInfo info) {
  switch (info.type) {
    case token_type_undefined:           return "undefined";
    case token_type_legacy_token:        return "legacy token";
    case token_type_id_token:            return "id token";
    case token_type_custom_token:        return "custom token";
    case token_type_oauth2_access_token: return "oauth2 access";
    default:                             return "unknown";
  }
}

const char* getTokenStatus(TokenInfo info) {
  switch (info.status) {
    case token_status_uninitialized:  return "uninitialized";
    case token_status_on_initialize:  return "initializing";
    case token_status_on_signing:     return "signing";
    case token_status_on_request:     return "requesting";
    case token_status_on_refresh:     return "refreshing";
    case token_status_ready:          return "ready";
    case token_status_error:          return "error";
    default:                          return "unknown";
  }
}

void tokenStatusCallback(TokenInfo info) {
  Serial.printf("⚙ Token: type=%s, status=%s\n",
                getTokenType(info),
                getTokenStatus(info));
}

//——— Setup ——————————————————————————————————————————
void setup() {
  Serial.begin(115200);

  // DHT11
  dht.begin();

  // LCD
  lcd.init();
  lcd.backlight();
  lcd.setCursor(0,0);
  lcd.print("Connecting WiFi");

  // Wi-Fi
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  while (WiFi.status() != WL_CONNECTED) {
    lcd.print(".");
    delay(300);
  }
  lcd.clear();
  lcd.print("IP:");
  lcd.setCursor(3,0);
  lcd.print(WiFi.localIP().toString());

  // Firebase config
  config.api_key            = API_KEY;
  config.database_url       = DATABASE_URL;
  config.token_status_callback = tokenStatusCallback;

  // **Sign in** existing user—no more EMAIL_EXISTS
  auth.user.email    = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Initialize Firebase
  Firebase.begin(&config, &auth);
  Firebase.reconnectWiFi(true);

  // Give time for first token event to appear
  delay(1500);
  lcd.clear();
}

//——— Main Loop ——————————————————————————————————————
void loop() {
  // Read sensor
  float temperature = dht.readTemperature();
  float humidity    = dht.readHumidity();

  if (isnan(temperature) || isnan(humidity)) {
    Serial.println("DHT error!");
    return;
  }

  // Send every 1 second
  if (Firebase.ready() && (millis() - lastMillis > 2000)) {
    lastMillis = millis();

    // Firebase writes
    if (!Firebase.RTDB.setFloat(&fbdo, "/DHT11/Temperature", temperature))
      Serial.println("Temp send err: " + fbdo.errorReason());
    if (!Firebase.RTDB.setFloat(&fbdo, "/DHT11/Humidity", humidity))
      Serial.println("Hum send err: " + fbdo.errorReason());

    // Serial debug
    Serial.printf("T=%.1fC  H=%.1f%%\n", temperature, humidity);

    // Update LCD
    lcd.clear();
    lcd.setCursor(0,0);
    lcd.printf("T:%.1fC", temperature);
    lcd.setCursor(0,1);
    lcd.printf("H:%.1f%%", humidity);
    delay(1000);
  }
}

