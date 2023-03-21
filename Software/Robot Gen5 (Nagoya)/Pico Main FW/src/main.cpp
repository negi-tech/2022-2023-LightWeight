// ライブラリのインクルード
#include <Arduino.h>
#include <stdlib.h>
#include <Adafruit_SSD1306.h>
#include <Adafruit_NeoPixel.h>
#include <Adafruit_SSD1306.h>
#include <Servo.h>

// 自作ライブラリのインクルード
#include "MPU6050/gyro.h"
#include "line.h"

// 定数の宣言
#define SERIAL_BAUD 115200
#define MOTOR_BAUD 115200
#define IR_BAUD 115200
#define ESP32_BAUD 115200
#define OPENMV_BAUD 115200
#define OpenMV Serial1
#define GOAL_LPF 0.1
#define TWO_THIRDS_PI PI * 2.0 / 3.0
#define CIRC_BASE pow(0.6, 1.0 / 20.0)
#define CIRC_WEIGHT 3.5
#define CIRC_SPEED 140
#define STRAIGHT_SPEED 180
#define ESC_LINE_SPEED 180
#define GOAL_WEIGHT 1.28
#define MAX_SIGNAL 2200
#define MIN_SIGNAL 1000
#define ESC_PIN D14
#define LED_PIN D15
#define LED_NUM 32
#define SCREEN_WIDTH 128
#define SCREEN_HEIGHT 64
#define SCREEN_ADDRESS 0x3C

const uint8_t button_pin[3] = {D18, D19, D20}; // ボタンのピン番号

// インスタンスの生成
Gyro gyro;
Line line;
SerialPIO motor(D17, D16, 32); // TX, RX, buffer size
SerialPIO ir(D0, D1, 32);
SerialPIO esp32(D22, D21, 32); // SerialPIOの多用により不具合の可能性あり
Servo esc;
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, -1);

// グローバル変数の宣言

// 全体の制御系
bool game_flag = false;           // ゲームの開始フラグ
bool line_set_threshold_flag = 0; // ラインセンサーの閾値設定フラグ
double battery_voltage = 0;
bool battery_flag = 0;

// モーターの制御系

// ロボットから見た相対的な角度
double move_angle = 0; // 進行方向（-PI ~ PI）
double goal_angle = 0; // ゴールの角度（-PI ~ PI）
double goal_angle_LPF = 0;

// コート全体を俯瞰した時の絶対的な角度
// ゲーム開始時の正面（敵ゴール）方向を０°とし、右回転が正、左回転が負とする。
double abs_move_angle = 0;
double abs_goal_angle = 0;
double abs_goal_angle_LPF = 0;
double abs_line_angle = 0;

// その他
double machine_angle = 0; // ロボットに向かせる角度（-PI ~ PI）
bool goal_flag = 0;       // 0:ゴールなし, 1:ゴールあり
uint8_t goal_flag_ratio = 0;
uint8_t goal_flag_count = 0;
uint8_t speed = 0;      // 速度（0~254）
uint8_t motor_flag = 0; // 0: normal, 1: release, 2 or others: brake（0~254）
unsigned long long display_refresh_time = 0;
unsigned long long start_time = 0;
unsigned long long end_time = 0;
int fps = 0;

// 赤外線センサー制御系
double ir_angle = 0; // 赤外線センサーの角度（-PI ~ PI）
double abs_ir_angle = 0;
uint8_t ir_dist = 0; // 赤外線センサーの距離（0~254[cm]）
uint8_t ir_flag = 0; // 0: normal, 1: stop (0~254)

// 関数の宣言 (setup, loop以外は宣言はここで行い、定義は後で行う)
void motor_uart_send(void); // モーターの制御系の送信関数
void ir_uart_recv(void);    // 赤外線センサーの制御系の受信関数
void openmv_uart_recv(void);
void push_button(uint gpio, uint32_t events); // ボタンの処理
void line_set_threshold(void);                // 閾値の設定
void bldc_init(void);
void bldc_drive(uint16_t volume);
double normalize_angle(double angle); // 角度を-πからπまでに調整
void esc_line(void);

