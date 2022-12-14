#include <Arduino.h>
#include <ir_sensor.h>
#include <moving_average.h>

IR ir;

void setup() {
    pinMode(s0, OUTPUT);
    pinMode(s1, OUTPUT);
    pinMode(s2, OUTPUT);
    pinMode(s3, OUTPUT);
    pinMode(SIG_pin, INPUT);
    pinMode(A0, INPUT);

    digitalWrite(s0, LOW);
    digitalWrite(s1, LOW);
    digitalWrite(s2, LOW);
    digitalWrite(s3, LOW);

    Serial.begin(115200);
    Serial1.begin(9600);

    ir.begin();
}

void loop() {
    ir.IR_get();

    // ir.angle_read();
    ir.IRpin_read();
    delay(100);
    // ir.radius_read();

    int a = (ir.angle_PI * PI + PI) * 100;
    byte data[2];
    data[0] = byte(a);
    data[1] = byte(a >> 8);
    Serial1.write(255);
    Serial1.write(data, 2);
    // Serial.println("Boys Be Ambitious.");
}