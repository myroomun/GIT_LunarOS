/*
 * Types.h
 *
 *  Created on: 2015. 9. 17.
 *      Author: user
 */

#ifndef TYPES_H_
#define TYPES_H_

#define	BYTE	unsigned char
#define	WORD	unsigned short
#define	DWORD	unsigned int
#define	QWORD	unsigned long
#define BOOL	unsigned char

#define TRUE	1
#define	FALSE	0
#define NULL	0

#pragma pack(push, 1)

typedef struct kCharacterStruct
{
	BYTE bCharacter;
	BYTE bAttribute;
} CHARACTER;
#pragma pack(pop)



#endif /* TYPES_H_ */
