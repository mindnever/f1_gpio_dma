/* board_hw_defs.c */

#include "pios.h"

#define TIM_Channel_0 0

#define PIOS_BOARD_PIN(id,g,nr,t, ch, dmach) \
    [id] = { .gpio = GPIO##g, .pin_nr = nr, .timer = t, .timer_channel = TIM_Channel_##ch, .dma_channel = dmach},

//struct pios_board_pin board_io_pins[] = {
//
//    [PIOS_BOARD_RECEIVER_PIN3] = {
//        .gpio = GPIOB,
//        .pin_nr = 6,
//        .timer = TIM4,
//        .timer_channel = TIM_Channel_1,
//        .dma_channel = DMA1_Channel1,
//    },
//    
//    [PIOS_BOARD_RECEIVER_PIN4] = {
//        .gpio = GPIOB,
//        .pin_nr = 5,
//        .timer = TIM3,
//        .timer_channel = TIM_Channel_2,
//        .remap = { /* remap */
//            .timer = GPIO_PartialRemap_TIM3,
//        }
//    },
//
//    [PIOS_BOARD_FLEXI_PIN4] = {
//        .gpio = GPIOB,
//        .pin_nr = 11,
//        .timer = TIM2,
//        .timer_channel = TIM_Channel_4,
//        .dma_channel = DMA1_Channel7,
//        .remap = {
//            .timer = GPIO_PartialRemap2_TIM2,
//        }
//    },
//
////    PIOS_BOARD_PIN(PIOS_BOARD_RECEIVER_PIN3, B, 6, TIM4, 1, DMA1_Channel1)
////    PIOS_BOARD_PIN(PIOS_BOARD_RECEIVER_PIN4, B, 5, TIM3, 2, 0)
//    PIOS_BOARD_PIN(PIOS_BOARD_RECEIVER_PIN5, B, 0, TIM3, 3, DMA1_Channel2)
//    PIOS_BOARD_PIN(PIOS_BOARD_RECEIVER_PIN6, B, 1, TIM3, 4, DMA1_Channel3)
//    PIOS_BOARD_PIN(PIOS_BOARD_RECEIVER_PIN7, A, 0, TIM2, 1, DMA1_Channel5)
//    PIOS_BOARD_PIN(PIOS_BOARD_RECEIVER_PIN8, A, 1, TIM2, 2, DMA1_Channel7)
//
//    PIOS_BOARD_PIN(PIOS_BOARD_SERVO_PIN1, B, 9, TIM4, 4, 0)
//    PIOS_BOARD_PIN(PIOS_BOARD_SERVO_PIN2, B, 8, TIM4, 3, DMA1_Channel5)
//    PIOS_BOARD_PIN(PIOS_BOARD_SERVO_PIN3, B, 7, TIM4, 2, DMA1_Channel4)
//    PIOS_BOARD_PIN(PIOS_BOARD_SERVO_PIN4, A, 8, TIM1, 1, DMA1_Channel2)
//    PIOS_BOARD_PIN(PIOS_BOARD_SERVO_PIN5, B, 4, TIM3, 1, DMA1_Channel6)
//    PIOS_BOARD_PIN(PIOS_BOARD_SERVO_PIN6, A, 2, TIM2, 3, DMA1_Channel1)
//    
//    PIOS_BOARD_PIN(PIOS_BOARD_MAIN_PIN3, A, 9, TIM1, 2, DMA1_Channel3)
//    PIOS_BOARD_PIN(PIOS_BOARD_MAIN_PIN4, A, 10, TIM1, 3, DMA1_Channel6)
//
//    PIOS_BOARD_PIN(PIOS_BOARD_FLEXI_PIN3, B, 10, TIM2, 3, DMA1_Channel1)
////    PIOS_BOARD_PIN(PIOS_BOARD_FLEXI_PIN4, B, 11, TIM2, 4, DMA1_Channel7)
//
//    PIOS_BOARD_PIN(PIOS_BOARD_SWD_PIN3, A, 13, 0, 0, 0)
//    PIOS_BOARD_PIN(PIOS_BOARD_SWD_PIN4, A, 14, 0, 0, 0)
//};


//struct pios_usart_config usart_main_config = {
//    .rx = PIOS_BOARD_MAIN_RX,
//    .tx = PIOS_BOARD_MAIN_TX
//};
//
//struct pios_usart_config usart_flexi_config = {
//    .rx = PIOS_BOARD_FLEXI_RX,
//    .tx = PIOS_BOARD_FLEXI_TX,
//};

