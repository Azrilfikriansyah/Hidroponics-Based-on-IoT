#include <SoftwareSerial.h>
#include <Arduino.h>
#include <WiFi.h>
#include <Firebase_ESP_Client.h>

// SoftwareSerial untuk komunikasi dengan perangkat sensor dari nano
SoftwareSerial temp(35, 34); // RX, TX

// Define WiFi credentials
#define WIFI_SSID "Goceng Dulu."
#define WIFI_PASSWORD "Hesesare2022"

// #define WIFI_SSID "Azril"
// #define WIFI_PASSWORD "11111111"

// #define WIFI_SSID "Lunaryx"
// #define WIFI_PASSWORD "NinerNiner"

// Define Firebase API Key, Project ID, and user credentials
#define API_KEY "AIzaSyC-jw2lt2upg-wRcfdGJuUIGubZHu1uCB8"
#define FIREBASE_PROJECT_ID "verticalgardenapp"
#define USER_EMAIL "allenalvandanae1745@gmail.com"
#define USER_PASSWORD "Immortallove<3"

// Initialize Firebase Data object, Firebase authentication, and configuration
FirebaseData fbdo;
FirebaseAuth auth;
FirebaseConfig config;

// Control variables
bool isAutomaticMode = false;
float targetPPM = 0.0;
float targetVolume = 0.0;
const float AUTOMATIC_PPM = 950; // PPM value for automatic mode
const float AUTOMATIC_VOLUME = 20.0; // Volume value for automatic mode
const float AUTOMATIC_PH = 7.0; // pH value for automatic mode

// Define pins for pumps
#define NUTRIENT_PUMP_PIN 4
#define WATER_PUMP_PIN 15
#define PH_DOWN_PUMP_PIN 5
#define PH_UP_PUMP_PIN 19

// Sensor values
float temperature, humidity, tdsValue, phValue, distance;

void setup() {
  // Startup
  Serial.begin(9600);
  temp.begin(9600);

  // Connect to Wi-Fi
  connectToWiFi();

  // Connect to Firebase
  connectToFirebase();

  // Initialize pump pins
  pinMode(NUTRIENT_PUMP_PIN, OUTPUT);
  pinMode(WATER_PUMP_PIN, OUTPUT);
  pinMode(PH_DOWN_PUMP_PIN, OUTPUT);
  pinMode(PH_UP_PUMP_PIN, OUTPUT);

  digitalWrite(NUTRIENT_PUMP_PIN, HIGH);
  digitalWrite(WATER_PUMP_PIN, HIGH);
  digitalWrite(PH_DOWN_PUMP_PIN, HIGH);
  digitalWrite(PH_UP_PUMP_PIN, HIGH);
}

void connectToWiFi() {
  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  Serial.print("Connecting to Wi-Fi");
  while (WiFi.status() != WL_CONNECTED) {
    Serial.print(".");
    delay(300);
  }
  Serial.println();
  Serial.print("Connected with IP: ");
  Serial.println(WiFi.localIP());
}

void connectToFirebase() {
  // Print Firebase client version
  Serial.printf("Firebase Client v%s\n\n", FIREBASE_CLIENT_VERSION);

  // Assign the API key
  config.api_key = API_KEY;

  // Assign the user sign-in credentials
  auth.user.email = USER_EMAIL;
  auth.user.password = USER_PASSWORD;

  // Begin Firebase with configuration and authentication
  Firebase.begin(&config, &auth);

  // Reconnect to Wi-Fi if necessary
  Firebase.reconnectWiFi(true);
}

void uploadToFirebase() {
  FirebaseJson sensorData;

  // Create JSON structure for sensor data
  sensorData.set("fields/TEMPERATURE_c/stringValue", String(temperature, 0));
  sensorData.set("fields/HUMIDITY/stringValue", String(humidity, 0));
  sensorData.set("fields/TDS_ppm/stringValue", String(tdsValue, 0));
  sensorData.set("fields/pH/stringValue", String(phValue, 2));
  sensorData.set("fields/Distance/stringValue", String(distance, 0));

  // Print the JSON content to be uploaded for debugging
  Serial.print("Uploading sensor data: ");
  Serial.println(sensorData.raw());

  // Attempt to patch the document in Firestore
  String path = "/Sensor/Data";
  if (Firebase.Firestore.patchDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), sensorData.raw(), "TEMPERATURE_c,HUMIDITY,TDS_ppm,pH,Distance")) {
    Serial.println("Sensor data uploaded successfully.");
  } else {
    Serial.println("Failed to upload sensor data.");
    Serial.println("REASON: " + fbdo.errorReason());
  }
}