void setup(void)
{
        // put your setup code here, to run once:
        Serial.begin(SERIAL_BAUD);
        OpenMV.setTX(D12);
        OpenMV.setRX(D13);
        OpenMV.begin(OPENMV_BAUD);
        motor.begin(MOTOR_BAUD);
        while (!motor)
                ;
        ir.begin(IR_BAUD);
        while (!ir)
                ;
        esp32.begin(ESP32_BAUD);
        gyro.begin();
        display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
        line.begin();
        // bldc_init();
        pinMode(button_pin[0], INPUT_PULLUP);
        pinMode(button_pin[1], INPUT_PULLUP);
        pinMode(button_pin[2], INPUT_PULLUP);
        gpio_set_irq_enabled_with_callback(button_pin[0], GPIO_IRQ_EDGE_FALL, true, push_button);
        gpio_set_irq_enabled_with_callback(button_pin[1], GPIO_IRQ_EDGE_FALL, true, push_button);
        gpio_set_irq_enabled_with_callback(button_pin[2], GPIO_IRQ_EDGE_FALL, true, push_button);
        gpio_set_irq_enabled(button_pin[0], GPIO_IRQ_EDGE_FALL, true);
        gpio_set_irq_enabled(button_pin[1], GPIO_IRQ_EDGE_FALL, true);
        gpio_set_irq_enabled(button_pin[2], GPIO_IRQ_EDGE_FALL, true);
        // bldc_drive(0);
        analogReadResolution(10);

        Adafruit_NeoPixel *led = NULL;
        led = new Adafruit_NeoPixel(LED_NUM, LED_PIN, NEO_GRB + NEO_KHZ800);
        for (int i = 0; i < LED_NUM; i++)
        {
                led->setPixelColor(i, led->Color(255, 0, 0));
        }
        led->setBrightness(200);
        led->show();

        delete (led);
}

