#include "bot_topic.h"
/*==================MicroROS訂閱發布服務動作========================*/
bot_interfaces__action__MoveDistance_FeedbackMessage feedback;
bot_interfaces__action__MoveDistance_GetResult_Response result={0};
/*==================MicroROS執行器&節點==================*/
rclc_executor_t executor;
rclc_support_t support;
rcl_allocator_t allocator;
rcl_node_t node;
rcl_timer_t timer;

// 定意動作服務
rclc_action_server_t action_server;
rclc_action_goal_handle_t *g_goal_handle = NULL;

uint8_t pwm = 0;float main_goal = 0;float error_goal=0;
bool goal_active = false;
Adafruit_NeoPixel pixels(NUM_PIXELS, LED_PIN, NEO_GRB + NEO_KHZ800);
float front_length = 0, rear_length = 0;
enum states
{
    WAITING_AGENT,
    AGENT_AVAILABLE,
    AGENT_CONNECTED,
    AGENT_DISCONNECTED
} state;   


void goal_callback(rcl_timer_t *timer, int64_t last_call_time) {
    (void) last_call_time;
    
    if (timer == NULL || !goal_active || g_goal_handle == NULL) {
        result.result.success = false;
        return;
    }


    // 發布 feedback
    feedback.feedback.feedback = main_goal-error_goal;//也可以選擇feedback error_goal
    rclc_action_publish_feedback(g_goal_handle, &feedback);

    // 檢查是否完成
    float error = fabs(main_goal - error_goal);
    if (error <= 0.1) {
        result.result.success = true;
        rclc_action_send_result(g_goal_handle, GOAL_STATE_SUCCEEDED, &result);
        goal_active = false;
        g_goal_handle = NULL;
        return;
    }

    // 處理 cancel
    if (g_goal_handle->goal_cancelled) {
        result.result.success = false;
        rclc_action_send_result(g_goal_handle, GOAL_STATE_CANCELED, &result);
        goal_active = false;
        g_goal_handle = NULL;
        return;
    }
}

rcl_ret_t handle_goal(rclc_action_goal_handle_t *goal_handle, void *context){
    (void)context;

    auto *req = (bot_interfaces__action__MoveDistance_SendGoal_Request *)goal_handle->ros_goal_request;
    goal_active = true;
    // 取得目標值
    main_goal = req->goal.goal;
    // 目標值不在範圍直接拒絕
    if (main_goal > 50.0 || main_goal < 0.0) {
        return RCL_RET_ACTION_GOAL_REJECTED;
    }
     g_goal_handle = goal_handle;
     return RCL_RET_ACTION_GOAL_ACCEPTED;
}
bool handle_cancel(rclc_action_goal_handle_t * goal_handle, void * context) {
    (void) context;
    (void) goal_handle;

    return true;  // 總是接受取消請求
}




void loop_bot_control()
{    
  if(goal_active && g_goal_handle && error_goal < main_goal)
  {
    error_goal = error_goal + 0.1;
  }else if(goal_active && g_goal_handle && error_goal > main_goal)
  {
    error_goal = error_goal - 0.1;
  }else
  {
    error_goal = main_goal;
  } 
}




void initializeHardware(){
    // 初始化引脚模式

}

void loop_bot_transport()
{
    switch (state)
    {
    case WAITING_AGENT:
        // 每隔 2 秒 ping Agent
        EXECUTE_EVERY_N_MS(2000,
            state = (RMW_RET_OK == rmw_uros_ping_agent(100, 5)) ? AGENT_AVAILABLE : WAITING_AGENT;
        );
        RGB(255, 0, 0,50,true); // red閃爍
        break;

    case AGENT_AVAILABLE:
        // 嘗試建立傳輸（建立 ROS node、publisher/subscriber 等）
        if (create_bot_transport()) {
            state = AGENT_CONNECTED;
        } else {
            state = WAITING_AGENT;
            destory_bot_transport(); // 清掉失敗建立的資源
        }
        break;

    case AGENT_CONNECTED:
        // 定時 ping 檢查 Agent 是否還在線
        EXECUTE_EVERY_N_MS(2000,
            if (RMW_RET_OK != rmw_uros_ping_agent(100, 5)) {
                state = AGENT_DISCONNECTED;
            }
        );

        if (state == AGENT_CONNECTED) {
            // 如果還沒同步時間，嘗試同步
            if (!rmw_uros_epoch_synchronized()) {
                RCSOFTCHECK(rmw_uros_sync_session(1000));
                if (rmw_uros_epoch_synchronized()) {
                    setTime(rmw_uros_epoch_millis() / 1000 + SECS_PER_HOUR * 8);
                }
            }
            RGB(50, 50, 50,50,true); // 白燈
            RCSOFTCHECK(rclc_executor_spin_some(&executor, RCL_MS_TO_NS(100)));
        }
        break;
    case AGENT_DISCONNECTED:
        destory_bot_transport();  // 釋放原本的 ROS 資源
        state = WAITING_AGENT;        // 回到等待 agent 的狀態
        break;

    default:
        state = WAITING_AGENT;
        break;
    }

    if (state != AGENT_CONNECTED) {
        delay(10); // idle 時稍作延遲以節省資源
    }
}

