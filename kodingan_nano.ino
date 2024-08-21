#include "DFRobot_PH.h"
#include <EEPROM.h>
#include <DHT.h>
#include <SoftwareSerial.h>
#include "GravityTDS.h"
#include <Wire.h>

#define DHTPIN 12 
#define TdsSensorPin A0
#define PH_PIN A7
#define ultrasonicTrigPin 8
#define ultrasonicEchoPin 10

#define DHTTYPE DHT22
DHT dht(DHTPIN, DHTTYPE);
GravityTDS gravityTds;
DFRobot_PH ph;
SoftwareSerial temp(2, 3); 

float voltage, pH, phValue, temperature=25.0; 
float tdsValue = 0.0;

void setup() {
    Serial.begin(9600);
    temp.begin(9600);

    gravityTds.setPin(TdsSensorPin);
    gravityTds.setAref(5.0);  
    gravityTds.setAdcRange(1024);  
    gravityTds.begin();  
    dht.begin();
    ph.begin();
    pinMode(ultrasonicTrigPin, OUTPUT);
    pinMode(ultrasonicEchoPin, INPUT);
    Serial.println("Setup complete.");
}

void loop() {
    static unsigned long timepoint = millis();
    if (millis() - timepoint > 1000) { 
        timepoint = millis();
        float humidity = dht.readHumidity();
        float h = humidity + 15;
        float temper = dht.readTemperature();
        float t = temper - 3;

        if (isnan(h) || isnan(t)) {
            Serial.println("Failed to read from DHT sensor!");
        } else {
            temperature = t; 
        }

        gravityTds.setTemperature(temperature); 
        gravityTds.update();  
        tdsValue = gravityTds.getTdsValue();  

        voltage = analogRead(PH_PIN) / 1024.0 * 5000;  
        pH = ph.readPH(voltage, temperature);
        // phValue = pH;
        phValue = ( 0.1589*pH) + 5.5128;

        long duration, jarak, distance; 
        digitalWrite(ultrasonicTrigPin, LOW);
        delayMicroseconds(2);
        digitalWrite(ultrasonicTrigPin, HIGH);
        delayMicroseconds(10);
        digitalWrite(ultrasonicTrigPin, LOW);
        duration = pulseIn(ultrasonicEchoPin, HIGH);
        jarak = (duration / 2) / 29.1;
        distance = 1.0478 * jarak;

        Serial.print("Temperature: ");
        Serial.print(temperature, 1);
        Serial.println(" *C");

        Serial.print("Humidity: ");
        Serial.print(h);
        Serial.println(" %");

        Serial.print("TDS Value: ");
        Serial.print(tdsValue, 0);
        Serial.println(" ppm");

        Serial.print("pH Value: ");
        Serial.println(phValue, 2);

        Serial.print("Distance: ");
        Serial.print(distance);
        Serial.println(" cm");

        ph.calibration(voltage, temperature);

        temp.print("T:");
        temp.print(temperature, 1);
        temp.print(",H:");
        temp.print(h);
        temp.print(",TDS:");
        temp.print(tdsValue, 0);
        temp.print(",pH:");
        temp.print(phValue, 2);
        temp.print(",D:");
        temp.print(distance);
        temp.println();
        
        delay(2000);
    }
}
