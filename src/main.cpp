#include <Arduino.h>
#include "bot_topic.h"


void microros_task(void *param)
{
  while (1)
  {
    loop_bot_transport();
  }
}

void setup()
{
  Serial0.begin(921600);
  set_microros_serial_transports(Serial0);
  delay(2000);
  initializeHardware();
  xTaskCreatePinnedToCore(microros_task, "microros_task", 10280, NULL, 1, NULL, 0); // 核心0

}

void loop()
{
  loop_bot_control();
  // 要有點延遲不然發布同時動作會直接做完動作看不到過程
  delay(100);
}