/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_DMA DMA Functions
 * @brief PiOS DMA functionality
 * @{
 *
 * @file       pios_dma.c
 * @author     The LibrePilot Project, http://www.librepilot.org Copyright (C) 2017.
 * @brief      DMA functions implementation
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

#include "pios_dma.h"
#include <stdbool.h>

/* F1 (F3?) implementation */

typedef enum {
    PIOS_DMA_REQUEST_MAGIC = 0xD03AF1F2
} pios_dma_request_magic_t;

struct pios_dma_request {
    pios_dma_request_magic_t magic;
    pios_dma_stream_t regs;

    struct pios_dma_callbacks callbacks;
    uint32_t callback_context;

    struct pios_dma_queue *queue;
    struct pios_dma_request *next;
};

struct pios_dma_queue {
    struct pios_dma_request *head;
    struct pios_dma_request **tail_next;
    
    pios_dma_stream_t *stream;

    DMA_TypeDef *dma;
    uint8_t dma_isr_shift;
};

#define CHANNEL_NR_DMA2_MASK 0x80

#if !defined(PIOS_INCLUDE_FREERTOS)
#define PIOS_DMA_REQUEST_MAX 5
static uint8_t dma_request_count;
static struct pios_dma_request dma_request_buffer[PIOS_DMA_REQUEST_MAX];
#endif /* PIOS_INCLUDE_FREERTOS */


static struct pios_dma_queue dma_queue[7+5]; /* 140 bytes on F1, maybe allocate when needed? */

static void PIOS_DMA_Begin(struct pios_dma_request *dma_req)
{
    DMA_Channel_TypeDef *hw = dma_req->queue->stream;
    
    hw->CCR &= ~(DMA_CCR1_EN);
    while(hw->CCR & DMA_CCR1_EN) { }

    hw->CMAR = dma_req->regs.CMAR;
    hw->CPAR = dma_req->regs.CPAR;
    hw->CNDTR = dma_req->regs.CNDTR;

    if(dma_req->callbacks.setup) {
        dma_req->callbacks.setup((uint32_t)dma_req, dma_req->callback_context);
    }

    hw->CCR = dma_req->regs.CCR | DMA_CCR1_EN;
}

static void PIOS_DMA_Generic_IRQHandler(struct pios_dma_queue *queue)
{
    // dequeue whatever was there
    struct pios_dma_request *dma_req = queue->head;
    
    uint32_t dma_isr = queue->dma->ISR >> queue->dma_isr_shift;

    // clear all flags
    queue->dma->IFCR = DMA_ISR_GIF1 << queue->dma_isr_shift;

    if(dma_req) {
    
        /* dequeue on complete & error */
        bool begin_next = false;
        
        if(dma_isr & (DMA_ISR_TCIF1|DMA_ISR_TEIF1))
        {
            queue->head = dma_req->next;
            if(!queue->head) {
                queue->tail_next = &queue->head;
            } else {
                begin_next = true;
            }
        }
        
        if((dma_isr & DMA_ISR_TCIF1) && dma_req->callbacks.complete) {
            dma_req->callbacks.complete((uint32_t)dma_req, dma_req->callback_context);
        }
        if((dma_isr & DMA_ISR_TEIF1) && dma_req->callbacks.error) {
            dma_req->callbacks.error((uint32_t)dma_req, dma_req->callback_context);
        }
        if((dma_isr & DMA_ISR_HTIF1) && dma_req->callbacks.halftransfer) {
            dma_req->callbacks.halftransfer((uint32_t)dma_req, dma_req->callback_context);
        }
        
        if(begin_next) {
            PIOS_DMA_Begin(queue->head);
        }
    }
}

int32_t PIOS_DMA_Init(uint32_t *dma, const struct pios_dma_config *config)
{
    PIOS_DEBUG_Assert(dma);
    PIOS_DEBUG_Assert(config);

#if !defined(PIOS_INCLUDE_FREERTOS)
    PIOS_DEBUG_Assert(dma_request_count < PIOS_DMA_REQUEST_MAX);
    struct pios_dma_request *dma_req = &dma_request_buffer[dma_request_count++];
#else
    struct pios_dma_request *dma_req = (struct pios_dma_request *)pios_malloc(sizeof(*dma_req));
#endif

    DMA_Init(&dma_req->regs, &config->init);
    
    dma_req->callbacks = config->callbacks;
    
    if(dma_req->callbacks.complete) {
        DMA_ITConfig(&dma_req->regs, DMA_IT_TC, ENABLE);
    }
    if(dma_req->callbacks.halftransfer) {
        DMA_ITConfig(&dma_req->regs, DMA_IT_HT, ENABLE);
    }
    if(dma_req->callbacks.error) {
        DMA_ITConfig(&dma_req->regs, DMA_IT_TE, ENABLE);
    }
    
    // need to figure out:
    // - dma irq flags
    // - dma instance (1 or 2)
    // - dma queue number
    
    uint8_t queue_nr = 0;
    uint8_t irq_channel = 0;
    
    switch((uint32_t)config->stream) {
        case (uint32_t)DMA1_Channel1:
            queue_nr = 0;
            irq_channel = DMA1_Channel1_IRQn;
            break;
        case (uint32_t)DMA1_Channel2:
            queue_nr = 1;
            irq_channel = DMA1_Channel2_IRQn;
            break;
        case (uint32_t)DMA1_Channel3:
            queue_nr = 2;
            irq_channel = DMA1_Channel3_IRQn;
            break;
        case (uint32_t)DMA1_Channel4:
            queue_nr = 3;
            irq_channel = DMA1_Channel4_IRQn;
            break;
        case (uint32_t)DMA1_Channel5:
            queue_nr = 4;
            irq_channel = DMA1_Channel5_IRQn;
            break;
        case (uint32_t)DMA1_Channel6:
            queue_nr = 5;
            irq_channel = DMA1_Channel6_IRQn;
            break;
        case (uint32_t)DMA1_Channel7:
            queue_nr = 6;
            irq_channel = DMA1_Channel7_IRQn;
            break;
#if defined(STM32F3)
        case (uint32_t)DMA2_Channel1:
            queue_nr = 7;
            irq_channel = DMA2_Channel1_IRQn;
            break;
        case (uint32_t)DMA2_Channel2:
            queue_nr = 8;
            irq_channel = DMA2_Channel2_IRQn;
            break;
        case (uint32_t)DMA2_Channel3:
            queue_nr = 9;
            irq_channel = DMA2_Channel3_IRQn;
            break;
        case (uint32_t)DMA2_Channel4:
            queue_nr = 10;
            irq_channel = DMA2_Channel4_IRQn;
            break;
        case (uint32_t)DMA2_Channel5:
            queue_nr = 11;
            irq_channel = DMA2_Channel5_IRQn;
            break;
#endif /* STM32F3 */
    }
    
    struct pios_dma_queue *queue = &dma_queue[queue_nr];
    
    // irq disable?
    
    if(!queue->stream) {
        queue->stream = config->stream;
        queue->head = 0;
        queue->tail_next = &queue->head;
        
        if(queue_nr < 7) {
            queue->dma_isr_shift = queue_nr * 4;
            queue->dma = DMA1;
        } else {
            queue->dma_isr_shift = (queue_nr - 7) * 4;
            queue->dma = DMA2;
        }

        // also enable nvic
        
        NVIC_InitTypeDef irqInit = config->irq;
        
        irqInit.NVIC_IRQChannel = irq_channel;
        irqInit.NVIC_IRQChannelCmd = ENABLE;
        
        NVIC_Init(&irqInit);
    }
    
    dma_req->queue = queue;

    // irq enable

    dma_req->magic = PIOS_DMA_REQUEST_MAGIC;
    *dma = (uint32_t) dma_req;
    
    return 0;
}

void PIOS_DMA_SetMemoryBaseAddr(uint32_t dma, void *memptr, uint16_t size)
{
    struct pios_dma_request *dma_req = (struct pios_dma_request *)dma; // validate?
    
    dma_req->regs.CMAR = (uint32_t)memptr;
    dma_req->regs.CNDTR = size;
}

void PIOS_DMA_SetPeripheralBaseAddr(uint32_t dma, void *periph)
{
    struct pios_dma_request *dma_req = (struct pios_dma_request *)dma; // validate?
    
    dma_req->regs.CPAR = (uint32_t)periph;
}


void PIOS_DMA_Queue(uint32_t dma, uint32_t callback_context)
{
    struct pios_dma_request *dma_req = (struct pios_dma_request *)dma; // validate?

    // irq disable

    dma_req->callback_context = callback_context;
    
    *(dma_req->queue->tail_next) = dma_req;
    dma_req->next = 0;
    dma_req->queue->tail_next = &dma_req->next;

    // irq enable

    if(dma_req->queue->head == dma_req) {
        PIOS_DMA_Begin(dma_req);
    }
}

/* IRQ handlers */

void DMA1_Channel1_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[0]);
}

void DMA1_Channel2_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[1]);
}

void DMA1_Channel3_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[2]);
}

void DMA1_Channel4_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[3]);
}

void DMA1_Channel5_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[4]);
}

void DMA1_Channel6_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[5]);
}

void DMA1_Channel7_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[6]);
}

void DMA2_Channel1_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[7]);
}

void DMA2_Channel2_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[8]);
}

void DMA2_Channel3_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[9]);
}

void DMA2_Channel4_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[10]);
}

void DMA2_Channel5_IRQHandler(void)
{
    PIOS_DMA_Generic_IRQHandler(&dma_queue[11]);
}
