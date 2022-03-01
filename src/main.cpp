#include <Arduino.h>
#include <WiFiNINA.h>
#include <PubSubClient.h>
#include <SAMD21turboPWM.h>
#include "credentials.h"

const char *ssid = networkSSID;
const char *password = networkPASSWORD;
const char *mqttServer = mqttSERVER;
const char *mqttUsername = mqttUSERNAME;
const char *mqttPassword = mqttPASSWORD;

char subTopic[] = "npy124/feeds/window-fan";

const int pwmRes = 1000;
const int pwmPin = 5;
const int pwmMin = 60;
const int pwmMax = 100;
int pwmDuty = 0;
int pwmMs = 0;

WiFiClient wifiClient;
PubSubClient client(wifiClient);
TurboPWM pwm;

void setup_wifi()
{
  delay(10);

  // We start by connecting to a WiFi network
  Serial.println();
  Serial.print("Connecting to ");
  Serial.println(ssid);

  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }

  randomSeed(micros());

  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

void setup_pwm()
{
  delay(10);

  Serial.println();
  Serial.println("Setting up pwm");

  pwm.setClockDivider(1, false);
  pwm.timer(0, 1, pwmRes, true);
  pwm.analogWrite(pwmPin, pwmMs);

  Serial.print("output frequency=");
  Serial.print(pwm.frequency(0));
  Serial.println("Hz");
}

void callback(char *topic, byte *payload, unsigned int length)
{
  Serial.print("Message arrived [");
  Serial.print(topic);
  Serial.print("] ");
  Serial.println();

  Serial.print("Payload length: ");
  Serial.println(length);

  char p[4];
  strncpy(p, (char *)payload, (length > sizeof(p)) ? sizeof(p) : length);
  p[sizeof(p)-1] = 0;

  pwmDuty = atoi(p);
  Serial.print("Requested duty cycle: ");
  Serial.println(pwmDuty);

  if (pwmDuty < 0) {
    pwmMs = 0;
  } else if (pwmDuty == 0) {
    pwmMs = 0;
  } else if (100 < pwmDuty) {
    pwmMs = 1000;
  } else {
    pwmMs = 10 * pwmMin + pwmDuty * (pwmMax - pwmMin) / 10;
  }

  pwm.analogWrite(pwmPin, pwmMs);
  Serial.print("Running fan at ");
  Serial.println(pwmMs);
}

void reconnect()
{
  // Loop until we're reconnected
  while (!client.connected())
  {
    Serial.print("Attempting MQTT connection...");
    // Create a random client ID
    String clientId = "ArduinoClient-";
    clientId += String(random(0xffff), HEX);
    // Attempt to connect
    if (client.connect(clientId.c_str(), mqttUsername, mqttPassword))
    {
      Serial.println("connected");
      // ... and resubscribe
      client.subscribe(subTopic);
    }
    else
    {
      Serial.print("failed, rc=");
      Serial.print(client.state());
      Serial.println(" try again in 5 seconds");
      // Wait 5 seconds before retrying
      delay(5000);
    }
  }
}

void setup()
{
  Serial.begin(115200);

  setup_wifi();
  setup_pwm();

  client.setServer(mqttServer, 1883);
  client.setCallback(callback);
}

void loop()
{
  if (!client.connected())
  {
    reconnect();
  }
  client.loop();
}
