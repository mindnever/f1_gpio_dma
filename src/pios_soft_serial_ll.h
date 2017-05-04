/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SOFT_SERIAL Low level support functions
 * @brief PiOS Soft Serial Low level functionality
 * @{
 *
 * @file       pios_soft_serial_ll.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief      Soft Serial functions header
 * @see        The GNU Public License (GPL) Version 3
 *
 *****************************************************************************/
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */
#ifndef PIOS_SOFT_SERIAL_LL_H
#define PIOS_SOFT_SERIAL_LL_H

#include "pios.h"

typedef void (*pios_soft_serial_ll_edgedetect_cb)(uint32_t dev, uint32_t context);

int32_t PIOS_Soft_Serial_LL_EdgeDetect_Init(uint32_t *dev, pios_soft_serial_ll_edgedetect_cb callback, uint32_t context);

enum PIOS_SOFT_SERIAL_LL_EdgeDetect_Polarity {
    PIOS_SOFT_SERIAL_LL_EDGEDETECT_RISING = EXTI_Trigger_Rising,
    PIOS_SOFT_SERIAL_LL_EDGEDETECT_FALLING = EXTI_Trigger_Falling,
};

void PIOS_Soft_Serial_LL_EdgeDetect_Configure(uint32_t dev,
                                              const struct stm32_gpio *pin,
                                              enum PIOS_SOFT_SERIAL_LL_EdgeDetect_Polarity polarity);

void PIOS_Soft_Serial_LL_EdgeDetect_Cmd(uint32_t dev, FunctionalState NewState);

struct pios_soft_serial_ll_gpio {
    struct stm32_gpio pin;
};

void PIOS_Soft_Serial_LL_GPIO_Init(struct pios_soft_serial_ll_gpio *llg, const struct stm32_gpio *pin);


#endif /* PIOS_SOFT_SERIAL_LL_H */
