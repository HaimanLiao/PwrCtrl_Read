/*
* *******************************************************************************
*   @(#)Copyright (C) 2013-2020
* *******************************************************************************
*/
/*
* *******************************************************************************
* FILE NAME         : msg.c
* SYSTEM NAME       : list、ringbuffer等方式的集合
* BLOCK NAME        :
* PROGRAM NAME      :
* MDLULE FORM       :
* -------------------------------------------------------------------------------
* AUTHOR            : wenlong wan
* DEPARTMENT        :
* DATE              : 20200228
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Include
* *******************************************************************************
*/

#include "option.h"


/*
* *******************************************************************************
*                                  Public variables
* *******************************************************************************
*/



/*
* *******************************************************************************
*                                  Private variables
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Private function prototypes
* *******************************************************************************
*/

/*
* *******************************************************************************
*                                  Private functions
* *******************************************************************************
*/

/*
* *******************************************************************************
* MODULE    : add_check
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
unsigned char add_check(unsigned char *buf, int len)
{
    unsigned char val = 0;
    int i = 0;
    for (; i<len; i++)
    {
        val += buf[i];
    }
    return val;
}

/*
* *******************************************************************************
* MODULE    : hex2string
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
void hex2string(unsigned char *hex, int len, char *str)
{
	int i = 0;
	for (; i<len; i++)
	{
		sprintf(str+i*2, "%02X", hex[i]);
	}
}

/*
* *******************************************************************************
* MODULE    : hex2ascii
* ABSTRACT  : 
* FUNCTION  : 
* ARGUMENT  : void
* NOTE      : 
* RETURN    : 
* CREATE    : 
*           : V0.01
* UPDATE    : 
* *******************************************************************************
*/
void hex2ascii(unsigned char *hex, int len, char *str)
{
    int i = 0;
    for (; i<len; i++)
    {
        sprintf(str+i, "%C", hex[i]);
    }
}