bool create_bot_transport()
{
    delay(500);
    // 預設的記憶體分配器 allocator
    allocator = rcl_get_default_allocator();
    // RCSOFTCHECK 是一個巨集，用來檢查函式執行結果是否錯誤，若錯誤會印出錯誤訊息並結束程式。
    const unsigned int timer_timeout = 50;
    // 呼叫 rclc_support_init 初始化 ROS 2 執行時支援庫，傳入 allocator
    RCSOFTCHECK(rclc_support_init(&support, 0, NULL, &allocator));
    // 呼叫 rclc_node_init_default 初始化 ROS 2 節點，傳入節點名稱、命名空間與支援庫
    RCSOFTCHECK(rclc_node_init_default(&node, "esp32_action", "", &support));
    // 使用預設設定建立動作服務器
    RCSOFTCHECK(rclc_action_server_init_default(
        &action_server,
        &node,
        &support,
        ROSIDL_GET_ACTION_TYPE_SUPPORT(bot_interfaces, MoveDistance),
        "move_distance"
    ));
 
    // 呼叫 rclc_timer_init_default 初始化 ROS 2 計時器，傳入支援庫、計時器週期與回呼函式
    RCSOFTCHECK(rclc_timer_init_default(&timer, &support, RCL_MS_TO_NS(timer_timeout), goal_callback));
    // 呼叫 rclc_executor_init 初始化 ROS 2 執行器，傳入支援庫、執行緒數量與記憶體分配器
    RCSOFTCHECK(rclc_executor_init(&executor, &support.context, 6, &allocator));
    // 呼叫 rclc_executor_add_action_server 將動作服務器加入執行器，傳入執行器、服務器、handle 數量、feedback、feedback 大小、goal/cancel 回呼與 context
    rclc_executor_add_action_server(
        &executor,
        &action_server,
        1,  // handles_number
        &feedback,
        sizeof(bot_interfaces__action__MoveDistance_Feedback),
        handle_goal,
        handle_cancel,
        (void *)&action_server
    );
    // 呼叫 rclc_executor_add_timer 將計時器加入執行器，傳入執行器與計時器。
    RCSOFTCHECK(rclc_executor_add_timer(&executor, &timer));
    return true;
}

// 用於釋放 ROS 2 節點中的相關資源
bool destory_bot_transport()
{  
    // 取得 ROS 2 context 中的 RMW context，並指派給 rmw_context 變數
    rmw_context_t *rmw_context = rcl_context_get_rmw_context(&support.context);
    // 設定 RMW context 的實體銷毀會話逾時為 0，代表立即返回不等待逾時
    (void)rmw_uros_set_context_entity_destroy_session_timeout(rmw_context, 0);
    // 用於銷毀 ROS 2 動作服務器（Action Server）
    RCSOFTCHECK(rclc_action_server_fini(&action_server, &node));
    // 用於銷毀 ROS 2 計時器（Timer）
    RCSOFTCHECK(rcl_timer_fini(&timer));
    // 用於停止執行器（Executor）並釋放相關資源
    RCSOFTCHECK(rclc_executor_fini(&executor));
    // 用於銷毀 ROS 2 節點（Node）
    RCSOFTCHECK(rcl_node_fini(&node));
    // 用於釋放支援庫中分配的資源
    rclc_support_fini(&support);
    return true;
}
uint8_t led_colors[NUM_PIXELS][3] = {
  {255, 0, 0},    // 紅
  {0, 255, 0},    // 綠
  {0, 0, 255},    // 藍
  {255, 255, 0},  // 黃
  {0, 255, 255},  // 青
  {255, 0, 255},  // 紫
  {255, 255, 255},// 白
  {128, 0, 255},  // 紫藍
  {255, 128, 0}   // 橘
};
uint8_t light=0 ,random_color=0;
bool updown = false;
void RGB(uint8_t r, uint8_t g, uint8_t b, uint8_t Brightness, bool Brightness_on )
{
  // RGB LED 亮度控制
  // r, g, b: RGB LED 的顏色值 (0~255)
  // Brightness: LED 亮度 (0~255)
  // Brightness_on: 是否開啟亮度控制 (true/false)
  
  // 設定亮度
  // 如果 Brightness_on 為 true，則設定亮度為指定值
  // 否則，將亮度設為 0 (關閉亮度)
  // 設定 RGB LED 顏色

  if(Brightness_on)
  {
    if(updown)
    {
      light --;
      if(light == 0)
      {
        updown = false;
      }
    }
    else
    {
      light ++;
      if(light == Brightness)
      {
        updown = true;
      }
    }
    pixels.setBrightness(light);

  }
  else
  {
    pixels.setBrightness(10);
  }
  
  pixels.setPixelColor(0, pixels.Color(r, g, b));
  pixels.show();
}
