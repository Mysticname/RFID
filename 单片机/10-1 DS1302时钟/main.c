#include <REGX52.H>
#include "LCD1602.h"
#include "DS1302.h"

// 定义倒计时时间（20分钟）
#define COUNTDOWN_MINUTES 20

// 倒计时结束标志
bit countdownFinished = 0;

// 倒计时开始时间（存储为秒）
unsigned long startTime = 0;

/**
  * @brief  获取当前时间总秒数
  * @param  无
  * @retval 当前时间的总秒数
  */
unsigned long GetCurrentTimeInSeconds()
{
    DS1302_ReadTime(); // 读取当前时间
    // 计算从00:00:00开始的秒数
    return (unsigned long)DS1302_Time[3] * 3600UL + 
           (unsigned long)DS1302_Time[4] * 60UL + 
           (unsigned long)DS1302_Time[5];
}

/**
  * @brief  设置倒计时开始时间
  * @param  无
  * @retval 无
  */
void StartCountdown()
{
    // 设置DS1302时间为0时0分0秒
    DS1302_Time[3] = 0; // 小时
    DS1302_Time[4] = 0; // 分钟
    DS1302_Time[5] = 0; // 秒钟
    DS1302_SetTime();   // 写入DS1302
    
    // 记录开始时间（0秒）
    startTime = 0;
}

/**
  * @brief  主函数
  * @param  无
  * @retval 无
  */
void main()
{
    LCD_Init();         // 初始化LCD
    DS1302_Init();      // 初始化DS1302
    
    // 显示静态字符
    LCD_ShowString(1,1,"CountDown: ");
    LCD_ShowString(2,1,"  :  ");  // 预留时间显示位置
    
    // 启动倒计时
    StartCountdown();
    
    while(1)
    {
        // 获取当前时间（从开始计时起的秒数）
        unsigned long currentTime = GetCurrentTimeInSeconds();
        
        // 计算已过去的时间（秒）
        unsigned long elapsedTime = currentTime - startTime;
        
        // 计算剩余时间（秒）
        unsigned long remainingTime = COUNTDOWN_MINUTES * 60 - elapsedTime;
			
         // 计算分钟和秒数
        unsigned char minutes = (unsigned char)(remainingTime / 60);
        unsigned char seconds = (unsigned char)(remainingTime % 60);
			
        // 检查是否倒计时结束
        if (remainingTime <= 0)
        {
            countdownFinished = 1;
            remainingTime = 0; // 确保不为负值
        }
        
       
        
        // 显示倒计时时间
        LCD_ShowNum(2,1,minutes,2); // 显示分钟
        LCD_ShowNum(2,4,seconds,2); // 显示秒钟
        
        // 如果倒计时结束，显示完成信息
        if (countdownFinished)
        {
            LCD_ShowString(1,1,"CountDown Finish!");
            // 在此可以添加其他操作，如蜂鸣器报警
        }
    }
}