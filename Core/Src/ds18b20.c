#include "ds18b20.h"

//
uint8_t LastDeviceFlag;
uint8_t LastDiscrepancy;
uint8_t LastFamilyDiscrepancy;
uint8_t ROM_NO[8];
extern uint8_t Dev_ID[8][8];
extern uint8_t Dev_Cnt;


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

uint8_t ds18b20_SearchRom(uint8_t *Addr) {
    uint8_t id_bit_number; 
		uint8_t last_zero, rom_byte_number, search_result;
		uint8_t id_bit, cmp_id_bit;
		uint8_t rom_byte_mask, search_direction;
		
		//проинициализируем переменные
		id_bit_number = 1;
		last_zero = 0;
		rom_byte_number = 0;
		rom_byte_mask = 1;
		search_result = 0;
		if (!LastDeviceFlag)
		{
			ds18b20_Reset();
			ds18b20_WriteByte(0xF0);
		}
		do
		{
			id_bit = ds18b20_ReadBit();
			cmp_id_bit = ds18b20_ReadBit();
			if ((id_bit == 1) && (cmp_id_bit == 1))
				break;
			else
			{
				if (id_bit != cmp_id_bit)
					search_direction = id_bit; // bit write value for search
				else
				{
					if (id_bit_number < LastDiscrepancy)
						search_direction = ((ROM_NO[rom_byte_number] & rom_byte_mask) > 0);
					else
						search_direction = (id_bit_number == LastDiscrepancy);
					if (search_direction == 0)
					{
						last_zero = id_bit_number;
						if (last_zero < 9)
						LastFamilyDiscrepancy = last_zero;
					}
				}
				if (search_direction == 1)
					ROM_NO[rom_byte_number] |= rom_byte_mask;
				else
					ROM_NO[rom_byte_number] &= ~rom_byte_mask;
				ds18b20_WriteBit(search_direction);
				id_bit_number++;
				rom_byte_mask <<= 1;
				if (rom_byte_mask == 0)
				{
					rom_byte_number++;
					rom_byte_mask = 1;
				}
			}
		} while(rom_byte_number < 8); // ��������� ����� � 0 �� 7 � �����
		if (!(id_bit_number < 65))
		{
			// search successful so set LastDiscrepancy,LastDeviceFlag,search_result
			LastDiscrepancy = last_zero;
			// check for last device
			if (LastDiscrepancy == 0)
				LastDeviceFlag = 1;
			search_result = 1;	
		}
		if (!search_result || !ROM_NO[0])
		{
			LastDiscrepancy = 0;
			LastDeviceFlag = 0;
			LastFamilyDiscrepancy = 0;
			search_result = 0;
		}
		else
		{
			for (int i = 0; i < 8; i++) Addr[i] = ROM_NO[i];
		}	
		return search_result;
}

uint8_t ds18b20_init(uint8_t mode)
{
	int i = 0, j=0;
  uint8_t dt[8];
  if(mode==SKIP_ROM)
  {
		if(ds18b20_Reset()) return 1;
		//SKIP ROM
		ds18b20_WriteByte(0xCC);
		//WRITE SCRATCHPAD
		ds18b20_WriteByte(0x4E);
		//TH REGISTER 100 ��������
		ds18b20_WriteByte(0x64);
		//TL REGISTER - 30 ��������
		ds18b20_WriteByte(0x9E);
		//Resolution 12 bit
		ds18b20_WriteByte(RESOLUTION_12BIT);
  }
	else
	{
		for(i=1;i<=8;i++)
		{
			if(ds18b20_SearchRom(dt))
			{
				Dev_Cnt++;
				memcpy(Dev_ID[Dev_Cnt-1],dt,sizeof(dt));
			}
			else break;
		}
		for(i=1;i<=Dev_Cnt;i++)
		{
			if(ds18b20_Reset()) return 1;
			//Match Rom
			ds18b20_WriteByte(0x55);
			for(j=0;j<=7;j++)
			{
				ds18b20_WriteByte(Dev_ID[i-1][j]);
			}
			//WRITE SCRATCHPAD
			ds18b20_WriteByte(0x4E);
			//TH REGISTER 100 ��������
			ds18b20_WriteByte(0x64);
			//TL REGISTER - 30 ��������
			ds18b20_WriteByte(0x9E);
			//Resolution 12 bit
			ds18b20_WriteByte(RESOLUTION_12BIT);
		}
	}
  return 0;
}
//----------------------------------------------------------
void ds18b20_MeasureTemperCmd(uint8_t mode, uint8_t DevNum)
{
  int i = 0;
  ds18b20_Reset();
  if(mode==SKIP_ROM)
  {
    //SKIP ROM
    ds18b20_WriteByte(0xCC);
  }
	else
	{
		//Match Rom
		ds18b20_WriteByte(0x55);
		for(i=0;i<=7;i++)
		{
			ds18b20_WriteByte(Dev_ID[DevNum-1][i]);
		}
	}
  //CONVERT T
  ds18b20_WriteByte(0x44);
}
//----------------------------------------------------------
void ds18b20_ReadStratcpad(uint8_t mode, uint8_t *Data, uint8_t DevNum)
{
  uint8_t i;
  ds18b20_Reset();
  if(mode == SKIP_ROM)
  {
    //SKIP ROM
    ds18b20_WriteByte(0xCC);
  }
	else
	{
		//Match Rom
		ds18b20_WriteByte(0x55);
		for(i=0;i<=7;i++)
		{
			ds18b20_WriteByte(Dev_ID[DevNum-1][i]);
		}
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

//	Подключение второго датчика ds18b20



