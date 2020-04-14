#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <Thread.h>
#include <StaticThreadController.h>

String clientId = "Radlogger-";

const char *ssid = "UGH-IoT";
const char *password = "7DzcrxYFU8yPBtad";
const char *mqtt_server = "10.0.1.1";

WiFiClient wifiClient;
PubSubClient client(wifiClient);

Thread threadPublishCounts = Thread();
Thread threadPublishDose = Thread();

StaticThreadController<2> threadController(&threadPublishCounts, &threadPublishDose);

const unsigned int logCountsPeriod = 10000;
const unsigned int logDosePeriod = 60000;
const unsigned int tubeIndex = 151;

unsigned long counts[2] = {0, 0};
unsigned long lastReconnectAttempt = 0;

void intPulse()
{
  digitalWrite(2, HIGH);
  counts[0]++;
}

void callback(char *topic, byte *payload, unsigned int length) {}

boolean reconnect()
{
  digitalWrite(2, HIGH);

  if (client.connect(clientId.c_str()))
  {
    client.publish("sensors/rads/debug", String(millis()).c_str());
  }

  return client.connected();
}

void threadPublishCountsCallback()
{
  digitalWrite(2, HIGH);

  String _payload = String(counts[0]);

  Serial.println("threadPublishCountsCallback:\t" + String(millis() / 1000) + "\tcounts: " + _payload);
  client.publish("sensors/rads/counts", _payload.c_str());

  counts[1] += counts[0];
  counts[0] = 0;
}

void threadPublishDoseCallback()
{
  digitalWrite(2, HIGH);

  String _payload = String(counts[1] / (float)tubeIndex);

  Serial.println("threadPublishDoseCallback:\t" + String(millis() / 1000) + "\tdose: " + _payload + " uSv/h");
  client.publish("sensors/rads/dose", _payload.c_str());

  counts[1] = 0;
}

void setup()
{
  pinMode(2, OUTPUT);
  pinMode(4, INPUT);

  digitalWrite(2, HIGH);

  Serial.begin(115200);
  WiFi.begin(ssid, password);

  while (WiFi.status() != WL_CONNECTED)
  {
    delay(1000);
  }

  Serial.println(WiFi.localIP());

  clientId += String(random(0xffff), HEX);

  client.setServer(mqtt_server, 1883);
  client.setCallback(callback);

  threadPublishCounts.enabled = true;
  threadPublishCounts.setInterval(logCountsPeriod);
  threadPublishCounts.onRun(threadPublishCountsCallback);

  threadPublishDose.enabled = true;
  threadPublishDose.setInterval(logDosePeriod);
  threadPublishDose.onRun(threadPublishDoseCallback);

  attachInterrupt(digitalPinToInterrupt(4), intPulse, FALLING);
}

void loop()
{
  digitalWrite(2, LOW);

  if (!client.connected())
  {
    unsigned long now = millis();

    if (now - lastReconnectAttempt > 5000)
    {
      lastReconnectAttempt = now;

      if (reconnect())
      {
        lastReconnectAttempt = 0;
      }
    }
  }
  else
  {
    client.loop();
  }

  threadController.run();
}