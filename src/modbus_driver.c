#include "stm8s.h"
#include "modbus_driver.h"
#include "crc.h"
#include "rs485.h"

uint8_t buffer[MB_BUFFER_SIZE];
MB_FRAME reply_frame;

uint8_t  MB_DisIn[MB_DI_AMOUNT / 8 + 1];     //离散量输入，位，只读
uint8_t  MB_Coils[MB_COILS_AMOUNT / 8 + 1];  //线圈，      位，读写
uint16_t MB_InReg[MB_IR_AMOUNT];             //输入寄存器，字，只读
uint16_t MB_HoldReg[MB_HR_AMOUNT];           //保持寄存器，字，读写
extern uint8_t mb_local_address;

/*
//将ASCII码值转换成16进制数值
//pbuffer为缓冲区指针，大小为MB_BUFFER_SIZE
//length为缓冲区内数据长度
//16进制0~9对应ASCII码值30~39，16进制A~F(10~15)对应ASCII码值41~46
void MB_DRIVER_AsciiToHex(uint8_t * pbuffer, uint8_t length)
{
    uint8_t i, j;
    uint8_t temp_hi;
    uint8_t temp_low;
    j = 0;
    for(i = 1; i < length - 3; i++)
    {
        if(pbuffer[i] < 40)  temp_hi = pbuffer[i] - 30;  //高4位值为0~9
        else                 temp_hi = pbuffer[i] - 31;  //高4位值为A~F(10~15)
        i++;
        if(pbuffer[i] < 40)  temp_low = pbuffer[i] - 30; //低4位值为0~9
        else                 temp_low = pbuffer[i] - 31; //低4位值为A~F(10~15)
        pbuffer[j++] = temp_hi * 16 +  temp_low;         //转换成16进制数值
    }	
}
//将16进制数值转换成ASCII码值 
//pbuffer为缓冲区指针，大小为MB_BUFFER_SIZE
//length为缓冲区内数据长度
//16进制0~9对应ASCII码值30~39，16进制A~F(10~15)对应ASCII码值41~46
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
    pbuffer[j++] = 0x3A;                                 //添加起始字符“冒号”
    for(i = 0; i < length; i++)
    {
    	temp_hi = buffer[i] / 16 ;                       //取高4位值
        temp_low = buffer[i] % 16 ;                      //取低4位值
        if(temp_hi < 10)   pbuffer[j++] = temp_hi + 30;  //高4位值为0~9
        else               pbuffer[j++] = temp_hi + 31;  //高4位值为A~F(10~15)
        if(temp_low < 10)  pbuffer[j++] = temp_low + 30; //低4位值为0~9
        else               pbuffer[j++] = temp_low + 31; //低4位值为A~F(10~15)      	
    }
    pbuffer[j++] = 0x0D;                                 //添加结束字符“回车”
    pbuffer[j++] = 0x0A;                                 //添加结束字符“换行”
}*/

//发送数据  
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
    length = index;                                 //需要校验的字节总长度
    crc_result = CRC16(buffer, length);             //计算校验字
    buffer[index++] = *((uint8_t*)&(crc_result) + 1); //校验高字节 
    buffer[index++] = *((uint8_t*)&crc_result);       //校验低字节  
    length = index;                                 //需要发送的字节总长度

#ifdef  _MB_ASCII                                   //如果为ASCII模式
    MB_DRIVER_HexToAscii(buffer, length);              //将16进制数值转换成ASCII码值
    length = length*2 + 3;
