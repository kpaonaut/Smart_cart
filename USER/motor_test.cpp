#include "stm32f10x.h"

void TIMx_PWMInit(uint16_t prescaler, uint16_t period, uint16_t pulse)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
    TIM_OCInitTypeDef TimOCInitStructure;
    /* GPIO settings below */
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE); // GPIO clock
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_AF_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1 | GPIO_Pin_2 | GPIO_Pin_3;
    GPIO_Init(GPIOA, &GPIO_InitStructure);
    /* GPIO settings above */
    /* TIMx clock settings below */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);
    TIM_TimeBaseInitStructure.TIM_Prescaler = prescaler;
    TIM_TimeBaseInitStructure.TIM_Period = period;
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseInit(TIM5, &TIM_TimeBaseInitStructure);
    /* TIMx clock settings above */
    /* PWM config below */
    TimOCInitStructure.TIM_OCMode = TIM_OCMode_PWM1;
    TimOCInitStructure.TIM_OCPolarity = TIM_OCPolarity_High;
    TimOCInitStructure.TIM_Pulse = pulse; // pulse evaluate
    TimOCInitStructure.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OC1Init(TIM1, &TimOCInitStructure);
    TIM_OC2Init(TIM1, &TimOCInitStructure);
    TIM_OC3Init(TIM1, &TimOCInitStructure);
    TIM_OC4Init(TIM1, &TimOCInitStructure); // channel 1-4 config
    TIM_OC1PreloadConfig(TIM5, TIM_OCPreload_Enable);
    TIM_OC2PreloadConfig(TIM5, TIM_OCPreload_Enable);
    TIM_OC3PreloadConfig(TIM5, TIM_OCPreload_Enable);
    TIM_OC4PreloadConfig(TIM5, TIM_OCPreload_Enable); // channel 1-4 config
    TIM_ARRPreloadConfig(TIM5, ENABLE);
    TIM_Cmd(TIM5, ENABLE);
    TIM_CtrlPWMOutputs(TIM5, ENABLE);
    /* PWM config above */
    return;
}

#define encoder_counter_reload 8000 // rotate each round, encoder around 200
void EncodeInit(void)
{
    GPIO_InitTypeDef GPIO_InitStructure; // PB6, PB7
    TIM_TimeBaseInitTypeDef  TIM4_TimeBaseStructure; // TIM4 CH1, 2 - PB6, 7 - Encoder_R1
    TIM_ICInitTypeDef TIM4_ICInitStructure;

    /* TIM4 Clock && GPIO enable */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    /* Configure PB6, 7 */
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_6 | GPIO_Pin_7;       
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING; 
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; 
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    /* Configure TIM4 */
    TIM4_TimeBaseStructure.TIM_Prescaler = 0; // TIM clock = 72MHz        
    TIM4_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM4_TimeBaseStructure.TIM_Period = encoder_counter_reload - 1; // what value?
    TIM4_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM4_TimeBaseStructure.TIM_RepetitionCounter = 0;
    TIM_TimeBaseInit(TIM4, &TIM4_TimeBaseStructure);

    TIM_EncoderInterfaceConfig(TIM4, TIM_EncoderMode_TI12, TIM_ICPolarity_Rising,
                               TIM_ICPolarity_Rising); // 配置编码器模式，4倍频??? 需要测试一下！是不是要改成双向计数！

    TIM_ICStructInit(&TIM4_ICInitStructure);
    TIM4_ICInitStructure.TIM_ICFilter = 6; // filter, choose what value?
    TIM_ICInit(TIM4, &TIM4_ICInitStructure);

    TIM_ClearFlag(TIM4, TIM_FLAG_Update);
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);

    TIM_SetCounter(TIM4, 0); // TIM4 -> CNT = 0
    TIM_Cmd(TIM4, ENABLE);
}

static int last_cnt, counts, v; // signed! cuz v can be negative
inline int get_encoder_counts()
{
    counts = TIM4 -> CNT - last_cnt;
    if (counts < - encoder_counter_reload / 2) // counter overflowed
        counts = encoder_counter_reload + v;
    last_cnt = TIM4 -> CNT;
    return counts;
}

// THESE PARSMS NEED TO BE CUSTOMIZED!
#define spokes_num 11
#define reduction_ratio 19.8
#define sysTick_period 50 // update v every 50ms
#define wheel_perimeter 220 // in mm

extern "C" void SysTick_Handler() // does "void" have to be written within ()?
{
    v = get_encoder_counts() * wheel_perimeter * 1000 / (spokes_num * reduction_ratio * sysTick_period); // update velocity
    // v in mm/s; all vals must be signed int32 so that the multiplication doesn't overflow
    return;
}

void systickInit()
{
    SysTick_Config(SystemCoreClock / 1000 * sysTick_period); // SystemCoreClock == 72MHz
    return;
}
// 测试后把上面的代码扩展成两个轮子的！
// 数脉冲宽度需要取五个或多个（维护队列）做平均滤波！

/* initialize TIM8 for input detection for encoder  */
// void TIM8_Cap_Init(u16 arr, u16 psc)
// {      
//     TIM_ICInitTypeDef  TIM5_ICInitStructure;
//     TIM_TimeBaseInitTypeDef  TIM_TimeBaseStructure;
//     NVIC_InitTypeDef NVIC_InitStructure;
  
//     RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM5, ENABLE);    //使能TIM5时钟  
      
//     //TIM5的配置  
//     TIM_TimeBaseStructure.TIM_Period = arr;     //重装载值  
//     TIM_TimeBaseStructure.TIM_Prescaler =psc;   //分频系数   
//     TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;      //tDTS = tCK_INT  
//     TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;  //向上计数  
//     TIM_TimeBaseInit(TIM5, &TIM_TimeBaseStructure);   
    
//     //TIM5输入捕获配置  
//     TIM5_ICInitStructure.TIM_Channel = TIM_Channel_1; //我们用通道1  
//     TIM5_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;    //上升沿捕获  
//     TIM5_ICInitStructure.TIM_ICSelection = TIM_ICSelection_DirectTI; //映射到TI1  
//     TIM5_ICInitStructure.TIM_ICPrescaler = TIM_ICPSC_DIV1;   //无预分频器   
//     TIM5_ICInitStructure.TIM_ICFilter = 0x00; //IC1F=0000 ，无滤波器  
//     TIM_ICInit(TIM5, &TIM5_ICInitStructure);  
      
//     //中断优先级配置  
//     NVIC_InitStructure.NVIC_IRQChannel = TIM5_IRQn;    
//     NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;    
//     NVIC_InitStructure.NVIC_IRQChannelSubPriority = 0;    
//     NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;   
//     NVIC_Init(&NVIC_InitStructure);    
      
//     TIM_ITConfig(TIM5,TIM_IT_Update|TIM_IT_CC1,ENABLE);//打开更新中断和捕获中断  
      
//     TIM_Cmd(TIM5,ENABLE );  //使能定时器5  
     
// } 

int NOTmain()
{
    uint16_t prescaler = 72 - 1; // 0~65535
    uint16_t period = 49;
    uint16_t pulse = 30;
    TIMx_PWMInit(prescaler, period, pulse); // set timer for motor
    systickInit();
    while (1);
	  return 0;
}