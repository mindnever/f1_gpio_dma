#ifndef PIOS_TIM_H
#define PIOS_TIM_H

#include "pios.h"

uint32_t PIOS_TIM_Ck_Int(TIM_TypeDef *timer);

#define PIOS_TIM_CHANNEL_DIER_CCxDE(tim_chan) (TIM_DIER_CC1DE << (tim_chan >> 2))
#define PIOS_TIM_CHANNEL_DIER_CCxIE(tim_chan) (TIM_DIER_CC1IE << (tim_chan >> 2))

#endif /* PIOS_TIM_H */
