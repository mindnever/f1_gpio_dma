/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SOFT_SERIAL Functions
 * @brief PiOS Soft Serial functionality
 * @{
 *
 * @file       pios_soft_serial.h
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
#ifndef PIOS_SOFT_SERIAL_H
#define PIOS_SOFT_SERIAL_H

#include "pios.h"
#include "pios_com.h"
#include "pios_dma.h"

extern struct pios_com_driver pios_soft_serial_driver; /* half duplex driver */

struct pios_soft_serial_config {
    pios_dma_stream_t *dma_stream;
    TIM_TypeDef *timer;
    uint8_t tim_channel;
};

int32_t PIOS_Soft_Serial_Init(uint32_t *dev, const struct pios_soft_serial_config *config);

#define PIOS_IOCTL_SOFT_SERIAL_SET_RXGPIO     COM_IOCTL(COM_IOCTL_TYPE_SOFT_SERIAL, 4, struct stm32_gpio)
#define PIOS_IOCTL_SOFT_SERIAL_SET_TXGPIO     COM_IOCTL(COM_IOCTL_TYPE_SOFT_SERIAL, 5, struct stm32_gpio)

#endif /* PIOS_SOFT_SERIAL_H */
