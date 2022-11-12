#include <Arduino.h>
#include <motor_control.h>

#define TRACK_SPEED 100
#define LINE_SPEED 100
#define WRAPAROUND_SPEED 100

void ir_get();
void gyro_get();
void line_get();

const float LINE_ANGLE[32] = {
    1.0 * PI / 16.0,   2.0 * PI / 16.0,   3.0 * PI / 16.0,   4.0 * PI / 16.0,
    5.0 * PI / 16.0,   6.0 * PI / 16.0,   7.0 * PI / 16.0,   8.0 * PI / 16.0,
    9.0 * PI / 16.0,   10.0 * PI / 16.0,  11.0 * PI / 16.0,  12.0 * PI / 16.0,
    13.0 * PI / 16.0,  14.0 * PI / 16.0,  15.0 * PI / 16.0,  16.0 * PI / 16.0,
    -15.0 * PI / 16.0, -14.0 * PI / 16.0, -13.0 * PI / 16.0, -12.0 * PI / 16.0,
    -11.0 * PI / 16.0, -10.0 * PI / 16.0, -9.0 * PI / 16.0,  -8.0 * PI / 16.0,
    -7.0 * PI / 16.0,  -6.0 * PI / 16.0,  -5.0 * PI / 16.0,  -4.0 * PI / 16.0,
    -3.0 * PI / 16.0,  -2.0 * PI / 16.0,  -1.0 * PI / 16.0,  0.0 * PI / 16.0,
};

bool battery_voltage_flag = false;
int line_flag[32];
bool line_whole_flag;
float line_deg = 0;
float ir_deg = 0;
float ir_dist = 0;
float gyro_deg = 0;
float line_x[32];
float line_y[32];

motor_control Motor;

void setup() {
    // put your setup code here, to run once:
    // Serial.begin(115200);
    Serial.begin(115200);
    Serial2.begin(115200);
    Serial3.begin(115200);
    Serial4.begin(115200);
    Serial5.begin(115200);
    Motor.begin();
    delay(1500);
    for (int i = 0; i < 32; i++) {
        line_x[i] = sin(LINE_ANGLE[i]);
        line_y[i] = cos(LINE_ANGLE[i]);
    }
}

