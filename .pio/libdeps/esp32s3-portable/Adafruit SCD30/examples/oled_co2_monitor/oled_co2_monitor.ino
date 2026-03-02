// A simple CO2 meter using the Adafruit SCD30 breakout and the Adafruit 128x32 OLEDs
#include <Adafruit_SCD30.h>
#include <Adafruit_SSD1306.h>

Adafruit_SCD30  scd30;
Adafruit_SSD1306 display = Adafruit_SSD1306(128, 32, &Wire);

void setup(void) {
  Serial.begin(115200);
  while (!Serial) delay(10);     // will pause Zero, Leonardo, etc until serial console opens

  Serial.println("SCD30 OLED CO2 meter!");

  // Try to initialize!
  if (!scd30.begin()) {
    Serial.println("Failed to find SCD30 chip");
    while (1) { delay(10); }
  }
  Serial.println("SCD30 Found!");


  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, 0x3C)) { // Address 0x3C for 128x32
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  if (!scd30.setMeasurementInterval(2)){
    Serial.println("Failed to set measurement interval");
    while(1) {delay(10);}
  }
  Serial.print("Measurement Interval: "); 
  Serial.print(scd30.getMeasurementInterval()); 
  Serial.println(" seconds");
  
  display.display();
  delay(500); // Pause for half second

  display.setTextSize(2);
  display.setTextColor(WHITE);
  display.setRotation(0);
}


void loop() {
  if (scd30.dataReady()) {
    display.clearDisplay();
    display.setCursor(0,0);
    display.setTextSize(2);

    Serial.println("Data available!");

    if (!scd30.read()){
      Serial.println("Error reading sensor data");
      display.println("READ ERR");
      display.display();
      return;
    }

    Serial.print("CO2: ");
    Serial.print(scd30.CO2, 3);
    Serial.println(" ppm");
    Serial.println("");

    display.println("CO2:");
    display.print(scd30.CO2, 2);

    display.setTextSize(1);

    display.setCursor(100, 20);
    display.println(" ppm");
    display.display();
  }

  delay(100);
}