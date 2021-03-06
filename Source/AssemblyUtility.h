/*
 * AssemblyUtility.h
 *
 *  Created on: 2015. 9. 21.
 *      Author: user
 */

#ifndef ASSEMBLYUTILITY_H_
#define ASSEMBLYUTILITY_H_


#include "Types.h"
#include "Task.h"

BYTE kInPortByte( WORD wPort );
void kOutPortByte( WORD wPort, BYTE bData );
void kLoadGDTR( QWORD qwGDTRAddress );
void kLoadTR( WORD wTSSSegmentOffset );
void kLoadIDTR( QWORD qwIDTRAddress );
void kEnableInterrupt( void );
void kDisableInterrupt( void );
QWORD kReadRFLAGS( void );
QWORD kReadTSC( void );
void kSwitchContext( CONTEXT* pstCurrentContext, CONTEXT* pstNextCOntext );
void kHlt( void );
BOOL kTestAndSet( volatile BYTE* pbDestination, BYTE bCompare, BYTE bSource );
void kInitializeFPU( void );
void kSetFPUContext( void* pvFPUContext );
void kLoadFPUContext( void* pvFPUContext );
void kSetTS( void );
void kClearTS( void );
WORD kInPortWort( WORD PORT );
void kOutPortWord( WORD wPort, WORD wData );

#endif /* ASSEMBLYUTILITY_H_ */
