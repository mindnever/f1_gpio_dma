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

#include <stdbool.h>
#include <string.h>

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

static void PIOS_Soft_Serial_DMA_Setup(uint32_t dma_handle, uint32_t context);
static void PIOS_Soft_Serial_DMA_Complete(uint32_t dma_handle, uint32_t context);
static void PIOS_Soft_Serial_DMA_Error(uint32_t dma_handle, uint32_t context);



typedef enum {
    PIOS_SOFT_SERIAL_MAGIC = 0x50F75E81
} pios_soft_serial_magic_t;

#define DMA_BUFFER_SIZE (1 + 9 + 2)
#define DMA_NUM_BUFFERS 2

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
    
    bool tx_queued;

    uint8_t dma_buffer_free;

    uint32_t dma_buffer[DMA_NUM_BUFFERS][DMA_BUFFER_SIZE];
    
    uint32_t rx_dma;
    uint32_t tx_dma;
    
    uint16_t tim_dma_source;
};

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

    dev->word_len = PIOS_COM_Word_length_8b;
    dev->parity = PIOS_COM_Parity_No;
    dev->stop_bits = PIOS_COM_StopBits_1;
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

    PIOS_DMA_Init(&dev->tx_dma, &dma_config);
    
    dma_config.init.DMA_DIR = DMA_DIR_PeripheralSRC;
    
    PIOS_DMA_Init(&dev->rx_dma, &dma_config);

    switch(dev->cfg->tim_channel) {
        case TIM_Channel_1:
            dev->tim_dma_source = TIM_DMA_CC1;
            break;
        case TIM_Channel_2:
            dev->tim_dma_source = TIM_DMA_CC2;
            break;
        case TIM_Channel_3:
            dev->tim_dma_source = TIM_DMA_CC3;
            break;
        case TIM_Channel_4:
            dev->tim_dma_source = TIM_DMA_CC4;
            break;
    }

    *id = (uint32_t) dev;
    
    return 0;
}

static void PIOS_Soft_Serial_Set_Baud(uint32_t id, uint32_t baud)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, id);
    
    if(baud == 0) {        /* no change */
        return;
    }
    
    uint32_t timer_clock;

    RCC_ClocksTypeDef clocks;

    RCC_GetClocksFreq(&clocks);

#if defined(STM32F1) || defined(STM32F3)
    // APB1 prescaler is 2, so TIM2,3,4,5,6,7,12,13,14 are running at APB1 x2 rate = HCLK
    // APB2 prescaler is 1, so TIM1 & TIM8 are running at APB2 x1 rate = HCLK
    timer_clock = clocks.HCLK_Frequency;
#elif defined(STM32F4)
    if (timer == TIM1 || timer == TIM8 || timer == TIM9 || timer == TIM10 || timer == TIM11) {
        timer_clock = clocks.PCLK2_Frequency;
    } else {
        timer_clock = clocks.PCLK1_Frequency;
    }
#endif
    
    TIM_SetAutoreload(dev->cfg->timer, (timer_clock / baud) - 1);
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
    
    dev->tx_queued = true;
    
    PIOS_Soft_Serial_Tx_Start_Internal(dev);
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

// set rx gpio, init, PIOS_DMA_SetPeripheralBaseAddr()
// set tx gpio, init, PIOS_DMA_SetPeripheralBaseAddr()

    return -1;
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
    }
    
    uint16_t enc_size = PIOS_Soft_Serial_Encode(dev, b, buffer);
    
    PIOS_DMA_SetMemoryBaseAddr(dev->tx_dma, buffer, enc_size);
    PIOS_DMA_Queue(dev->tx_dma, (uint32_t) dev);
}

static void PIOS_Soft_Serial_DMA_Setup(uint32_t dma_handle, uint32_t context)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, context);
    
    // 1. disable RX edge detection (rx & tx)
    // 2. GPIO change direction based on current mode (!!)

    TIM_DMACmd(dev->cfg->timer, dev->tim_dma_source, ENABLE);
}

static void PIOS_Soft_Serial_DMA_Complete(uint32_t dma_handle, uint32_t context)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, context);

    TIM_DMACmd(dev->cfg->timer, dev->tim_dma_source, DISABLE); // Stop generating requests
}

static void PIOS_Soft_Serial_DMA_Error(uint32_t dma_handle, uint32_t context)
{
    PIOS_SOFT_SERIAL_VALIDATE_AND_ASSERT(dev, context);
    
    TIM_DMACmd(dev->cfg->timer, dev->tim_dma_source, DISABLE); // Stop generating requests
}

