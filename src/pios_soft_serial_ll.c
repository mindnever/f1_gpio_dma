/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SOFT_SERIAL Low level support functions
 * @brief PiOS Soft Serial Low level functionality
 * @{
 *
 * @file       pios_soft_serial_ll.c
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

#include "pios_soft_serial_ll.h"
#include "pios_irq.h"
#include "pios_exti.h"

#include <string.h>

#define EXTI_LINENONE 0

struct pios_soft_serial_ll_edgedetect_device {
    uint32_t exti_line;
};

#if !defined(PIOS_INCLUDE_FREERTOS)
# ifndef PIOS_SOFT_SERIAL_EDGE_DETECT_MAX_DEV
#  define PIOS_SOFT_SERIAL_EDGE_DETECT_MAX_DEV 5
# endif
static uint8_t edgedetect_dev_count;
static struct pios_soft_serial_ll_edgedetect_device edgedetect_device[PIOS_SOFT_SERIAL_EDGE_DETECT_MAX_DEV];
#endif /* PIOS_INCLUDE_FREERTOS */


int32_t PIOS_Soft_Serial_LL_EdgeDetect_Init(uint32_t *id, pios_soft_serial_ll_edgedetect_cb callback, uint32_t context)
{
#ifdef PIOS_INCLUDE_FREERTOS
    struct pios_soft_serial_ll_edgedetect_device *dev = (struct pios_soft_serial_ll_edgedetect_device *)pios_malloc(sizeof(*dev));
#else
    PIOS_DEBUG_Assert(edgedetect_dev_count < PIOS_SOFT_SERIAL_EDGE_DETECT_MAX_DEV);
    struct pios_soft_serial_ll_edgedetect_device *dev = &edgedetect_device[edgedetect_dev_count++];
#endif

    memset(dev, 0, sizeof(*dev));

    dev->exti_line = EXTI_LINENONE;

    return 0;
}

static bool PIOS_Soft_Serial_LL_EdgeDetect_Vector()
{
    return false;
}

void PIOS_Soft_Serial_LL_EdgeDetect_Configure(uint32_t id,
                                              const struct stm32_gpio *pin,
                                              enum PIOS_SOFT_SERIAL_LL_EdgeDetect_Polarity polarity)
{
    struct pios_soft_serial_ll_edgedetect_device *dev = (struct pios_soft_serial_ll_edgedetect_device *)id;
    // DeInit old one
    if(dev->exti_line != EXTI_LINENONE) {
        struct pios_exti_cfg cfg = {
            .vector = PIOS_Soft_Serial_LL_EdgeDetect_Vector,
            .line = dev->exti_line,
            .exti = {
                .init = {
                    .EXTI_Line = dev->exti_line,
                    .EXTI_Mode = EXTI_Mode_Interrupt,
                }
            }
        };
        PIOS_EXTI_DeInit(&cfg);
    }
    
    dev->exti_line = pin->init.GPIO_Pin;
    
    // Init new one
    if(dev->exti_line != EXTI_LINENONE) {
        struct pios_exti_cfg cfg = {
            .vector = PIOS_Soft_Serial_LL_EdgeDetect_Vector,
            .line = dev->exti_line,
            .pin = *pin,
            .exti = {
                .init = {
                    .EXTI_Line = dev->exti_line,
                    .EXTI_Mode = EXTI_Mode_Interrupt,
                    .EXTI_Trigger = polarity,
                    .EXTI_LineCmd = ENABLE,
                }
            },
            .irq = {
                .init = {
                    .NVIC_IRQChannelPreemptionPriority = PIOS_IRQ_PRIO_HIGHEST,
                    .NVIC_IRQChannelSubPriority = 0,
                    .NVIC_IRQChannelCmd = ENABLE,
                }
            }
        };
        
        PIOS_EXTI_Init(&cfg);
    }
}

void PIOS_Soft_Serial_LL_EdgeDetect_Cmd(uint32_t dev, FunctionalState NewState)
{

}

void PIOS_Soft_Serial_LL_GPIO_Init(struct pios_soft_serial_ll_gpio *llg, const struct stm32_gpio *pin)
{
    if(pin) {
        llg->pin = *pin;
    }

    PIOS_DEBUG_Assert(llg->pin.gpio);
    
    PIOS_IRQ_Disable();
    {
      GPIO_Init(llg->pin.gpio, &llg->pin.init);
    }
    PIOS_IRQ_Enable();
}
