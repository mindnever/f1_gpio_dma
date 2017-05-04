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

#include <string.h>

struct pios_soft_serial_ll_edgedetect_device {
    
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

    return 0;
}

void PIOS_Soft_Serial_LL_EdgeDetect_Configure(uint32_t dev, const struct pios_soft_serial_ll_edgedetect_config *config)
{

}

void PIOS_Soft_Serial_LL_EdgeDetect_Cmd(uint32_t dev, FunctionalState NewState)
{

}

void PIOS_Soft_Serial_LL_GPIO_Init(struct pios_soft_serial_ll_gpio *llg,
                                   GPIO_TypeDef *gpio,
                                   uint16_t pin,
                                   uint32_t mode)
{
    
}
