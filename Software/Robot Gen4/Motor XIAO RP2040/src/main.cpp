#include <Arduino.h>
#include <motor.h>

Motor motor;

float gyro_angle = 0.0;
float machine_angle = 0.0;
int speed = 0;
float vx = 0.0;
float vy = 0.0; // ロボットから見た相対的な向き
int flag = 1;

void setup()
{
        // put your setup code here, to run once:
        Serial.begin(115200);
        digitalWrite(D2, LOW);
        digitalWrite(D3, LOW);
        digitalWrite(D0, LOW);
        digitalWrite(D1, LOW);
        digitalWrite(D8, LOW);
        digitalWrite(D9, LOW);
        digitalWrite(D4, LOW);
        digitalWrite(D5, LOW);
        delay(3000);
        motor.begin();
}

void loop()
{
        // put your main code here, to run repeatedly:
        while (1)
        {
                motor.cal(1, 0, 100, 0, 0);
        }
        if (flag)
        {
                motor.brake();
        }
        else
        {
                motor.cal(vx, vy, speed, machine_angle, gyro_angle);
                // motor.cal(1.0, 1.0, speed, 0.0, gyro_angle);
        }
}

void setup1()
{
        Serial1.begin(115200);
        while (!Serial1)
        {
        }
}
void loop1()
{
        if (Serial1.available() > 8)
        {
                int recv_data = Serial1.read();
                if (recv_data == 255)
                {
                        int data[8]; //[0] Vx, [1] Vy, [2] Speed. [3] Gyro(下位8bit),
                                     //[4] Gyro(上位8bit), [5] ロボの向き(下位8bit),
                                     //[6] ロボの向き(上位8bit), [7]フラグ (0:通常
                                     // 1:ブレーキ)
                        data[0] = Serial1.read();
                        data[1] = Serial1.read();
                        data[2] = Serial1.read();
                        data[3] = Serial1.read();
                        data[4] = Serial1.read();
                        data[5] = Serial1.read();
                        data[6] = Serial1.read();
                        data[7] = Serial1.read();
                        vx = (data[0] - 100) / 100.0;
                        vy = (data[1] - 100) / 100.0;
                        speed = data[2];
                        int recv_gyro = data[3] + (data[4] << 8);
                        gyro_angle = (recv_gyro / 100.0) - PI;
                        int recv_machine_angle = data[5] + (data[6] << 8);
                        machine_angle = (recv_machine_angle / 100.0) - PI;
                        flag = data[7];
                        // Serial.println(vx);
                }
        }
}