void loop(void)
{
        if (line_set_threshold_flag)
        {
                line_set_threshold();
        }
        game_flag = 1;
        while (game_flag)
        {
                start_time = micros();
                motor_flag = 0;
                battery_voltage = analogRead(A2) * 3.3 / 1023.0 * 4.0;
                // Serial.println(battery_voltage);
                /*if (battery_voltage < 11.0)
                {
                        game_flag = false;
                        battery_flag = true;
                }*/
                gyro.getEuler();
                if (millis() - display_refresh_time > 100)
                {
                        display.clearDisplay();
                        display.drawCircle(32, 32, 23, WHITE);
                        display.drawLine(4, 32, 60, 32, WHITE);
                        display.drawLine(32, 4, 32, 60, WHITE);
                        int16_t ir_x, ir_y;
                        ir_x = 32 + 23 * sin(ir_angle);
                        ir_y = 32 - 23 * cos(ir_angle);
                        display.fillCircle(ir_x, ir_y, 3, WHITE);
                        int16_t gyro_x = 32 + 25 * sin(gyro.angle);
                        int16_t gyro_y = 32 - 25 * cos(gyro.angle);
                        display.drawLine(32, 32, gyro_x, gyro_y, WHITE);
                        display.setCursor(65, 0);
                        display.setTextSize(1);
                        display.setTextColor(WHITE);
                        display.print("IR:");
                        display.print(ir_angle * 180 / PI, 1);
                        display.setCursor(65, 8);
                        display.print("GY:");
                        display.print(gyro.angle * 180 / PI, 1);
                        display.setCursor(65, 16);
                        display.print("FPS:");
                        display.print(fps);
                        display.setCursor(65, 24);
                        display.print("BAT:");
                        display.print(battery_voltage, 1);
                        display.setCursor(65, 32);
                        display.print("L:");
                        if (line.on_line)
                        {
                                display.print(line.line_theta * 180 / PI, 1);
                        }
                        else
                        {
                                display.print("OFF");
                        }
                        display.setCursor(65, 40);
                        display.print("Desined By");
                        display.setCursor(73, 48);
                        display.setTextSize(2);
                        display.print("Koji");
                        display.display();
                        display_refresh_time = millis();
                }
                ir_uart_recv();
                openmv_uart_recv();
                line.read();
                // line.debug();

                if (line.on_line)
                {
                        abs_line_angle = line.line_theta + gyro.angle;
                        if (abs_line_angle > PI)
                        {
                                abs_line_angle -= TWO_PI;
                        }
                        else if (abs_line_angle <= -PI)
                        {
                                abs_line_angle += TWO_PI;
                        }
                }
                // Serial.println(abs_line_angle);
                abs_ir_angle = ir_angle + gyro.angle;
                abs_ir_angle = fmod(abs_ir_angle, TWO_PI);
                if (abs_ir_angle > PI)
                {
                        abs_ir_angle -= TWO_PI;
                }
                if (abs_ir_angle < -PI)
                {
                        abs_ir_angle += TWO_PI;
                }
                // Serial.println(abs_goal_angle_LPF);
                // Serial.println(ir_angle);
                if (line.on_line)
                {
                        if (abs_line_angle > PI * 3.0 / 8.0 && abs_line_angle <= PI * 5.0 / 8.0) // ライン右
                        {
                                // Serial.println("right");
                                if (line.line_state_flag == 0 || line.line_state_flag == 2)
                                {
                                        line.line_state_flag = 2;
                                        if (abs_ir_angle > PI / 3.0 && abs_ir_angle < PI * 2.0 / 3.0)
                                        {
                                                move_angle = 0;
                                                speed = 0;
                                                motor_flag = 1;
                                                // Serial.println("stop");
                                        }
                                        else if (abs_ir_angle <= PI / 3.0 && abs_ir_angle >= 0)
                                        {
                                                move_angle = -PI / 9.0;
                                                speed = 100;
                                                motor_flag = 0;
                                                // Serial.println("front");
                                        }
                                        else if (abs_ir_angle >= PI * 2.0 / 3.0 || abs_ir_angle <= -PI / 3.0)
                                        {
                                                move_angle = -PI * 8.0 / 9.0;
                                                speed = 100;
                                                motor_flag = 0;
                                                // Serial.println("back");
                                        }
                                        else if (abs_ir_angle < 0 && abs_ir_angle >= -PI / 3.0)
                                        {
                                                line.on_line = false;
                                                // Serial.println("front");
                                        }
                                }
                                else
                                {
                                        esc_line();
                                }
                        }
                        else if (abs_line_angle <= -PI * 3.0 / 8.0 && abs_line_angle > -PI * 5.0 / 8.0) // ライン左
                        {
                                // Serial.println("left");
                                if (line.line_state_flag == 0 || line.line_state_flag == 4)
                                {
                                        line.line_state_flag = 4;
                                        if (abs_ir_angle < -PI / 3.0 && abs_ir_angle > -PI * 2.0 / 3.0)
                                        {
                                                move_angle = 0;
                                                speed = 0;
                                                motor_flag = 1;
                                                // Serial.println("stop");
                                        }
                                        else if (abs_ir_angle >= -PI / 3.0 && abs_ir_angle <= 0)
                                        {
                                                move_angle = PI / 9.0;
                                                speed = 100;
                                                motor_flag = 0;
                                                // Serial.println("front");
                                        }
                                        else if (abs_ir_angle <= -PI * 2.0 / 3.0 || abs_ir_angle >= PI / 3.0)
                                        {
                                                move_angle = PI * 8.0 / 9.0;
                                                speed = 100;
                                                motor_flag = 0;
                                                // Serial.println("back");
                                        }
                                        else if (abs_ir_angle > 0 && abs_ir_angle <= PI / 3.0)
                                        {
                                                line.on_line = false;
                                                // Serial.println("front");
                                        }
                                }
                                else
                                {
                                        esc_line();
                                }
                        }
                        else if (abs_line_angle <= PI / 8.0 && abs_line_angle > -PI / 8.0) // ライン前
                        {
                                // Serial.println("front");
                                if (line.line_state_flag == 0 || line.line_state_flag == 1)
                                {
                                        line.line_state_flag = 1;
                                        esc_line();
                                }
                                else
                                {
                                        esc_line();
                                }
                        }
                        else if (abs_line_angle <= PI * 3.0 / 8.0 && abs_line_angle > PI / 8.0) // ライン右前
                        {
                                // Serial.println("right front");
                                if (line.line_state_flag == 0 || line.line_state_flag == 5)
                                {
                                        line.line_state_flag = 5;
                                        esc_line();
                                }
                                else
                                {
                                        esc_line();
                                }
                        }
                        else if (abs_line_angle <= -PI / 8.0 && abs_line_angle > -PI * 3.0 / 8.0) // ライン左前
                        {
                                // Serial.println("left front");
                                if (line.line_state_flag == 0 || line.line_state_flag == 7)
                                {
                                        line.line_state_flag = 7;
                                        esc_line();
                                }
                                else
                                {
                                        esc_line();
                                }
                        }
                        else if (abs_line_angle <= PI * 7.0 / 8.0 && abs_line_angle > PI * 5.0 / 8.0) // ライン右後
                        {
                                // Serial.println("right back");
                                if (line.line_state_flag == 0 || line.line_state_flag == 6)
                                {
                                        line.line_state_flag = 6;
                                        esc_line();
                                }
                                else
                                {
                                        esc_line();
                                }
                        }
                        else if (abs_line_angle <= -PI * 5.0 / 8.0 && abs_line_angle > -PI * 7.0 / 8.0) // ライン左後
                        {
                                // Serial.println("left back");
                                if (line.line_state_flag == 0 || line.line_state_flag == 8)
                                {
                                        line.line_state_flag = 8;
                                        esc_line();
                                }
                                else
                                {
                                        esc_line();
                                }
                        }
                        else // ライン後
                        {
                                // Serial.println("back");
                                if (line.line_state_flag == 0 || line.line_state_flag == 3)
                                {
                                        line.line_state_flag = 3;
                                        if (abs_ir_angle <= -PI * 7.0 / 9.0 || abs_ir_angle > PI * 7.0 / 9.0)
                                        {
                                                esc_line();
                                        }
                                        else if (abs_ir_angle > -PI * 7.0 / 9.0 && abs_ir_angle <= -PI / 3.0)
                                        {
                                                move_angle = -PI / 3.0;
                                                speed = 100;
                                                motor_flag = 0;
                                        }
                                        else if (abs_ir_angle > -PI / 3.0 && abs_ir_angle < PI / 3.0)
                                        {
                                                move_angle = ir_angle;
                                                speed = 100;
                                                motor_flag = 0;
                                        }
                                        else if (abs_ir_angle >= PI / 3.0 && abs_ir_angle <= PI * 7.0 / 9.0)
                                        {
                                                move_angle = PI / 3.0;
                                                speed = 100;
                                                motor_flag = 0;
                                        }
                                }
                                else
                                {
                                        esc_line();
                                }
                        }
                        /*
                                move_angle = line.line_theta + PI;
                                move_angle = fmod(move_angle, TWO_PI);
                                if (move_angle > PI)
                                {
                                        move_angle -= TWO_PI;
                                }
                                speed = ESC_LINE_SPEED;
                                motor_flag = 3;
                        */
                }
                if (!line.on_line)
                {
                        line.line_state_flag = 0;

                        // if (goal_flag)
                        //{
                        //         machine_angle = abs_goal_angle_LPF * GOAL_WEIGHT;
                        // }
                        // else
                        //{
                        //         machine_angle = 0;
                        // }

                        if (goal_flag)
                        {
                                if (goal_flag_ratio < 100)
                                {
                                        if (abs_goal_angle_LPF > 0)
                                        {
                                                if (goal_flag_ratio <= 96)
                                                {
                                                        goal_flag_ratio += 4;
                                                }
                                                else
                                                {
                                                        goal_flag_ratio = 100;
                                                }
                                        }
                                        if (abs_goal_angle_LPF < 0)
                                        {
                                                if (goal_flag_ratio <= 92)
                                                {
                                                        goal_flag_ratio += 8;
                                                }
                                                else
                                                {
                                                        goal_flag_ratio = 100;
                                                }
                                        }
                                }
                        }
                        else
                        {
                                if (goal_flag_ratio > 0)
                                {
                                        goal_flag_ratio -= 1;
                                }
                        }
                        if (goal_flag_count < 100)
                        {
                                machine_angle = 0;
                                goal_flag_count += 1;
                        }
                        else
                        {
                                machine_angle = abs_goal_angle_LPF * GOAL_WEIGHT * goal_flag_ratio / 100;
                                if (machine_angle > HALF_PI)
                                {
                                        machine_angle = HALF_PI;
                                }
                                else if (machine_angle < -HALF_PI)
                                {
                                        machine_angle = -HALF_PI;
                                }
                        }
                        machine_angle = normalize_angle(machine_angle);
                        if (ir_flag)
                        {
                                float circ_exp = pow(CIRC_BASE, ir_dist);

                                // circ_exp = 0;
                                // if (ir_dist > 40)
                                //{
                                //         circ_exp = 0;
                                // }
                                // else
                                //{
                                //         circ_exp = 1;
                                // }
                                if (abs(ir_angle) < PI / 3.0 || abs(ir_angle) > PI * 3.0 / 4.0)
                                {
                                        // circ_exp =  1;
                                        move_angle = ir_angle + constrain(ir_angle * circ_exp * CIRC_WEIGHT, -PI / 2.0, PI / 2.0);
                                        if (abs(move_angle) < PI / 6.0)
                                        {
                                                speed = STRAIGHT_SPEED;
                                        }
                                        else
                                        {
                                                speed = CIRC_SPEED;
                                        }
                                }
                                else
                                {
                                        abs_move_angle = PI;
                                        move_angle = abs_move_angle - machine_angle;
                                        speed = STRAIGHT_SPEED;
                                }
                        }
                        else
                        {
                                move_angle = 0;
                                speed = 0;
                                motor_flag = 0;
                        }
                }

                move_angle = normalize_angle(move_angle);
                // Serial.println(machine_angle);
                motor_uart_send();
                if (Serial.available())
                {
                        uint16_t volume = Serial.readStringUntil('\n').toInt();
                        bldc_drive(volume);
                }
                delay(10);
                end_time = micros();
                fps = 1000000.0 / (end_time - start_time);
        }
        move_angle = 0;
        speed = 0;
        motor_flag = 2;
        motor_uart_send();
}

