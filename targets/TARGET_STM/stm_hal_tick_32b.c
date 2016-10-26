/* mbed Microcontroller Library
 * Copyright (c) 2014, STMicroelectronics
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice,
 *    this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 * 3. Neither the name of STMicroelectronics nor the names of its contributors
 *    may be used to endorse or promote products derived from this software
 *    without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */
#include "hal_tick.h"

// A 32-bit timer is used
#if !TIM_MST_16BIT

#define DEBUG_TICK 0 // Set to 1 to toggle a pin (see below which pin) at each tick

TIM_HandleTypeDef TimMasterHandle;
uint32_t PreviousVal = 0;

void us_ticker_irq_handler(void);

void timer_irq_handler(void) {
    // Channel 1 for mbed timeout
    if (__HAL_TIM_GET_FLAG(&TimMasterHandle, TIM_FLAG_CC1) == SET) {
        if (__HAL_TIM_GET_IT_SOURCE(&TimMasterHandle, TIM_IT_CC1) == SET) {
            __HAL_TIM_CLEAR_IT(&TimMasterHandle, TIM_IT_CC1);
            us_ticker_irq_handler();
        }
    }

    // Channel 2 for HAL tick
    if (__HAL_TIM_GET_FLAG(&TimMasterHandle, TIM_FLAG_CC2) == SET) {
        if (__HAL_TIM_GET_IT_SOURCE(&TimMasterHandle, TIM_IT_CC2) == SET) {
            __HAL_TIM_CLEAR_IT(&TimMasterHandle, TIM_IT_CC2);
            uint32_t val = __HAL_TIM_GetCounter(&TimMasterHandle);
            if ((val - PreviousVal) >= HAL_TICK_DELAY) {
                // Increment HAL variable
                HAL_IncTick();
                // Prepare next interrupt
                __HAL_TIM_SetCompare(&TimMasterHandle, TIM_CHANNEL_2, val + HAL_TICK_DELAY);
                PreviousVal = val;
#if DEBUG_TICK > 0
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_6);
#endif
            }
        }
    }
}

// Reconfigure the HAL tick using a standard timer instead of systick.
HAL_StatusTypeDef HAL_InitTick(uint32_t TickPriority) {
    RCC_ClkInitTypeDef RCC_ClkInitStruct;
    uint32_t PclkFreq;

    // Get clock configuration
    // Note: PclkFreq contains here the Latency (not used after)
    HAL_RCC_GetClockConfig(&RCC_ClkInitStruct, &PclkFreq);

    // Get timer clock value
    PclkFreq = TIM_MST_GET_PCLK_FREQ();

    // Enable timer clock
    TIM_MST_RCC;

    // Reset timer
    TIM_MST_RESET_ON;
    TIM_MST_RESET_OFF;

    // Configure time base
    TimMasterHandle.Instance = TIM_MST;
    TimMasterHandle.Init.Period            = 0xFFFFFFFF;

    // TIMxCLK = PCLKx when the APB prescaler = 1 else TIMxCLK = 2 * PCLKx
    if (RCC_ClkInitStruct.APB1CLKDivider == RCC_HCLK_DIV1) {
        TimMasterHandle.Init.Prescaler   = (uint16_t)((PclkFreq) / 1000000) - 1; // 1 us tick
    }
    else {
        TimMasterHandle.Init.Prescaler   = (uint16_t)((PclkFreq * 2) / 1000000) - 1; // 1 us tick
    }

    TimMasterHandle.Init.ClockDivision     = 0;
    TimMasterHandle.Init.CounterMode       = TIM_COUNTERMODE_UP;
    TimMasterHandle.Init.RepetitionCounter = 0;
    HAL_TIM_OC_Init(&TimMasterHandle);

    NVIC_SetVector(TIM_MST_IRQ, (uint32_t)timer_irq_handler);
    NVIC_EnableIRQ(TIM_MST_IRQ);

    // Channel 1 for mbed timeout
    HAL_TIM_OC_Start(&TimMasterHandle, TIM_CHANNEL_1);

    // Channel 2 for HAL tick
    HAL_TIM_OC_Start(&TimMasterHandle, TIM_CHANNEL_2);
    PreviousVal = __HAL_TIM_GetCounter(&TimMasterHandle);
    __HAL_TIM_SetCompare(&TimMasterHandle, TIM_CHANNEL_2, PreviousVal + HAL_TICK_DELAY);
    __HAL_TIM_ENABLE_IT(&TimMasterHandle, TIM_IT_CC2);

#if DEBUG_TICK > 0
    __GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.Pin = GPIO_PIN_6;
    GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
    GPIO_InitStruct.Pull = GPIO_PULLUP;
    GPIO_InitStruct.Speed = GPIO_SPEED_FAST;
    HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
#endif

    return HAL_OK;
}

void HAL_SuspendTick(void)
{
    TimMasterHandle.Instance = TIM_MST;

    // Disable HAL tick and us_ticker update interrupts (used for 32 bit counter)
    __HAL_TIM_DISABLE_IT(&TimMasterHandle, TIM_IT_CC2);
}

void HAL_ResumeTick(void)
{
    TimMasterHandle.Instance = TIM_MST;

    // Enable HAL tick and us_ticker update interrupts (used for 32 bit counter)
    __HAL_TIM_ENABLE_IT(&TimMasterHandle, TIM_IT_CC2);
}

#endif // !TIM_MST_16BIT