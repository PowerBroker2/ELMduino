#pragma once
#include <stdint.h>

#define PID_MAX_ENTRIES 185

typedef struct 
{
	uint16_t pid;		 //OBD-II PID number
	uint8_t rsp_len; //Length of the response payload 
}PID_T;



extern const PID_T pid_table[PID_MAX_ENTRIES];