void setup1(void)
{
}

void loop1(void)
{

        delay(100);
}

void motor_uart_send(void)
{
        byte buf[8];
        buf[0] = constrain(motor_flag, 0, 254);   // 0: normal, 1: release, 2 or others: brake (0~254)
        uint16_t tmp = (move_angle + PI) * 100.0; //-PI ~ PI -> 0 ~ 200PI
        buf[1] = tmp & 0b0000000001111111;        // 下位7bit
        buf[2] = tmp >> 7;                        // 上位2bit
        tmp = (gyro.angle + PI) * 100.0;          //-PI ~ PI -> 0 ~ 200PI
        buf[3] = tmp & 0b0000000001111111;        // 下位7bit
        buf[4] = tmp >> 7;                        // 上位3bit
        tmp = (machine_angle + PI) * 100.0;
        buf[5] = tmp & 0b0000000001111111; // 下位7bit
        buf[6] = tmp >> 7;                 // 上位3bit
        buf[7] = constrain(speed, 0, 254); // constrain(speed, 0, 254); // 0~254
        motor.write(255);                  // ヘッダー
        motor.write(buf, 8);
        motor.write(254);
        // Serial.println("Send");
}

void ir_uart_recv(void)
{
        ir.write(255); // ヘッダー
        unsigned long long request_time = micros();
        while (!ir.available())
        {
                if (micros() - request_time > 10000)
                {
                        break;
                }
        }
        byte header = ir.read();
        if (header != 255)
                return;
        unsigned long long wait_time = micros();
        while (ir.available() < 4)
        {
                if (micros() - wait_time > 10000)
                {
                        while (ir.available())
                        {
                                ir.read();
                        }
                        break;
                }
        }
        byte buf[4];
        ir.readBytes(buf, 4);
        if (buf[0] != 255 && buf[1] != 255 && buf[2] != 255 && buf[3] != 255)
        {
                ir_flag = buf[0];                                // 0: normal, 1: stop (0~254)
                ir_angle = (buf[1] + buf[2] * 128) / 100.0 - PI; // 0 ~ 200PI -> -PI ~ PI
                ir_dist = buf[3];                                // 0~254[cm]
        }
}

