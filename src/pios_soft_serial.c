/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SOFT_SERIAL Functions
 * @brief PiOS Soft Serial functionality
 * @{
 *
 * @file       pios_soft_serial.c
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

#include "pios_soft_serial.h"
#include "pios_soft_serial_ll.h"
#include "pios_irq.h"
#include "pios_tim.h"
#include "pios_usart.h"

#include <stdbool.h>
#include <string.h>

/* PIOS_COM driver methods */
static void     PIOS_Soft_Serial_Set_Baud(uint32_t id, uint32_t baud);
static void     PIOS_Soft_Serial_Set_Config(uint32_t id, enum PIOS_COM_Word_Length word_len, enum PIOS_COM_Parity parity, enum PIOS_COM_StopBits stop_bits, uint32_t baud_rate);
static void     PIOS_Soft_Serial_Tx_Start(uint32_t id, uint16_t tx_bytes_avail);
static void     PIOS_Soft_Serial_Rx_Start(uint32_t id, uint16_t rx_bytes_avail);
static void     PIOS_Soft_Serial_Bind_Rx_Cb(uint32_t id, pios_com_callback rx_in_cb, uint32_t context);
static void     PIOS_Soft_Serial_Bind_Tx_Cb(uint32_t id, pios_com_callback tx_out_cb, uint32_t context);
static int32_t  PIOS_Soft_Serial_Ioctl(uint32_t id, uint32_t ctl, void *param);

struct pios_com_driver pios_soft_serial_driver = {
    .set_baud = PIOS_Soft_Serial_Set_Baud,
    .set_config = PIOS_Soft_Serial_Set_Config,
    .tx_start = PIOS_Soft_Serial_Tx_Start,
    .rx_start = PIOS_Soft_Serial_Rx_Start,
    .bind_rx_cb = PIOS_Soft_Serial_Bind_Rx_Cb,
    .bind_tx_cb = PIOS_Soft_Serial_Bind_Tx_Cb,
    .ioctl = PIOS_Soft_Serial_Ioctl,
};

/* pios_dma callbacks */
static void PIOS_Soft_Serial_DMA_Setup(uint32_t dma_handle, uint32_t context);
static void PIOS_Soft_Serial_DMA_Complete(uint32_t dma_handle, uint32_t context);
static void PIOS_Soft_Serial_DMA_Error(uint32_t dma_handle, uint32_t context);

/* edge detect callback */
static void PIOS_Soft_Serial_Edge_Detected(uint32_t dev, uint32_t context);

typedef enum {
    PIOS_SOFT_SERIAL_MAGIC = 0x50F75E81
} pios_soft_serial_magic_t;

#define DMA_BUFFER_SIZE (1 + 9 + 2)
#define DMA_NUM_BUFFERS 2

typedef enum {
    STATE_IDLE,
    STATE_RX_WAIT,
    STATE_RX_DATA,
    STATE_TX_DATA,
} pios_soft_serial_state_t;

struct pios_soft_serial_gpio {
    struct pios_soft_serial_ll_gpio ll;
    uint32_t dma;
};

struct pios_soft_serial_device {
    pios_soft_serial_magic_t magic;
    
    pios_com_callback rx_in_cb;
    uint32_t rx_in_context;
    
    pios_com_callback tx_out_cb;
    uint32_t tx_out_context;
    
    const struct pios_soft_serial_config *cfg;
    
    enum PIOS_COM_Word_Length word_len;
    enum PIOS_COM_Parity parity;
    enum PIOS_COM_StopBits stop_bits;
    enum PIOS_USART_Inverted inverted;
    
    bool tx_pending;

    uint8_t dma_buffer_free;

    uint32_t dma_buffer[DMA_NUM_BUFFERS][DMA_BUFFER_SIZE];
    
    uint16_t tim_dma_source;
    
    pios_soft_serial_state_t state;
    
    uint32_t edge_detect;
    
    struct pios_soft_serial_gpio rx;
    struct pios_soft_serial_gpio tx;
};

/* private functions */
static uint32_t *PIOS_Soft_Serial_GetDMABuffer(struct pios_soft_serial_device *dev);
static void PIOS_Soft_Serial_FreeDMABuffer(struct pios_soft_serial_device *dev, uint32_t *buffer);
static uint16_t PIOS_Soft_Serial_Encode(struct pios_soft_serial_device *dev, uint8_t data, uint32_t *buffer);
static void PIOS_Soft_Serial_Tx_Start_Internal(struct pios_soft_serial_device *dev);


#if !defined(PIOS_INCLUDE_FREERTOS)
# ifndef PIOS_SOFT_SERIAL_MAX_DEV
#  define PIOS_SOFT_SERIAL_MAX_DEV 5
# endif
static uint8_t soft_serial_count;
static struct pios_soft_serial_device soft_serial_device[PIOS_SOFT_SERIAL_MAX_DEV];
#endif /* PIOS_INCLUDE_FREERTOS */


bool PIOS_Soft_Serial_Validate(struct pios_soft_serial_device *dev)
{
    return dev && (dev->magic == PIOS_SOFT_SERIAL_MAGIC);
}

#define PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(__d, __id) \
struct pios_soft_serial_device *__d = (struct pios_soft_serial_device *)__id; \
bool valid = PIOS_Soft_Serial_Validate(__d); \
PIOS_DEBUG_Assert(valid)



int32_t PIOS_Soft_Serial_Init(uint32_t *id, const struct pios_soft_serial_config *config)
{
    PIOS_DEBUG_Assert(config);
    PIOS_DEBUG_Assert(id);

#ifdef PIOS_INCLUDE_FREERTOS
    struct pios_soft_serial_device *dev = (struct pios_soft_serial_device *)pios_malloc(sizeof(*dev));
#else
    PIOS_DEBUG_Assert(soft_serial_count < PIOS_SOFT_SERIAL_MAX_DEV);
    struct pios_soft_serial_device *dev = &soft_serial_device[soft_serial_count++];
#endif

    memset(dev, 0, sizeof(*dev));

    dev->magic = PIOS_SOFT_SERIAL_MAGIC;
    dev->cfg = config;

    dev->state = STATE_IDLE;

    dev->word_len = PIOS_COM_Word_length_8b;
    dev->parity = PIOS_COM_Parity_No;
    dev->stop_bits = PIOS_COM_StopBits_1;
    dev->inverted = PIOS_USART_Inverted_None;
    dev->dma_buffer_free = 0xff;
    
    /* initialize timer base */
    TIM_TimeBaseInitTypeDef timeBaseInit = {
        .TIM_Prescaler = 0,
        .TIM_CounterMode = TIM_CounterMode_Up,
        .TIM_Period = 0, /* PIOS_Soft_Serial_Set_Baud() will calculate ARR value */
        .TIM_ClockDivision = TIM_CKD_DIV1,
    };
    
    TIM_TimeBaseInit(dev->cfg->timer, &timeBaseInit);

    TIM_Cmd(dev->cfg->timer, ENABLE);

    PIOS_Soft_Serial_Set_Baud((uint32_t) dev, 9600);

    struct pios_dma_config dma_config = {
        .init = {
            .DMA_M2M = DMA_M2M_Disable,
            .DMA_Priority = DMA_Priority_Medium,
            .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word,
            .DMA_MemoryDataSize = DMA_MemoryDataSize_Word,
            .DMA_MemoryInc = DMA_MemoryInc_Enable,
            .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
            .DMA_DIR = DMA_DIR_PeripheralDST,
//            .DMA_BufferSize = ndt,
//            .DMA_MemoryBaseAddr = (uint32_t)buffer,
//            .DMA_PeripheralBaseAddr = (uint32_t)&io.gpio->BSRR,
        },
        .stream = dev->cfg->dma_stream,
//        .irq = { // NVIC irq priority only
//            .
//        }
        .callbacks = {
            .setup = PIOS_Soft_Serial_DMA_Setup,
            .complete = PIOS_Soft_Serial_DMA_Complete,
            .error = PIOS_Soft_Serial_DMA_Error,
        }
    };

    PIOS_DMA_Init(&dev->tx.dma, &dma_config);
    
    dma_config.init.DMA_DIR = DMA_DIR_PeripheralSRC;
    
    PIOS_DMA_Init(&dev->rx.dma, &dma_config);
    
    dev->tim_dma_source = PIOS_TIM_CHANNEL_DIER_CCxDE(dev->cfg->tim_channel);
    
    PIOS_Soft_Serial_LL_EdgeDetect_Init(&dev->edge_detect, PIOS_Soft_Serial_Edge_Detected, (uint32_t) dev);

    *id = (uint32_t) dev;
    
    return 0;
}

static void PIOS_Soft_Serial_Set_Baud(uint32_t id, uint32_t baud)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, id);
    
    if(baud == 0) {        /* no change */
        return;
    }

    TIM_SetAutoreload(dev->cfg->timer, (PIOS_TIM_Ck_Int(dev->cfg->timer) / baud) - 1);
}

