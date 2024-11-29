#include <Arduino.h>
// #include <WiFi.h>
#include <WiFiUdp.h>
#include <ArtnetWifi.h>
#include <FastLED.h>

//Wifi settings - be sure to replace these with the WiFi network that your computer is connected to
const char* ssid = "Artnet_Clubs";
const char* password = "artnetartnet";

// LED Strip
const int numLeds = 29; // Change if your setup has more or less LED's
const int numberOfChannels = numLeds * 3; // Total number of DMX channels you want to receive (1 led = 3 channels)
#define DATA_PIN 1 //The data pin that the WS2812 strips are connected to.
CRGB leds[numLeds];

// Artnet settings
ArtnetWifi artnet;
const int startUniverse = 0;

bool sendFrame = 1;
int previousDataLength = 0;

// connect to wifi – returns true if successful or false if not
boolean ConnectWifi_2(void)
{
  boolean state = true;
  int i = 0;

  WiFi.begin(ssid, password);
  Serial.println("");
  Serial.println("Connecting to WiFi");

  // Wait for connection
  Serial.print("Connecting");
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    if (i > 20){
      state = false;
      break;
    }
    i++;
  }
  if (state){
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("");
    Serial.println("Connection failed.");
  }

  return state;
}

boolean ConnectWifi(void) {
  boolean state = true;
  int i = 0;

  WiFi.mode(WIFI_STA); // Ensure it's in station mode
  WiFi.begin(ssid, password);
  Serial.println("\nConnecting to WiFi...");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
    Serial.print("WiFi Status: ");
    Serial.println(WiFi.status());

    // Debug WiFi status codes
    switch (WiFi.status()) {
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

    if (i > 40) { // Increase timeout to 20 seconds
      state = false;
      break;
    }
    i++;
  }

  if (state) {
    Serial.println("\nConnected to WiFi!");
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  } else {
    Serial.println("\nFailed to connect to WiFi.");
  }

  return state;
}

void onDmxFrame(uint16_t universe, uint16_t length, uint8_t sequence, uint8_t* data)
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
  FastLED.addLeds<WS2812, DATA_PIN, GRB>(leds, numLeds);
  // Switch on light - not connected = red:
  leds[0] = CRGB(255, 0, 0);
  FastLED.setBrightness(15);
  FastLED.show();
  Serial.begin(115200);
  ConnectWifi();
  // Switch on light - connected = green:
  leds[0] = CRGB(0, 255, 0);
  FastLED.show();
  artnet.begin();
  FastLED.setBrightness(100);
  // onDmxFrame will execute every time a packet is received by the ESP32
  artnet.setArtDmxCallback(onDmxFrame);
}

void loop()
{
  static int time_stamp = millis();
  // we call the read function inside the loop
  artnet.read();
  if(millis() > (time_stamp + 5000))
  {
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
    time_stamp = millis();
  }
}
