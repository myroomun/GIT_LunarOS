/*
 * ModeSwitch.h
 *
 *  Created on: 2015. 9. 18.
 *      Author: user
 */

#ifndef MODESWITCH_H_
#define MODESWITCH_H_

#include "Types.h"

void kReadCPUID( DWORD dwEAX, DWORD* pdwEAX, DWORD* pdwEBX, DWORD* pdwECX, DWORD* pdwEDX );
void kSwitchAndExecute64bitKernel( void );


#endif /* MODESWITCH_H_ */
