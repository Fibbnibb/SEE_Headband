#include <WiFi.h>
#include <HTTPClient.h>
#include <Wire.h>
#include <Adafruit_ADS1X15.h>
#include <Adafruit_MPU6050.h>
#include <Adafruit_Sensor.h>
#include <Adafruit_DRV2605.h>

// Define the I2C address for the TCA9548A I2C multiplexer
#define TCA_ADDR 0x70

// WiFi credentials and Messaging API details
//const char* ssid = "mlive";
//const char* password = "dontusemywifi";
//const char* ssid = "get your own wifi";
//const char* password = "hgom4245";
//String phoneNumber = "+353892439462"; // Example phone number
//String apiKey = "4407592"; // Example API key

const char* ssid = "mlive";
const char* password = "dontusemywifi";
String phoneNumber = "+353856393145"; // Example phone number
String apiKey = "4473681"; // Example API key

Adafruit_MPU6050 mpu;
Adafruit_DRV2605 drv1, drv2, drv3;

const int trigPin1 = 32, echoPin1 = 33;
const int trigPin2 = 5, echoPin2 = 18;
const int trigPin3 = 26, echoPin3 = 27;
bool mpuInitialized = false; // Flag to track MPU6050 initialization

#define SOUND_SPEED 0.034
#define TIMEOUT_MICROSECONDS 25000  // 25 milliseconds timeout for responsiveness

void tcaSelect(uint8_t i) {
  if (i > 7) return;
  Wire.beginTransmission(TCA_ADDR);
  Wire.write(1 << i);
  Wire.endTransmission();
}

bool setupDRV2605(uint8_t tcaChannel, Adafruit_DRV2605 &drv) {
  tcaSelect(tcaChannel);
  if (!drv.begin()) {
    Serial.print("Failed to initialize DRV2605 on channel ");
    Serial.println(tcaChannel);
    return false;
  }
  drv.selectLibrary(1);
  drv.setMode(DRV2605_MODE_INTTRIG);
  return true;
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("Connected to WiFi");

  Wire.begin(); // Initialize I2C
  mpuInitialized = mpu.begin(); // Attempt to initialize MPU6050
  if (!mpuInitialized) {
    Serial.println("MPU6050 not found, continuing without it...");
  } else {
    mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
    mpu.setGyroRange(MPU6050_RANGE_2000_DEG);
  }
  //Wire.begin(); // Initialize I2C
  //if (!mpu.begin()) {
    //Serial.println("MPU6050 not found");
    //while (1) yield();
  //}

  //mpu.setAccelerometerRange(MPU6050_RANGE_16_G);
  //mpu.setGyroRange(MPU6050_RANGE_2000_DEG);

  pinMode(trigPin1, OUTPUT); pinMode(echoPin1, INPUT);
  pinMode(trigPin2, OUTPUT); pinMode(echoPin2, INPUT);
  pinMode(trigPin3, OUTPUT); pinMode(echoPin3, INPUT);

  // Initialize DRV2605 devices
  if (!setupDRV2605(1, drv1) || !setupDRV2605(2, drv2) || !setupDRV2605(3, drv3)) {    
    Serial.println("Setup failed, check hardware!");
  } else {
    Serial.println("All DRV2605 devices configured");
  }
}

void loop() {
  sensors_event_t a, g, temp;
  mpu.getEvent(&a, &g, &temp);

  // Fall detection logic
  //if (abs(g.gyro.x) > 90 || abs(g.gyro.y) > 100 || abs(g.gyro.z) > 90) {
    //Serial.println("Fall detected! Sending message...");
    //sendMessage("Urgent: A fall has been detected!");
    //delay(10000); // Delay to avoid repeated messages
  //}

  //if (a.acceleration.y < -9.8 || a.acceleration.y > 9.8) {
    //Serial.println("Condition to send message met");
    //sendMessage("Device is upside down!");
    //Serial.println("After send message"); // Check if it returns from sendMessage
    //delay(10000);
  //}

  // Measure distances and trigger haptic feedback
  float distanceCm1 = measureDistance(trigPin1, echoPin1);
  float distanceCm2 = measureDistance(trigPin2, echoPin2);
  float distanceCm3 = measureDistance(trigPin3, echoPin3);

  Serial.print(distanceCm1);
  Serial.print(",");
  Serial.print(distanceCm2);
  Serial.print(",");
  Serial.println(distanceCm3);

  triggerHaptic(1, drv1, distanceCm1);
  triggerHaptic(2, drv2, distanceCm2);
  triggerHaptic(3, drv3, distanceCm3);

  delay(600); // Delay for loop readability and management
}

float measureDistance(int trigPin, int echoPin) {
  digitalWrite(trigPin, LOW);
  delayMicroseconds(2);
  digitalWrite(trigPin, HIGH);
  delayMicroseconds(10);
  digitalWrite(trigPin, LOW);
  long duration = pulseIn(echoPin, HIGH, TIMEOUT_MICROSECONDS);
  if (duration == 0) {
    //Serial.println("No echo received or out of range");
    return -1.0;  // No echo received or object is out of range
  } else {
    float distance = duration * SOUND_SPEED / 2;
    if (distance < 10 || distance > 400) {
      //Serial.print("Distance out of range: ");
      //Serial.println(distance);
      return -1.0;  // Distance out of expected range
    }
    return distance;
  }
}

void sendMessage(String message) {
  if (WiFi.status() == WL_CONNECTED) {
    HTTPClient http;
    String encodedMessage = message;
    encodedMessage.replace(" ", "%20"); // URL encode the message
    String url = "https://api.callmebot.com/whatsapp.php?phone=" + phoneNumber 
                  + "&apikey=" + apiKey + "&text=" + encodedMessage;
    http.begin(url);
    int httpResponseCode = http.GET();
    if (httpResponseCode == 200) {
      Serial.println("Message sent successfully");
    } else {
      Serial.print("Error sending the message, HTTP response code: ");
      Serial.println(httpResponseCode);
    }
    http.end();
  }
}

void triggerHaptic(uint8_t tcaChannel, Adafruit_DRV2605 &drv, float distance) {
    int effect = 1; // Default effect
    String intensity = "Low";

    if (distance == -1.0) {
        Serial.println("Distance out of reliable range, no haptic feedback.");
        return;  // Skip haptic feedback if the distance is out of range
    } else if (distance < 10) {
        effect = 12; // Maximum intensity for very close objects
        intensity = "Maximum";
    } else if (distance < 20) {
        effect = 28;  // Strong effect for close proximity
        intensity = "Strong";
    } else if (distance < 100) {
        effect = 36;  // Medium effect
        intensity = "Medium";
    } else if (distance <= 400) {
        effect = 6;   // Mild effect for distant objects
        intensity = "Mild";
    }
    
    //Serial.print("Distance: ");
    //Serial.print(distance);
    //Serial.print(" cm - Vibration Intensity: ");
    //Serial.println(intensity);

    tcaSelect(tcaChannel);
    drv.setWaveform(0, effect);
    drv.setWaveform(1, 0);
    drv.go();
}
