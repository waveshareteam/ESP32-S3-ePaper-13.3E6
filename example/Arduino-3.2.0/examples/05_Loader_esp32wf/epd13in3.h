/**
  ******************************************************************************
  * @file    edp13in3.h
  * @author  Waveshare Team
  * @version V1.0.0
  * @date    2023-09-26
  * @brief   This file describes initialisation of 13.3k e-Papers
  *
  ******************************************************************************
  */

int EPD_13in3k_init() 
{
    EPD_Reset();
    EPD_WaitUntilIdle_high();

    EPD_SendCommand(0x12);
    EPD_WaitUntilIdle_high();

    EPD_SendCommand(0x0C);
    EPD_SendData(0xAE);
    EPD_SendData(0xC7);  
    EPD_SendData(0xC3);
    EPD_SendData(0xC0);
    EPD_SendData(0x80);

    EPD_SendCommand(0x01); 
    EPD_SendData(0xA7);
    EPD_SendData(0x02);  
    EPD_SendData(0x00);

    EPD_SendCommand(0x11);
    EPD_SendData(0x03);

    EPD_SendCommand(0x44);
    EPD_SendData(0x00);
    EPD_SendData(0x00);
    EPD_SendData(0xBF);
    EPD_SendData(0x03); 

    EPD_SendCommand(0x45); 
    EPD_SendData(0x00);
    EPD_SendData(0x00);  
    EPD_SendData(0xA7);
    EPD_SendData(0x02);

    EPD_SendCommand(0x3C);
    EPD_SendData(0x05);

    EPD_SendCommand(0x18);
    EPD_SendData(0x80);

    EPD_SendCommand(0x4E);
    EPD_SendData(0x00);

    EPD_SendCommand(0x4F);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    EPD_SendCommand(0x24);
    return 0;
}


void EPD_13in3k_Show(void)
{
    EPD_SendCommand(0x22);
	EPD_SendData(0xF7);
    EPD_SendCommand(0x20);
    EPD_WaitUntilIdle_high();


    EPD_SendCommand(0x10); // DEEP_SLEEP
    EPD_SendData(0x03);
}



/**********************************************************************/


int EPD_13in3b_init() 
{
    EPD_Reset();
    EPD_WaitUntilIdle_high();

    EPD_SendCommand(0x12);
    EPD_WaitUntilIdle_high();

    EPD_SendCommand(0x0C);
    EPD_SendData(0xAE);
    EPD_SendData(0xC7);  
    EPD_SendData(0xC3);
    EPD_SendData(0xC0);
    EPD_SendData(0x80);

    EPD_SendCommand(0x01); 
    EPD_SendData(0xA7);
    EPD_SendData(0x02);  
    EPD_SendData(0x00);

    EPD_SendCommand(0x11);
    EPD_SendData(0x03);

    EPD_SendCommand(0x44);
    EPD_SendData(0x00);
    EPD_SendData(0x00);
    EPD_SendData(0xBF);
    EPD_SendData(0x03); 

    EPD_SendCommand(0x45); 
    EPD_SendData(0x00);
    EPD_SendData(0x00);  
    EPD_SendData(0xA7);
    EPD_SendData(0x02);

    EPD_SendCommand(0x3C);
    EPD_SendData(0x01);

    EPD_SendCommand(0x18);
    EPD_SendData(0x80);

    EPD_SendCommand(0x4E);
    EPD_SendData(0x00);

    EPD_SendCommand(0x4F);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    EPD_SendCommand(0x24);
    return 0;
}


void EPD_13in3b_Show(void)
{
    EPD_SendCommand(0x22);
	EPD_SendData(0xF7);
    EPD_SendCommand(0x20);
    EPD_WaitUntilIdle_high();


    EPD_SendCommand(0x10); // DEEP_SLEEP
    EPD_SendData(0x03);
}


/*************************13.3E6**********************************/
void EPD_CS_ALL(UBYTE Value)
{
    digitalWrite(PIN_SPI_CS_M, Value);
    digitalWrite(PIN_SPI_CS_S, Value);
}

