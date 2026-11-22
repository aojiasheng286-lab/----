#ifndef __SERVO_H
#define __SERVO_H

#include "main.h"

void servo_init(void);            // 上电调一次：启动 PWM，摆到 90 度
void servo_set_angle(int angle);  // 设定角度（0 ~ 180）
int  servo_get_angle(void);       // 读当前角度

#endif
