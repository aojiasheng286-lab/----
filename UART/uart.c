#define message_len 17
#include "mpu6050.h"
#include "uart.h"
#include "usart.h"
#include "key.h"    
#include "my_adc.h"
#include <stdio.h>
#include "page.h"
#include "servo.h"  
#include <string.h>


static uint8_t rx_data;// 接收上位机发来的字符
static char tx_buff[160];    //发送缓冲区
static uint8_t uart_event = 0;   //抢下来的按键事件，等发送时刻用

















//启动回调函数
void uart_start_receive(void)
{
	HAL_UART_Receive_IT(&huart1, &rx_data, 1);//中断接收，传接收缓冲区的地址，rx.data给了static:
}

//后面可以试一下idle+dma
















static uint8_t do_command(char c)//发消息要注意避免这些玩意开头
{
    if (c == '1')
    {
        mode = MODE_BREATH;
    }
    else if (c == '2')
    {
        mode = MODE_SLOW;
    }
    else if (c == '3')
    {
        mode = MODE_FAST;
    }
    else if (c == '4')
    {
        mode = MODE_FLOW;
    }
    else if (c == '5')
    {
        mode = MODE_OFF;
    }
    else if (c == 'p' || c == 'P')
    {
        page_request_next();
        return 1;
    }
    else if (c == 'a')            // 舵机角度 -30
    {
        servo_set_angle(servo_get_angle() - 30);
        return 1;
    }
    else if (c == 'd')            // 舵机角度 +30
    {
        servo_set_angle(servo_get_angle() + 30);
        return 1;
    }
    else
    {
        return 0;// 不是命令字符
    }

    return 1;
}
















//先判断下 发来的信息，是消息还是命令，\n||\r这一行表示结束，如果字符两个及以上就判断为消息，存入历史消息
                                                       // 入锅一个的话就是上面的模式






                                                       












//中断回调函数
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	//这里必须STATIC不然每次都是0
    static char    line[message_len];// 正在收的这一行
    static uint8_t len = 0;// 这一行已经收了多少个字符

    if (huart->Instance != USART1)
    {
        return;
    }







    //消息
    if (rx_data == '\n' || rx_data == '\r')
    {
        line[len] = '\0';               //补上字符串的结尾

        if (len >= 2)                   //2 个字符以上 -> 当成一条消息存起来
        {
            msg_push(line);
        }

        len = 0;                        //清空，准备收下一行
    }
    


    
    else
    {
        uint8_t is_command = 0;

        /* 如果这一行还是空的，先看看这个字符是不是命令 *///have 5 apples
        if (len == 0)
        {
            is_command = do_command(rx_data);
        }

        
        if (is_command == 0)
        {
            if (len < message_len - 1)
            {
                line[len] = rx_data;
                len++;
            }
        }
    }

    HAL_UART_Receive_IT(&huart1, &rx_data, 1);   // 别忘了重新打开接收，被坑过一次了
		//再回到上面一次，搞完之后去搞个最强下届合金套好吧
}
























// 串口错误回调
// 发生溢出(ORE)等错误后 HAL 会终止接收，不重新挂上就永久收不到数据，只能复位
void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART1)
    {
        __HAL_UART_CLEAR_OREFLAG(huart);
        huart->RxState = HAL_UART_STATE_READY;
        HAL_UART_Receive_IT(&huart1, &rx_data, 1);
    }
}







//         uart_process_task(); 

//         nav_key_task();        
//         led_mode_task();       
//         page_task();          












//串口定时发送任务
void uart_process_task(void)//main.c里面调用
{
    //key_event 几十微秒后就被 led_mode_task 清掉了，而串口 500ms 才发一次
    //所以先抢一份存着，不然等发送的时候它早就变回 0 了
    if (key_event != 0)
    {
        uart_event = key_event;
    }
//这里的static是保险，防止刷爆了
    static uint32_t last_uart_time = 0;

    if (HAL_GetTick() - last_uart_time >= 500)//定时500ms发一次啊
    {
        last_uart_time = HAL_GetTick();
        float v = adc_get_voltage();

        char prefix[16] = "";//这是给按键事件的变量

        //这一拍期间按过键，就把事件做成前缀，跟数据一起发出去
        if (uart_event != 0)
        {
            sprintf(prefix, "[KEY%d] ", uart_event);
            uart_event = 0;
        }

        //上位机显示的数据跟着当前页面走
        switch (page_current())//这里不能直接用page，因为page是static变量，相当于是必要的封装入口，只能读不能改
        {
            case PAGE_ADC:
                sprintf(tx_buff, "%s[P1 ADC] V=%.2f S=%d\r\n",
                        prefix, v, servo_get_angle());
                break;

            case PAGE_MPU:
                sprintf(tx_buff, "%s[P2 MPU] A=%.2f,%.2f,%.2f G=%.0f,%.0f,%.0f T=%.1f\r\n",
                        prefix,
                        g_mpu.ax_g, g_mpu.ay_g, g_mpu.az_g,
                        g_mpu.gx_dps, g_mpu.gy_dps, g_mpu.gz_dps, g_mpu.temp_c);
                break;

            case PAGE_LED:
                sprintf(tx_buff, "%s[P3 LED] mode=%d C=%d,%d,%d%%\r\n",
                        prefix, mode,
                        page_led_duty(1), page_led_duty(2), page_led_duty(3));//返回的数据
                break;

            case PAGE_MSG:
                sprintf(tx_buff, "%s[P4 MSG] %s\r\n", prefix, page_msg_status());
                break;

            default:
                sprintf(tx_buff, "%s[P?]\r\n", prefix);
                break;
        }

        HAL_UART_Transmit_DMA(&huart1, (uint8_t*)tx_buff, strlen(tx_buff));
    }
}