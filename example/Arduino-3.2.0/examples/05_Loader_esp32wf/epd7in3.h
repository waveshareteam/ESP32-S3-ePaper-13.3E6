/**
  ******************************************************************************
  * @file    edp7in3.h
  * @author  Waveshare Team
  * @version V1.0.0
  * @date    2023-09-26
  * @brief   This file describes initialisation of 7.3E6 e-Papers
  *
  ******************************************************************************
  */

int EPD_7in3E_init() 
{
    Serial.print("\r\nEPD7in3E6");
    EPD_Reset();
    EPD_WaitUntilIdle();
    delay(30);

    EPD_SendCommand(0xAA);    // CMDH
    EPD_SendData(0x49);
    EPD_SendData(0x55);
    EPD_SendData(0x20);
    EPD_SendData(0x08);
    EPD_SendData(0x09);
    EPD_SendData(0x18);

    EPD_SendCommand(0x01);//
    EPD_SendData(0x3F);

    EPD_SendCommand(0x00);  
    EPD_SendData(0x5F);
    EPD_SendData(0x69);

    EPD_SendCommand(0x03);
    EPD_SendData(0x00);
    EPD_SendData(0x54);
    EPD_SendData(0x00);
    EPD_SendData(0x44); 

    EPD_SendCommand(0x05);
    EPD_SendData(0x40);
    EPD_SendData(0x1F);
    EPD_SendData(0x1F);
    EPD_SendData(0x2C);

    EPD_SendCommand(0x06);
    EPD_SendData(0x6F);
    EPD_SendData(0x1F);
    EPD_SendData(0x17);
    EPD_SendData(0x49);

    EPD_SendCommand(0x08);
    EPD_SendData(0x6F);
    EPD_SendData(0x1F);
    EPD_SendData(0x1F);
    EPD_SendData(0x22);

    EPD_SendCommand(0x30);
    EPD_SendData(0x03);
    
    EPD_SendCommand(0x50);
    EPD_SendData(0x3F);

    EPD_SendCommand(0x60);
    EPD_SendData(0x02);
    EPD_SendData(0x00);

    EPD_SendCommand(0x61);
    EPD_SendData(0x03);
    EPD_SendData(0x20);
    EPD_SendData(0x01); 
    EPD_SendData(0xE0);

    EPD_SendCommand(0x84);
    EPD_SendData(0x01);

    EPD_SendCommand(0xE3);
    EPD_SendData(0x2F);

    EPD_SendCommand(0x10);
    return 0;
}


void EPD_7in3E_Show(void)
{
    EPD_SendCommand(0x04); // POWER_ON
    EPD_WaitUntilIdle();

    //Second setting 
    EPD_SendCommand(0x06);
    EPD_SendData(0x6F);
    EPD_SendData(0x1F);
    EPD_SendData(0x17);
    EPD_SendData(0x49);

    EPD_SendCommand(0x12); // DISPLAY_REFRESH
    EPD_SendData(0x00);
    EPD_WaitUntilIdle();
    
    EPD_SendCommand(0x02); // POWER_OFF
    EPD_SendData(0X00);
    EPD_WaitUntilIdle();

    EPD_SendCommand(0x07); // DEEP_SLEEP
    EPD_SendData(0XA5);
}
















