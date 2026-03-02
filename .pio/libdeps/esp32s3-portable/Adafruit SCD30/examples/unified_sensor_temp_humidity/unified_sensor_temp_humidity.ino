// Unified sensor demo for readings from Adafruit SCD30
#include <Adafruit_SCD30.h>
#include <Adafruit_Sensor.h>

Adafruit_SCD30  scd30;

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("Adafruit SCD30 Temperature and humidity unified sensor test!");

  // Try to initialize!
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }

  Serial.println("SCD30 Found!");
}


void loop() {
  if(scd30.dataReady()){
    Serial.println("Data available!");
    sensors_event_t temp;
    sensors_event_t humidity;

    if (!scd30.getEvent(&humidity, &temp)){
      Serial.println("Error reading sensor data");
      return;
    }

    Serial.print("Temperature: ");
    Serial.print(temp.temperature);
    Serial.println(" degrees C");
    
    Serial.print("Relative Humidity: ");
    Serial.print(humidity.relative_humidity);
    Serial.println(" %");
    Serial.println("");
  }

  delay(100);
}