/**
 ******************************************************************************
 *
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_EXTI External Interrupt Handlers
 * @brief External interrupt handler functions
 * @{
 *
 * @file       pios_exti.c
 * @author     The OpenPilot Team, http://www.openpilot.org Copyright (C) 2010.
 * @brief      External Interrupt Handlers
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

#include "pios.h"
#include "pios_exti.h"
#ifdef PIOS_INCLUDE_EXTI

/* Map EXTI line to full config */
#define EXTI_MAX_LINES 16

static pios_exti_vector_t pios_exti_vector[EXTI_MAX_LINES];

static uint8_t PIOS_EXTI_line_to_IRQn(uint32_t line)
{
    switch (line) {
    case EXTI_Line0:
        return EXTI0_IRQn;

    case EXTI_Line1:
        return EXTI1_IRQn;

    case EXTI_Line2:
        return EXTI2_IRQn;

    case EXTI_Line3:
        return EXTI3_IRQn;

    case EXTI_Line4:
        return EXTI4_IRQn;

    case EXTI_Line5:
    case EXTI_Line6:
    case EXTI_Line7:
    case EXTI_Line8:
    case EXTI_Line9:
        return EXTI9_5_IRQn;

    case EXTI_Line10:
    case EXTI_Line11:
    case EXTI_Line12:
    case EXTI_Line13:
    case EXTI_Line14:
    case EXTI_Line15:
        return EXTI15_10_IRQn;
    }

    PIOS_Assert(0);
    return 0xFF;
}

static uint8_t PIOS_EXTI_line_to_index(uint32_t line)
{
    switch (line) {
    case EXTI_Line0: return 0;

    case EXTI_Line1: return 1;

    case EXTI_Line2: return 2;

    case EXTI_Line3: return 3;

    case EXTI_Line4: return 4;

    case EXTI_Line5: return 5;

    case EXTI_Line6: return 6;

    case EXTI_Line7: return 7;

    case EXTI_Line8: return 8;

    case EXTI_Line9: return 9;

    case EXTI_Line10: return 10;

    case EXTI_Line11: return 11;

    case EXTI_Line12: return 12;

    case EXTI_Line13: return 13;

    case EXTI_Line14: return 14;

    case EXTI_Line15: return 15;
    }

    PIOS_Assert(0);
    return 0xFF;
}

uint8_t PIOS_EXTI_gpio_port_to_exti_source_port(GPIO_TypeDef *gpio_port)
{
    switch ((uint32_t)gpio_port) {
    case (uint32_t)GPIOA: return GPIO_PortSourceGPIOA;

    case (uint32_t)GPIOB: return GPIO_PortSourceGPIOB;

    case (uint32_t)GPIOC: return GPIO_PortSourceGPIOC;

    case (uint32_t)GPIOD: return GPIO_PortSourceGPIOD;

    case (uint32_t)GPIOE: return GPIO_PortSourceGPIOE;

    case (uint32_t)GPIOF: return GPIO_PortSourceGPIOF;

    case (uint32_t)GPIOG: return GPIO_PortSourceGPIOG;
    }

    PIOS_Assert(0);
    return 0xFF;
}

int32_t PIOS_EXTI_Init(const struct pios_exti_cfg *cfg)
{
    PIOS_Assert(cfg);

    /* Connect this config to the requested vector */
    uint8_t line_index = PIOS_EXTI_line_to_index(cfg->line);

    if (pios_exti_vector[line_index]) {
        /* Someone else already has this mapped */
        return -1;
    }

    /* Bind the vector to the exti line */
    pios_exti_vector[line_index] = cfg->vector;

    /* Initialize the GPIO pin */
    GPIO_Init(cfg->pin.gpio, &cfg->pin.init);

    /* Set up the EXTI interrupt source */
    GPIO_EXTILineConfig(PIOS_EXTI_gpio_port_to_exti_source_port(cfg->pin.gpio),
                        __builtin_ctz(cfg->pin.init.GPIO_Pin));

    EXTI_Init(&cfg->exti.init);

    /* Enable the interrupt channel */
    {
        NVIC_InitTypeDef irq = cfg->irq.init;
        irq.NVIC_IRQChannel = PIOS_EXTI_line_to_IRQn(cfg->line);

        NVIC_Init(&irq);
    }

    return 0;
}

int32_t PIOS_EXTI_DeInit(const struct pios_exti_cfg *cfg)
{
    uint8_t line_index = PIOS_EXTI_line_to_index(cfg->line);

    if (pios_exti_vector[line_index] == cfg->vector) {
        EXTI_InitTypeDef disable = cfg->exti.init;
        disable.EXTI_LineCmd = DISABLE;

        EXTI_Init(&disable);
        pios_exti_vector[line_index] = 0;

        return 0;
    }

    return -1;
}

static bool PIOS_EXTI_generic_irq_handler(uint8_t line_index)
{
    return pios_exti_vector[line_index] ? pios_exti_vector[line_index]() : false;
}

