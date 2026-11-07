/**
 * ЛР1 — bring-up STM32-платформи (форма B, Wokwi)
 * Проєкт: Motion-Mirroring Teleoperator (тема 6)
 *
 * MCU:   STM32C031C6 (Nucleo-C031C6), HAL, 48 МГц від HSI48
 * Інтерфейси:
 *   USART2  (PA2/PA3)  — ST-Link VCP, printf, 115200 8N1
 *   I2C1    (PB8/PB9)  — MPU-6050 @0x68, 100 кГц
 *   ADC1    (PA0, IN0) — потенціометр (ручне керування / калібрування)
 *   TIM3    (PA6/PA7)  — PWM 50 Гц на два серво (pan/tilt)
 *   GPIO    (PA10)     — кнопка режиму, (PA5) — світлодіод LD4
 *
 * Структура: функції MX_*_Init() відповідають тому, що генерує STM32CubeMX
 * (див. firmware.ioc). Код користувача лише перевіряє ініціалізацію,
 * ідентифікує датчики й виводить «tick N» щосекунди.
 */
#include "main.h"
#include <stdio.h>
#include <math.h>

I2C_HandleTypeDef  hi2c1;
UART_HandleTypeDef huart2;
ADC_HandleTypeDef  hadc1;
TIM_HandleTypeDef  htim3;

void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static HAL_StatusTypeDef MX_USART2_UART_Init(void);
static HAL_StatusTypeDef MX_I2C1_Init(void);
static HAL_StatusTypeDef MX_ADC1_Init(void);
static HAL_StatusTypeDef MX_TIM3_Init(void);

/* ---------------------------------------------------------------------- */
/* Допоміжні функції                                                      */
/* ---------------------------------------------------------------------- */
static void report(const char *name, HAL_StatusTypeDef st)
{
  static const char *names[] = { "HAL_OK", "HAL_ERROR", "HAL_BUSY", "HAL_TIMEOUT" };
  printf("[%s] %-22s -> %s\r\n", st == HAL_OK ? " OK " : "FAIL", name,
         names[st <= HAL_TIMEOUT ? st : HAL_ERROR]);
}

static void i2c_scan(void)
{
  printf("I2C1 scan:");
  int found = 0;
  for (uint16_t a = 0x08; a < 0x78; a++) {
    if (HAL_I2C_IsDeviceReady(&hi2c1, (uint16_t)(a << 1), 2, 5) == HAL_OK) {
      printf(" 0x%02X", a);
      found++;
    }
  }
  printf(found ? "  (%d device(s))\r\n" : " none\r\n", found);
}

static uint16_t adc_read(void)
{
  uint16_t v = 0;
  if (HAL_ADC_Start(&hadc1) == HAL_OK &&
      HAL_ADC_PollForConversion(&hadc1, 10) == HAL_OK) {
    v = (uint16_t)HAL_ADC_GetValue(&hadc1);
  }
  HAL_ADC_Stop(&hadc1);
  return v;                                       /* 0..4095 (12 біт) */
}

static void servo_set_us(uint32_t channel, uint32_t us)
{
  if (us < SERVO_MIN_US) us = SERVO_MIN_US;
  if (us > SERVO_MAX_US) us = SERVO_MAX_US;
  __HAL_TIM_SET_COMPARE(&htim3, channel, us);
}

/* кут -90..+90° -> 1000..2000 мкс */
static uint32_t deg_to_us(int deg)
{
  if (deg < -90) deg = -90;
  if (deg > 90)  deg = 90;
  return (uint32_t)(SERVO_MID_US + deg * 500 / 90);
}

static int mpu_ok = 0;
static int use_soft_i2c = 0;   /* 1 = апаратний I2C1 не зміг прочитати -> bit-bang */