void readControlSettings() {
  // Read control settings from Firestore
  String path = "/Control/settings";
  Serial.print("Get control settings document... ");
  if (Firebase.Firestore.getDocument(&fbdo, FIREBASE_PROJECT_ID, "", path.c_str(), "")) {
    Serial.printf("ok\n");

    // Create a FirebaseJson object and set content with received payload
    FirebaseJson payload;
    payload.setJsonData(fbdo.payload().c_str());

    // Get the mode from FirebaseJson object 
    FirebaseJsonData jsonData;
    if (payload.get(jsonData, "fields/mode/stringValue", true)) {
      String mode = jsonData.stringValue;
      if (mode == "automatic") {
        isAutomaticMode = true;
      } else if (mode == "manual") {
        isAutomaticMode = false;
      }

      // Get the target PPM value from FirebaseJson object 
      if (payload.get(jsonData, "fields/targetPPM/stringValue", true)) {
        targetPPM = jsonData.stringValue.toFloat();
        Serial.print("target PPM: ");
        Serial.println(targetPPM);
      } else {
        Serial.println("Failed to get targetPPM from JSON.");
      }

      // Get the target Volume value from FirebaseJson object 
      if (payload.get(jsonData, "fields/targetVolume/stringValue", true)) {
        targetVolume = jsonData.stringValue.toFloat();
        Serial.print("target Volume: ");
        Serial.println(targetVolume);
      } else {
        Serial.println("Failed to get targetVolume from JSON.");
      }

      Serial.print("Mode: ");
      Serial.println(mode);
    } else {
      Serial.println("Failed to get mode from JSON.");
    }
  } else {
    Serial.print("Failed to read control settings: ");
    Serial.println(fbdo.errorReason());
  }
}

void controlPump() {
  float target_PPM = isAutomaticMode ? AUTOMATIC_PPM : targetPPM;
  float target_Volume = isAutomaticMode ? AUTOMATIC_VOLUME : targetVolume;

  // Logic to control pump based on targetPPM
  if (tdsValue < target_PPM) {
    // Turn on nutrient pump
    digitalWrite(NUTRIENT_PUMP_PIN, LOW);

  } else {
    // Turn off both pumps
    digitalWrite(NUTRIENT_PUMP_PIN, HIGH);

  }

  // Logic to control pump based on targetVolume
  if (distance > target_Volume) {
    // Turn on water pump
    digitalWrite(WATER_PUMP_PIN, LOW);
  } else {
    // Turn off water pump
    digitalWrite(WATER_PUMP_PIN, HIGH);
  }

  // Logic to control pump based on targetPH
  if (phValue < 6.0 ) {
    // Turn on pH up pump
    digitalWrite(PH_UP_PUMP_PIN, LOW);
    // Turn off pH down pump
    digitalWrite(PH_DOWN_PUMP_PIN, HIGH);
  } else if (phValue > 7.5) {
    // Turn on pH down pump+++
    digitalWrite(PH_DOWN_PUMP_PIN, LOW);
    // Turn off pH up pump
    digitalWrite(PH_UP_PUMP_PIN, HIGH);
  } else {
    // Turn off both pH pumps
    digitalWrite(PH_DOWN_PUMP_PIN, HIGH);
    digitalWrite(PH_UP_PUMP_PIN, HIGH);
  }
}

void readSensorData() {
  if (temp.available() > 0) {
    String data = temp.readStringUntil('\n');
    sscanf(data.c_str(), "T:%f,H:%f,TDS:%f,pH:%f,D:%f", &temperature, &humidity, &tdsValue, &phValue, &distance);

    Serial.print("Received data -> ");
    Serial.print("Temperature: ");
    Serial.print(temperature);
    Serial.print(" *C, Humidity: ");
    Serial.print(humidity);
    Serial.print(" %, TDS: ");
    Serial.print(tdsValue);
    Serial.print(" ppm, pH: ");
    Serial.print(phValue);
    Serial.print(", Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
  }
}

void loop() {
  // Reconnect Wi-Fi if disconnected
  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Wi-Fi disconnected, reconnecting...");
    connectToWiFi();
  }

  // Read sensor data
  readSensorData();

  // Upload sensor data to Firebase
  uploadToFirebase();

  // Read control settings from Firebase
  readControlSettings();

  // Control the pumps based on sensor data and control settings
  controlPump();

  // Wait for 10 seconds before next loop
  delay(2000);
}
