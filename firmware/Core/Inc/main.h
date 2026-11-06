/**
 * main.h — спільні оголошення прошивки ЛР1
 * MCU: STM32C031C6 (Nucleo-C031C6, симулятор Wokwi)
 */
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32c0xx_hal.h"

/* ---- Розводка пінів (див. docs/hardware-components.md) ---------------- */
#define LED_Pin            GPIO_PIN_5      /* PA5  = D13, LD4 на платі      */
#define LED_GPIO_Port      GPIOA
#define BTN_Pin            GPIO_PIN_10     /* PA10 = D2, кнопка на GND      */
#define BTN_GPIO_Port      GPIOA
#define POT_Pin            GPIO_PIN_0      /* PA0  = A0, ADC1_IN0           */
#define POT_GPIO_Port      GPIOA
#define SERVO_PAN_Pin      GPIO_PIN_6      /* PA6  = D12, TIM3_CH1 (AF1)    */
#define SERVO_TILT_Pin     GPIO_PIN_7      /* PA7  = D11, TIM3_CH2 (AF1)    */
#define SERVO_GPIO_Port    GPIOA
#define I2C1_SCL_Pin       GPIO_PIN_8      /* PB8  = D15, I2C1_SCL (AF6)    */
#define I2C1_SDA_Pin       GPIO_PIN_9      /* PB9  = D14, I2C1_SDA (AF6)    */
#define I2C1_GPIO_Port     GPIOB
#define VCP_TX_Pin         GPIO_PIN_2      /* PA2  = USART2_TX (AF1)        */
#define VCP_RX_Pin         GPIO_PIN_3      /* PA3  = USART2_RX (AF1)        */

/* ---- MPU-6050 --------------------------------------------------------- */
#define MPU6050_ADDR       (0x68u << 1)    /* AD0 = GND -> 0x68            */
#define MPU6050_WHO_AM_I   0x75u
#define MPU6050_PWR_MGMT_1 0x6Bu
#define MPU6050_ACCEL_XOUT 0x3Bu

/* ---- Серво: TIM3, 1 тік = 1 мкс, період 20 мс (50 Гц) ----------------- */
#define SERVO_MIN_US       1000u
#define SERVO_MAX_US       2000u
#define SERVO_MID_US       1500u

extern I2C_HandleTypeDef  hi2c1;
extern UART_HandleTypeDef huart2;
extern ADC_HandleTypeDef  hadc1;
extern TIM_HandleTypeDef  htim3;

void Error_Handler(void);
void HAL_TIM_MspPostInit(TIM_HandleTypeDef *htim);

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