/* ---------------------------------------------------------------------- */
/* Програмний I2C (bit-bang) на PB8/PB9 через HAL_GPIO, open-drain.        */
/* Потрібен лише як обхід: модель I2C STM32C0 у Wokwi не тактує SCL під    */
/* час читання (див. docs/report.md, проблема 7). На реальному залізі     */
/* апаратний HAL_I2C працює, і цей код не вмикається.                     */
/* ---------------------------------------------------------------------- */
#define SCL_H() HAL_GPIO_WritePin(I2C1_GPIO_Port, I2C1_SCL_Pin, GPIO_PIN_SET)
#define SCL_L() HAL_GPIO_WritePin(I2C1_GPIO_Port, I2C1_SCL_Pin, GPIO_PIN_RESET)
/* SDA «відпущено» = вхід з pull-up (лінію тягне вгору резистор або slave
 * тягне вниз); «0» = вихід, низький рівень. Так читання не залежить від
 * того, як модель відображає IDR для open-drain виходу. */
static void sda_mode(uint32_t mode)
{
  GPIO_InitTypeDef g = {0};
  g.Pin = I2C1_SDA_Pin; g.Mode = mode; g.Pull = GPIO_PULLUP; g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(I2C1_GPIO_Port, &g);
}
#define SDA_H() sda_mode(GPIO_MODE_INPUT)
#define SDA_L() do { HAL_GPIO_WritePin(I2C1_GPIO_Port, I2C1_SDA_Pin, GPIO_PIN_RESET); \
                     sda_mode(GPIO_MODE_OUTPUT_OD); } while (0)
#define SDA_R() HAL_GPIO_ReadPin(I2C1_GPIO_Port, I2C1_SDA_Pin)

static void sw_delay(void) { for (volatile int i = 0; i < 40; i++) { } }  /* ~5 мкс */

static void sw_i2c_init(void)
{
  GPIO_InitTypeDef g = {0};
  HAL_I2C_DeInit(&hi2c1);
  g.Pin   = I2C1_SCL_Pin | I2C1_SDA_Pin;
  g.Mode  = GPIO_MODE_OUTPUT_OD;
  g.Pull  = GPIO_PULLUP;
  g.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(I2C1_GPIO_Port, &g);
  SDA_H(); SCL_H(); sw_delay();
}
static void sw_start(void) { SDA_H(); SCL_H(); sw_delay(); SDA_L(); sw_delay(); SCL_L(); }
static void sw_stop(void)  { SDA_L(); sw_delay(); SCL_H(); sw_delay(); SDA_H(); sw_delay(); }
static int sw_write(uint8_t b)          /* повертає 1, якщо ACK */
{
  for (int i = 7; i >= 0; i--) {
    if (b & (1u << i)) SDA_H(); else SDA_L();
    sw_delay(); SCL_H(); sw_delay(); SCL_L();
  }
  SDA_H(); sw_delay(); SCL_H(); sw_delay();
  int ack = (SDA_R() == GPIO_PIN_RESET);
  SCL_L();
  return ack;
}
static uint8_t sw_read(int ack)
{
  uint8_t b = 0;
  SDA_H();
  for (int i = 0; i < 8; i++) {
    sw_delay(); SCL_H(); sw_delay();
    b = (uint8_t)((b << 1) | (SDA_R() == GPIO_PIN_SET));
    SCL_L();
  }
  if (ack) SDA_L(); else SDA_H();
  sw_delay(); SCL_H(); sw_delay(); SCL_L(); SDA_H();
  return b;
}

/* Читання регістрів MPU-6050 (register pointer write + repeated start + read) */
static HAL_StatusTypeDef mpu_read(uint8_t reg, uint8_t *buf, uint16_t n)
{
  if (!use_soft_i2c)
    return HAL_I2C_Mem_Read(&hi2c1, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT, buf, n, 20);
  sw_start();
  if (!sw_write(MPU6050_ADDR) || !sw_write(reg)) { sw_stop(); return HAL_ERROR; }
  sw_start();
  if (!sw_write(MPU6050_ADDR | 1u)) { sw_stop(); return HAL_ERROR; }
  for (uint16_t i = 0; i < n; i++) buf[i] = sw_read(i + 1 < n);
  sw_stop();
  return HAL_OK;
}

static HAL_StatusTypeDef mpu_write(uint8_t reg, uint8_t val)
{
  if (!use_soft_i2c)
    return HAL_I2C_Mem_Write(&hi2c1, MPU6050_ADDR, reg, I2C_MEMADD_SIZE_8BIT, &val, 1, 20);
  sw_start();
  int ok = sw_write(MPU6050_ADDR) && sw_write(reg) && sw_write(val);
  sw_stop();
  return ok ? HAL_OK : HAL_ERROR;
}

