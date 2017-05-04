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

typedef enum {
    PIOS_SOFT_SERIAL_LL_EDGEDETECT_RISING,
    PIOS_SOFT_SERIAL_LL_EDGEDETECT_FALLING,
} PIOS_SOFT_SERIAL_LL_EdgeDetect_Polarity;

struct pios_soft_serial_ll_edgedetect_config {
    GPIO_TypeDef *gpio;
    uint16_t pin;

    PIOS_SOFT_SERIAL_LL_EdgeDetect_Polarity polarity;

    TIM_TypeDef *timer;
    uint8_t tim_channel;
};

void PIOS_Soft_Serial_LL_EdgeDetect_Configure(uint32_t dev, const struct pios_soft_serial_ll_edgedetect_config *config);

void PIOS_Soft_Serial_LL_EdgeDetect_Cmd(uint32_t dev, FunctionalState NewState);

struct pios_soft_serial_ll_gpio {
    volatile uint32_t *mode_bb;
#if defined(STM32F1)
    uint32_t pupd_value;
    volatile uint32_t *pupd_bsrr;
#endif
};

#define PIOS_SOFT_SERIAL_LL_GPIO_ITYPE_PULLUP    0b0001
#define PIOS_SOFT_SERIAL_LL_GPIO_ITYPE_PULLDOWN  0b0000

#define PIOS_SOFT_SERIAL_LL_GPIO_OTYPE_PP        0b0010
#define PIOS_SOFT_SERIAL_LL_GPIO_OTYPE_OD        0b0000

#if defined(STM32F3) || defined(STM32F4)
#define PIOS_SOFT_SERIAL_LL_GPIO_INPUT(llg) \
    (llg).mode_bb[0] = 0; /* GPIO_MODERx0 */ \
    (llg).mode_bb[1] = 0; /* GPIO_MODERx1 */

#define PIOS_SOFT_SERIAL_LL_GPIO_OUTPUT(llg) \
    (llg).mode_bb[0] = 0; /* GPIO_MODERx0 */ \
    (llg).mode_bb[1] = 1; /* GPIO_MODERx1 */

#elif defined(STM32F1)
/* Input with PuPd
 * MODE 00, CNF 01
 */
#define PIOS_SOFT_SERIAL_LL_GPIO_INPUT(llg) \
    (llg).mode_bb[0] = 0; /* GPIO_MODEy0 */ \
    (llg).mode_bb[1] = 0; /* GPIO_MODEy1 */ \
    (llg).mode_bb[2] = 0; /* GPIO_CNFy0 */ \
    (llg).mode_bb[3] = 1; /* GPIO_CNFy1 */ \
    *((llg).pupd_bsrr) = (llg).pupd_value /* set pull up-down state */

/* Output Push-pull mode, Max. output speed 10 MHz
 * MODE 01
 * CNF 00
 */
#define PIOS_SOFT_SERIAL_LL_GPIO_OUTPUT(llg) \
    (llg).mode_bb[0] = 0; /* GPIO_MODEy0 */ \
    (llg).mode_bb[1] = 1; /* GPIO_MODEy1 */ \
    (llg).mode_bb[2] = 0; /* GPIO_CNFy0 */ \
    (llg).mode_bb[3] = 0; /* GPIO_CNFy1 */
#endif

void PIOS_Soft_Serial_LL_GPIO_Init(struct pios_soft_serial_ll_gpio *llg,
                                   GPIO_TypeDef *gpio,
                                   uint16_t pin,
                                   uint32_t mode);


#endif /* PIOS_SOFT_SERIAL_LL_H */
