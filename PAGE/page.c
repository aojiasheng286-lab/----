#include "page.h"
#include "oled.h"
#include "key.h"        // key_event / ev_short / mode
#include "my_adc.h"     // adc_get_voltage()
#include "mpu6050.h"    // g_mpu
#include "tim.h"
#include <stdio.h>
#include <string.h>

///page.c
   //管 OLED 显示：现在第几页、每页画什么
   //KEY3 翻页，上位机发 p 也能翻
 //  6 个页面：ADC / MPU / LED / 消息 / -------秒表 / ----------倒计时（燃尽了） 


/* ===== 页面状态 ===== */

static volatile PageId  page           = PAGE_ADC;  // 当前页
static volatile uint8_t page_next_flag = 0;         // 置 1 = 请求切下一页

void page_request_next(void)//这玩意是给上位机发P用的
{
    page_next_flag = 1;
}

PageId page_current(void)//void表示不接受任何参数
{
    return page;
}




















//三个键都是下拉输入，松开读到0，按下读到1


//KEY3：翻页
static uint8_t key3_pressed(void)
{
    static uint8_t last = 0;   
    static uint8_t cnt  = 0;//电平变了以后，持续了多少毫秒
    uint8_t now = HAL_GPIO_ReadPin(KEY3_GPIO_Port, KEY3_Pin);

    if (now == last)           
    {
        cnt = 0;
        return 0;
    }

    cnt++;                     

    if (cnt < 20)              
    {
        return 0;
    }

    cnt  = 0;
    last = now;               

    if (now == 1)             
    {
        return 1;
    }

    return 0;                
}

//KEY4：消息上一条
static uint8_t key4_pressed(void)
{
    static uint8_t last = 0;   
    static uint8_t cnt  = 0;   
    uint8_t now = HAL_GPIO_ReadPin(KEY4_GPIO_Port, KEY4_Pin);

    if (now == last)           
    {
        cnt = 0;
        return 0;
    }

    cnt++;                     
		

    if (cnt < 20)              
    {
        return 0;
    }

    cnt  = 0;
    last = now;                

    if (now == 1)            
    {
        return 1;
    }

    return 0;                  
}

//KEY5：消息下一条
static uint8_t key5_pressed(void)
{
    static uint8_t last = 0;   
    static uint8_t cnt  = 0;   
    uint8_t now = HAL_GPIO_ReadPin(KEY5_GPIO_Port, KEY5_Pin);

    if (now == last)           
    {
        cnt = 0;
        return 0;
    }

    cnt++;                   

    if (cnt < 20)             //消抖判定
    {
        return 0;
    }

    cnt  = 0;
    last = now;                

    if (now == 1)           
    {
        return 1;
    }

    return 0;                 
	}
static volatile uint8_t msg_up_flag   = 0;   // KEY4 按了一下 消息上一条
static volatile uint8_t msg_down_flag = 0;   // KEY5 按了一下消息下一条

	














//扫描入口，1ms 进一次
void page_key_scan(void)
{
    if (key3_pressed())        // KEY3：切下一页
    {
        page_next_flag = 1;
    }

    if (key4_pressed())        // KEY4：消息上一条
    {
        msg_up_flag = 1;
    }

    if (key5_pressed())        // KEY5：消息下一条
    {
        msg_down_flag = 1;
    }
}














//10行17列





//消息历史
#define MSG_MAX 10    
#define MSG_LEN 17    

static char    msg[MSG_MAX][MSG_LEN];
static uint8_t msg_lie = 0;  
static uint8_t msg_view = 0;  

void msg_push(const char *s)
{
    uint8_t i;

    if (msg_lie < MSG_MAX)                    
    {
        strncpy(msg[msg_lie], s, MSG_LEN - 1);//防止溢出
        msg[msg_lie][MSG_LEN - 1] = '\0';
        msg_lie++;
    }
    else                                    
    {
        for (i = 0; i < MSG_MAX - 1; i++)     
        {   
            strcpy(msg[i], msg[i + 1]);
        }

        strncpy(msg[MSG_MAX - 1], s, MSG_LEN - 1);   
        msg[MSG_MAX - 1][MSG_LEN - 1] = '\0';
    }

    msg_view = msg_lie - 1;                   
}





















//前后切消息
void nav_key_task(void)
{
    if (msg_up_flag == 1)                    // KEY4 按了一下
    {
        msg_up_flag = 0;

        if (page_current() == PAGE_MSG)      // 消息页：上一条//意思是在消息页面那一页
        {
            if (msg_view > 0)                // 还没到第一条，就往前挪一格
            {
                msg_view--;
            }
        }

    }

    if (msg_down_flag == 1)                  // KEY5 按了一下
    {
        msg_down_flag = 0;

        if (page_current() == PAGE_MSG)      // 消息页：下一条
        {
            if (msg_view + 1 < msg_lie)      // 还没到最后一条，就往后挪一格
            {
                msg_view++;
            }
        }

    }
}




















//串口
///第 ch 号灯（1/2/3）的亮度百分比
uint16_t page_led_duty(uint8_t ch)//返回比较寄存器的值//而且还会返回到串口那里去

