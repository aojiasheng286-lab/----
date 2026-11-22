#include "servo.h"
#include "tim.h"        // 用到 htim2


//TIM2  PSC = 71、ARR = 19999
       
 

static int servo_angle = 90;    // 当前角度，上电先摆到中间

void servo_init(void)
{
    HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);   // 启动 PWM 输出
    servo_set_angle(90);                        // 摆到 90 度
}

void servo_set_angle(int angle)
{
    uint16_t pulse;

    if (angle < 0)                  // 限制在 0 ~ 180，防止舵机撞到底
    {
        angle = 0;
    }

    if (angle > 180)
    {
        angle = 180;
    }

    servo_angle = angle;

    /* 角度换算成脉宽：
       0 度   -> 500
       180 度 -> 500 + 2000 = 2500 */
    pulse = 500 + (uint16_t)(angle * 2000 / 180);

    __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_1, pulse);
}

int servo_get_angle(void)
{
    return servo_angle;
}