#ifdef PIOS_INCLUDE_FREERTOS
#define PIOS_EXTI_HANDLE_LINE(line, woken)                      \
    if (EXTI_GetITStatus(EXTI_Line##line) != RESET) {       \
        EXTI_ClearITPendingBit(EXTI_Line##line);        \
        woken = PIOS_EXTI_generic_irq_handler(line) ? pdTRUE : woken; \
    }
#else
#define PIOS_EXTI_HANDLE_LINE(line, woken)                      \
    if (EXTI_GetITStatus(EXTI_Line##line) != RESET) {       \
        EXTI_ClearITPendingBit(EXTI_Line##line);        \
        PIOS_EXTI_generic_irq_handler(line);            \
    }
#endif

/* Bind Interrupt Handlers */

static void PIOS_EXTI_0_irq_handler(void)
{
#ifdef PIOS_INCLUDE_FREERTOS
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
#else
    __attribute__((unused)) bool xHigherPriorityTaskWoken; // dummy variable
#endif
    PIOS_EXTI_HANDLE_LINE(0, xHigherPriorityTaskWoken);
#ifdef PIOS_INCLUDE_FREERTOS
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
#endif
}
void EXTI0_IRQHandler(void) __attribute__((alias("PIOS_EXTI_0_irq_handler")));

static void PIOS_EXTI_1_irq_handler(void)
{
#ifdef PIOS_INCLUDE_FREERTOS
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
#else
    __attribute__((unused)) bool xHigherPriorityTaskWoken; // dummy variable
#endif
    PIOS_EXTI_HANDLE_LINE(1, xHigherPriorityTaskWoken);
#ifdef PIOS_INCLUDE_FREERTOS
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
#endif
}
void EXTI1_IRQHandler(void) __attribute__((alias("PIOS_EXTI_1_irq_handler")));

static void PIOS_EXTI_2_irq_handler(void)
{
#ifdef PIOS_INCLUDE_FREERTOS
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
#else
    __attribute__((unused)) bool xHigherPriorityTaskWoken; // dummy variable
#endif
    PIOS_EXTI_HANDLE_LINE(2, xHigherPriorityTaskWoken);
#ifdef PIOS_INCLUDE_FREERTOS
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
#endif
}
void EXTI2_IRQHandler(void) __attribute__((alias("PIOS_EXTI_2_irq_handler")));

static void PIOS_EXTI_3_irq_handler(void)
{
#ifdef PIOS_INCLUDE_FREERTOS
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
#else
    __attribute__((unused)) bool xHigherPriorityTaskWoken; // dummy variable
#endif
    PIOS_EXTI_HANDLE_LINE(3, xHigherPriorityTaskWoken);
#ifdef PIOS_INCLUDE_FREERTOS
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
#endif
}
void EXTI3_IRQHandler(void) __attribute__((alias("PIOS_EXTI_3_irq_handler")));

static void PIOS_EXTI_4_irq_handler(void)
{
#ifdef PIOS_INCLUDE_FREERTOS
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
#else
    __attribute__((unused)) bool xHigherPriorityTaskWoken; // dummy variable
#endif
    PIOS_EXTI_HANDLE_LINE(4, xHigherPriorityTaskWoken);
#ifdef PIOS_INCLUDE_FREERTOS
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
#endif
}
void EXTI4_IRQHandler(void) __attribute__((alias("PIOS_EXTI_4_irq_handler")));

static void PIOS_EXTI_9_5_irq_handler(void)
{
#ifdef PIOS_INCLUDE_FREERTOS
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
#else
    __attribute__((unused)) bool xHigherPriorityTaskWoken; // dummy variable
#endif
    PIOS_EXTI_HANDLE_LINE(5, xHigherPriorityTaskWoken);
    PIOS_EXTI_HANDLE_LINE(6, xHigherPriorityTaskWoken);
    PIOS_EXTI_HANDLE_LINE(7, xHigherPriorityTaskWoken);
    PIOS_EXTI_HANDLE_LINE(8, xHigherPriorityTaskWoken);
    PIOS_EXTI_HANDLE_LINE(9, xHigherPriorityTaskWoken);
#ifdef PIOS_INCLUDE_FREERTOS
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
#endif
}
void EXTI9_5_IRQHandler(void) __attribute__((alias("PIOS_EXTI_9_5_irq_handler")));

static void PIOS_EXTI_15_10_irq_handler(void)
{
#ifdef PIOS_INCLUDE_FREERTOS
    portBASE_TYPE xHigherPriorityTaskWoken = pdFALSE;
#else
    __attribute__((unused)) bool xHigherPriorityTaskWoken; // dummy variable
#endif
    PIOS_EXTI_HANDLE_LINE(10, xHigherPriorityTaskWoken);
    PIOS_EXTI_HANDLE_LINE(11, xHigherPriorityTaskWoken);
    PIOS_EXTI_HANDLE_LINE(12, xHigherPriorityTaskWoken);
    PIOS_EXTI_HANDLE_LINE(13, xHigherPriorityTaskWoken);
    PIOS_EXTI_HANDLE_LINE(14, xHigherPriorityTaskWoken);
    PIOS_EXTI_HANDLE_LINE(15, xHigherPriorityTaskWoken);
#ifdef PIOS_INCLUDE_FREERTOS
    portEND_SWITCHING_ISR(xHigherPriorityTaskWoken);
#endif
}
void EXTI15_10_IRQHandler(void) __attribute__((alias("PIOS_EXTI_15_10_irq_handler")));

#endif /* PIOS_INCLUDE_EXTI */

/**
 * @}
 * @}
 */
