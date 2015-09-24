/*
 * AssemblyUtility.h
 *
 *  Created on: 2015. 9. 21.
 *      Author: user
 */

#ifndef ASSEMBLYUTILITY_H_
#define ASSEMBLYUTILITY_H_

#include "Types.h"

BYTE kInPortByte( WORD wPort );
void kOutPortByte( WORD wPort, BYTE bData );
void kLoadGDTR( QWORD qwGDTRAddress );
void kLoadTR( WORD wTSSSegmentOffset );
void kLoadIDTR( QWORD qwIDTRAddress );
void kEnableInterrupt( void );
void kDisableInterrupt( void );
QWORD kReadRFLAGS( void );
QWORD kReadTSC( void );

#endif /* ASSEMBLYUTILITY_H_ */