void loop() {
    /*
    while (battery_voltage_flag) {
        Motor.stop();
        Serial.println("Please charge the battery");
    }*/
    gyro_get();
    // line_get();
    ir_get();
    if (line_whole_flag) {
        Motor.cal(line_deg + 180, LINE_SPEED, 0, gyro_deg);
        Serial.println("Line On");
    } else {
        if (gyro_deg > 90 || gyro_deg < -90) {
            Motor.cal(0, 0, 0, gyro_deg);
        } else {
            if (ir_deg >= 0 && ir_deg < 20) {
                Motor.cal(ir_deg * 1.5, WRAPAROUND_SPEED, 0, gyro_deg);
            } else if (ir_deg >= 20 && ir_deg < 45) {
                Motor.cal(105, WRAPAROUND_SPEED, 0, gyro_deg);
            } else if (ir_deg >= 45 && ir_deg < 90) {
                Motor.cal(120, WRAPAROUND_SPEED, 0, gyro_deg);
            } else if (ir_deg >= 90 && ir_deg < 150) {
                Motor.cal(180, WRAPAROUND_SPEED, 0, gyro_deg);
                Serial.println("180");
            } else if (ir_deg >= 150 && ir_deg < 180) {
                Motor.cal(-150, WRAPAROUND_SPEED, 0, gyro_deg);
            } else if (ir_deg >= -180 && ir_deg < -150) {
                Motor.cal(150, WRAPAROUND_SPEED, 0, gyro_deg);
            } else if (ir_deg >= -150 && ir_deg < -90) {
                Motor.cal(180, WRAPAROUND_SPEED, 0, gyro_deg);
            } else if (ir_deg >= -90 && ir_deg < -45) {
                Motor.cal(-120, WRAPAROUND_SPEED, 0, gyro_deg);
            } else if (ir_deg >= -45 && ir_deg < -20) {
                Motor.cal(-105, WRAPAROUND_SPEED, 0, gyro_deg);
            } else if (ir_deg >= -20 && ir_deg < 0) {
                Motor.cal(ir_deg * 1.5, WRAPAROUND_SPEED, 0, gyro_deg);
            } else {
                Motor.cal(0, 0, 0, 0);
            }
        }
        delay(5);
        /*
       if (line_whole_flag) {
           Motor.cal(line_deg + 180, LINE_SPEED, 0, gyro_deg);
           Serial.println("Line On");
       } else {
           float Dcos = ir_dist * cos(ir_deg * PI / 180.0);
           if (Dcos >= 30) {
               float x = ir_dist * sin(ir_deg * PI / 180.0);
               float y = ir_dist * cos(ir_deg * PI / 180.0) - 30;
               float deg = 90 - atan2(y, x) * 180.0 / PI;
               deg = fmod(deg, 360);
               deg = deg > 180 ? deg - 360 : deg;
               Motor.cal(deg, TRACK_SPEED, 0, gyro_deg);
           } else if (Dcos < 30 && abs(ir_deg) > 20) {
               float x = ir_dist * sin(ir_deg * PI / 180.0);
               float y = ir_dist * cos(ir_deg * PI / 180.0) - 30;
               float deg = 90 - atan2(y, x) * 180.0 / PI;
               deg = fmod(deg, 360);
               deg = deg > 180 ? deg - 360 : deg;
               Motor.cal(deg, TRACK_SPEED, 0, gyro_deg);
           } else {
               Motor.stop();
           }
       }
       */
        /*
         if (line_whole_flag) {
             Motor.cal(line_deg + 180, LINE_SPEED, 0, gyro_deg);
             Serial.println("Line On");
         } else {
             float Dcos = ir_dist * cos(ir_deg * PI / 180.0);
             if (Dcos >= 30) {
                 float x = ir_dist * sin(ir_deg * PI / 180.0);
                 float y = ir_dist * cos(ir_deg * PI / 180.0) - 30;
                 float deg = 90 - atan2(y, x) * 180.0 / PI;
                 deg = fmod(deg, 360);
                 deg = deg > 180 ? deg - 360 : deg;
                 Motor.cal(deg, TRACK_SPEED, 0, gyro_deg);
             } else if (Dcos < 30 && abs(ir_deg) > 20) {
                 float x = ir_dist * sin(ir_deg * PI / 180.0);
                 float y = ir_dist * cos(ir_deg * PI / 180.0) - 30;
                 float deg = 90 - atan2(y, x) * 180.0 / PI;
                 deg = fmod(deg, 360);
                 deg = deg > 180 ? deg - 360 : deg;
                 Motor.cal(deg, TRACK_SPEED, 0, gyro_deg);
             } else {
                 Motor.stop();
             }
         }
         */
    }
}

void ir_get() {
    Serial2.write(255);
    while (!Serial2.available()) {
    }
    int recv_data = Serial2.read();
    if (recv_data == 255) {
        while (!Serial2.available()) {
        }
        int sign = Serial2.read();
        int _strech_ir_deg = Serial2.read();
        int ir_dist1 = Serial2.read();
        int ir_dist2 = Serial2.read();
        ir_deg = (float)_strech_ir_deg / 255.0 * 180.0;
        if (sign == 0) {
            ir_deg *= -1.0;
        }
        ir_deg += gyro_deg;
        ir_dist = (float)(ir_dist1 * 10 + (float)ir_dist2 / 10.0);
    }
}

