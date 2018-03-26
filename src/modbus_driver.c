#include "stm8s.h"
#include "modbus_driver.h"
#include "crc.h"
#include "rs485.h"

uint8_t buffer[MB_BUFFER_SIZE];
MB_FRAME reply_frame;

uint8_t  MB_DisIn[MB_DI_AMOUNT / 8 + 1];     //��ɢ�����룬λ��ֻ��
uint8_t  MB_Coils[MB_COILS_AMOUNT / 8 + 1];  //��Ȧ��      λ����д
uint16_t MB_InReg[MB_IR_AMOUNT];             //����Ĵ������֣�ֻ��
uint16_t MB_HoldReg[MB_HR_AMOUNT];           //���ּĴ������֣���д
extern uint8_t mb_local_address;

/*
//��ASCII��ֵת����16������ֵ
//pbufferΪ������ָ�룬��СΪMB_BUFFER_SIZE
//lengthΪ�����������ݳ���
//16����0~9��ӦASCII��ֵ30~39��16����A~F(10~15)��ӦASCII��ֵ41~46
void MB_DRIVER_AsciiToHex(uint8_t * pbuffer, uint8_t length)
{
    uint8_t i, j;
    uint8_t temp_hi;
    uint8_t temp_low;
    j = 0;
    for(i = 1; i < length - 3; i++)
    {
        if(pbuffer[i] < 40)  temp_hi = pbuffer[i] - 30;  //��4λֵΪ0~9
        else                 temp_hi = pbuffer[i] - 31;  //��4λֵΪA~F(10~15)
        i++;
        if(pbuffer[i] < 40)  temp_low = pbuffer[i] - 30; //��4λֵΪ0~9
        else                 temp_low = pbuffer[i] - 31; //��4λֵΪA~F(10~15)
        pbuffer[j++] = temp_hi * 16 +  temp_low;         //ת����16������ֵ
    }	
}
//��16������ֵת����ASCII��ֵ 
//pbufferΪ������ָ�룬��СΪMB_BUFFER_SIZE
//lengthΪ�����������ݳ���
//16����0~9��ӦASCII��ֵ30~39��16����A~F(10~15)��ӦASCII��ֵ41~46
void MB_DRIVER_HexToAscii(uint8_t * pbuffer, uint8_t length)
{
    uint8_t i, j;
    uint8_t temp_hi;
    uint8_t temp_low;    
    //uint8_t buffer[MB_BUFFER_SIZE / 2];
    for(i = 0; i < length; i++)
    {
        buffer[i] = pbuffer[i];	
    }	
    j = 0;
    pbuffer[j++] = 0x3A;                                 //�����ʼ�ַ���ð�š�
    for(i = 0; i < length; i++)
    {
    	temp_hi = buffer[i] / 16 ;                       //ȡ��4λֵ
        temp_low = buffer[i] % 16 ;                      //ȡ��4λֵ
        if(temp_hi < 10)   pbuffer[j++] = temp_hi + 30;  //��4λֵΪ0~9
        else               pbuffer[j++] = temp_hi + 31;  //��4λֵΪA~F(10~15)
        if(temp_low < 10)  pbuffer[j++] = temp_low + 30; //��4λֵΪ0~9
        else               pbuffer[j++] = temp_low + 31; //��4λֵΪA~F(10~15)      	
    }
    pbuffer[j++] = 0x0D;                                 //��ӽ����ַ����س���
    pbuffer[j++] = 0x0A;                                 //��ӽ����ַ������С�
}*/