void push_button(uint gpio, uint32_t events)
{
        if (gpio == button_pin[0])
        {
                gpio_set_irq_enabled(button_pin[0], GPIO_IRQ_EDGE_FALL, false);
                unsigned long long time1 = millis();
                while (digitalRead(button_pin[0]) == 0)
                {
                        if (millis() - time1 > 1000)
                        {
                                break;
                        }
                }
                // gyro.getEuler();
                // gyro.angle_offset = gyro.angle;
                game_flag = 1;
                // Serial.println("game start");
                gpio_set_irq_enabled(button_pin[0], GPIO_IRQ_EDGE_FALL, true);
        }
        else if (gpio == button_pin[1])
        {
                gpio_set_irq_enabled(button_pin[1], GPIO_IRQ_EDGE_FALL, false);
                unsigned long long time1 = millis();
                while (digitalRead(button_pin[1]) == 0)
                {
                        if (millis() - time1 > 1000)
                        {
                                break;
                        }
                }
                if (!game_flag)
                {
                        line_set_threshold_flag = 1;
                }
                else
                {
                        gpio_set_irq_enabled(button_pin[1], GPIO_IRQ_EDGE_FALL, true);
                }
        }
        else if (gpio == button_pin[2])
        {
                gpio_set_irq_enabled(button_pin[2], GPIO_IRQ_EDGE_FALL, false);
                unsigned long long time1 = millis();
                while (digitalRead(button_pin[2]) == 0)
                {
                        if (millis() - time1 > 1000)
                        {
                                break;
                        }
                }
                if (!line_set_threshold_flag)
                {
                        game_flag = 0;
                        // Serial.println("game stop");
                        speed = 0;
                        move_angle = 0;
                        motor_flag = 1;
                        motor_uart_send();
                        display.clearDisplay();
                        display.setCursor(0, 0);
                        display.print("Waiting");
                        display.display();
                }
                gpio_set_irq_enabled(button_pin[2], GPIO_IRQ_EDGE_FALL, true);
        }
}

