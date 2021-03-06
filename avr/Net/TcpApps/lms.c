/*
 * Copyright (C) 2008 Bjoern Biesenbach <bjoern@bjoern-b.de>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public
 * License along with this program; if not, write to the
 * Free Software Foundation, Inc., 59 Temple Place - Suite 330,
 * Boston, MA 02111-1307, USA.
 *
 */

/*
 * This is the most simple server to obtain data for the 64x16 display
 */
#include <avr/wdt.h>
#include <string.h>
#include "lms.h"
#include "../uip/uip.h"
#include "../../led_matrix.h"
#include "../../../common/protocol.h"

void LedNetReset(LedNetMessage *net_msg,uint8_t data_byte)
{
    // let the watchdog reset the avr
    wdt_enable(WDTO_15MS);
    while(1);
}

void LedNetSelectFont(LedNetMessage *net_msg,uint8_t data_byte)
{
    font = font_table[data_byte]; 
}

void LedNetPutString(LedNetMessage *net_msg,uint8_t data_byte)
{
    /* beware! max length of string is 100 */
    static char string[101];
    static uint16_t byte_counter = 0;
    if(byte_counter < 100)
    {
        string[byte_counter++] = (char)data_byte;
    }
    else
        byte_counter = 0;
    if(byte_counter >= net_msg->byte_count)
    {
        byte_counter = 0;
        putString(backbuffer,backbuffer+16*4,string,0,1);
        swap_buffers();
        memset(backbuffer,0,sizeof(uint16_t)*16*4*2);
        memset(string,0,sizeof(string));
    }
}

void LedNetFuncRawData(LedNetMessage *net_msg,uint8_t data_byte)
{
    static byte_counter = 0;

    *(((uint8_t*)backbuffer)+(byte_counter++)) = data_byte;
    if(byte_counter == sizeof(uint16_t)*4*16*2)
    {
        byte_counter = 0;
        swap_buffers();
        memset(backbuffer,0,sizeof(uint16_t)*16*4*2);
    }
}

LedNetFunc LedNetFuncTable[] = {
    LedNetFuncRawData,
    LedNetReset,
    LedNetSelectFont,
    LedNetPutString
};

void LMSInit()
{
    uip_listen(HTONS(PORT_LMS));
}

void LMSCall(   uint8_t* pBuffer,
                uint16_t nBytes,
                struct tLMS* pSocket)
{
    uint16_t counter;

    /* wird nur direkt nach connect ausgefuehrt */
    if(uip_connected())
    {
        pSocket->byte_counter = 0;
        pSocket->has_message = 0;
    }
    else if(uip_newdata() || uip_acked())
    {
        for(counter=0;counter < nBytes; counter++)
        {
            if(!pSocket->has_message)
            {
                *(((uint8_t*)&(pSocket->cur_message))+(pSocket->byte_counter++)) = *pBuffer++;
                if(pSocket->byte_counter == sizeof(LedNetMessage))
                {
                    pSocket->has_message = 1;
                    pSocket->byte_counter = 0;
                }
            }
            else
            {
                LedNetFuncTable[pSocket->cur_message.func_id](&(pSocket->cur_message),*pBuffer++);
                pSocket->byte_counter++;
                if(pSocket->byte_counter >= pSocket->cur_message.byte_count)
                {
                    pSocket->has_message = 0;
                    pSocket->byte_counter = 0;
                }
            }
        }
    }
}

