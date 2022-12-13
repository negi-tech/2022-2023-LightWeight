#include <Arduino.h>

#include "motor.h"

Motor motor;

void setup() {
    // put your setup code here, to run once:
    Serial.begin(115200);
    motor.begin();
    delay(5000);
}

void loop() {
    if (motor.c == 0) { //ライン踏んでないとき
        motor.speed = motor.default_speed;
        motor.cal();
    } else { //ライン踏んだとき
        motor.speed = motor.default_speed;
        motor.cal();
    }
}

void setup1() {
    Serial1.begin(115200);
    delay(5000);
}

void loop1() {
    if (Serial1.available() > 5) {
        int recv_data = Serial1.read();
        if (recv_data == 255) {
            int data[5];  //[0][1]:gyro, [2][3]:move
            data[0] = Serial1.read();
            data[1] = Serial1.read();
            data[2] = Serial1.read();
            data[3] = Serial1.read();
            data[4] = Serial1.read();
            int recv_gyro = data[0] + (data[1] << 8);
            motor.gyro_angle = (recv_gyro / 100.0) - PI;
            // Serial.println(motor.gyro_angle);
            int recv_move = data[2] + (data[3] << 8);
            motor.c = data[4];  // 1:line, 0:move
            if (motor.c == 1) {
                motor.move_angle = recv_move / 100.0;
                if (motor.move_angle > PI) {
                    motor.move_angle -= 2 * PI;
                }
            } else {
                motor.move_angle = (recv_move / 100.0) - PI;
            }
        }
    }
}