static void mpu6050_probe(void)
{
  uint8_t id = 0;
  HAL_StatusTypeDef st = mpu_read(MPU6050_WHO_AM_I, &id, 1);
  if (st != HAL_OK || id != 0x68) {
    printf("HW I2C1 read: st=%d err=0x%lX id=0x%02X -> fallback to bit-bang I2C\r\n",
           st, HAL_I2C_GetError(&hi2c1), id);
    use_soft_i2c = 1;
    sw_i2c_init();
    st = mpu_read(MPU6050_WHO_AM_I, &id, 1);
    printf("soft-I2C read: st=%d id=0x%02X\r\n", st, id);
  }
  printf("MPU-6050 WHO_AM_I (0x75) = 0x%02X  [%s] %s\r\n", id, use_soft_i2c ? "soft-I2C" : "HAL I2C1",
         (st == HAL_OK && id == 0x68) ? "-> identified" : "-> NOT FOUND");
  if (st == HAL_OK && id == 0x68) {
    /* вивести з режиму сну: PWR_MGMT_1 = 0 */
    mpu_write(MPU6050_PWR_MGMT_1, 0x00);
    mpu_ok = 1;
  }
}

/* Повертає pitch/roll у градусах (з акселерометра) */
static int mpu6050_angles(int *pitch, int *roll)
{
  uint8_t b[6];
  if (!mpu_ok || mpu_read(MPU6050_ACCEL_XOUT, b, 6) != HAL_OK)
    return 0;
  float ax = (int16_t)((b[0] << 8) | b[1]);
  float ay = (int16_t)((b[2] << 8) | b[3]);
  float az = (int16_t)((b[4] << 8) | b[5]);
  *pitch = (int)(atan2f(-ax, sqrtf(ay * ay + az * az)) * 57.2958f);
  *roll  = (int)(atan2f(ay, az) * 57.2958f);
  return 1;
}

/* ---------------------------------------------------------------------- */
int main(void)
{
  HAL_Init();
  SystemClock_Config();
  MX_GPIO_Init();
  HAL_StatusTypeDef st_uart = MX_USART2_UART_Init();

  printf("\r\n=== hello from STM32 ===\r\n");
  printf("LR1 bring-up | MCU STM32C031C6 | SYSCLK = %lu Hz\r\n", HAL_RCC_GetSysClockFreq());
  report("MX_USART2_UART_Init", st_uart);
  report("MX_I2C1_Init", MX_I2C1_Init());
  report("MX_ADC1_Init", MX_ADC1_Init());
  report("MX_TIM3_Init", MX_TIM3_Init());
  report("HAL_TIM_PWM_Start CH1", HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1));
  report("HAL_TIM_PWM_Start CH2", HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_2));

  i2c_scan();
  mpu6050_probe();
  printf("Button on PA10: switch MANUAL (pot) / MIRROR (IMU)\r\n\r\n");

  int mirror = 0;                       /* 0 = MANUAL, 1 = MIRROR */
  GPIO_PinState prev_btn = GPIO_PIN_SET;
  uint32_t t_ctrl = 0, t_tick = 0, ticks = 0;
  int pitch = 0, roll = 0;
  uint16_t pot = 0;

  while (1)
  {
    uint32_t now = HAL_GetTick();

    /* кнопка (активний нуль), реакція по фронту натискання */
    GPIO_PinState btn = HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin);
    if (prev_btn == GPIO_PIN_SET && btn == GPIO_PIN_RESET) {
      HAL_Delay(20);                    /* простий антидребезг */
      if (HAL_GPIO_ReadPin(BTN_GPIO_Port, BTN_Pin) == GPIO_PIN_RESET) {
        mirror = !mirror;
        printf("button pressed -> mode %s\r\n", mirror ? "MIRROR" : "MANUAL");
      }
    }
    prev_btn = btn;

    /* контур керування серво, 20 мс (50 Гц = частота PWM) */
    if (now - t_ctrl >= 20) {
      t_ctrl = now;
      pot = adc_read();
      if (mirror && mpu6050_angles(&pitch, &roll)) {
        servo_set_us(TIM_CHANNEL_1, deg_to_us(roll));
        servo_set_us(TIM_CHANNEL_2, deg_to_us(pitch));
      } else {
        uint32_t us = SERVO_MIN_US + (uint32_t)pot * (SERVO_MAX_US - SERVO_MIN_US) / 4095u;
        servo_set_us(TIM_CHANNEL_1, us);
        servo_set_us(TIM_CHANNEL_2, SERVO_MID_US);
      }
    }

    /* щосекунди: tick + стан системи */
    if (now - t_tick >= 1000) {
      t_tick = now;
      ticks++;
      HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
      mpu6050_angles(&pitch, &roll);
      printf("tick %lu | mode=%s pot=%u pitch=%d roll=%d pwm1=%lu pwm2=%lu\r\n",
             ticks, mirror ? "MIRROR" : "MANUAL", pot, pitch, roll,
             __HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_1),
             __HAL_TIM_GET_COMPARE(&htim3, TIM_CHANNEL_2));
    }
  }
}

