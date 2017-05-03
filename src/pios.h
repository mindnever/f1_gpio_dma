#ifndef _PIOS_H_
#define _PIOS_H_

#include "stm32f10x_conf.h"
#include <stdint.h>
#include <stdbool.h>
#include <pios_delay.h>

#include "pios_board.h"

#define PIOS_DEBUG_Assert(x) assert_param(x)

#define PIOS_IRQ_PRIO_LOW        12              // lower than RTOS
#define PIOS_IRQ_PRIO_MID        8               // higher than RTOS
#define PIOS_IRQ_PRIO_HIGH       5               // for SPI, ADC, I2C etc...
#define PIOS_IRQ_PRIO_HIGHEST    4               // for USART etc...

struct stm32_gpio {
    GPIO_TypeDef     *gpio;
    GPIO_InitTypeDef init;
    uint8_t pin_source;
};

struct pios_board_pin_af {
    uint8_t timer;
    uint8_t usart;
    uint8_t i2c;
    uint8_t spi;
};

struct pios_board_pin_remap {
    uint32_t timer;
    uint32_t usart;
    uint32_t i2c;
    uint32_t spi;
};

struct pios_board_pin {
    /* GPIO */
    GPIO_TypeDef *gpio;
    uint8_t pin_nr;
    /* Timer */
    TIM_TypeDef *timer;
    uint8_t timer_channel; /* channel number [1..4]*/
    /* Timer DMA */
    DMA_Channel_TypeDef *dma_channel;

#ifdef STM32F1
    struct pios_board_pin_remap remap;
#else
    struct pios_board_pin_af af;
#endif
};

extern struct pios_board_pin board_io_pins[];

#endif /* _PIOS_H_ */
