/*
 * InterruptHandler.h
 *
 *  Created on: 2015. 9. 22.
 *      Author: user
 */

#ifndef INTERRUPTHANDLER_H_
#define INTERRUPTHANDLER_H_

#include "Types.h"

void kCommonExceptionHandler( int iVectorNumber, QWORD qwErrorCode );
void kCommonInterruptHandler( int iVectorNumber );
void kKeyboardHandler( int iVectorNumber );


#endif /* INTERRUPTHANDLER_H_ */