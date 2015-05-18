#ifndef __BC_UTIL_H__
#define __BC_UTIL_H__

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>


#define ERROR(fmt, args...) printf("\033[31m\033[1m[ERR]\033[0m%s:" fmt "\n",__func__, ## args)
#define WARN(fmt, args...) 	printf("\033[33m\033[1m[WARN]:\033[0m%s:" fmt "\n",__func__, ## args)
#define PRINT(fmt, args...)	printf(fmt "\n", ## args)
#ifdef DEBUG_ENABLE
#define INFO(fmt, args...) 	printf("\033[34m\033[1m[INFO]:\033[0m%s:" fmt "\n",__func__, ## args)
#define DEBUG(fmt, args...) printf("\033[32m\033[1m[DBG]:\033[0m%s:" fmt "\n",__func__, ## args)
#else
#define INFO(fmt, args...)
#define DEBUG(fmt, args...)
//#define PRINT(fmt,args...)
#endif


/*Y must be power of 2*/
#define ALIGN_UP(x,y)   (((x) + ((y) - 1)) & ~((y) - 1))
#define ALIGN_DOWN(x,y) ((x) & ~((y) - 1))

#define BC_MIN(x,y)		(((x) < (y))?(x):(y))	
#define BC_MAX(x,y)		(((x) > (y))?(x):(y))	

#define BC_ENABLE  	0x01
#define BC_DISABLE 	0x00
#define BC_FLUSH	0x02
#define BC_FLUSH_WAIT_ACK 0x04



#endif /*__BC_UTIL_H__*/
