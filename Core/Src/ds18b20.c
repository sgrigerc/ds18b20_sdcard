#include "ds18b20.h"
//--------------------------------------------------
void DelayMicro(__IO uint32_t micros)
{
micros *= (SystemCoreClock / 1000000)/9;
/* Wait till done */
while (micros--) ;
}
//--------------------------------------------------
void port_init(void)
{
    // Включаем тактирование GPIOB
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;

    // Сброс всех настроек PB11
    GPIOB->MODER &= ~GPIO_MODER_MODER11;          // Очистка битов MODER для PB11
    GPIOB->OTYPER &= ~GPIO_OTYPER_OT_11;          // Установка push-pull (временно)
    GPIOB->OSPEEDR &= ~GPIO_OSPEEDER_OSPEEDR11;   // Low speed
    GPIOB->PUPDR &= ~GPIO_PUPDR_PUPDR11;          // No pull-up/pull-down

    // Настройка как General Purpose Output
    GPIOB->MODER |= GPIO_MODER_MODER11_0;         // output mode
    GPIOB->OTYPER |= GPIO_OTYPER_OT_11;           // open-drain
}
//--------------------------------------------------
uint8_t ds18b20_Reset(void)
{
    uint8_t status;

    GPIOB->BSRR = GPIO_BSRR_BR11;   // LOW
    DelayMicro(650);

    GPIOB->BSRR = GPIO_BSRR_BS11;   // HIGH
    DelayMicro(65);

    status = (GPIOB->IDR & GPIO_IDR_IDR_11) ? 1 : 0;
    DelayMicro(600);

    return status;
}
//----------------------------------------------------------
uint8_t ds18b20_ReadBit(void)
{
    uint8_t bit = 0;

    GPIOB->BSRR = GPIO_BSRR_BR11;   // LOW
    DelayMicro(2);

    GPIOB->BSRR = GPIO_BSRR_BS11;   // HIGH
    DelayMicro(13);

    bit = (GPIOB->IDR & GPIO_IDR_IDR_11) ? 1 : 0;
    DelayMicro(45);

    return bit;
}
//-----------------------------------------------
uint8_t ds18b20_ReadByte(void)
{
    uint8_t data = 0;
    for (uint8_t i = 0; i < 8; i++) {
        data >>= 1;
        if (ds18b20_ReadBit()) {
            data |= 0x80;
        }
    }
    return data;
}
//-----------------------------------------------
void ds18b20_WriteBit(uint8_t bit)
{
    GPIOB->BSRR = GPIO_BSRR_BR11;   // LOW
    DelayMicro(bit ? 3 : 65);

    GPIOB->BSRR = GPIO_BSRR_BS11;   // HIGH
    DelayMicro(bit ? 65 : 3);
}
//-----------------------------------------------
void ds18b20_WriteByte(uint8_t dt)
{
  for (uint8_t i = 0; i < 8; i++)
  {
    ds18b20_WriteBit(dt >> i & 1);
    //Delay Protection
    DelayMicro(5);
  }
}
//-----------------------------------------------
uint8_t ds18b20_init(uint8_t mode)
{
	if(ds18b20_Reset()) return 1;
  if(mode==SKIP_ROM)
  {
		//SKIP ROM
		ds18b20_WriteByte(0xCC);
		//WRITE SCRATCHPAD
		ds18b20_WriteByte(0x4E);
		//TH REGISTER 100 градусов
		ds18b20_WriteByte(0x64);
		//TL REGISTER - 30 градусов
		ds18b20_WriteByte(0x9E);
		//Resolution 12 bit
		ds18b20_WriteByte(RESOLUTION_12BIT);
  }
  return 0;
}
//----------------------------------------------------------
void ds18b20_MeasureTemperCmd(uint8_t mode, uint8_t DevNum)
{
  ds18b20_Reset();
  if(mode==SKIP_ROM)
  {
    //SKIP ROM
    ds18b20_WriteByte(0xCC);
  }
  //CONVERT T
  ds18b20_WriteByte(0x44);
}
//----------------------------------------------------------
void ds18b20_ReadStratcpad(uint8_t mode, uint8_t *Data, uint8_t DevNum)
{
  uint8_t i;
  ds18b20_Reset();
  if(mode==SKIP_ROM)
  {
    //SKIP ROM
    ds18b20_WriteByte(0xCC);
  }
  //READ SCRATCHPAD
  ds18b20_WriteByte(0xBE);
  for(i=0;i<8;i++)
  {
    Data[i] = ds18b20_ReadByte();
  }
}
//----------------------------------------------------------
uint8_t ds18b20_GetSign(uint16_t dt)
{
  //Проверим 11-й бит
  if (dt&(1<<11)) return 1;
  else return 0;
}
//----------------------------------------------------------
float ds18b20_Convert(uint16_t dt)
{
  float t;
  t = (float) ((dt&0x07FF)>>4); //отборосим знаковые и дробные биты
  //Прибавим дробную часть
  t += (float)(dt&0x000F) / 16.0f;
  return t;
}
//----------------------------------------------------------