static void PIOS_Soft_Serial_Set_Config(uint32_t id, enum PIOS_COM_Word_Length word_len, enum PIOS_COM_Parity parity, enum PIOS_COM_StopBits stop_bits, uint32_t baud_rate)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, id);
    
    if(word_len != PIOS_COM_Word_length_Unchanged) {
        dev->word_len = word_len;
    }
    
    if(parity != PIOS_COM_Parity_Unchanged) {
        dev->parity = parity;
    }
    
    if(stop_bits != PIOS_COM_StopBits_Unchanged) {
        dev->stop_bits = stop_bits;
    }
    
    PIOS_Soft_Serial_Set_Baud(id, baud_rate);
}

static void PIOS_Soft_Serial_Tx_Start(uint32_t id, uint16_t tx_bytes_avail)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, id);

    PIOS_IRQ_Disable();
    
    bool pending = dev->tx_pending;
    
    dev->tx_pending = true;
    
    PIOS_IRQ_Enable();
    
    if(!pending) {
        PIOS_Soft_Serial_Tx_Start_Internal(dev);
    }
}

static void PIOS_Soft_Serial_Rx_Start(uint32_t id, uint16_t rx_bytes_avail)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, id);
}

static void PIOS_Soft_Serial_Bind_Rx_Cb(uint32_t id, pios_com_callback rx_in_cb, uint32_t context)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, id);

    /*
     * Order is important in these assignments since ISR uses _cb
     * field to determine if it's ok to dereference _cb and _context
     */
    dev->rx_in_context = context;
    dev->rx_in_cb = rx_in_cb;
}