int EPD_13in3E_init() 
{
    Serial.print("\r\nEPD13in3E6");
    EPD_Reset();
    EPD_WaitUntilIdle();

    digitalWrite(PIN_SPI_CS_M, 0);
    EPD_SendCommand_13in3E6(0x74);
	EPD_SendData_13in3E6(0xC0);
    EPD_SendData_13in3E6(0x1C);
    EPD_SendData_13in3E6(0x1C);
    EPD_SendData_13in3E6(0xCC);
    EPD_SendData_13in3E6(0xCC);
    EPD_SendData_13in3E6(0xCC);
    EPD_SendData_13in3E6(0x15);
    EPD_SendData_13in3E6(0x15);
    EPD_SendData_13in3E6(0x55);
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0xF0);
	EPD_SendData_13in3E6(0x49);
    EPD_SendData_13in3E6(0x55);
    EPD_SendData_13in3E6(0x13);
    EPD_SendData_13in3E6(0x5D);
    EPD_SendData_13in3E6(0x05);
    EPD_SendData_13in3E6(0x10);
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0x00);
	EPD_SendData_13in3E6(0xDF);
    EPD_SendData_13in3E6(0x69);
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0x50);
	EPD_SendData_13in3E6(0xF7);
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0x60);
	EPD_SendData_13in3E6(0x03);
    EPD_SendData_13in3E6(0x03);
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0x86);
	EPD_SendData_13in3E6(0x10);
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0xE3);
	EPD_SendData_13in3E6(0x22);
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0xE0);
	EPD_SendData_13in3E6(0x01);
    EPD_CS_ALL(1);

    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0x61);
	EPD_SendData_13in3E6(0x04);
    EPD_SendData_13in3E6(0xB0);
    EPD_SendData_13in3E6(0x03);
    EPD_SendData_13in3E6(0x20);
    EPD_CS_ALL(1);

    digitalWrite(PIN_SPI_CS_M, 0);
    EPD_SendCommand_13in3E6(0x01);
	EPD_SendData_13in3E6(0x0F);
    EPD_SendData_13in3E6(0x00);
    EPD_SendData_13in3E6(0x28);
    EPD_SendData_13in3E6(0x2C);
    EPD_SendData_13in3E6(0x28);
    EPD_SendData_13in3E6(0x38);
    EPD_CS_ALL(1);

    digitalWrite(PIN_SPI_CS_M, 0);
    EPD_SendCommand_13in3E6(0xB6);
	EPD_SendData_13in3E6(0x07);
    EPD_CS_ALL(1);

    digitalWrite(PIN_SPI_CS_M, 0);
    EPD_SendCommand_13in3E6(0x06);
	EPD_SendData_13in3E6(0xE8);
    EPD_SendData_13in3E6(0x28);
    EPD_CS_ALL(1);

    digitalWrite(PIN_SPI_CS_M, 0);
    EPD_SendCommand_13in3E6(0xB7);
	EPD_SendData_13in3E6(0x01);
    EPD_CS_ALL(1);
	
    digitalWrite(PIN_SPI_CS_M, 0);
    EPD_SendCommand_13in3E6(0x05);
	EPD_SendData_13in3E6(0xE8);
    EPD_SendData_13in3E6(0x28);
    EPD_CS_ALL(1);

    digitalWrite(PIN_SPI_CS_M, 0);
    EPD_SendCommand_13in3E6(0xB0);
	EPD_SendData_13in3E6(0x01);
    EPD_CS_ALL(1);

    digitalWrite(PIN_SPI_CS_M, 0);
    EPD_SendCommand_13in3E6(0xB1);
	EPD_SendData_13in3E6(0x02);
    EPD_CS_ALL(1);

    digitalWrite(PIN_SPI_CS_M, 0);
    EPD_SendCommand_13in3E6(0x10);
    Serial.print("\r\nEPD13in3E6");
    return 0;
}


void EPD_13in3E_Show(void)
{
    EPD_CS_ALL(1);
    Serial.print("Write PON \r\n");
    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0x04); // POWER_ON
    EPD_CS_ALL(1);
    EPD_WaitUntilIdle();

    Serial.print("Write DRF \r\n");
    delay(50);
    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0x12);
	EPD_SendData_13in3E6(0x00);
    EPD_CS_ALL(1);
    EPD_WaitUntilIdle();

    Serial.print("Write POF \r\n");
    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0x02);
	EPD_SendData_13in3E6(0x00);
    EPD_CS_ALL(1);
    // EPD_WaitUntilIdle();
    Serial.print("Display Done!! \r\n");

    EPD_CS_ALL(0);
    EPD_SendCommand_13in3E6(0x07); // DEEP_SLEEP
    EPD_SendData_13in3E6(0XA5);
    EPD_CS_ALL(1);
}









