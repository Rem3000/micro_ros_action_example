#ifndef ___BOT_TOPIC_H__
#define ___BOT_TOPIC_H__
#include <Arduino.h>
#include <Wire.h>
#include <TimeLib.h> 
#include <micro_ros_platformio.h>
#include "micro_ros_transport_serial.h"
#include "micro_ros_transport_wifi_udp.h"
#include <rmw/qos_profiles.h>
#include <rcl/rcl.h>
#include <rclc/rclc.h>
#include <rclc/executor.h>
#include <bot_interfaces/action/move_distance.h>
#include <micro_ros_utilities/type_utilities.h>
#include <micro_ros_utilities/string_utilities.h>
#include <stdint.h>
#include <Adafruit_NeoPixel.h>

#define LED_PIN    48    // RGB LED 連接的腳位
#define NUM_PIXELS 9     // 通常只有一顆內建的 RGB LED
extern Adafruit_NeoPixel pixels;

#define RCCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)){RGB(20, 20, 20,50,true);}}
#define RCSOFTCHECK(fn) { rcl_ret_t temp_rc = fn; if((temp_rc != RCL_RET_OK)) {} else {}}
#define EXECUTE_EVERY_N_MS(MS, X)          \
    do                                     \
    {                                      \
        static volatile int64_t init = -1; \
        if (init == -1)                    \
        {                                  \
            init = millis();               \
        }                                  \
        if (millis() - init > MS)          \
        {                                  \
            X;                             \
            init = millis();               \
        }                                  \
    } while (0);


rcl_ret_t goal_callback(rclc_action_goal_handle_t *goal_handle, void *context);
rcl_ret_t handle_goal(rclc_action_goal_handle_t *goal_handle, void *context);
bool handle_cancel(rclc_action_goal_handle_t * goal_handle, void * context) ;
void loop_bot_control();
void initializeHardware();
void loop_bot_transport();
bool create_bot_transport();
bool destory_bot_transport();
void service_callback(const void *req, void *res);
float EncodertoLength(int32_t ticks);
void RGB(uint8_t r, uint8_t g, uint8_t b, uint8_t Brightness, bool Brightness_on );
void updateMotor(float speed, uint8_t index);

#endif // ___BOT_TOPIC_H__