static void PIOS_Soft_Serial_Bind_Tx_Cb(uint32_t id, pios_com_callback tx_out_cb, uint32_t context)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, id);

    /*
     * Order is important in these assignments since ISR uses _cb
     * field to determine if it's ok to dereference _cb and _context
     */
    dev->tx_out_context = context;
    dev->tx_out_cb = tx_out_cb;
}

static int32_t  PIOS_Soft_Serial_Ioctl(uint32_t id, uint32_t ctl, void *param)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, id);
    int32_t ret = -1;
    
    bool reconf_edge_detect = false;
    
    switch(ctl) {
        case PIOS_IOCTL_SOFT_SERIAL_SET_RXGPIO:
            {
                const struct stm32_gpio *pin = (const struct stm32_gpio *) param;
                
                PIOS_Soft_Serial_LL_GPIO_Init(&dev->rx.ll, pin);
                
                PIOS_DMA_SetPeripheralBaseAddr(dev->rx.dma, &pin->gpio->IDR);
                
                reconf_edge_detect = true;
                
                ret = 0;
            }
            break;
            
        case PIOS_IOCTL_SOFT_SERIAL_SET_TXGPIO:
            {
                const struct stm32_gpio *pin = (const struct stm32_gpio *) param;
                
                PIOS_Soft_Serial_LL_GPIO_Init(&dev->tx.ll, pin);
                
                PIOS_DMA_SetPeripheralBaseAddr(dev->tx.dma, &pin->gpio->BSRR);
                
                ret = 0;
            }
            break;
        
        case PIOS_IOCTL_USART_SET_INVERTED:
            {
                dev->inverted = *(enum PIOS_USART_Inverted *)param;

                reconf_edge_detect = true;
                
                ret = 0;
            }
            break;
    }

    if(reconf_edge_detect) {
        PIOS_Soft_Serial_LL_EdgeDetect_Configure(dev->edge_detect,
                                                 &dev->rx.ll.pin,
                                                 (dev->inverted & PIOS_USART_Inverted_Rx) ?
                                                 PIOS_SOFT_SERIAL_LL_EDGEDETECT_RISING : PIOS_SOFT_SERIAL_LL_EDGEDETECT_FALLING);
    }

    return ret;
}