void gyro_get() {
    Serial3.write(255);
    while (!Serial3.available()) {
    }
    int recv_data = Serial3.read();
    if (recv_data == 255) {
        while (!Serial3.available()) {
        }
        int sign = Serial3.read();
        int strech_deg = Serial3.read();
        int battery_voltage_flag = Serial3.read();
        gyro_deg = (float)strech_deg / 255.0 * 180.0;
        if (sign == 0) {
            gyro_deg *= -1.0;
        }
    }
}

void line_get() {
    line_whole_flag = false;
    for (int i = 0; i < 32; i++) {
        line_flag[i] = 0;
    }
    int recv_data[4] = {0};
    Serial5.write(254);

    while (!Serial5.available()) {
    }
    if (Serial5.read() == 255) {
        for (int i = 0; i < 2; i++) {
            while (!Serial5.available()) {
            }
            recv_data[i] = Serial5.read();
        }
    }
    Serial4.write(254);
    while (!Serial4.available()) {
    }
    if (Serial4.read() == 255) {
        for (int i = 2; i < 4; i++) {
            while (!Serial4.available()) {
            }
            recv_data[i] = Serial4.read();
        }
    }
    /*while (!Serial5.available()) {
    }
    if (Serial5.read() == 255) {
        for (int i = 0; i < 2; i++) {
            while (!Serial5.available()) {
            }
            recv_data[i] = Serial5.read();
        }
    }
    Serial4.write(254);
    while (!Serial4.available()) {
    }
    if (Serial4.read() == 255) {
        for (int i = 2; i < 4; i++) {
            while (!Serial4.available()) {
            }
            recv_data[i] = Serial4.read();
        }
    }//*/

    for (int i = 0; i < 4; i++) {
        if (recv_data[i] > 127) {
            line_flag[(i + 1) * 8 - 1] = 1;
            recv_data[i] -= 128;
            line_whole_flag = true;
        }
        if (recv_data[i] > 63) {
            line_flag[(i + 1) * 8 - 2] = 1;
            recv_data[i] -= 64;
            line_whole_flag = true;
        }
        if (recv_data[i] > 31) {
            line_flag[(i + 1) * 8 - 3] = 1;
            recv_data[i] -= 32;
            line_whole_flag = true;
        }
        if (recv_data[i] > 15) {
            line_flag[(i + 1) * 8 - 4] = 1;
            recv_data[i] -= 16;
            line_whole_flag = true;
        }
        if (recv_data[i] > 7) {
            line_flag[(i + 1) * 8 - 5] = 1;
            recv_data[i] -= 8;
            line_whole_flag = true;
        }
        if (recv_data[i] > 3) {
            line_flag[(i + 1) * 8 - 6] = 1;
            recv_data[i] -= 4;
            line_whole_flag = true;
        }
        if (recv_data[i] > 1) {
            line_flag[(i + 1) * 8 - 7] = 1;
            recv_data[i] -= 2;
            line_whole_flag = true;
        }
        if (recv_data[i] > 0) {
            line_flag[(i + 1) * 8 - 8] = 1;
            recv_data[i] -= 1;
            line_whole_flag = true;
        }
    }

    /*for (int i = 0; i < 16; i++){
        Serial.print(line_flag[i]);
        Serial.print("\t");
    }
    Serial.print("\n");*/

    if (line_whole_flag) {
        float line_x_sum = 0;
        float line_y_sum = 0;
        for (int i = 0; i < 32; i++) {
            line_x_sum += line_x[i] * line_flag[i];
            line_y_sum += line_y[i] * line_flag[i];
        }
        float line_rad = atan2(line_x_sum, line_y_sum);
        line_rad = fmod(line_rad, PI);
        line_deg = -degrees(line_rad);
        // Serial.println(line_deg);
    }
    // else{Serial.println("Yeah");}
    /*Serial4.write(252);
    for (int i = 0; i < 16; i++)
    {
        while (!Serial4.available())
        {
        }
        Serial.print(Serial4.read());
        Serial.print("\t");
    }
    Serial.print("\n");*/
    // Serial.println(line_deg);
}