//��������  
void MB_DRIVER_SendData(MB_FRAME* pframe)
{
    //uint8_t  buffer[MB_BUFFER_SIZE];
    uint16_t i;
    uint16_t index=0;			
    uint16_t length=0;
    uint16_t crc_result;
    buffer[index++]=(*pframe).Address;
    buffer[index++]=(*pframe).Function;
    for (i = 0; i < (*pframe).Length; i++)
    {
        buffer[index++] = (*pframe).Data[i];
    }
    length = index;                                 //��ҪУ����ֽ��ܳ���
    crc_result = CRC16(buffer, length);             //����У����
    buffer[index++] = *((uint8_t*)&(crc_result) + 1); //У����ֽ� 
    buffer[index++] = *((uint8_t*)&crc_result);       //У����ֽ�  
    length = index;                                 //��Ҫ���͵��ֽ��ܳ���

#ifdef  _MB_ASCII                                   //���ΪASCIIģʽ
    MB_DRIVER_HexToAscii(buffer, length);              //��16������ֵת����ASCII��ֵ
    length = length*2 + 3;
#endif        

	rs485_send(buffer,length);
		
}
//������(�ӻ�) 
#ifdef _ModServer 
/*
//������01������Ȧ  ����λ�����
void MB_DRIVER_ReadCoils(MB_FRAME *pframe)   
{
    //����֡    PDU
    //������  	1 �ֽ� 	0x01
    //��ʼ��ַ 	2 �ֽ�  0x0000 to 0xFFFF
    //��Ȧ���� 	2 �ֽ�  1 to 2000 (0x7D0)
	
    //��Ӧ֡    PDU
    //������ 	1 �ֽ� 	0x01
    //�ֽڼ���  1 �ֽ� 	n
    //��Ȧ״̬  n �ֽ� 	n = N or N+1
    //N = ������� / 8, �������������0, n = N+1;���� n = N
    
    uint8_t  *p;
    uint16_t i;	
    uint16_t start_addr;
    uint16_t quantity;
	
    p = (uint8_t *)&start_addr;       //��Ȧ�׵�ַ   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //��Ȧ����    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //��������ڹ涨��Χ��        	
    if((quantity >= 1)&&(quantity <= MB_COILS_AMOUNT))    
    {
    	//��ʼ��ַ��ȷ����ʼ��ַ+��������ڹ涨��Χ��
        if((start_addr < MB_COILS_AMOUNT)&&((start_addr + quantity) <= MB_COILS_AMOUNT))
        {   
            reply_frame.Function = 0x01;
            if ((quantity % 8) == 0) reply_frame.Length = quantity/8;    //�ֽڼ��� ������
            else                     reply_frame.Length = quantity/8 + 1;//�ֽڼ��� ������
            reply_frame.Data[0] = reply_frame.Length++;      //��Ӧ֡�ֽڼ��� ����	
            for (i = 1; i < reply_frame.Length + 1; i++) 
            {
            	reply_frame.Data[i] = 0x0;       //��Ȧ״̬��0		
            }
            for (i = 0; i < quantity; i++)       //����Ȧ״̬
            {
                if (MB_DRIVER_GetDisOrCoils(MB_Coils,start_addr + i)) //��Ȧ״̬Ϊ1
                {
                    switch(i % 8)                //һ���ֽ��е�ĳһλ
                    {
                        case 0: reply_frame.Data[i/8 + 1] |= 0x01; break;
                        case 1: reply_frame.Data[i/8 + 1] |= 0x02; break;
                        case 2: reply_frame.Data[i/8 + 1] |= 0x04; break;
                        case 3: reply_frame.Data[i/8 + 1] |= 0x08; break;
                        case 4: reply_frame.Data[i/8 + 1] |= 0x10; break;
                        case 5: reply_frame.Data[i/8 + 1] |= 0x20; break;
                        case 6: reply_frame.Data[i/8 + 1] |= 0x40; break;
                        case 7: reply_frame.Data[i/8 + 1] |= 0x80; break;
                    }
                }
                else                //��Ȧ״̬Ϊ0
                {
                    switch(i % 8)   //һ���ֽ��е�ĳһλ
                    {
                        case 0: reply_frame.Data[i/8 + 1] &= 0xfe; break;
                        case 1: reply_frame.Data[i/8 + 1] &= 0xfd; break;
                        case 2: reply_frame.Data[i/8 + 1] &= 0xfb; break;
                        case 3: reply_frame.Data[i/8 + 1] &= 0xf7; break;
                        case 4: reply_frame.Data[i/8 + 1] &= 0xef; break;
                        case 5: reply_frame.Data[i/8 + 1] &= 0xdf; break;
                        case 6: reply_frame.Data[i/8 + 1] &= 0xbf; break;
                        case 7: reply_frame.Data[i/8 + 1] &= 0x7f; break;
                    }
                }
            } 
        }
        else
        {
            reply_frame.Function = 0x81;  //��������     0x81
            reply_frame.Data[0]  = 0x02;  //�쳣��         0x02	  		
            reply_frame.Length   = 1;     //�������ֽڳ���  1
        }	
    }
    else
    {
        reply_frame.Function = 0x81;       //��������     0x81
        reply_frame.Data[0]  = 0x03;       //�쳣��         0x03	
        reply_frame.Length   = 1;          //�������ֽڳ���  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //������Ӧ֡ 
}



//������02������ɢ�����루��λ���룩  
void MB_DRIVER_ReadDisIn(MB_FRAME *pframe)
{
    //����֡    PDU
    //������  	1 �ֽ� 	0x02
    //��ʼ��ַ 	2 �ֽ�  0x0000 to 0xFFFF
    //�������� 	2 �ֽ�  1 to 2000 (0x7D0)
	
    //��Ӧ֡    PDU
    //������ 	1 �ֽ� 	0x02
    //�ֽڼ���  1 �ֽ� 	n
    //����״̬  n �ֽ� 	n = N or N+1
    //N = �������� / 8, �������������0, n = N+1;���� n = N
    ////MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;	
    uint16_t start_addr;
    uint16_t quantity;
	
    p = (uint8_t *)&start_addr;       //��ɢ�������׵�ַ   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //��ɢ����������    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //���������ڹ涨��Χ��        	
    if((quantity >= 1)&&(quantity <= MB_DI_AMOUNT))    
    {
    	//��ʼ��ַ��ȷ����ʼ��ַ+���������ڹ涨��Χ��
        if((start_addr < MB_DI_AMOUNT)&&((start_addr + quantity) <= MB_DI_AMOUNT))
        {   
            reply_frame.Function = 0x02;
            if ((quantity % 8) == 0) reply_frame.Length = quantity/8;    //�ֽڼ��� ������
            else                     reply_frame.Length = quantity/8 + 1;//�ֽڼ��� ������
            reply_frame.Data[0] = reply_frame.Length++;      //��Ӧ֡�ֽڼ��� ����	
            for (i = 1; i < reply_frame.Length + 1; i++) 
            {
            	reply_frame.Data[i] = 0x0;       //��ɢ������״̬��0		
            }
            for (i = 0; i < quantity; i++)       //����ɢ������״̬
            {
                if (MB_DRIVER_GetDisOrCoils(MB_DisIn,start_addr + i)) //��ɢ������״̬Ϊ1
                {
                    switch(i % 8)                //һ���ֽ��е�ĳһλ
                    {
                        case 0: reply_frame.Data[i/8 + 1] |= 0x01; break;
                        case 1: reply_frame.Data[i/8 + 1] |= 0x02; break;
                        case 2: reply_frame.Data[i/8 + 1] |= 0x04; break;
                        case 3: reply_frame.Data[i/8 + 1] |= 0x08; break;
                        case 4: reply_frame.Data[i/8 + 1] |= 0x10; break;
                        case 5: reply_frame.Data[i/8 + 1] |= 0x20; break;
                        case 6: reply_frame.Data[i/8 + 1] |= 0x40; break;
                        case 7: reply_frame.Data[i/8 + 1] |= 0x80; break;
                    }
                }
                else                //��ɢ������״̬Ϊ0
                {
                    switch(i % 8)   //һ���ֽ��е�ĳһλ
                    {
                        case 0: reply_frame.Data[i/8 + 1] &= 0xfe; break;
                        case 1: reply_frame.Data[i/8 + 1] &= 0xfd; break;
                        case 2: reply_frame.Data[i/8 + 1] &= 0xfb; break;
                        case 3: reply_frame.Data[i/8 + 1] &= 0xf7; break;
                        case 4: reply_frame.Data[i/8 + 1] &= 0xef; break;
                        case 5: reply_frame.Data[i/8 + 1] &= 0xdf; break;
                        case 6: reply_frame.Data[i/8 + 1] &= 0xbf; break;
                        case 7: reply_frame.Data[i/8 + 1] &= 0x7f; break;
                    }
                }
            } 
        }
        else
        {
            reply_frame.Function = 0x82;  //��������     0x82
            reply_frame.Data[0]  = 0x02;  //�쳣��         0x02	  		
            reply_frame.Length   = 1;     //�������ֽڳ���  1
        }	
    }
    else
    {
        reply_frame.Function = 0x82;       //��������     0x82
        reply_frame.Data[0]  = 0x03;       //�쳣��         0x03	
        reply_frame.Length   = 1;          //�������ֽڳ���  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //������Ӧ֡ 
}
*/
//������03�������ּĴ���  
void MB_DRIVER_ReadHoldReg(MB_FRAME *pframe)
{
    //����֡      PDU
    //������  	  1 �ֽ� 	0x03
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ�������  2 �ֽ�  1 to 125 (0x7D)
	
    //��Ӧ֡      PDU
    //������ 	  1   �ֽ�  0x03
    //�ֽ���      1   �ֽ�  2*n
    //�Ĵ���ֵ    2*n �ֽ�  
    //n = �Ĵ���������	

    ////MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;
    uint16_t index;	
    uint16_t start_addr;
    uint16_t quantity;
    uint16_t reg_data; 
       
    p = (uint8_t *)&start_addr;       //���ּĴ����׵�ַ   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //���ּĴ�������    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //���ּĴ��������ڹ涨��Χ��        	
    if((quantity >= 1)&&(quantity <= MB_HR_AMOUNT))    
    {
    	//��ʼ��ַ��ȷ����ʼ��ַ+���ּĴ��������ڹ涨��Χ��
        if((start_addr < MB_HR_AMOUNT)&&((start_addr + quantity) <= MB_HR_AMOUNT))
        { 
            index=0;
            reply_frame.Function = 0x03;                   //������
            reply_frame.Data[index++] = quantity * 2;      //�ֽڼ���
            reply_frame.Length = reply_frame.Data[0] + 1;  //�������ֽڳ���
            p=(uint8_t *) & reg_data;
            for (i = 0; i < quantity; i++)
            {
                reg_data = MB_HoldReg[i + start_addr];     //���ּĴ�����������ֵ  
                reply_frame.Data[index++] = *(p+1);        //���ֽ�  
                reply_frame.Data[index++] = *p;            //���ֽ�  
            }
        }
        else
        {
            reply_frame.Function = 0x83;  //��������     0x83
            reply_frame.Data[0]  = 0x02;  //�쳣��         0x02	  		
            reply_frame.Length   = 1;     //�������ֽڳ���  1
        }	
    }
    else
    {
        reply_frame.Function = 0x83;       //��������     0x83
        reply_frame.Data[0]  = 0x03;       //�쳣��         0x03	
        reply_frame.Length   = 1;          //�������ֽڳ���  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //������Ӧ֡ 
}

//������04��������Ĵ���  
void MB_DRIVER_ReadInReg(MB_FRAME *pframe)
{
    //����֡          PDU
    //������  	      1 �ֽ� 	0x04
    //��ʼ��ַ 	      2 �ֽ�  0x0000 to 0xFFFF
    //����Ĵ�������  2 �ֽ�  1 to 125 (0x7D)
	
    //��Ӧ֡        PDU
    //������ 	    1   �ֽ�  0x04
    //�ֽ���        1   �ֽ�  2*n
    //����Ĵ���ֵ  2*n �ֽ�  
    //n = ����Ĵ���������	

    ////MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;
    uint16_t index;	
    uint16_t start_addr;
    uint16_t quantity;
    //uint16_t reg_data; 
       
    p = (uint8_t *)&start_addr;       //����Ĵ����׵�ַ   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //����Ĵ�������    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //����Ĵ��������ڹ涨��Χ��        	
    if((quantity >= 1)&&(quantity <= MB_IR_AMOUNT))    
    {
    	//��ʼ��ַ��ȷ����ʼ��ַ+����Ĵ��������ڹ涨��Χ��
        if((start_addr < MB_IR_AMOUNT)&&((start_addr + quantity) <= MB_IR_AMOUNT))
        { 
            index=0;
            reply_frame.Function = 0x04;                   //������
            reply_frame.Data[index++] = quantity * 2;      //�ֽڼ���
            reply_frame.Length = reply_frame.Data[0] + 1;  //�������ֽڳ���
            //p=(uint8_t *) & reg_data;
            for (i = 0; i < quantity; i++)
            {
                //reg_data = MB_InReg[i + start_addr];       //����Ĵ�����������ֵ  
                p=(uint8_t *)&MB_InReg[i + start_addr];
                reply_frame.Data[index++] = *(p+1);        //���ֽ�  
                reply_frame.Data[index++] = *p;            //���ֽ�  
            }
        }
        else
        {
            reply_frame.Function = 0x84;  //��������     0x84
            reply_frame.Data[0]  = 0x02;  //�쳣��         0x02	  		
            reply_frame.Length   = 1;     //�������ֽڳ���  1
        }	
    }
    else
    {
        reply_frame.Function = 0x84;       //��������     0x84
        reply_frame.Data[0]  = 0x03;       //�쳣��         0x03	
        reply_frame.Length   = 1;          //�������ֽڳ���  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //������Ӧ֡ 
}

/*
//������05��д������Ȧ
void MB_DRIVER_WriteSingleCoil(MB_FRAME *pframe)
{
    //����֡       PDU
    //������  	   1 �ֽ� 	0x05
    //�����ַ 	   2 �ֽ�  0x0000 to 0xFFFF
    //���ֵ       2 �ֽ�  0x0000 or 0xFF00
	
    //��Ӧ֡       PDU
    //������  	   1 �ֽ� 	0x05
    //�����ַ     2 �ֽ�  0x0000 to 0xFFFF
    //���ֵ       2 �ֽ�  0x0000 or 0xFF00  	
	

    ////MB_FRAME reply_frame;
    uint16_t target_addr;
    uint16_t write_value;
    uint8_t  *p;
	
    p = (uint8_t *)&target_addr;
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&write_value;
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
	
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //���ֵΪ0x0000 �� 0xFF00    
    if ((write_value == 0x0)||(write_value == 0xff00))
    {
    	//�����ַ��ȷ
        if(target_addr < MB_COILS_AMOUNT)
        {
            reply_frame.Function = 0x05;
            reply_frame.Length = 0x04;
            reply_frame.Data[0] = (*pframe).Data[0];
            reply_frame.Data[1] = (*pframe).Data[1];
            reply_frame.Data[2] = (*pframe).Data[2];
            reply_frame.Data[3] = (*pframe).Data[3];
            MB_DRIVER_SetCoils(target_addr, write_value);   //д������Ȧ��ֵ 
        }
        else
        {
            reply_frame.Function = 0x85;  //��������     0x85
            reply_frame.Data[0]  = 0x02;  //�쳣��         0x02	  		
            reply_frame.Length   = 1;     //�������ֽڳ���  1
        }	
    }
    else
    {
        reply_frame.Function = 0x85;       //��������     0x85
        reply_frame.Data[0]  = 0x03;       //�쳣��         0x03	
        reply_frame.Length   = 1;          //�������ֽڳ���  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //������Ӧ֡ 
}
*/
//������06��д�������ּĴ���  
void MB_DRIVER_WriteSingleHoldReg(MB_FRAME *pframe)
{
    //����֡       PDU
    //������  	   1 �ֽ� 	0x06
    //�Ĵ�����ַ   2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ���ֵ     2 �ֽ�  0x0000 or 0xFFFF
	
    //��Ӧ֡       PDU
    //������  	   1 �ֽ� 	0x06
    //�Ĵ�����ַ   2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ���ֵ     2 �ֽ�  0x0000 or 0xFFFF 

    ////MB_FRAME reply_frame;
    uint16_t target_addr;
    uint16_t write_value;
    uint8_t  *p;
	
    p = (uint8_t *)&target_addr;
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&write_value;
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
	
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //�Ĵ�����ַ��ȷ
    if(target_addr < MB_HR_AMOUNT)
    {
        reply_frame.Function = 0x06;
        reply_frame.Length = 0x04;
        reply_frame.Data[0] = (*pframe).Data[0];
        reply_frame.Data[1] = (*pframe).Data[1];
        reply_frame.Data[2] = (*pframe).Data[2];
        reply_frame.Data[3] = (*pframe).Data[3];
        MB_HoldReg[target_addr] =  write_value;   //д�������ּĴ�����ֵ 
    }
    else
    {
            reply_frame.Function = 0x86;  //��������     0x85
            reply_frame.Data[0]  = 0x02;  //�쳣��         0x02	  		
            reply_frame.Length   = 1;     //�������ֽڳ���  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //������Ӧ֡ 
}
/*
//������07�����쳣״̬  
void MB_DRIVER_ReadExStatus(MB_FRAME *pframe)
{
    //����֡        PDU
    //������  	    1   �ֽ�  0x07
	
    //��Ӧ֡        PDU
    //������ 	    1   �ֽ�  0x07
    //�������      1   �ֽ�  0x00~0xff

    ////MB_FRAME reply_frame;
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    reply_frame.Function = 0x07;              //������
    reply_frame.Data[0] = MB_ExStatus;        //�������   
    reply_frame.Length = 1;                   //�������ֽڳ���  1
    MB_DRIVER_SendData(&reply_frame);                //������Ӧ֡ 
}

//������08�����
void MB_DRIVER_ReadDiagnose(MB_FRAME *pframe)
{
    //����֡        PDU
    //������  	    1   �ֽ�  0x08
    //�ӹ�����	    2   �ֽ�  ����
    //����         2*N  �ֽ�  ����
	
    //��Ӧ֡        PDU
    //������  	    1   �ֽ�  0x08
    //�ӹ�����	    2   �ֽ�  ����
    //����         2*N  �ֽ�  ����
    
    
    uint8_t *p;
    uint16_t fun_code;       
    p = (uint8_t*)&fun_code;         //�ӹ�����
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    switch(fun_code)
    {
        case 00: MB_DRIVER_BackData(pframe);break;  //�ӹ���00������ѯ������
        case 01: break;	
        case 02: break;
        case 03: break;
        case 04: break;
        case 05: break;                 	        	
    	default:  break;    
    }
	
} 
//��ϣ��ӹ���0x0000������ѯ������
void MB_DRIVER_BackData(MB_FRAME *pframe)
{
    //����֡        PDU
    //������  	    1   �ֽ�  0x08
    //�ӹ�����	    2   �ֽ�  0x0000
    //����         2*N  �ֽ�   ����
	
    //��Ӧ֡        PDU
    //������  	    1   �ֽ�  0x08
    //�ӹ�����	    2   �ֽ�  0x0000
    //����         2*N  �ֽ�   ����	
    uint8_t i;
    //MB_FRAME reply_frame;
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    reply_frame.Function = 0x08;                 //������
    reply_frame.Length = (*pframe).Length;       //�������ֽڳ���
    for(i = 0; i < reply_frame.Length; i++)
    {
    	reply_frame.Data[i] = (*pframe).Data[i]; //�ӹ����������
    }	
    MB_DRIVER_SendData(&reply_frame);                   //������Ӧ֡
}

//������15��д�����Ȧ     
void MB_DRIVER_WriteMoreCoils(MB_FRAME *pframe)
{
    //����֡      PDU
    //������  	  1 �ֽ� 	0x0F
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�������    2 �ֽ�  0x0001 to 0x07B0
    //�ֽڼ���    1 �ֽ�         n
    //���ֵ      n �ֽ�         
    
    //N = ������� / 8, �������������0, n = N+1;���� n = N
	
    //��Ӧ֡      PDU
    //������ 	  1 �ֽ�        0x0F
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�������    2 �ֽ�  0x0001 to 0x07B0
    

    //MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;
    uint16_t start_addr;    //��ʼ��ַ
    uint16_t quantity;      //��Ȧ����
    uint8_t  byte_num;      //�ֽڼ���
    uint8_t  length;          
    uint8_t  write_data;
    uint8_t  temp1, temp2, temp3;
       
    p = (uint8_t *)&start_addr;       //��Ȧ�׵�ַ   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //�������    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
    byte_num = (*pframe).Data[4];   //�ֽڼ���
		
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    if ((quantity % 8) == 0) length = quantity/8;     //�����ֽ��� 
    else                     length = quantity/8 + 1; //�����ֽ���    
    //��������ڹ涨��Χ�ڣ��ֽڼ�����ȷ        	
    if((quantity >= 1)&&(quantity <= MB_COILS_AMOUNT)&&(byte_num == length))    
    {
    	//��ʼ��ַ��ȷ����ʼ��ַ+��������ڹ涨��Χ��
        if((start_addr < MB_COILS_AMOUNT)&&((start_addr + quantity) <= MB_COILS_AMOUNT))
        { 
            reply_frame.Function = 0x0F;                  //������
            for(i = 0; i < 4; i++)
            {
                reply_frame.Data[i] = (*pframe).Data[i];  //��ʼ��ַ���������
            }
            reply_frame.Length = 4;                       //�������ֽڳ���
            for (i = 0; i < quantity; i++)                //д�����Ȧ��ֵ 
            {
            	temp1 = i / 8;
            	temp2 = i % 8;
            	temp3 = 0x01 << temp2;
            	write_data = (*pframe).Data[temp1 + 5] & temp3;
                MB_DRIVER_SetCoils(start_addr + i, write_data);        //д������Ȧ��ֵ             
            }
        }
        else
        {
            reply_frame.Function = 0x8F;  //��������     0x8F
            reply_frame.Data[0]  = 0x02;  //�쳣��         0x02	  		
            reply_frame.Length   = 1;     //�������ֽڳ���  1
        }	
    }
    else
    {
        reply_frame.Function = 0x8F;       //��������     0x8F
        reply_frame.Data[0]  = 0x03;       //�쳣��         0x03	
        reply_frame.Length   = 1;          //�������ֽڳ���  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //������Ӧ֡ 
}


//������16��д����Ĵ���     �����ּĴ�����
void MB_DRIVER_WriteMoreReg(MB_FRAME *pframe)
{
    //����֡      PDU
    //������  	  1 �ֽ� 	0x10
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ�������  2 �ֽ�  0x0001 to 0x007B
    //�ֽڼ���    1 �ֽ�       2*N
    //�Ĵ���ֵ    2*N�ֽ�      ֵ
    //N = �Ĵ�������
	
    //��Ӧ֡      PDU
    //������ 	  1 �ֽ�  0x10
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ�������  2 �ֽ�  1~123(0x007B)
    

    MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;
    uint16_t start_addr;    //��ʼ��ַ
    uint16_t quantity;      //�Ĵ�������
    uint8_t  byte_num;      //�ֽڼ���
       
    p = (uint8_t *)&start_addr;       //�Ĵ����׵�ַ   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //�Ĵ�������    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
    byte_num = (*pframe).Data[4];   //�ֽڼ���
		
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    
    //�Ĵ��������ڹ涨��Χ�ڣ��ֽڼ�����ȷ        	
    if((quantity >= 1)&&(quantity <= MB_HR_AMOUNT)&&(byte_num == quantity*2))    
    {
    	//��ʼ��ַ��ȷ����ʼ��ַ+�Ĵ��������ڹ涨��Χ��
        if((start_addr < MB_HR_AMOUNT)&&((start_addr + quantity) <= MB_HR_AMOUNT))
        { 
            reply_frame.Function = 0x10;                  //������
            for(i = 0; i < 4; i++)
            {
                reply_frame.Data[i] = (*pframe).Data[i];  //��ʼ��ַ�ͼĴ�������
            }
            reply_frame.Length = 4;                       //�������ֽڳ���
            for (i = 0; i < quantity; i++)                //д����Ĵ�����ֵ 
            {
                p = (uint8_t *)&MB_HoldReg[i + start_addr];            
                *(p + 1) = (*pframe).Data[2*i + 5];
                *p = (*pframe).Data[2*i + 6]; 
            }
        }
        else
        {
            reply_frame.Function = 0x90;  //��������     0x90
            reply_frame.Data[0]  = 0x02;  //�쳣��         0x02	  		
            reply_frame.Length   = 1;     //�������ֽڳ���  1
        }	
    }
    else
    {
        reply_frame.Function = 0x90;       //��������     0x90
        reply_frame.Data[0]  = 0x03;       //�쳣��         0x03	
        reply_frame.Length   = 1;          //�������ֽڳ���  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //������Ӧ֡ 
}

uint8_t MB_DRIVER_GetDisOrCoils(uint8_t coilordis[], uint16_t address)
{
    uint16_t offset;
    uint8_t  b,c;
    offset = address / 8;
    b = coilordis[offset];
    offset = address % 8;
    c = 0x01;
    c <<= offset;
    if ((b&c) == 0)   return false;
    else              return true;
}

void MB_DRIVER_SetCoils(uint16_t address,uint16_t data){
    
    if (data){
        //��1
        switch(address % 8){
            case 0: MB_Coils[address / 8] |= 0x01; break;
            case 1: MB_Coils[address / 8] |= 0x02; break;
            case 2: MB_Coils[address / 8] |= 0x04; break;
            case 3: MB_Coils[address / 8] |= 0x08; break;
            case 4: MB_Coils[address / 8] |= 0x10; break;
            case 5: MB_Coils[address / 8] |= 0x20; break;
            case 6: MB_Coils[address / 8] |= 0x40; break;
            case 7: MB_Coils[address / 8] |= 0x80; break;
            default: break;
        }
    } else {
        //��0
        switch(address%8){
            case 0: MB_Coils[address / 8] &= 0xfe; break;
            case 1: MB_Coils[address / 8] &= 0xfd; break;
            case 2: MB_Coils[address / 8] &= 0xfb; break;
            case 3: MB_Coils[address / 8] &= 0xf7; break;
            case 4: MB_Coils[address / 8] &= 0xef; break;
            case 5: MB_Coils[address / 8] &= 0xdf; break;
            case 6: MB_Coils[address / 8] &= 0xbf; break;
            case 7: MB_Coils[address / 8] &= 0x7f; break;
            default: break;
        }
    }
}
*/
#endif
/*
//���ص��ֽ�
uint8_t MB_DRIVER_GetLow(uint16_t data)
{
	uint8_t tmep;
	tmep = data;
	return(tmep);
}
//���ظ��ֽ�
uint8_t MB_DRIVER_GetHigh(uint16_t data)
{
	uint8_t tmep;
	tmep = (data >> 8);
	return(tmep);
}

//�ͻ���(����) 
#ifdef _ModClient
//������01������Ȧ  ����λ�����

uint8_t MB_ReadCoils(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity,uint8_t *ResData)
{
    uint16_t repeat_count;
    uint8_t i;
    MB_FRAME frame;
    frame.ComPort  = port_id;           //�˿ں�
    frame.Address  = slave_addr;        //���豸��ַ
    frame.Function = 0x01;              //���ܺ�
    frame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil��ʼ��ַ
    frame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    frame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil��λ����
    frame.Data[3]  = MB_DRIVER_GetLow(quantity);
    frame.Length   = 4;			//data�����ĸ��ֽ�
	
    repeat_count = 0 ; 
	while(repeat_count < MB_MAX_REPEAT)
	{
		MB_DRIVER_SendData(&frame);			//��������֡
		if (port_id == UART_COM0)
		{
			//������Ӧ��ʱ��ʱ��
			//vtimer_Reset(MB_COM0_RepeatTimerID);
			//MB_COM0_TimeOut=false;
			//while((MB_COM0_TimeOut==false)&&(MB_COM0_ResDeal==false));
			if(MB_COM0_ResDeal==true)
			{
				MB_COM0_ResDeal=false;
				if(MB_DLL_CRCCheck(&MB_COM0_Frame)==true)
				{
					if (MB_COM0_Frame.ucFunction!=0x01) return false;
					if (ResData==NULL) return true;
					for (i=0;i<MB_COM0_Frame.nLength;i++)
					{
						ResData[i]=MB_COM0_Frame.ucData[i];
					}
					return true;
				}
			}		
			repeat_count++;
		}
		if (port_id == UART_COM1)
		{
			//������Ӧ��ʱ��ʱ��
			//vtimer_Reset(MB_COM1_RepeatTimerID);	
			MB_COM1_TimeOut=false;
			while((MB_COM1_TimeOut==false)&&(MB_COM1_HasComm==false));
			if(MB_COM1_HasComm==true)
			{
				MB_COM1_HasComm=false;
				if(MB_DLL_CRCCheck(&MB_COM1_Frame)==true)
				{
					if (MB_COM1_Frame.ucFunction!=0x01) return false;
					if (ResData==NULL) return true;
					for (i=0;i<MB_COM1_Frame.nLength;i++)
					{
						ResData[i]=MB_COM1_Frame.ucData[i];
					}
					return true;
				}
			}		
			repeat_count++;
		}
	}
	return false;
}

//��������֡��������01������Ȧ  ����λ�����
void MB_SEND_ReadCoils(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //����֡    PDU
    //������  	1 �ֽ� 	0x01
    //��ʼ��ַ 	2 �ֽ�  0x0000 to 0xFFFF
    //��Ȧ���� 	2 �ֽ�  1 to 2000 (0x7D0)
	
    //��Ӧ֡    PDU
    //������ 	1 �ֽ� 	0x01
    //�ֽڼ���  1 �ֽ� 	n
    //��Ȧ״̬  n �ֽ� 	n = N or N+1
    //N = ������� / 8, �������������0, n = N+1;���� n = N
	
    //uint8_t i;
 //   MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //�˿ں�
    SendFrame.Address  = slave_addr;        //���豸��ַ
    SendFrame.Function = 0x01;              //���ܺ�
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil��ʼ��ַ
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil��λ����
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Length   = 4;			//data�����ĸ��ֽ�
	
    MB_DRIVER_SendData(&SendFrame);         //��������
}
//��������֡��������02������ɢ�����루��λ���룩
void MB_SEND_ReadDisIn(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //����֡    PDU
    //������  	1 �ֽ� 	0x02
    //��ʼ��ַ 	2 �ֽ�  0x0000 to 0xFFFF
    //�������� 	2 �ֽ�  1 to 2000 (0x7D0)
	
    //��Ӧ֡    PDU
    //������ 	1 �ֽ� 	0x02
    //�ֽڼ���  1 �ֽ� 	n
    //����״̬  n �ֽ� 	n = N or N+1
    //N = �������� / 8, �������������0, n = N+1;���� n = N
	
    //uint8_t i;
 //   MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //�˿ں�
    SendFrame.Address  = slave_addr;        //���豸��ַ
    SendFrame.Function = 0x02;              //���ܺ�
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil��ʼ��ַ
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil��λ����
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Length   = 4;			//data�����ĸ��ֽ�
	
    MB_DRIVER_SendData(&SendFrame);         //��������
}
//��������֡��������03�������ּĴ��� 
void MB_SEND_ReadHoldReg(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //����֡      PDU
    //������  	  1 �ֽ� 	0x03
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ�������  2 �ֽ�  1 to 125 (0x7D)
	
    //��Ӧ֡      PDU
    //������ 	  1   �ֽ�  0x03
    //�ֽ���      1   �ֽ�  2*n
    //�Ĵ���ֵ    2*n �ֽ�  
    //n = �Ĵ���������		
    
 //   MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //�˿ں�
    SendFrame.Address  = slave_addr;        //���豸��ַ
    SendFrame.Function = 0x03;              //���ܺ�
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil��ʼ��ַ
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil��λ����
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Length   = 4;			//data�����ĸ��ֽ�
	
    MB_DRIVER_SendData(&SendFrame);         //��������
}
//��������֡��������04��������Ĵ��� 
void MB_SEND_ReadInReg(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //����֡          PDU
    //������  	      1 �ֽ� 	0x04
    //��ʼ��ַ 	      2 �ֽ�  0x0000 to 0xFFFF
    //����Ĵ�������  2 �ֽ�  1 to 125 (0x7D)
	
    //��Ӧ֡        PDU
    //������ 	    1   �ֽ�  0x04
    //�ֽ���        1   �ֽ�  2*n
    //����Ĵ���ֵ  2*n �ֽ�  
    //n = ����Ĵ���������	
    
  //  MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //�˿ں�
    SendFrame.Address  = slave_addr;        //���豸��ַ
    SendFrame.Function = 0x04;              //���ܺ�
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil��ʼ��ַ
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil��λ����
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Length   = 4;	                //data�����ĸ��ֽ�
	
    MB_DRIVER_SendData(&SendFrame);         //��������
}
//��������֡��������05��д������Ȧ ��д����λ�����
void MB_SEND_WriteSingleCoil(uint8_t port_id,uint8_t slave_addr,uint16_t output_addr,uint16_t output_value)
{
    //����֡       PDU
    //������  	   1 �ֽ� 	0x05
    //�����ַ 	   2 �ֽ�  0x0000 to 0xFFFF
    //���ֵ       2 �ֽ�  0x0000 or 0xFF00
	
    //��Ӧ֡       PDU
    //������  	   1 �ֽ� 	0x05
    //�����ַ     2 �ֽ�  0x0000 to 0xFFFF
    //���ֵ       2 �ֽ�  0x0000 or 0xFF00	
    
  //  MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //�˿ں�
    SendFrame.Address  = slave_addr;        //���豸��ַ
    SendFrame.Function = 0x05;              //���ܺ�
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(output_addr);//coil�����ַ
    SendFrame.Data[1]  = MB_DRIVER_GetLow(output_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(output_value);  //coil���ֵ
    SendFrame.Data[3]  = MB_DRIVER_GetLow(output_value);
    SendFrame.Length   = 4;			//data�����ĸ��ֽ�
	
    MB_DRIVER_SendData(&SendFrame);         //��������
}
//��������֡��������06��д�������ּĴ���
void MB_SEND_WriteSingleHoldReg(uint8_t port_id,uint8_t slave_addr,uint16_t output_addr,uint16_t output_value)
{
    //����֡       PDU
    //������  	   1 �ֽ� 	0x06
    //�Ĵ�����ַ   2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ���ֵ     2 �ֽ�  0x0000 or 0xFFFF
	
    //��Ӧ֡       PDU
    //������  	   1 �ֽ� 	0x06
    //�Ĵ�����ַ   2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ���ֵ     2 �ֽ�  0x0000 or 0xFFFF 	
    
 //   MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //�˿ں�
    SendFrame.Address  = slave_addr;        //���豸��ַ
    SendFrame.Function = 0x06;              //���ܺ�
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(output_addr);//HoldReg�����ַ
    SendFrame.Data[1]  = MB_DRIVER_GetLow(output_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(output_value);//HoldReg���ֵ
    SendFrame.Data[3]  = MB_DRIVER_GetLow(output_value);
    SendFrame.Length   = 4;			//data�����ĸ��ֽ�
	
    MB_DRIVER_SendData(&SendFrame);         //��������
}

//��������֡��������15��д�����Ȧ
void MB_SEND_WriteMoreCoils(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //����֡      PDU
    //������  	  1 �ֽ� 	0x0F
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�������    2 �ֽ�  0x0001 to 0x07B0
    //�ֽڼ���    1 �ֽ�         n
    //���ֵ      n �ֽ�         
    
    //N = ������� / 8, �������������0, n = N+1;���� n = N
	
    //��Ӧ֡      PDU
    //������ 	  1 �ֽ�        0x0F
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�������    2 �ֽ�  0x0001 to 0x07B0 
    uint8_t i;
    uint8_t byte_num;      //�ֽڼ���
  //  MB_FRAME SendFrame;
    //������Ϊ��
    if(quantity % 8)  byte_num = quantity / 8 + 1;
    else              byte_num = quantity / 8;
    SendFrame.ComPort  = port_id;           //�˿ں�
    SendFrame.Address  = slave_addr;        //���豸��ַ
    SendFrame.Function = 0x0f;              //���ܺ�
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil��ʼ��ַ
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil��λ����
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Data[4]  = byte_num;
    for (i = 0; i < quantity; i++)       //��Ȧ״̬
    {
        if (MB_DRIVER_GetDisIn(start_addr + i)) //��Ȧ״̬Ϊ1
        {    
            switch(i % 8)                //һ���ֽ��е�ĳһλ
            {
                case 0: SendFrame.Data[i/8 + 5] |= 0x01; break;
                case 1: SendFrame.Data[i/8 + 5] |= 0x02; break;
                case 2: SendFrame.Data[i/8 + 5] |= 0x04; break;
                case 3: SendFrame.Data[i/8 + 5] |= 0x08; break;
                case 4: SendFrame.Data[i/8 + 5] |= 0x10; break;
                case 5: SendFrame.Data[i/8 + 5] |= 0x20; break;
                case 6: SendFrame.Data[i/8 + 5] |= 0x40; break;
                case 7: SendFrame.Data[i/8 + 5] |= 0x80; break;
            }
        }
        else                //��Ȧ״̬Ϊ0
        {
            switch(i % 8)   //һ���ֽ��е�ĳһλ
            {
                case 0: SendFrame.Data[i/8 + 5] &= ~0x01; break;
                case 1: SendFrame.Data[i/8 + 5] &= ~0x02; break;
                case 2: SendFrame.Data[i/8 + 5] &= ~0x04; break;
                case 3: SendFrame.Data[i/8 + 5] &= ~0x08; break;
                case 4: SendFrame.Data[i/8 + 5] &= ~0x10; break;
                case 5: SendFrame.Data[i/8 + 5] &= ~0x20; break;
                case 6: SendFrame.Data[i/8 + 5] &= ~0x40; break;
                case 7: SendFrame.Data[i/8 + 5] &= ~0x80; break;
            }
        }
    }
    SendFrame.Length   = 5 + byte_num;	//data���ֽ���
    MB_DRIVER_SendData(&SendFrame);         //��������
}
//��������֡��������16��д����Ĵ��������ּĴ�����
void MB_SEND_WriteMoreReg(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //����֡      PDU
    //������  	  1 �ֽ� 	0x10
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ�������  2 �ֽ�  0x0001 to 0x007B
    //�ֽڼ���    1 �ֽ�       2*N
    //�Ĵ���ֵ    2*N�ֽ�      ֵ
    //N = �Ĵ�������
	
    //��Ӧ֡      PDU
    //������ 	  1 �ֽ�  0x10
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ�������  2 �ֽ�  1~123(0x007B) 
    uint8_t i;
    uint8_t byte_num;      //�ֽڼ���
    uint8_t *p;
   // MB_FRAME SendFrame;
    //������Ϊ��
    byte_num = quantity * 2;
    SendFrame.ComPort  = port_id;           //�˿ں�
    SendFrame.Address  = slave_addr;        //���豸��ַ
    SendFrame.Function = 0x10;              //���ܺ�
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//��ʼ��ַ
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //�Ĵ�������
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Data[4]  = byte_num;
    for (i = 0; i < quantity; i++)       //�Ĵ���ֵ
    {
        p = (uint8_t*)& MB_HoldReg[i + start_addr];
        SendFrame.Data[2*i + 5] = *(p + 1);
        SendFrame.Data[2*i + 6] = *p;
    }
    SendFrame.Length   = 5 + byte_num;	//data���ֽ���
    MB_DRIVER_SendData(&SendFrame);         //��������
}


//������Ӧ֡��������03�������ּĴ���  
uint8_t MB_BACK_ReadHoldReg(MB_FRAME *pframe)
{
    //����֡      PDU
    //������  	  1 �ֽ� 	0x03
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ�������  2 �ֽ�  1 to 125 (0x7D)
	
    //��Ӧ֡      PDU
    //������ 	  1   �ֽ�  0x03
    //�ֽ���      1   �ֽ�  2*n
    //�Ĵ���ֵ    2*n �ֽ�  
    //n = �Ĵ���������	

    uint8_t  *p;
    uint16_t i;	
    uint16_t start_addr;
    uint16_t quantity;
    uint16_t reg_num; 
    
    p = (uint8_t *)&start_addr;       //���͵ı��ּĴ����׵�ַ   
    *p = SendFrame.Data[1];
    *(p + 1) = SendFrame.Data[0];
    p = (uint8_t *)&quantity;         //���͵ı��ּĴ�������    
    *p = SendFrame.Data[3];
    *(p + 1) = SendFrame.Data[2];        
     
    reg_num = (*pframe).Data[0]/2; //���յı��ּĴ�������       	
    if(quantity == reg_num)        //���յı��ּĴ���������ȷ
    {
        for (i = 0; i < reg_num; i++)
        {
            p = (uint8_t *)&MB_HoldReg[i + start_addr];//���ּĴ�����������ֵ  
            *(p + 1) = (*pframe).Data[2*i + 1];    //���ֽ�  
            *p = (*pframe).Data[2*i + 2];          //���ֽ�  
        }
        return true;
    }
    else
    {
	return false;
    }
}

//������Ӧ֡��������04��������Ĵ���  
uint8_t MB_BACK_ReadInReg(MB_FRAME *pframe)
{
    //����֡          PDU
    //������  	      1 �ֽ� 	0x04
    //��ʼ��ַ 	      2 �ֽ�  0x0000 to 0xFFFF
    //����Ĵ�������  2 �ֽ�  1 to 125 (0x7D)
	
    //��Ӧ֡        PDU
    //������ 	    1   �ֽ�  0x04
    //�ֽ���        1   �ֽ�  2*n
    //����Ĵ���ֵ  2*n �ֽ�  
    //n = ����Ĵ���������	

    uint8_t  *p;
    uint16_t i;	
    uint16_t start_addr;
    uint16_t quantity;
    uint16_t reg_num; 
    
    p = (uint8_t *)&start_addr;       //���͵�����Ĵ����׵�ַ   
    *p = SendFrame.Data[1];
    *(p + 1) = SendFrame.Data[0];
    p = (uint8_t *)&quantity;         //���͵�����Ĵ�������    
    *p = SendFrame.Data[3];
    *(p + 1) = SendFrame.Data[2];        
     
    reg_num = (*pframe).Data[0]/2; //���յ�����Ĵ�������       	
    if(quantity == reg_num)        //���յ�����Ĵ���������ȷ
    {
        for (i = 0; i < reg_num; i++)
        {
            p = (uint8_t *)&MB_InReg[i + start_addr];//����Ĵ�����������ֵ  
            *(p + 1) = (*pframe).Data[2*i + 1];    //���ֽ�  
            *p = (*pframe).Data[2*i + 2];          //���ֽ�  
        }
        return true;
    }
    else
    {
	return false;
    }
}
//������Ӧ֡��������06��д�������ּĴ���  
uint8_t MB_BACK_WriteSingleHoldReg(MB_FRAME *pframe)
{
    //����֡       PDU
    //������  	   1 �ֽ� 	0x06
    //�Ĵ�����ַ   2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ���ֵ     2 �ֽ�  0x0000 or 0xFFFF
	
    //��Ӧ֡       PDU
    //������  	   1 �ֽ� 	0x06
    //�Ĵ�����ַ   2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ���ֵ     2 �ֽ�  0x0000 or 0xFFFF 
    uint8_t  *p;
    uint16_t sen_addr;
    uint16_t receive_addr;
    uint16_t sen_val;
    uint16_t receive_val;
        
    p = (uint8_t *)&sen_addr;        //���͵ı��ּĴ�����ַ   
    *p = SendFrame.Data[1];
    *(p + 1) = SendFrame.Data[0];
    p = (uint8_t *)&sen_val;         //���͵ı��ּĴ���ֵ    
    *p = SendFrame.Data[3];
    *(p + 1) = SendFrame.Data[2];  
    
    p = (uint8_t *)&receive_addr;    //���յı��ּĴ�����ַ   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&receive_val;     //���յı��ּĴ���ֵ    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];           
    //��ַ����ֵ����ȷ       	
    if((sen_addr == receive_addr)&&(sen_val == receive_val))   
    {
        return true;
    }
    else
    {
	return false;
    }
}
//������Ӧ֡��������16��д����Ĵ���     �����ּĴ�����
uint8_t MB_BACK_WriteMoreReg(MB_FRAME *pframe)
{
    //����֡      PDU
    //������  	  1 �ֽ� 	0x10
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ�������  2 �ֽ�  0x0001 to 0x007B
    //�ֽڼ���    1 �ֽ�       2*N
    //�Ĵ���ֵ    2*N�ֽ�      ֵ
    //N = �Ĵ�������
	
    //��Ӧ֡      PDU
    //������ 	  1 �ֽ�  0x10
    //��ʼ��ַ 	  2 �ֽ�  0x0000 to 0xFFFF
    //�Ĵ�������  2 �ֽ�  1~123(0x007B)
    
    uint8_t  *p;
    uint16_t sen_addr;
    uint16_t receive_addr;
    uint16_t sen_quantity;
    uint16_t receive_quantity;
        
    p = (uint8_t *)&sen_addr;        //���͵ı��ּĴ�����ַ   
    *p = SendFrame.Data[1];
    *(p + 1) = SendFrame.Data[0];
    p = (uint8_t *)&sen_quantity;         //���͵ı��ּĴ�������    
    *p = SendFrame.Data[3];
    *(p + 1) = SendFrame.Data[2];  
    
    p = (uint8_t *)&receive_addr;    //���յı��ּĴ�����ַ   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&receive_quantity; //���յı��ּĴ�������    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];           
    //��ַ����������ȷ       	
    if((sen_addr == receive_addr)&&(sen_quantity == receive_quantity))   
    {
        return true;
    }
    else
    {
	return false;
    }
}      
 
#endif
*/
