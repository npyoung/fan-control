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

const int pwmRes = 250;
const int pwmPin = 5;
const int pwmUnstallDuty = 75;
const int pwmUnstallMs = 500;
int pwmCurrentDuty = 0;

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
  pwm.timer(0, 4, pwmRes, false);
  pwm.analogWrite(pwmPin, pwmCurrentDuty);

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

  int pwmDuty = atoi((char *)payload);
  Serial.print("Requested duty cycle: ");
  Serial.println(pwmDuty);
  
  if (pwmDuty < 0)
  {
    pwmDuty = 0;
  } else if (100 < pwmDuty) {
    pwmDuty = 100;
  }

  if ((pwmCurrentDuty < pwmDuty) && (pwmDuty < pwmUnstallDuty))
  {
    // Hit the fan hard to avoid stalling
    pwm.analogWrite(pwmUnstallDuty, pwmUnstallDuty * 10);
    delay(pwmUnstallMs);
  }

  pwmCurrentDuty = pwmDuty;
  pwm.analogWrite(pwmPin, pwmDuty * 10);
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
