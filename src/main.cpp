#include <Arduino.h>
// #include <WiFi.h>
#include <WiFiUdp.h>
#include <ArtnetWifi.h>
#include <FastLED.h>
#include <esp_adc_cal.h>

esp_adc_cal_characteristics_t adc_chars;
const adc_unit_t unit = ADC_UNIT_1;

// Wifi settings - be sure to replace these with the WiFi network that your computer is connected to
const char *ssid = "Artnet_Clubs";
const char *password = "artnetartnet";

// LED Strip
const int numLeds = 29;                   // Change if your setup has more or less LED's
const int numberOfChannels = numLeds * 3; // Total number of DMX channels you want to receive (1 led = 3 channels)
#define DATA_PIN 2                        // The data pin that the WS2812 strips are connected to.
#define ANALOG_PIN A1   
#define NUM_SAMPLES 10                    // Number of readings to average                  
#define DEFAULT_BRIGHTNESS 150            // Brightness for init, battery status, wifi status LED
CRGB leds[numLeds];

// Artnet settings
ArtnetWifi artnet;
const int startUniverse = 0;
bool sendFrame = 1;
int previousDataLength = 0;
bool connection_state = false;

void scan_wifi();

float readCalibratedVoltage() 
{
    uint32_t adc_raw = 0;

    // Take multiple readings and average
    for (int i = 0; i < NUM_SAMPLES; i++) {
        adc_raw += analogRead(ANALOG_PIN);
        delay(5);
    }
    adc_raw /= NUM_SAMPLES;

    // Convert ADC raw value to voltage using ESP32 calibration
    uint32_t voltage = esp_adc_cal_raw_to_voltage(adc_raw, &adc_chars);
    return voltage / 1000.0; // Convert mV to V
}

// connect to wifi – returns true if successful or false if not
boolean ConnectWifi(void)
{
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA); // Ensure it's in station mode
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi...");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status());

    // Debug WiFi status codes
    switch (WiFi.status())
    {
    case WL_NO_SSID_AVAIL:
      Serial.println("ERROR: SSID not available.");
      break;
    case WL_CONNECT_FAILED:
      Serial.println("ERROR: Incorrect password.");
      break;
    case WL_DISCONNECTED:
      Serial.println("ERROR: WiFi disconnected.");
      break;
    case WL_IDLE_STATUS:
      Serial.println("WiFi is idle, waiting...");
      break;
    }

    if (i > 40)
    { // Increase timeout to 20 seconds
      state = false;
      break;
    }
    i++;
  }

  if (state)
  {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }
  else
  {
    Serial.println("\nFailed to connect to WiFi.");
    scan_wifi();
  }

  return state;
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t *data)
{
  sendFrame = 1;
  // set brightness of the whole strip
  if (universe == 15)
  {
    FastLED.setBrightness(data[0]);
  }
  // read universe and put into the right part of the display buffer
  int led = 0;
  for (int i = 0; i < length / 3; i++)
  {
    led = i + (universe - startUniverse) * (previousDataLength / 3);
    if (led < numLeds)
    {
      leds[led] = CRGB(data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
      // printf("LED nr: %d R:%d G:%d B:%d\n", led, data[i * 3], data[i * 3 + 1], data[i * 3 + 2]);
    }
  }
  previousDataLength = length;
  // Serial.print("on DMX Frame: ");
  // Serial.print(millis());
  // printf(" Universe: %d previousDataLength = %d led = %d\n",universe, previousDataLength, led);
  FastLED.show();
}

void setup()
{
  // Initialize ADC Calibration
  esp_adc_cal_characterize(ADC_UNIT_1, ADC_ATTEN_DB_11, ADC_WIDTH_BIT_12, 1100, &adc_chars);
  Serial.begin(115200);
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, numLeds);
  FastLED.setBrightness(DEFAULT_BRIGHTNESS);
  float voltage = readCalibratedVoltage() * 2;  
  Serial.print("Calibrated Voltage: ");
  Serial.print(voltage, 3);
  Serial.println(" V");
  if(voltage < 3.0)
  {
    voltage = 3.0;
  }
  float percentage = (voltage - 3.0) / 1.2;
  Serial.print("Percentage: ");
  Serial.print(percentage, 3);
  Serial.println(" %");
  int num_leds_battery = percentage * numLeds;
  
  // Load light saber to percentage of battery
  for(int i = 0; i < num_leds_battery; i++)
  {
    leds[i] = CRGB(209, 134, 0); // Orange
    FastLED.show();
    delay(50);
  }

  delay(3000);
  
  // Remove light saber to zero from percentage start point
  for(int i = num_leds_battery; i > 0; i--)
  {
    leds[i] = CRGB(0, 0, 0); // Black
    FastLED.show();
    delay(30);
  }

  connection_state = ConnectWifi();
  // Switch on light
  if (connection_state)
  {
    leds[0] = CRGB(0, 255, 0); // Green
    FastLED.show();
  }
  else
  {
    leds[0] = CRGB(255, 0, 0); // Red
    FastLED.show();
  }

  artnet.begin();
  FastLED.setBrightness(100);
  // onDmxFrame will execute every time a packet is received by the ESP32
  artnet.setArtDmxCallback(onDmxFrame);
}

void print_connection_state()
{
  static int time_stamp = millis();
  if (millis() > (time_stamp + 5000))
  {
    Serial.print("Connection state: ");
    if (connection_state)
    {
      Serial.print("TRUE - ");
      Serial.print("IP address: ");
      Serial.println(WiFi.localIP());
    }
    else
    {
      Serial.println("FALSE");
    }
    time_stamp = millis();
  }
}

void loop()
{
  artnet.read();
  print_connection_state();
}

void scan_wifi()
{
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  delay(100);

  Serial.println("Scanning for WiFi networks...");
  int numberOfNetworks = WiFi.scanNetworks(); // Start the scan
  while (numberOfNetworks <= -1)
  {
    Serial.println("WiFi scan failed.");
    delay(5000);
  }
  Serial.printf("Found %d networks:\n", numberOfNetworks);

  // Loop through the networks and print their details
  for (int i = 0; i < numberOfNetworks; i++)
  {
    Serial.printf("SSID: %s, Signal Strength: %d dBm, Encryption Type: %s\n",
                  WiFi.SSID(i).c_str(), WiFi.RSSI(i),
                  (WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? "Open" : "Encrypted");
  }

  Serial.println();
}
