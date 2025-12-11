#include "Config.h"
#include "Network.h"
#include "Sensor.h"

void setup() {
  Serial.begin(115200);
  setupSensor();
  setupNetwork();
}

void loop() {
  loopSensor();
}