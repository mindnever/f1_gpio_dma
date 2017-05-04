/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DMA DMA Functions
 * @brief PiOS DMA functionality
 * @{
 *
 * @file       pios_dma.h
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief      DMA functions header
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

#ifndef PIOS_DMA_H
#define PIOS_DMA_H

#include "pios.h"

typedef void (* pios_dma_callback_t)(uint32_t dma_handle, uint32_t context);

struct pios_dma_callbacks {
    pios_dma_callback_t setup;
    pios_dma_callback_t complete;
    pios_dma_callback_t halftransfer;
    pios_dma_callback_t error;
};


#if defined(STM32F1) || defined(STM32F3)
typedef DMA_Channel_TypeDef pios_dma_stream_t;
#elif defined(STM32F4)
typedef DMA_Stream_TypeDef pios_dma_stream_t;
#endif


struct pios_dma_config {
    DMA_InitTypeDef init;
    pios_dma_stream_t *stream;
    struct pios_dma_callbacks callbacks;
    
    NVIC_InitTypeDef irq;
};

int32_t PIOS_DMA_Init(uint32_t *dma_handle, const struct pios_dma_config *config);

void PIOS_DMA_SetMemoryBaseAddr(uint32_t dma_handle, void *memptr, uint16_t size);
void PIOS_DMA_SetPeripheralBaseAddr(uint32_t dma_handle, __IO void *periph);

void PIOS_DMA_Queue(uint32_t dma_handle, uint32_t callback_context);

#endif /* PIOS_DMA_H */