{
    if (ch == 1)
    {
        return (uint16_t)(__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_1) / 10);
    }

    if (ch == 2)
    {
        return (uint16_t)(__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_2) / 10);
    }

    return (uint16_t)(__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_3) / 10);
}














//串口
//显示没消息判断//返回到串口那里
const char *page_msg_status(void)
{
    static char s[48];

    if (msg_lie == 0)
    {
        strcpy(s, "no message");
    }
    else
    {
        sprintf(s, "%d/%d %s", msg_view + 1, msg_lie, msg[msg_view]);
    }

    return s;
}






//-------------------------------------------------------------------------------




//这个line_text直接调用 OLED_PrintString ,keysking写的，用着不烧心，
//这里相当于得到第一个字符的地址，然后用oled.c里面的函数推着往后走
static void line_text(uint8_t row, const char *s)//const这个函数保证只读、不改
{//（字体 16 像素高，所以 ×16）
    OLED_PrintString(0, row * 16 ,(char *)s, &font16x16, OLED_COLOR_NORMAL);
}
//line_value 则是“画一行 名字=数值”：
static void line_value(uint8_t row, const char *name, float v)
{
    char buf[24];

    sprintf(buf, "%s=%.2f", name, v);
    line_text(row, buf);
}










//电压
static void page_render_adc(void)
{
    float   v = adc_get_voltage();
    uint8_t w;

    line_text(0, "1/4  ADC");//得到的就是第1个只是1的那个内存地址，然后内存地址加一往后推得到他们的字符。
    line_value(1, "V", v);


    //下面是画图
    w = (uint8_t)(v / 3.3f * 120.0f);     // 3.3V 对应满宽 120 像素，//绘制一个图像

    if (w > 120)
    {
        w = 120;
    }

    OLED_DrawRectangle(4, 40, 120, 16, OLED_COLOR_NORMAL);
    OLED_DrawFilledRectangle(4, 40, w, 16, OLED_COLOR_NORMAL);
}








//六轴
static void page_render_mpu(void)
{
    char buf[24];//全部重置

    line_text(0, "2/4  MPU");

    sprintf(buf, "A=%.1f,%.1f,%.1f", g_mpu.ax_g, g_mpu.ay_g, g_mpu.az_g);
    line_text(1, buf);

    sprintf(buf, "G=%.0f,%.0f,%.0f", g_mpu.gx_dps, g_mpu.gy_dps, g_mpu.gz_dps);
    line_text(2, buf);

    sprintf(buf, "T=%.1f", g_mpu.temp_c);
    line_text(3, buf);
}






//-------------------------------------------------------------------------------








//小灯
static const char *const mode_name[] = { "BREATH", "SLOW", "FAST", "FLOW", "OFF" };

static void page_render_led(void)//给屏幕用的啊
{
    /* TIM3 的 ARR = 999，所以 CCR 是 0~1000，除以 10 正好是 0~100% */
    uint16_t c1 = (uint16_t)__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_1);
    uint16_t c2 = (uint16_t)__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_2);
    uint16_t c3 = (uint16_t)__HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_3);

    char buf[24];//buf传进去的都是地址

    line_text(0, "3/4  LED");

    uint8_t m = mode;

    if (m >= MODE_MAX)              
    {
        m = 0;
    }

    sprintf(buf, "mode%d %s", m, mode_name[m]);
    line_text(1, buf);

    sprintf(buf, "C1=%d%% C2=%d%%", c1 / 10, c2 / 10);//两个%%才能输出%
    line_text(2, buf);

    sprintf(buf, "C3=%d%%", c3 / 10);
    line_text(3, buf);
}


















//消息
static void page_render_msg(void)
{
    char buf[MSG_LEN];

    if (msg_lie == 0)
    {
        line_text(0, "4/4  MSG");
        line_text(1, "NO MESSAGE");
        return;
    }

    if (msg_view >= msg_lie)
    {
        msg_view = 0;
    }

    sprintf(buf, "4/4  MSG %d/%d", msg_view + 1, msg_lie);
    line_text(0, buf);

    line_text(1, msg[msg_view]);      // 最多 16 个字符
}












//屏幕上显示的东西
static void page_render(void)
{
    OLED_NewFrame();                 // 清显存

    switch (page)
    {
        case PAGE_ADC:
            page_render_adc();
            break;

        case PAGE_MPU:
            page_render_mpu();
            break;

        case PAGE_LED:
            page_render_led();
            break;

        case PAGE_MSG:
            page_render_msg();
            break;


        default:
            page = PAGE_ADC;         
            break;
    }

    OLED_ShowFrame();                
}














void page_task(void)//main.c里面调用
{
    static uint32_t last = 0;

    
    if (page_next_flag)
    {
        page_next_flag = 0;
        switch (page)
        {
            case PAGE_ADC:  page = PAGE_MPU; break;//这里不一样是因为翻页系统
            case PAGE_MPU:  page = PAGE_LED; break;
            case PAGE_LED:  page = PAGE_MSG; break;

            default:        page = PAGE_ADC; break;// 最后一页再按 回到第一页
        }
    }

    //每 500ms 采一次数据，刷新一屏
    if (HAL_GetTick() - last >= 500)
    {
        last = HAL_GetTick();

        MPU6050_ReadAll(&g_mpu);// 不管在哪一页都采，串口那边要用
        page_render();
    }
}