/* ---------------------------------------------------------------------- */
/* Тактування: HSI48, HSIDIV=1 -> SYSCLK = HCLK = PCLK = 48 МГц           */
/* ---------------------------------------------------------------------- */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef osc = {0};
  RCC_ClkInitTypeDef clk = {0};

  osc.OscillatorType      = RCC_OSCILLATORTYPE_HSI;
  osc.HSIState            = RCC_HSI_ON;
  osc.HSIDiv              = RCC_HSI_DIV1;
  osc.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  if (HAL_RCC_OscConfig(&osc) != HAL_OK) Error_Handler();

  clk.ClockType      = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1;
  clk.SYSCLKSource   = RCC_SYSCLKSOURCE_HSI;
  clk.SYSCLKDivider  = RCC_SYSCLK_DIV1;
  clk.AHBCLKDivider  = RCC_HCLK_DIV1;
  clk.APB1CLKDivider = RCC_APB1_DIV1;
  if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_1) != HAL_OK) Error_Handler();
}

static HAL_StatusTypeDef MX_USART2_UART_Init(void)
{
  huart2.Instance            = USART2;
  huart2.Init.BaudRate       = 115200;
  huart2.Init.WordLength     = UART_WORDLENGTH_8B;
  huart2.Init.StopBits       = UART_STOPBITS_1;
  huart2.Init.Parity         = UART_PARITY_NONE;
  huart2.Init.Mode           = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl      = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling   = UART_OVERSAMPLING_16;
  huart2.Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
  huart2.Init.ClockPrescaler = UART_PRESCALER_DIV1;
  huart2.AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
  return HAL_UART_Init(&huart2);
}

static HAL_StatusTypeDef MX_I2C1_Init(void)
{
  hi2c1.Instance              = I2C1;
  hi2c1.Init.Timing           = 0x20303E5D;   /* 100 кГц @ 48 МГц (CubeMX) */
  hi2c1.Init.OwnAddress1      = 0;
  hi2c1.Init.AddressingMode   = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode  = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2      = 0;
  hi2c1.Init.OwnAddress2Masks = I2C_OA2_NOMASK;
  hi2c1.Init.GeneralCallMode  = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode    = I2C_NOSTRETCH_DISABLE;
  HAL_StatusTypeDef st = HAL_I2C_Init(&hi2c1);
  if (st == HAL_OK) st = HAL_I2CEx_ConfigAnalogFilter(&hi2c1, I2C_ANALOGFILTER_ENABLE);
  if (st == HAL_OK) st = HAL_I2CEx_ConfigDigitalFilter(&hi2c1, 0);
  return st;
}

