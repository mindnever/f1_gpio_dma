
#include <pios.h>

// Try with DMA1_Channel6, triggered by TIM3_CH1

#include "pios_dma.h"

extern const uint32_t SystemFrequency;

static uint32_t bsrr_buffer1[32];
static uint32_t bsrr_buffer2[32];

struct stm32_gpio led = {
    .gpio = GPIOC,
    .init = {
        .GPIO_Pin = GPIO_Pin_13,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP
    }
};

struct stm32_gpio io = {
    .gpio = GPIOA,
    .init = {
        .GPIO_Pin = GPIO_Pin_10,
        .GPIO_Speed = GPIO_Speed_2MHz,
        .GPIO_Mode = GPIO_Mode_Out_PP,
    }
};

void Setup_RCC()
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA | RCC_APB2Periph_GPIOC | RCC_APB2Periph_TIM1 | RCC_APB2Periph_AFIO, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    RCC_AHBPeriphClockCmd(RCC_AHBPeriph_DMA1, ENABLE);
}

void Setup_GPIO()
{
    GPIO_Init(led.gpio, &led.init);
    GPIO_Init(io.gpio, &io.init);
}

void Setup_TIMER(TIM_TypeDef *timer, uint8_t tim_channel, uint32_t baud)
{
    uint32_t timer_clock = SystemFrequency;
    
    TIM_TimeBaseInitTypeDef timerInitCfg = {
        .TIM_Prescaler         = 0,
        .TIM_ClockDivision     = TIM_CKD_DIV1,
        .TIM_CounterMode       = TIM_CounterMode_Up,
        .TIM_Period            = ((timer_clock / baud) - 1),
        .TIM_RepetitionCounter = 0x0000,
    };
    
    TIM_Cmd(timer, DISABLE);
    TIM_TimeBaseInit(timer, &timerInitCfg);
    TIM_InternalClockConfig(timer);
    TIM_ARRPreloadConfig(timer, ENABLE);

    TIM_CCxCmd(timer, tim_channel, TIM_CCx_Enable);

    TIM_Cmd(timer, ENABLE);
}

static void dma_transfer_complete(uint32_t dma_handle, uint32_t context)
{
    TIM_DMACmd(TIM3, TIM_DMA_CC1, DISABLE); // Stop generating requests
}

static void dma_transfer_setup(uint32_t dma_handle, uint32_t context)
{
    TIM_DMACmd(TIM3, TIM_DMA_CC1, ENABLE);
    
    PIOS_DMA_Queue(context, dma_handle);
}

void Setup_DMA(uint32_t *dma_handle, DMA_Channel_TypeDef *dma_channel, void *buffer, uint16_t ndt)
{
    struct pios_dma_config dma_config = {
        .init = {
            .DMA_M2M = DMA_M2M_Disable,
            .DMA_Priority = DMA_Priority_Medium,
            .DMA_PeripheralDataSize = DMA_PeripheralDataSize_Word,
            .DMA_MemoryDataSize = DMA_MemoryDataSize_Word,
            .DMA_MemoryInc = DMA_MemoryInc_Enable,
            .DMA_PeripheralInc = DMA_PeripheralInc_Disable,
            .DMA_DIR = DMA_DIR_PeripheralDST,
            .DMA_BufferSize = ndt,
            .DMA_MemoryBaseAddr = (uint32_t)buffer,
            .DMA_PeripheralBaseAddr = (uint32_t)&io.gpio->BSRR,
        },
        .stream = dma_channel,
        .callbacks = {
            .complete = dma_transfer_complete,
            .setup = dma_transfer_setup,
        }
        
    };

    PIOS_DMA_Init(dma_handle, &dma_config);
}

void assert_failed(uint8_t *file, uint32_t line)
{
    __asm("bkpt #1");
    while(1) { }
}

int main()
{
    PIOS_DELAY_Init();

    Setup_RCC();
    Setup_GPIO();
    
    uint32_t dma_handle1, dma_handle2;
    
    Setup_DMA(&dma_handle1, DMA1_Channel6, &bsrr_buffer1[0], sizeof(bsrr_buffer1) / sizeof(bsrr_buffer1[0]));
    Setup_DMA(&dma_handle2, DMA1_Channel6, &bsrr_buffer2[0], sizeof(bsrr_buffer2) / sizeof(bsrr_buffer2[0]));
    
    Setup_TIMER(TIM3, TIM_Channel_1, 57600);
    
    GPIO_WriteBit(io.gpio, io.init.GPIO_Pin, Bit_SET);
    
    PIOS_DMA_Queue(dma_handle1, dma_handle2);
    
    for(int i = 0; i < 32; ++i) {
        if(i & 4) {
            bsrr_buffer2[i] = (uint32_t)io.init.GPIO_Pin; // reset
        } else {
            bsrr_buffer2[i] = ((uint32_t)io.init.GPIO_Pin << 16); // set
        }
        if(i & 1) {
            bsrr_buffer1[i] = (uint32_t)io.init.GPIO_Pin; // reset
        } else {
            bsrr_buffer1[i] = ((uint32_t)io.init.GPIO_Pin << 16); // set
        }
    }
    
    int countdown = 0;
    while(1) {
        GPIO_WriteBit(led.gpio, led.init.GPIO_Pin, Bit_SET);
        PIOS_DELAY_WaitmS(100);
        GPIO_WriteBit(led.gpio, led.init.GPIO_Pin, Bit_RESET);
        PIOS_DELAY_WaitmS(50);
        if(countdown == 0) {
//            PIOS_DMA_Queue(dma_handle1);
//            PIOS_DMA_Queue(dma_handle2);
            countdown = 20;
        } else {
            --countdown;
        }
    }
    
    return 0;
}