static uint32_t *PIOS_Soft_Serial_GetDMABuffer(struct pios_soft_serial_device *dev)
{
    if(dev->dma_buffer_free == 0) {
        return 0;
    }

    uint32_t buffer_nr = __builtin_ctz(dev->dma_buffer_free);
    
    if(buffer_nr >= DMA_NUM_BUFFERS) {
        return 0;
    }
    
    /* mark this buffer as used */
    dev->dma_buffer_free &= ~(1 << buffer_nr);
    
    return dev->dma_buffer[buffer_nr];
}

static void PIOS_Soft_Serial_FreeDMABuffer(struct pios_soft_serial_device *dev, uint32_t *buffer)
{
    uint32_t buffer_nr = (buffer - dev->dma_buffer[0]) / DMA_BUFFER_SIZE;
    
    PIOS_DEBUG_Assert(buffer_nr < DMA_NUM_BUFFERS);

    dev->dma_buffer_free |= (1 << buffer_nr);
}

static uint16_t PIOS_Soft_Serial_Encode(struct pios_soft_serial_device *dev, uint8_t data, uint32_t *buffer)
{
    
    return 10;
}

static void PIOS_Soft_Serial_Tx_Start_Internal(struct pios_soft_serial_device *dev)
{
    /* 1. get dma buffer */
    /* 2. tx_callback */
    /* 3. encode */
    /* 4. queue_dma */
    
    if(!dev->tx_out_cb) {
        return;
    }
    
    uint32_t *buffer = PIOS_Soft_Serial_GetDMABuffer(dev);
    if(!buffer) {
        return;
    }

    bool task_woken = false;
    uint8_t b;

    if(dev->tx_out_cb(dev->tx_out_context, &b, 1, 0, &task_woken) != 1) {
        PIOS_Soft_Serial_FreeDMABuffer(dev, buffer);
        dev->tx_pending = false;
    }
    
    uint16_t enc_size = PIOS_Soft_Serial_Encode(dev, b, buffer);
    
    PIOS_DMA_SetMemoryBaseAddr(dev->tx.dma, buffer, enc_size);
    PIOS_DMA_Queue(dev->tx.dma, (uint32_t) dev);
}

static void PIOS_Soft_Serial_DMA_Setup(uint32_t dma_handle, uint32_t context)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, context);
    
    /* Disable start bit detection */
    PIOS_Soft_Serial_LL_EdgeDetect_Cmd(dev->edge_detect, DISABLE);
    
    /* Reconfigure GPIO */
    struct pios_soft_serial_gpio *gs = 0;
    
    if(dma_handle == dev->rx.dma) {
        gs = &dev->rx;
    } else if(dma_handle == dev->tx.dma) {
        gs = &dev->tx;
    } else {
        PIOS_DEBUG_Assert(0); //
    }
    
    PIOS_Soft_Serial_LL_GPIO_Init(&gs->ll, 0);

    /* Start generating DMA requests */
    /* Should we adjust appropriate CCR now? */

    TIM_DMACmd(dev->cfg->timer, dev->tim_dma_source, ENABLE);
}

static void PIOS_Soft_Serial_DMA_Complete(uint32_t dma_handle, uint32_t context)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, context);

    TIM_DMACmd(dev->cfg->timer, dev->tim_dma_source, DISABLE); // Stop generating requests
    
    // Should go to next state
    //
}

static void PIOS_Soft_Serial_DMA_Error(uint32_t dma_handle, uint32_t context)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, context);
    
    TIM_DMACmd(dev->cfg->timer, dev->tim_dma_source, DISABLE); // Stop generating requests
}

static void PIOS_Soft_Serial_Edge_Detected(__attribute__((unused)) uint32_t edge_detect_dev, uint32_t context)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, context);

}
