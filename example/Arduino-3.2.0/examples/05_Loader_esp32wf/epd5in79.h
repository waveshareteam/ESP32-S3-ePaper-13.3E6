/**
  ******************************************************************************
  * @file    edp5in79.h
  * @author  Waveshare Team
  * @version V1.0
  * @date    23-Dec-2025
  * @brief   This file describes initialisation of 5.79b e-Papers
  *
  ******************************************************************************
  */

#define EPD_5in79b_WIDTH       792
#define EPD_5in79b_HEIGHT      272

void EPD_5in79b_WaitUntilIdle(void)
{
	UBYTE busy;
	do
	{
		busy = digitalRead(PIN_SPI_BUSY);
        delay(10);   
	}
	while(busy);   
	delay(200);     
}

int EPD_5in79B_init() 
{
    EPD_Reset();
    EPD_5in79b_WaitUntilIdle();
    EPD_SendCommand(0x12);  
    EPD_5in79b_WaitUntilIdle();
    
    EPD_SendCommand(0x11);
    EPD_SendData(0x01);
    EPD_SendCommand(0x44);
    EPD_SendData(0x00);
    EPD_SendData(0x31);
    EPD_SendCommand(0x45);
    EPD_SendData(0x0f);
    EPD_SendData(0x01);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    EPD_SendCommand(0x4e);
    EPD_SendData(0x00);
    EPD_SendCommand(0x4f);
    EPD_SendData(0x0f);
    EPD_SendData(0x01);

    EPD_SendCommand(0x91);
    EPD_SendData(0x00);

    EPD_SendCommand(0xC4);
    EPD_SendData(0x31);
    EPD_SendData(0x00);
    EPD_SendCommand(0xC5);
    EPD_SendData(0x0f);
    EPD_SendData(0x01);
    EPD_SendData(0x00);
    EPD_SendData(0x00);

    EPD_SendCommand(0xCe);
    EPD_SendData(0x31);
    EPD_SendCommand(0xCf);
    EPD_SendData(0x0f);
    EPD_SendData(0x01);

    EPD_SendCommand(0x24);
    
    return 0;
}  

void EPD_5in79B_Show() 
{
    EPD_SendCommand(0x22);
    EPD_SendData(0xF7);
    EPD_SendCommand(0x20);
    delay(100);
    EPD_5in79b_WaitUntilIdle();

    EPD_SendCommand(0x10);
    EPD_SendData(0x03);
    Serial.printf("1111");
}

