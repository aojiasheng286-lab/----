// #include "clock.h"
// #include <stdio.h>

  
// static uint32_t clock_ms = 0;

// /* 上次累加时的时刻 */
// static uint32_t clock_last = 0;

// /* 1 = 正在跑，0 = 停着 */
// static uint8_t clock_run = 0;


// void clock_init(void)
// {
//     clock_ms   = 0;
//     clock_last = HAL_GetTick();
//     clock_run  = 0;
// }

// /* 主循环每一圈调一次，负责把时间往前推 */
// void clock_task(void)
// {
//     uint32_t now = HAL_GetTick();

//     if (clock_run == 1)
//     {
//         clock_ms = clock_ms + (now - clock_last);
//     }

    
//     clock_last = now;
// }

// void clock_start(void)
// {
//     clock_last = HAL_GetTick();//记下起点
//     clock_run  = 1;
// }

// void clock_pause(void)
// {
//     clock_run = 0;
// }

// void clock_clear(void)
// {
//     clock_run = 0;
//     clock_ms  = 0;
// }

// uint32_t clock_get_ms(void)
// {
//     return clock_ms;
// }

// uint8_t clock_is_running(void)
// {
//     return clock_run;
// }



// //直接暴力换算
// void clock_to_hms(uint32_t ms, char *buf)
// {
//     uint32_t total_s = ms / 1000; // 一共多少整秒
//     uint32_t h       = total_s / 3600;         // 多少整小时
//     uint32_t rest    = total_s - h * 3600;     // 扣掉小时，剩下多少秒
//     uint32_t m       = rest / 60;              // 剩下多少整分钟
//     uint32_t s       = rest - m * 60;          // 再扣掉分钟，就是秒

//     sprintf(buf, "%02d:%02d:%02d", (int)h, (int)m, (int)s);
// }


