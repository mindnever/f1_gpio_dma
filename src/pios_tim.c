#include "pios_tim.h"

#define PERIPH_BASE_MASK 0xffff0000

uint32_t PIOS_TIM_Ck_Int(__attribute__((unused)) TIM_TypeDef *timer)
{
    RCC_ClocksTypeDef clocks;

    RCC_GetClocksFreq(&clocks);

    uint32_t timer_clock = 0;
    uint32_t apb_hclk_div = 0;

    switch((uint32_t)timer & PERIPH_BASE_MASK)
    {
        case APB1PERIPH_BASE:
            timer_clock = clocks.PCLK1_Frequency;
            apb_hclk_div = RCC->CFGR & RCC_CFGR_PPRE1_2;
            break;
        case APB2PERIPH_BASE:
            timer_clock = clocks.PCLK2_Frequency;
            apb_hclk_div = RCC->CFGR & RCC_CFGR_PPRE2_2;
            break;
    }

    if(apb_hclk_div) {
        timer_clock *= 2;
    }

    return timer_clock;
}

