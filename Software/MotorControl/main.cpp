#include "Arduino.h"
#include "src/motorcontrol.h"

int power = 150;

void setup()
{
  // put your setup code here, to run once:
  motor_init();
}

void loop()
{
  // put your main code here, to run repeatedly:
  motor_control(0, power);
}