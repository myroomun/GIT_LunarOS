/*
 * Utility.h
 *
 *  Created on: 2015. 9. 21.
 *      Author: user
 */

#ifndef UTILITY_H_
#define UTILITY_H_

#include "Types.h"

void kMemSet( void* pvDestination, BYTE bData, int iSize );
int kMemCpy( void* pvDestination, const void* pvSource, int iSize );
int kMemCmp( const void* pvDestination, const void* pvSource, int iSize );

#endif /* UTILITY_H_ */
