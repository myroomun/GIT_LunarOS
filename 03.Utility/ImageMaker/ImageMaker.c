/*
 * ImageMaker.c
 *
 *  Created on: 2015. 9. 17.
 *      Author: user
 */


#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <io.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

#define BYTEOFSECTOR 512

int AdjustInSectorSize( int , int );
void WriteKernelInformation(int, int, int);
int CopyFile(int, int);

int main(int argc, char* argv[])
{
	int iSourceFd;
	int iTargetFd;
	int iBootLoaderSize;
	int iKernel32SectorCount;
	int iKernel64SectorCount;
	int iSourceSize;

	if ( argc < 4 )
	{
		fprintf(stderr, "[ERROR] ImageMaker.exe BootLoader.bin Kernel32.bin Kernel64.bin\n");
		exit(-1);
	}
	// Disk.img 생성 및 트렁큰으로 엶
	if( (iTargetFd = open("Disk.img", O_RDWR|O_CREAT|O_TRUNC|O_BINARY, S_IREAD|S_IWRITE)) == -1 )
	{
		fprintf(stderr, "[ERROR] Disk.img open fail\n", argv[1]);
		exit(-1);
	}
	// 부트로더 파일을 열어서 모든 내용을 디스크 이미지 파일로 복사
	printf("[INFO] Copy boot loader to image file\n");
	if((iSourceFd = open( argv[1], O_RDONLY|O_BINARY)) == -1)
	{
		fprintf(stderr, "[ERROR] %s open fail\n",argv[1]);
		exit(-1);
	}
	iSourceSize = CopyFile(iSourceFd, iTargetFd);
	close(iSourceFd);

	iBootLoaderSize = AdjustInSectorSize( iTargetFd, iSourceSize );
	printf("[INFO]%s size = [%d] and sector count = [%d]\n", argv[1], iSourceSize, iBootLoaderSize);

	// 32bit Kernel File을 열어서 디스크 이미지로 복사

	printf("[INFO] Copy protected mode kernel to image file\n");

	if((iSourceFd = open( argv[2], O_RDONLY|O_BINARY)) == -1 )
	{
		fprintf(stderr, "[ERROR] %s open fail\n", argv[2]);
	}

	iSourceSize = CopyFile(iSourceFd, iTargetFd);
	close(iSourceFd);
	iKernel32SectorCount = AdjustInSectorSize( iTargetFd, iSourceSize );
	printf("[INFO]%s size = [%d] and sector count = [%d]\n", argv[2], iSourceSize, iKernel32SectorCount);

	// 64bit Kernel File을 열어서 디스크 이미지로 복사

	printf("[INFO] Copy IA-32e mode kernel to image file\n");

		if((iSourceFd = open( argv[3], O_RDONLY|O_BINARY)) == -1 )
		{
			fprintf(stderr, "[ERROR] %s open fail\n", argv[3]);
		}

		iSourceSize = CopyFile(iSourceFd, iTargetFd);
		close(iSourceFd);
		iKernel64SectorCount = AdjustInSectorSize( iTargetFd, iSourceSize );
		printf("[INFO]%s size = [%d] and sector count = [%d]\n", argv[3], iSourceSize, iKernel64SectorCount);

	// 디스크 이미지에 커널 정보 갱신

	printf("[INFO] Start to write kernel information\n");
	WriteKernelInformation(iTargetFd, iKernel32SectorCount + iKernel64SectorCount, iKernel32SectorCount);
	printf("[INFO] Image file create complete\n");
	close(iTargetFd);
	return 0;
}

int AdjustInSectorSize(int fd, int iSourceSize)
{
	int i;
	int iAdjustSizeToSector;
	char cCh;
	int iSectorCount;

	iAdjustSizeToSector = iSourceSize % BYTEOFSECTOR;
	cCh = 0x00;

	if( iAdjustSizeToSector != 0 )
	{
		iAdjustSizeToSector = 512 - iAdjustSizeToSector;
		printf("[INFO] File size [%lu] and fill [%u] byte\n",iSourceSize, iAdjustSizeToSector);
		for(i = 0 ; i < iAdjustSizeToSector ; i++)
			{
				write(fd, &cCh, 1);
			}
	}
	else
	{
		printf("[INFO] File size is aligned along with 512 bytes\n");
	}
	iSectorCount = (iSourceSize + iAdjustSizeToSector) / BYTEOFSECTOR;
	return iSectorCount;
}

void WriteKernelInformation( int iTarget, int iTotalKernelSectorCount, int iKernel32SectorCount)
{
	unsigned short usData;
	long lPosition;

	lPosition = lseek( iTarget, (off_t)0x149, SEEK_SET);
	if( lPosition == -1 )
	{
		fprintf(stderr,"lseek fail. Return value = %d, errno = %d, %d\n",lPosition, errno, SEEK_SET);
		exit(-1);
	}
	usData = (unsigned short) iTotalKernelSectorCount; // KernelSize
	write(iTarget, &usData, 2);
	usData = (unsigned short) iKernel32SectorCount; // KernelSize
	write(iTarget, &usData, 2);

	printf("[INFO]Total sector count except boot loader [%d]\n",iTotalKernelSectorCount);
	printf("[INFO]Protected sector count [%d]\n",iKernel32SectorCount);
}

int CopyFile(int sourcefd, int targetfd)
{
	int iSourceFileSize;
	int iRead;
	int iWrite;
	char vcBuffer[BYTEOFSECTOR];

	iSourceFileSize = 0;

	while(1)
	{
		iRead = read(sourcefd, vcBuffer, sizeof(vcBuffer));
		iWrite = write(targetfd, vcBuffer, iRead);
		if( iRead != iWrite)
		{
			fprintf(stderr, "[ERROR] iRead != iWrite.. \n");
			exit(-1);
		}
		iSourceFileSize += iRead;
		if(iRead != sizeof(vcBuffer))
		{
			break;
		}
	}
	return iSourceFileSize;
}
