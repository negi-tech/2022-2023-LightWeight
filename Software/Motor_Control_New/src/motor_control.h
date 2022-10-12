#include <Arduino.h>

#define MOTOR_NUM 4
#define Kp 0.5
#define Kd 0.001
#define Ki 0.001
#define DELTA_TIME 0.01
#define MOTOR_POWER 200

class motor_control
{
public:
    void begin();
    void cal(float vel_x, float vel_y, int speed, float target_deg, float current_deg);
    void move(float power[MOTOR_NUM]);
    void posture_spin(float gyro_degree);

private:
    const int MOTOR_PIN[MOTOR_NUM][2] = {{3, 2}, {10, 9}, {12, 11}, {18, 13}};
    const int MOTOR_POS[MOTOR_NUM] = {240, 300, 60, 120};
    float COS[360];
    float SIN[360];
    float P = 0;
    float I = 0;
    float D = 0;
    float dir_diff[2] = {0,0};
};

void motor_control::begin()
{
    for (int i = 0; i < MOTOR_NUM; i++)
    {
        pinMode(MOTOR_PIN[i][0], OUTPUT);
        pinMode(MOTOR_PIN[i][1], OUTPUT);
        digitalWriteFast(MOTOR_PIN[i][0], LOW);
        digitalWriteFast(MOTOR_PIN[i][1], LOW);
    }
    for (int i = 0; i < 360; i++)
    {
        COS[i] = cos(i * PI / 180);
        SIN[i] = sin(i * PI / 180);
    }
}

void motor_control::cal(float vel_x, float vel_y, int speed, float target_deg, float current_deg)
{
    float power[MOTOR_NUM];
    for (int i = 0; i < MOTOR_NUM; i++)
    {
        power[i] = -vel_x * COS[MOTOR_POS[i]] + vel_y * SIN[MOTOR_POS[i]];
    }
    float max_power = 0;
    for (int i = 0; i < MOTOR_NUM; i++)
    {
        if (abs(power[i]) > max_power)
        {
            max_power = abs(power[i]);
        }
    }
    if (max_power != 0)
    {
        for (int i = 0; i < MOTOR_NUM; i++)
        {
            power[i] = power[i] / max_power * speed;
        }
    }
    float diff_deg = target_deg - current_deg;
    float vel_theta = Kp * diff_deg;
    for (int i = 0; i < MOTOR_NUM; i++)
    {
        power[i] += vel_theta;
    }
    for (int i = 0; i < MOTOR_NUM; i++)
    {
        power[i] = constrain(power[i], -255, 255);
    }
    //Serial.print("current_deg: ");
    //Serial.print(current_deg);
    Serial.print(" power: ");
    for (int i = 0; i < MOTOR_NUM; i++)
    {
        Serial.print(power[i]);
        Serial.print(" ");
    }
    move(power);
}

void motor_control::move(float power[MOTOR_NUM])
{
    Serial.println();
    for (int i = 0; i < MOTOR_NUM; i++)
    {
        int dir = 0;
        if (power[i] < 0)
        {
            dir = 1;
            power[i] = -power[i];
        }
        analogWrite(MOTOR_PIN[i][0], round(power[i]));
        digitalWriteFast(MOTOR_PIN[i][1], dir);
    }
}

void motor_control::posture_spin(float gyro_degree)
{
    dir_diff[1] = gyro_degree;
    P = dir_diff[1] * Kp;
    I = I + ((dir_diff[1] + dir_diff[0]) / 2) * DELTA_TIME * Ki;
    D = (dir_diff[1] - dir_diff[0]) / DELTA_TIME * Kd;
    float spin_power = P + I + D;
    if (spin_power >= 0){
         for (int i = 0; i < MOTOR_NUM; i++){
            analogWrite(MOTOR_PIN[i][0], round(abs(spin_power)));
            digitalWriteFast(MOTOR_PIN[i][1], 1);
         }
    }
    else{
        for (int i = 0; i < MOTOR_NUM; i++){
            analogWrite(MOTOR_PIN[i][0], round(abs(spin_power)));
            digitalWriteFast(MOTOR_PIN[i][1], 0);
         }
    }
    //Serial.println(round(abs(spin_power)));
    dir_diff[0] = dir_diff[1];
}