void line_set_threshold()
{
        Serial.println("Start auto threshold setting");
        Serial.println("Please push middle button to start");
        Serial.println("wait...");
        display.clearDisplay();
        display.setCursor(0, 0);
        display.setTextSize(1);
        display.setTextColor(WHITE);
        display.print("Auto Set Threshold");
        display.setCursor(0, 8);
        display.print("Please Push Mid-Button");
        display.display();
        uint8_t cnt = 0;
        delay(1000);
        while (1)
        {

                if (digitalRead(button_pin[1]) == HIGH)
                {
                        cnt = 0;
                        delay(10);
                }
                else
                {
                        cnt++;
                        delay(10);
                }
                if (cnt > 5)
                {
                        break;
                }
        }
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Start");
        display.setCursor(0, 8);
        display.print("Move the Robot");
        display.display();
        delay(1000);
        // Serial.println("run");
        line.set_threshold();
        delay(1000);
        display.clearDisplay();
        display.setCursor(0, 0);
        display.print("Ended");
        display.setCursor(0, 8);
        display.print(line.ave_threshold);
        display.display();
        // Serial.println("End auto threshold setting");
        line_set_threshold_flag = false;
        gpio_set_irq_enabled(button_pin[1], GPIO_IRQ_EDGE_FALL, true);
}

void openmv_uart_recv(void)
{
        if (Serial1.available())
        {
                if (Serial1.read() == 255)
                {
                        while (Serial1.available() < 3)
                                ;
                        uint8_t buf[3];
                        buf[0] = Serial1.read();
                        buf[1] = Serial1.read();
                        buf[2] = Serial1.read();
                        goal_angle = (buf[0] + buf[1] * 128.0) / 100.0 - PI;
                        goal_flag = buf[2];
                        if (abs(goal_angle) > PI)
                        {
                                goal_flag = 0;
                        }
                        if (goal_flag)
                        {
                                Serial.println(goal_angle / PI * 180.0);
                                abs_goal_angle = goal_angle + gyro.angle;
                                abs_goal_angle_LPF = abs_goal_angle_LPF * GOAL_LPF + abs_goal_angle * (1.0 - GOAL_LPF);
                                abs_goal_angle_LPF = normalize_angle(abs_goal_angle_LPF);
                        }
                }
        }
}

void bldc_init(void)
{
        esc.attach(ESC_PIN);
        esc.writeMicroseconds(MAX_SIGNAL);
        delay(2000);
        esc.writeMicroseconds(MIN_SIGNAL);
        delay(2000);
}

void bldc_drive(uint16_t volume)
{
        esc.writeMicroseconds(volume);
}

double normalize_angle(double angle)
{
        if (angle > 0)
        {
                angle = fmod(angle, TWO_PI);
                if (angle > PI)
                {
                        angle = angle - TWO_PI;
                }
        }
        else if (angle < 0)
        {
                angle = fmod(abs(angle), TWO_PI);
                if (angle > PI)
                {
                        angle = angle - TWO_PI;
                }
                angle = -angle;
        }
        return angle;
}

void esc_line(void)
{
        move_angle = line.line_theta + PI;
        if (move_angle > PI)
        {
                move_angle -= 2.0 * PI;
        }
        // move_angle = abs_move_angle - gyro.angle;
        speed = ESC_LINE_SPEED;
        motor_flag = 3;
}
