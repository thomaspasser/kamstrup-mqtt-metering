// Serial bus stuff
#include <SoftwareSerial.h>
#include "mbusparser.h"
SoftwareSerial softSerial(14, 12); // RX, TX

uint8_t receiveBuffer[500];
MbusStreamParser streamParser(receiveBuffer, sizeof(receiveBuffer));

// WIFI and MQTT stuff
#include <ESP8266WiFi.h>
#include <PubSubClient.h>

const char* wifi_ssid = "";
const char* wifi_password = "";
const char* mqtt_server = "";
const char* mqtt_username = "";
const char* mqtt_password = "";

WiFiClient espClient;
PubSubClient client(espClient);

void setup_wifi() {

  delay(10);
  // We start by connecting to a WiFi network

  WiFi.mode(WIFI_STA);
  WiFi.begin(wifi_ssid, wifi_password);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
  }

  randomSeed(micros());
}


void reconnect() {
  // Loop until we're reconnected
  while (!client.connected()) {
    // Create a random client ID
    String clientId = "ESP8266Client-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqtt_username, mqtt_password)) {
      // Once connected, publish an announcement...
      client.publish("outTopic", "connected!");
    } else {
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup() {
  // Open serial communications and wait for port to open:
  softSerial.begin(2400);


  setup_wifi();
  client.setServer(mqtt_server, 1883);
  client.setBufferSize(500);
}

void loop() {
  
  if (!client.connected()) {
    reconnect();
  }
  client.loop();


  
  if (softSerial.available()){
    if (streamParser.pushData(softSerial.read())) {
      VectorView frame = streamParser.getFrame();
        if (streamParser.getContentType() == MbusStreamParser::COMPLETE_FRAME) {
          client.publish("outTopic", &frame.front()+11, frame.size()-14);
        }
 
    }
  }
}