#endif        

	rs485_send(buffer,length);
		
}
//服务器(从机) 
#ifdef _ModServer 
/*
//功能码01：读线圈  （读位输出）
void MB_DRIVER_ReadCoils(MB_FRAME *pframe)   
{
    //请求帧    PDU
    //功能码  	1 字节 	0x01
    //起始地址 	2 字节  0x0000 to 0xFFFF
    //线圈数量 	2 字节  1 to 2000 (0x7D0)
	
    //响应帧    PDU
    //功能码 	1 字节 	0x01
    //字节计数  1 字节 	n
    //线圈状态  n 字节 	n = N or N+1
    //N = 输出数量 / 8, 如果余数不等于0, n = N+1;否则 n = N
    
    uint8_t  *p;
    uint16_t i;	
    uint16_t start_addr;
    uint16_t quantity;
	
    p = (uint8_t *)&start_addr;       //线圈首地址   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //线圈数量    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //输出数量在规定范围内        	
    if((quantity >= 1)&&(quantity <= MB_COILS_AMOUNT))    
    {
    	//起始地址正确，起始地址+输出数量在规定范围内
        if((start_addr < MB_COILS_AMOUNT)&&((start_addr + quantity) <= MB_COILS_AMOUNT))
        {   
            reply_frame.Function = 0x01;
            if ((quantity % 8) == 0) reply_frame.Length = quantity/8;    //字节计数 不发送
            else                     reply_frame.Length = quantity/8 + 1;//字节计数 不发送
            reply_frame.Data[0] = reply_frame.Length++;      //响应帧字节计数 发送	
            for (i = 1; i < reply_frame.Length + 1; i++) 
            {
            	reply_frame.Data[i] = 0x0;       //线圈状态清0		
            }
            for (i = 0; i < quantity; i++)       //读线圈状态
            {
                if (MB_DRIVER_GetDisOrCoils(MB_Coils,start_addr + i)) //线圈状态为1
                {
                    switch(i % 8)                //一个字节中的某一位
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
                else                //线圈状态为0
                {
                    switch(i % 8)   //一个字节中的某一位
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
            reply_frame.Function = 0x81;  //错误功能码     0x81
            reply_frame.Data[0]  = 0x02;  //异常码         0x02	  		
            reply_frame.Length   = 1;     //数据区字节长度  1
        }	
    }
    else
    {
        reply_frame.Function = 0x81;       //错误功能码     0x81
        reply_frame.Data[0]  = 0x03;       //异常码         0x03	
        reply_frame.Length   = 1;          //数据区字节长度  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //发送响应帧 
}



//功能码02：读离散量输入（读位输入）  
void MB_DRIVER_ReadDisIn(MB_FRAME *pframe)
{
    //请求帧    PDU
    //功能码  	1 字节 	0x02
    //起始地址 	2 字节  0x0000 to 0xFFFF
    //输入数量 	2 字节  1 to 2000 (0x7D0)
	
    //响应帧    PDU
    //功能码 	1 字节 	0x02
    //字节计数  1 字节 	n
    //输入状态  n 字节 	n = N or N+1
    //N = 输入数量 / 8, 如果余数不等于0, n = N+1;否则 n = N
    ////MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;	
    uint16_t start_addr;
    uint16_t quantity;
	
    p = (uint8_t *)&start_addr;       //离散量输入首地址   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //离散量输入数量    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //输入数量在规定范围内        	
    if((quantity >= 1)&&(quantity <= MB_DI_AMOUNT))    
    {
    	//起始地址正确，起始地址+输入数量在规定范围内
        if((start_addr < MB_DI_AMOUNT)&&((start_addr + quantity) <= MB_DI_AMOUNT))
        {   
            reply_frame.Function = 0x02;
            if ((quantity % 8) == 0) reply_frame.Length = quantity/8;    //字节计数 不发送
            else                     reply_frame.Length = quantity/8 + 1;//字节计数 不发送
            reply_frame.Data[0] = reply_frame.Length++;      //响应帧字节计数 发送	
            for (i = 1; i < reply_frame.Length + 1; i++) 
            {
            	reply_frame.Data[i] = 0x0;       //离散量输入状态清0		
            }
            for (i = 0; i < quantity; i++)       //读离散量输入状态
            {
                if (MB_DRIVER_GetDisOrCoils(MB_DisIn,start_addr + i)) //离散量输入状态为1
                {
                    switch(i % 8)                //一个字节中的某一位
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
                else                //离散量输入状态为0
                {
                    switch(i % 8)   //一个字节中的某一位
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
            reply_frame.Function = 0x82;  //错误功能码     0x82
            reply_frame.Data[0]  = 0x02;  //异常码         0x02	  		
            reply_frame.Length   = 1;     //数据区字节长度  1
        }	
    }
    else
    {
        reply_frame.Function = 0x82;       //错误功能码     0x82
        reply_frame.Data[0]  = 0x03;       //异常码         0x03	
        reply_frame.Length   = 1;          //数据区字节长度  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //发送响应帧 
}
*/
//功能码03：读保持寄存器  
void MB_DRIVER_ReadHoldReg(MB_FRAME *pframe)
{
    //请求帧      PDU
    //功能码  	  1 字节 	0x03
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //寄存器数量  2 字节  1 to 125 (0x7D)
	
    //响应帧      PDU
    //功能码 	  1   字节  0x03
    //字节数      1   字节  2*n
    //寄存器值    2*n 字节  
    //n = 寄存器的数量	

    ////MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;
    uint16_t index;	
    uint16_t start_addr;
    uint16_t quantity;
    uint16_t reg_data; 
       
    p = (uint8_t *)&start_addr;       //保持寄存器首地址   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //保持寄存器数量    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //保持寄存器数量在规定范围内        	
    if((quantity >= 1)&&(quantity <= MB_HR_AMOUNT))    
    {
    	//起始地址正确，起始地址+保持寄存器数量在规定范围内
        if((start_addr < MB_HR_AMOUNT)&&((start_addr + quantity) <= MB_HR_AMOUNT))
        { 
            index=0;
            reply_frame.Function = 0x03;                   //功能码
            reply_frame.Data[index++] = quantity * 2;      //字节计数
            reply_frame.Length = reply_frame.Data[0] + 1;  //数据区字节长度
            p=(uint8_t *) & reg_data;
            for (i = 0; i < quantity; i++)
            {
                reg_data = MB_HoldReg[i + start_addr];     //保持寄存器的整型数值  
                reply_frame.Data[index++] = *(p+1);        //高字节  
                reply_frame.Data[index++] = *p;            //低字节  
            }
        }
        else
        {
            reply_frame.Function = 0x83;  //错误功能码     0x83
            reply_frame.Data[0]  = 0x02;  //异常码         0x02	  		
            reply_frame.Length   = 1;     //数据区字节长度  1
        }	
    }
    else
    {
        reply_frame.Function = 0x83;       //错误功能码     0x83
        reply_frame.Data[0]  = 0x03;       //异常码         0x03	
        reply_frame.Length   = 1;          //数据区字节长度  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //发送响应帧 
}

//功能码04：读输入寄存器  
void MB_DRIVER_ReadInReg(MB_FRAME *pframe)
{
    //请求帧          PDU
    //功能码  	      1 字节 	0x04
    //起始地址 	      2 字节  0x0000 to 0xFFFF
    //输入寄存器数量  2 字节  1 to 125 (0x7D)
	
    //响应帧        PDU
    //功能码 	    1   字节  0x04
    //字节数        1   字节  2*n
    //输入寄存器值  2*n 字节  
    //n = 输入寄存器的数量	

    ////MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;
    uint16_t index;	
    uint16_t start_addr;
    uint16_t quantity;
    //uint16_t reg_data; 
       
    p = (uint8_t *)&start_addr;       //输入寄存器首地址   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //输入寄存器数量    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    //输入寄存器数量在规定范围内        	
    if((quantity >= 1)&&(quantity <= MB_IR_AMOUNT))    
    {
    	//起始地址正确，起始地址+输入寄存器数量在规定范围内
        if((start_addr < MB_IR_AMOUNT)&&((start_addr + quantity) <= MB_IR_AMOUNT))
        { 
            index=0;
            reply_frame.Function = 0x04;                   //功能码
            reply_frame.Data[index++] = quantity * 2;      //字节计数
            reply_frame.Length = reply_frame.Data[0] + 1;  //数据区字节长度
            //p=(uint8_t *) & reg_data;
            for (i = 0; i < quantity; i++)
            {
                //reg_data = MB_InReg[i + start_addr];       //输入寄存器的整型数值  
                p=(uint8_t *)&MB_InReg[i + start_addr];
                reply_frame.Data[index++] = *(p+1);        //高字节  
                reply_frame.Data[index++] = *p;            //低字节  
            }
        }
        else
        {
            reply_frame.Function = 0x84;  //错误功能码     0x84
            reply_frame.Data[0]  = 0x02;  //异常码         0x02	  		
            reply_frame.Length   = 1;     //数据区字节长度  1
        }	
    }
    else
    {
        reply_frame.Function = 0x84;       //错误功能码     0x84
        reply_frame.Data[0]  = 0x03;       //异常码         0x03	
        reply_frame.Length   = 1;          //数据区字节长度  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //发送响应帧 
}

/*
//功能码05：写单个线圈
void MB_DRIVER_WriteSingleCoil(MB_FRAME *pframe)
{
    //请求帧       PDU
    //功能码  	   1 字节 	0x05
    //输出地址 	   2 字节  0x0000 to 0xFFFF
    //输出值       2 字节  0x0000 or 0xFF00
	
    //响应帧       PDU
    //功能码  	   1 字节 	0x05
    //输出地址     2 字节  0x0000 to 0xFFFF
    //输出值       2 字节  0x0000 or 0xFF00  	
	

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
    //输出值为0x0000 或 0xFF00    
    if ((write_value == 0x0)||(write_value == 0xff00))
    {
    	//输出地址正确
        if(target_addr < MB_COILS_AMOUNT)
        {
            reply_frame.Function = 0x05;
            reply_frame.Length = 0x04;
            reply_frame.Data[0] = (*pframe).Data[0];
            reply_frame.Data[1] = (*pframe).Data[1];
            reply_frame.Data[2] = (*pframe).Data[2];
            reply_frame.Data[3] = (*pframe).Data[3];
            MB_DRIVER_SetCoils(target_addr, write_value);   //写单个线圈的值 
        }
        else
        {
            reply_frame.Function = 0x85;  //错误功能码     0x85
            reply_frame.Data[0]  = 0x02;  //异常码         0x02	  		
            reply_frame.Length   = 1;     //数据区字节长度  1
        }	
    }
    else
    {
        reply_frame.Function = 0x85;       //错误功能码     0x85
        reply_frame.Data[0]  = 0x03;       //异常码         0x03	
        reply_frame.Length   = 1;          //数据区字节长度  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //发送响应帧 
}
*/
//功能码06：写单个保持寄存器  
void MB_DRIVER_WriteSingleHoldReg(MB_FRAME *pframe)
{
    //请求帧       PDU
    //功能码  	   1 字节 	0x06
    //寄存器地址   2 字节  0x0000 to 0xFFFF
    //寄存器值     2 字节  0x0000 or 0xFFFF
	
    //响应帧       PDU
    //功能码  	   1 字节 	0x06
    //寄存器地址   2 字节  0x0000 to 0xFFFF
    //寄存器值     2 字节  0x0000 or 0xFFFF 

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
    //寄存器地址正确
    if(target_addr < MB_HR_AMOUNT)
    {
        reply_frame.Function = 0x06;
        reply_frame.Length = 0x04;
        reply_frame.Data[0] = (*pframe).Data[0];
        reply_frame.Data[1] = (*pframe).Data[1];
        reply_frame.Data[2] = (*pframe).Data[2];
        reply_frame.Data[3] = (*pframe).Data[3];
        MB_HoldReg[target_addr] =  write_value;   //写单个保持寄存器的值 
    }
    else
    {
            reply_frame.Function = 0x86;  //错误功能码     0x85
            reply_frame.Data[0]  = 0x02;  //异常码         0x02	  		
            reply_frame.Length   = 1;     //数据区字节长度  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //发送响应帧 
}
/*
//功能码07：读异常状态  
void MB_DRIVER_ReadExStatus(MB_FRAME *pframe)
{
    //请求帧        PDU
    //功能码  	    1   字节  0x07
	
    //响应帧        PDU
    //功能码 	    1   字节  0x07
    //输出数据      1   字节  0x00~0xff

    ////MB_FRAME reply_frame;
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    reply_frame.Function = 0x07;              //功能码
    reply_frame.Data[0] = MB_ExStatus;        //输出数据   
    reply_frame.Length = 1;                   //数据区字节长度  1
    MB_DRIVER_SendData(&reply_frame);                //发送响应帧 
}

//功能码08：诊断
void MB_DRIVER_ReadDiagnose(MB_FRAME *pframe)
{
    //请求帧        PDU
    //功能码  	    1   字节  0x08
    //子功能码	    2   字节  ……
    //数据         2*N  字节  ……
	
    //响应帧        PDU
    //功能码  	    1   字节  0x08
    //子功能码	    2   字节  ……
    //数据         2*N  字节  ……
    
    
    uint8_t *p;
    uint16_t fun_code;       
    p = (uint8_t*)&fun_code;         //子功能吗
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    switch(fun_code)
    {
        case 00: MB_DRIVER_BackData(pframe);break;  //子功能00：返回询问数据
        case 01: break;	
        case 02: break;
        case 03: break;
        case 04: break;
        case 05: break;                 	        	
    	default:  break;    
    }
	
} 
//诊断，子功能0x0000：返回询问数据
void MB_DRIVER_BackData(MB_FRAME *pframe)
{
    //请求帧        PDU
    //功能码  	    1   字节  0x08
    //子功能码	    2   字节  0x0000
    //数据         2*N  字节   ……
	
    //响应帧        PDU
    //功能码  	    1   字节  0x08
    //子功能码	    2   字节  0x0000
    //数据         2*N  字节   ……	
    uint8_t i;
    //MB_FRAME reply_frame;
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    reply_frame.Function = 0x08;                 //功能码
    reply_frame.Length = (*pframe).Length;       //数据区字节长度
    for(i = 0; i < reply_frame.Length; i++)
    {
    	reply_frame.Data[i] = (*pframe).Data[i]; //子功能码和数据
    }	
    MB_DRIVER_SendData(&reply_frame);                   //发送响应帧
}

//功能码15：写多个线圈     
void MB_DRIVER_WriteMoreCoils(MB_FRAME *pframe)
{
    //请求帧      PDU
    //功能码  	  1 字节 	0x0F
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //输出数量    2 字节  0x0001 to 0x07B0
    //字节计数    1 字节         n
    //输出值      n 字节         
    
    //N = 输出数量 / 8, 如果余数不等于0, n = N+1;否则 n = N
	
    //响应帧      PDU
    //功能码 	  1 字节        0x0F
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //输出数量    2 字节  0x0001 to 0x07B0
    

    //MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;
    uint16_t start_addr;    //起始地址
    uint16_t quantity;      //线圈数量
    uint8_t  byte_num;      //字节计数
    uint8_t  length;          
    uint8_t  write_data;
    uint8_t  temp1, temp2, temp3;
       
    p = (uint8_t *)&start_addr;       //线圈首地址   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //输出数量    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
    byte_num = (*pframe).Data[4];   //字节计数
		
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    if ((quantity % 8) == 0) length = quantity/8;     //计算字节数 
    else                     length = quantity/8 + 1; //计算字节数    
    //输出数量在规定范围内，字节计数正确        	
    if((quantity >= 1)&&(quantity <= MB_COILS_AMOUNT)&&(byte_num == length))    
    {
    	//起始地址正确，起始地址+输出数量在规定范围内
        if((start_addr < MB_COILS_AMOUNT)&&((start_addr + quantity) <= MB_COILS_AMOUNT))
        { 
            reply_frame.Function = 0x0F;                  //功能码
            for(i = 0; i < 4; i++)
            {
                reply_frame.Data[i] = (*pframe).Data[i];  //起始地址和输出数量
            }
            reply_frame.Length = 4;                       //数据区字节长度
            for (i = 0; i < quantity; i++)                //写多个线圈的值 
            {
            	temp1 = i / 8;
            	temp2 = i % 8;
            	temp3 = 0x01 << temp2;
            	write_data = (*pframe).Data[temp1 + 5] & temp3;
                MB_DRIVER_SetCoils(start_addr + i, write_data);        //写单个线圈的值             
            }
        }
        else
        {
            reply_frame.Function = 0x8F;  //错误功能码     0x8F
            reply_frame.Data[0]  = 0x02;  //异常码         0x02	  		
            reply_frame.Length   = 1;     //数据区字节长度  1
        }	
    }
    else
    {
        reply_frame.Function = 0x8F;       //错误功能码     0x8F
        reply_frame.Data[0]  = 0x03;       //异常码         0x03	
        reply_frame.Length   = 1;          //数据区字节长度  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //发送响应帧 
}


//功能码16：写多个寄存器     （保持寄存器）
void MB_DRIVER_WriteMoreReg(MB_FRAME *pframe)
{
    //请求帧      PDU
    //功能码  	  1 字节 	0x10
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //寄存器数量  2 字节  0x0001 to 0x007B
    //字节计数    1 字节       2*N
    //寄存器值    2*N字节      值
    //N = 寄存器数量
	
    //响应帧      PDU
    //功能码 	  1 字节  0x10
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //寄存器数量  2 字节  1~123(0x007B)
    

    MB_FRAME reply_frame;
    uint8_t  *p;
    uint16_t i;
    uint16_t start_addr;    //起始地址
    uint16_t quantity;      //寄存器数量
    uint8_t  byte_num;      //字节计数
       
    p = (uint8_t *)&start_addr;       //寄存器首地址   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&quantity;         //寄存器数量    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];
    byte_num = (*pframe).Data[4];   //字节计数
		
		
    reply_frame.ComPort = (*pframe).ComPort;
    reply_frame.Address = mb_local_address;
    
    //寄存器数量在规定范围内，字节计数正确        	
    if((quantity >= 1)&&(quantity <= MB_HR_AMOUNT)&&(byte_num == quantity*2))    
    {
    	//起始地址正确，起始地址+寄存器数量在规定范围内
        if((start_addr < MB_HR_AMOUNT)&&((start_addr + quantity) <= MB_HR_AMOUNT))
        { 
            reply_frame.Function = 0x10;                  //功能码
            for(i = 0; i < 4; i++)
            {
                reply_frame.Data[i] = (*pframe).Data[i];  //起始地址和寄存器数量
            }
            reply_frame.Length = 4;                       //数据区字节长度
            for (i = 0; i < quantity; i++)                //写多个寄存器的值 
            {
                p = (uint8_t *)&MB_HoldReg[i + start_addr];            
                *(p + 1) = (*pframe).Data[2*i + 5];
                *p = (*pframe).Data[2*i + 6]; 
            }
        }
        else
        {
            reply_frame.Function = 0x90;  //错误功能码     0x90
            reply_frame.Data[0]  = 0x02;  //异常码         0x02	  		
            reply_frame.Length   = 1;     //数据区字节长度  1
        }	
    }
    else
    {
        reply_frame.Function = 0x90;       //错误功能码     0x90
        reply_frame.Data[0]  = 0x03;       //异常码         0x03	
        reply_frame.Length   = 1;          //数据区字节长度  1
    }	
    MB_DRIVER_SendData(&reply_frame);             //发送响应帧 
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
        //置1
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
        //置0
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
//返回低字节
uint8_t MB_DRIVER_GetLow(uint16_t data)
{
	uint8_t tmep;
	tmep = data;
	return(tmep);
}
//返回高字节
uint8_t MB_DRIVER_GetHigh(uint16_t data)
{
	uint8_t tmep;
	tmep = (data >> 8);
	return(tmep);
}

//客户机(主机) 
#ifdef _ModClient
//功能码01：读线圈  （读位输出）

uint8_t MB_ReadCoils(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity,uint8_t *ResData)
{
    uint16_t repeat_count;
    uint8_t i;
    MB_FRAME frame;
    frame.ComPort  = port_id;           //端口号
    frame.Address  = slave_addr;        //从设备地址
    frame.Function = 0x01;              //功能号
    frame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil起始地址
    frame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    frame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil的位数量
    frame.Data[3]  = MB_DRIVER_GetLow(quantity);
    frame.Length   = 4;			//data区有四个字节
	
    repeat_count = 0 ; 
	while(repeat_count < MB_MAX_REPEAT)
	{
		MB_DRIVER_SendData(&frame);			//发送请求帧
		if (port_id == UART_COM0)
		{
			//启动响应超时定时器
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
			//启动响应超时定时器
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

//发送请求帧：功能码01：读线圈  （读位输出）
void MB_SEND_ReadCoils(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //请求帧    PDU
    //功能码  	1 字节 	0x01
    //起始地址 	2 字节  0x0000 to 0xFFFF
    //线圈数量 	2 字节  1 to 2000 (0x7D0)
	
    //响应帧    PDU
    //功能码 	1 字节 	0x01
    //字节计数  1 字节 	n
    //线圈状态  n 字节 	n = N or N+1
    //N = 输出数量 / 8, 如果余数不等于0, n = N+1;否则 n = N
	
    //uint8_t i;
 //   MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //端口号
    SendFrame.Address  = slave_addr;        //从设备地址
    SendFrame.Function = 0x01;              //功能号
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil起始地址
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil的位数量
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Length   = 4;			//data区有四个字节
	
    MB_DRIVER_SendData(&SendFrame);         //发送数据
}
//发送请求帧：功能码02：读离散量输入（读位输入）
void MB_SEND_ReadDisIn(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //请求帧    PDU
    //功能码  	1 字节 	0x02
    //起始地址 	2 字节  0x0000 to 0xFFFF
    //输入数量 	2 字节  1 to 2000 (0x7D0)
	
    //响应帧    PDU
    //功能码 	1 字节 	0x02
    //字节计数  1 字节 	n
    //输入状态  n 字节 	n = N or N+1
    //N = 输入数量 / 8, 如果余数不等于0, n = N+1;否则 n = N
	
    //uint8_t i;
 //   MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //端口号
    SendFrame.Address  = slave_addr;        //从设备地址
    SendFrame.Function = 0x02;              //功能号
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil起始地址
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil的位数量
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Length   = 4;			//data区有四个字节
	
    MB_DRIVER_SendData(&SendFrame);         //发送数据
}
//发送请求帧：功能码03：读保持寄存器 
void MB_SEND_ReadHoldReg(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //请求帧      PDU
    //功能码  	  1 字节 	0x03
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //寄存器数量  2 字节  1 to 125 (0x7D)
	
    //响应帧      PDU
    //功能码 	  1   字节  0x03
    //字节数      1   字节  2*n
    //寄存器值    2*n 字节  
    //n = 寄存器的数量		
    
 //   MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //端口号
    SendFrame.Address  = slave_addr;        //从设备地址
    SendFrame.Function = 0x03;              //功能号
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil起始地址
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil的位数量
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Length   = 4;			//data区有四个字节
	
    MB_DRIVER_SendData(&SendFrame);         //发送数据
}
//发送请求帧：功能码04：读输入寄存器 
void MB_SEND_ReadInReg(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //请求帧          PDU
    //功能码  	      1 字节 	0x04
    //起始地址 	      2 字节  0x0000 to 0xFFFF
    //输入寄存器数量  2 字节  1 to 125 (0x7D)
	
    //响应帧        PDU
    //功能码 	    1   字节  0x04
    //字节数        1   字节  2*n
    //输入寄存器值  2*n 字节  
    //n = 输入寄存器的数量	
    
  //  MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //端口号
    SendFrame.Address  = slave_addr;        //从设备地址
    SendFrame.Function = 0x04;              //功能号
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil起始地址
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil的位数量
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Length   = 4;	                //data区有四个字节
	
    MB_DRIVER_SendData(&SendFrame);         //发送数据
}
//发送请求帧：功能码05：写单个线圈 （写单个位输出）
void MB_SEND_WriteSingleCoil(uint8_t port_id,uint8_t slave_addr,uint16_t output_addr,uint16_t output_value)
{
    //请求帧       PDU
    //功能码  	   1 字节 	0x05
    //输出地址 	   2 字节  0x0000 to 0xFFFF
    //输出值       2 字节  0x0000 or 0xFF00
	
    //响应帧       PDU
    //功能码  	   1 字节 	0x05
    //输出地址     2 字节  0x0000 to 0xFFFF
    //输出值       2 字节  0x0000 or 0xFF00	
    
  //  MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //端口号
    SendFrame.Address  = slave_addr;        //从设备地址
    SendFrame.Function = 0x05;              //功能号
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(output_addr);//coil输出地址
    SendFrame.Data[1]  = MB_DRIVER_GetLow(output_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(output_value);  //coil输出值
    SendFrame.Data[3]  = MB_DRIVER_GetLow(output_value);
    SendFrame.Length   = 4;			//data区有四个字节
	
    MB_DRIVER_SendData(&SendFrame);         //发送数据
}
//发送请求帧：功能码06：写单个保持寄存器
void MB_SEND_WriteSingleHoldReg(uint8_t port_id,uint8_t slave_addr,uint16_t output_addr,uint16_t output_value)
{
    //请求帧       PDU
    //功能码  	   1 字节 	0x06
    //寄存器地址   2 字节  0x0000 to 0xFFFF
    //寄存器值     2 字节  0x0000 or 0xFFFF
	
    //响应帧       PDU
    //功能码  	   1 字节 	0x06
    //寄存器地址   2 字节  0x0000 to 0xFFFF
    //寄存器值     2 字节  0x0000 or 0xFFFF 	
    
 //   MB_FRAME SendFrame;
    SendFrame.ComPort  = port_id;           //端口号
    SendFrame.Address  = slave_addr;        //从设备地址
    SendFrame.Function = 0x06;              //功能号
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(output_addr);//HoldReg输出地址
    SendFrame.Data[1]  = MB_DRIVER_GetLow(output_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(output_value);//HoldReg输出值
    SendFrame.Data[3]  = MB_DRIVER_GetLow(output_value);
    SendFrame.Length   = 4;			//data区有四个字节
	
    MB_DRIVER_SendData(&SendFrame);         //发送数据
}

//发送请求帧：功能码15：写多个线圈
void MB_SEND_WriteMoreCoils(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //请求帧      PDU
    //功能码  	  1 字节 	0x0F
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //输出数量    2 字节  0x0001 to 0x07B0
    //字节计数    1 字节         n
    //输出值      n 字节         
    
    //N = 输出数量 / 8, 如果余数不等于0, n = N+1;否则 n = N
	
    //响应帧      PDU
    //功能码 	  1 字节        0x0F
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //输出数量    2 字节  0x0001 to 0x07B0 
    uint8_t i;
    uint8_t byte_num;      //字节计数
  //  MB_FRAME SendFrame;
    //余数不为零
    if(quantity % 8)  byte_num = quantity / 8 + 1;
    else              byte_num = quantity / 8;
    SendFrame.ComPort  = port_id;           //端口号
    SendFrame.Address  = slave_addr;        //从设备地址
    SendFrame.Function = 0x0f;              //功能号
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//coil起始地址
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //coil的位数量
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Data[4]  = byte_num;
    for (i = 0; i < quantity; i++)       //线圈状态
    {
        if (MB_DRIVER_GetDisIn(start_addr + i)) //线圈状态为1
        {    
            switch(i % 8)                //一个字节中的某一位
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
        else                //线圈状态为0
        {
            switch(i % 8)   //一个字节中的某一位
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
    SendFrame.Length   = 5 + byte_num;	//data区字节数
    MB_DRIVER_SendData(&SendFrame);         //发送数据
}
//发送请求帧：功能码16：写多个寄存器（保持寄存器）
void MB_SEND_WriteMoreReg(uint8_t port_id,uint8_t slave_addr,uint16_t start_addr,uint16_t quantity)
{
    //请求帧      PDU
    //功能码  	  1 字节 	0x10
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //寄存器数量  2 字节  0x0001 to 0x007B
    //字节计数    1 字节       2*N
    //寄存器值    2*N字节      值
    //N = 寄存器数量
	
    //响应帧      PDU
    //功能码 	  1 字节  0x10
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //寄存器数量  2 字节  1~123(0x007B) 
    uint8_t i;
    uint8_t byte_num;      //字节计数
    uint8_t *p;
   // MB_FRAME SendFrame;
    //余数不为零
    byte_num = quantity * 2;
    SendFrame.ComPort  = port_id;           //端口号
    SendFrame.Address  = slave_addr;        //从设备地址
    SendFrame.Function = 0x10;              //功能号
    SendFrame.Data[0]  = MB_DRIVER_GetHigh(start_addr);//起始地址
    SendFrame.Data[1]  = MB_DRIVER_GetLow(start_addr);
    SendFrame.Data[2]  = MB_DRIVER_GetHigh(quantity);  //寄存器数量
    SendFrame.Data[3]  = MB_DRIVER_GetLow(quantity);
    SendFrame.Data[4]  = byte_num;
    for (i = 0; i < quantity; i++)       //寄存器值
    {
        p = (uint8_t*)& MB_HoldReg[i + start_addr];
        SendFrame.Data[2*i + 5] = *(p + 1);
        SendFrame.Data[2*i + 6] = *p;
    }
    SendFrame.Length   = 5 + byte_num;	//data区字节数
    MB_DRIVER_SendData(&SendFrame);         //发送数据
}


//返回响应帧：功能码03：读保持寄存器  
uint8_t MB_BACK_ReadHoldReg(MB_FRAME *pframe)
{
    //请求帧      PDU
    //功能码  	  1 字节 	0x03
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //寄存器数量  2 字节  1 to 125 (0x7D)
	
    //响应帧      PDU
    //功能码 	  1   字节  0x03
    //字节数      1   字节  2*n
    //寄存器值    2*n 字节  
    //n = 寄存器的数量	

    uint8_t  *p;
    uint16_t i;	
    uint16_t start_addr;
    uint16_t quantity;
    uint16_t reg_num; 
    
    p = (uint8_t *)&start_addr;       //发送的保持寄存器首地址   
    *p = SendFrame.Data[1];
    *(p + 1) = SendFrame.Data[0];
    p = (uint8_t *)&quantity;         //发送的保持寄存器数量    
    *p = SendFrame.Data[3];
    *(p + 1) = SendFrame.Data[2];        
     
    reg_num = (*pframe).Data[0]/2; //接收的保持寄存器数量       	
    if(quantity == reg_num)        //接收的保持寄存器数量正确
    {
        for (i = 0; i < reg_num; i++)
        {
            p = (uint8_t *)&MB_HoldReg[i + start_addr];//保持寄存器的整型数值  
            *(p + 1) = (*pframe).Data[2*i + 1];    //高字节  
            *p = (*pframe).Data[2*i + 2];          //低字节  
        }
        return true;
    }
    else
    {
	return false;
    }
}

//返回响应帧：功能码04：读输入寄存器  
uint8_t MB_BACK_ReadInReg(MB_FRAME *pframe)
{
    //请求帧          PDU
    //功能码  	      1 字节 	0x04
    //起始地址 	      2 字节  0x0000 to 0xFFFF
    //输入寄存器数量  2 字节  1 to 125 (0x7D)
	
    //响应帧        PDU
    //功能码 	    1   字节  0x04
    //字节数        1   字节  2*n
    //输入寄存器值  2*n 字节  
    //n = 输入寄存器的数量	

    uint8_t  *p;
    uint16_t i;	
    uint16_t start_addr;
    uint16_t quantity;
    uint16_t reg_num; 
    
    p = (uint8_t *)&start_addr;       //发送的输入寄存器首地址   
    *p = SendFrame.Data[1];
    *(p + 1) = SendFrame.Data[0];
    p = (uint8_t *)&quantity;         //发送的输入寄存器数量    
    *p = SendFrame.Data[3];
    *(p + 1) = SendFrame.Data[2];        
     
    reg_num = (*pframe).Data[0]/2; //接收的输入寄存器数量       	
    if(quantity == reg_num)        //接收的输入寄存器数量正确
    {
        for (i = 0; i < reg_num; i++)
        {
            p = (uint8_t *)&MB_InReg[i + start_addr];//输入寄存器的整型数值  
            *(p + 1) = (*pframe).Data[2*i + 1];    //高字节  
            *p = (*pframe).Data[2*i + 2];          //低字节  
        }
        return true;
    }
    else
    {
	return false;
    }
}
//返回响应帧：功能码06：写单个保持寄存器  
uint8_t MB_BACK_WriteSingleHoldReg(MB_FRAME *pframe)
{
    //请求帧       PDU
    //功能码  	   1 字节 	0x06
    //寄存器地址   2 字节  0x0000 to 0xFFFF
    //寄存器值     2 字节  0x0000 or 0xFFFF
	
    //响应帧       PDU
    //功能码  	   1 字节 	0x06
    //寄存器地址   2 字节  0x0000 to 0xFFFF
    //寄存器值     2 字节  0x0000 or 0xFFFF 
    uint8_t  *p;
    uint16_t sen_addr;
    uint16_t receive_addr;
    uint16_t sen_val;
    uint16_t receive_val;
        
    p = (uint8_t *)&sen_addr;        //发送的保持寄存器地址   
    *p = SendFrame.Data[1];
    *(p + 1) = SendFrame.Data[0];
    p = (uint8_t *)&sen_val;         //发送的保持寄存器值    
    *p = SendFrame.Data[3];
    *(p + 1) = SendFrame.Data[2];  
    
    p = (uint8_t *)&receive_addr;    //接收的保持寄存器地址   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&receive_val;     //接收的保持寄存器值    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];           
    //地址和数值都正确       	
    if((sen_addr == receive_addr)&&(sen_val == receive_val))   
    {
        return true;
    }
    else
    {
	return false;
    }
}
//返回响应帧：功能码16：写多个寄存器     （保持寄存器）
uint8_t MB_BACK_WriteMoreReg(MB_FRAME *pframe)
{
    //请求帧      PDU
    //功能码  	  1 字节 	0x10
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //寄存器数量  2 字节  0x0001 to 0x007B
    //字节计数    1 字节       2*N
    //寄存器值    2*N字节      值
    //N = 寄存器数量
	
    //响应帧      PDU
    //功能码 	  1 字节  0x10
    //起始地址 	  2 字节  0x0000 to 0xFFFF
    //寄存器数量  2 字节  1~123(0x007B)
    
    uint8_t  *p;
    uint16_t sen_addr;
    uint16_t receive_addr;
    uint16_t sen_quantity;
    uint16_t receive_quantity;
        
    p = (uint8_t *)&sen_addr;        //发送的保持寄存器地址   
    *p = SendFrame.Data[1];
    *(p + 1) = SendFrame.Data[0];
    p = (uint8_t *)&sen_quantity;         //发送的保持寄存器数量    
    *p = SendFrame.Data[3];
    *(p + 1) = SendFrame.Data[2];  
    
    p = (uint8_t *)&receive_addr;    //接收的保持寄存器地址   
    *p = (*pframe).Data[1];
    *(p + 1) = (*pframe).Data[0];
    p = (uint8_t *)&receive_quantity; //接收的保持寄存器数量    
    *p = (*pframe).Data[3];
    *(p + 1) = (*pframe).Data[2];           
    //地址和数量都正确       	
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