static HAL_StatusTypeDef MX_ADC1_Init(void)
{
  ADC_ChannelConfTypeDef ch = {0};

  hadc1.Instance                   = ADC1;
  hadc1.Init.ClockPrescaler        = ADC_CLOCK_SYNC_PCLK_DIV2;
  hadc1.Init.Resolution            = ADC_RESOLUTION_12B;
  hadc1.Init.DataAlign             = ADC_DATAALIGN_RIGHT;
  hadc1.Init.ScanConvMode          = ADC_SCAN_DISABLE;
  hadc1.Init.EOCSelection          = ADC_EOC_SINGLE_CONV;
  hadc1.Init.LowPowerAutoWait      = DISABLE;
  hadc1.Init.LowPowerAutoPowerOff  = DISABLE;
  hadc1.Init.ContinuousConvMode    = DISABLE;
  hadc1.Init.NbrOfConversion       = 1;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv      = ADC_SOFTWARE_START;
  hadc1.Init.ExternalTrigConvEdge  = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.DMAContinuousRequests = DISABLE;
  hadc1.Init.Overrun               = ADC_OVR_DATA_OVERWRITTEN;
  hadc1.Init.SamplingTimeCommon1   = ADC_SAMPLETIME_39CYCLES_5;
  hadc1.Init.OversamplingMode      = DISABLE;
  hadc1.Init.TriggerFrequencyMode  = ADC_TRIGGER_FREQ_HIGH;
  HAL_StatusTypeDef st = HAL_ADC_Init(&hadc1);
  if (st != HAL_OK) return st;

  ch.Channel      = ADC_CHANNEL_0;
  ch.Rank         = ADC_REGULAR_RANK_1;
  ch.SamplingTime = ADC_SAMPLINGTIME_COMMON_1;
  return HAL_ADC_ConfigChannel(&hadc1, &ch);
}

static HAL_StatusTypeDef MX_TIM3_Init(void)
{
  TIM_ClockConfigTypeDef  clk = {0};
  TIM_MasterConfigTypeDef mst = {0};
  TIM_OC_InitTypeDef      oc  = {0};

  htim3.Instance               = TIM3;
  htim3.Init.Prescaler         = 48 - 1;        /* 48 МГц / 48 = 1 МГц   */
  htim3.Init.CounterMode       = TIM_COUNTERMODE_UP;
  htim3.Init.Period            = 20000 - 1;     /* 20 мс -> 50 Гц         */
  htim3.Init.ClockDivision     = TIM_CLOCKDIVISION_DIV1;
  htim3.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE;
  HAL_StatusTypeDef st = HAL_TIM_Base_Init(&htim3);
  if (st != HAL_OK) return st;

  clk.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if ((st = HAL_TIM_ConfigClockSource(&htim3, &clk)) != HAL_OK) return st;
  if ((st = HAL_TIM_PWM_Init(&htim3)) != HAL_OK) return st;

  mst.MasterOutputTrigger = TIM_TRGO_RESET;
  mst.MasterSlaveMode     = TIM_MASTERSLAVEMODE_DISABLE;
  if ((st = HAL_TIMEx_MasterConfigSynchronization(&htim3, &mst)) != HAL_OK) return st;

  oc.OCMode     = TIM_OCMODE_PWM1;
  oc.Pulse      = SERVO_MID_US;
  oc.OCPolarity = TIM_OCPOLARITY_HIGH;
  oc.OCFastMode = TIM_OCFAST_DISABLE;
  if ((st = HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_1)) != HAL_OK) return st;
  if ((st = HAL_TIM_PWM_ConfigChannel(&htim3, &oc, TIM_CHANNEL_2)) != HAL_OK) return st;

  HAL_TIM_MspPostInit(&htim3);
  return HAL_OK;
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef g = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_RESET);
  g.Pin   = LED_Pin;
  g.Mode  = GPIO_MODE_OUTPUT_PP;
  g.Pull  = GPIO_NOPULL;
  g.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &g);

  g.Pin  = BTN_Pin;
  g.Mode = GPIO_MODE_INPUT;
  g.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(BTN_GPIO_Port, &g);
}

void Error_Handler(void)
{
  __disable_irq();
  while (1) {
    /* швидке блимання LD4 як ознака фатальної помилки */
    HAL_GPIO_TogglePin(LED_GPIO_Port, LED_Pin);
    for (volatile uint32_t i = 0; i < 200000; i++) { }
  }
}

#ifdef USE_FULL_ASSERT
void assert_failed(uint8_t *file, uint32_t line)
{
  printf("assert failed: %s:%lu\r\n", file, line);
}
#endif
