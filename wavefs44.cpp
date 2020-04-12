/* 
 *	Copyright (c) 2000 by WTC
 *
 *  This file is part of WAVEFS44, a free sound sampling rate converter
 *	
 *  WAVEFS44 is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  WAVEFS44 is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#include "global.h"
#include <math.h>

#define M_PI		3.1415926535897932384626433832795
#define ISRM		147
#define OSRM		160
#define WINSIZ		16384
#define MAXFIRODR	65535
#define ALPHA		10.0

typedef struct {
	short l;
	short r;
}	SmplData;

static int firodr[5] = {0, 4095, 12287, 24575, 32767};
static int lpfcof[5] = {0, 19400, 21200, 21400, 21600};

static int sptr, sptrr, eptr, eptrr, ismd, ismr, winpos, iptr, firodrv;
static double hfir[MAXFIRODR+1];
static double suml, sumr, frqc, frqs, regv, wfnc, divisor, hgain;
static SmplData smpld[WINSIZ*2], smplo, *spp;

static double bessel0(double);
static void DownKernel(FILE *file);

__forceinline short SaturateRound(double flt)
{
	int tmp;

	__asm
	{
		fld		[flt]
		fistp	[tmp]
	}

	return (tmp<-32768) ? -32768 : ((tmp>32767) ? 32767 : tmp);
}

void InitialSRC()
{
	int i;

	frqs = 44100*OSRM;					// virtual high sampling rate
	frqc = lpfcof[SRC_Flag];			// cutoff freq.
	firodrv = firodr[SRC_Flag];			// FIR order = firodrv*2+1

	divisor = bessel0(ALPHA);
	hgain = (2.0*ISRM*frqc)/frqs;
	hfir[0] = hgain;

	for (i=1; i<=firodrv; i++)
	{
		regv = (double)(i*i)/(firodrv*firodrv);
		wfnc = bessel0(sqrt(1.0-regv)*ALPHA)/divisor;
		regv = (2.0*M_PI*frqc*i)/frqs;
		hfir[i] = (hgain*sin(regv)*wfnc)/regv;
	}

	ismd=OSRM/ISRM;
	ismr=OSRM%ISRM;

	sptr = -(firodrv/ISRM);
	sptrr = -(firodrv%ISRM);
	eptr = firodrv/ISRM;
	eptrr = firodrv%ISRM;

	ZeroMemory(smpld, sizeof(smpld));

	winpos = 0;
	iptr = 0;
}

void Wavefs44(FILE *file, int size, unsigned char *buffer)
{
	if (winpos < WINSIZ)
	{
		if ((winpos + (size>>2)) < WINSIZ)
			memcpy(&smpld[winpos], buffer, size);
		else
		{
			memcpy(&smpld[winpos], buffer, (WINSIZ-winpos)<<2);
			DownKernel(file);
			memcpy(&smpld[WINSIZ], buffer+((WINSIZ-winpos)<<2), size-((WINSIZ-winpos)<<2));
		}
		winpos += size>>2;
	}
	else
	{
		if ((winpos + (size>>2)) < (WINSIZ<<1))
		{
			memcpy(&smpld[winpos], buffer, size);
			winpos += size>>2;
		}
		else
		{
			memcpy(&smpld[winpos], buffer, ((WINSIZ<<1)-winpos)<<2);
			DownKernel(file);
			memcpy(&smpld[0], buffer+(((WINSIZ<<1)-winpos)<<2), size-(((WINSIZ<<1)-winpos)<<2));
			winpos += (size>>2) - (WINSIZ<<1);
		}
	}
}

void EndSRC(FILE *file)
{
	int i;

	if (winpos < WINSIZ)
	{
		for (i=WINSIZ-1; i>=winpos; i--)
		{
			smpld[i].l = 0;
			smpld[i].r = 0;
		}

		DownKernel(file);
	}
	else
	{
		for (i=WINSIZ-1; i>=winpos; i--)
		{
			smpld[WINSIZ+i].l = 0;
			smpld[WINSIZ+i].r = 0;
		}

		DownKernel(file);
	}
}

bool CheckWAV()
{
	HMMIO hmmio;
	MMCKINFO mmckinfoParent;
	MMCKINFO mmckinfoSubchunk;
	WAVEFORMATEX wfx;

	int FmtSize;
	bool Error_Flag =false;

	hmmio = mmioOpen(szInput, NULL, MMIO_READ | MMIO_ALLOCBUF);

	mmckinfoParent.fccType = mmioFOURCC('W','A','V','E'); 
	if (mmioDescend(hmmio, (LPMMCKINFO) &mmckinfoParent, NULL, MMIO_FINDRIFF))
		Error_Flag = true;

	mmckinfoSubchunk.ckid = mmioFOURCC('f','m','t',' '); 
	if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK)) 
		Error_Flag = true;

	FmtSize = mmckinfoSubchunk.cksize;

	if (mmioRead(hmmio, (HPSTR) &wfx, FmtSize) != FmtSize)
		Error_Flag = true;
 
	if (wfx.wFormatTag!=WAVE_FORMAT_PCM || wfx.nChannels!=2 || wfx.nBlockAlign!=4 ||
		(wfx.nSamplesPerSec!=48000 && wfx.nSamplesPerSec!=44100))
		Error_Flag = true;

	mmioAscend(hmmio, &mmckinfoSubchunk, 0);

	mmckinfoSubchunk.ckid = mmioFOURCC('d','a','t','a'); 
	if (mmioDescend(hmmio, &mmckinfoSubchunk, &mmckinfoParent, MMIO_FINDCHUNK)) 
		Error_Flag = true;

	mmioClose(hmmio, 0);

	if (Error_Flag)
		return false;
	else
		return true;
}

void StartWAV(FILE *file, unsigned char format)
{
	int i;

	WAVEFORMATEX wfx;

	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = (format & 0x07) + 1;
	if ((format & 0x30) == 0)
		wfx.nSamplesPerSec = 48000;
	else
		wfx.nSamplesPerSec = 96000;
	if ((format & 0xc0) == 0)
		wfx.wBitsPerSample = 16;
	else
		wfx.wBitsPerSample = 24;
	wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	fwrite("RIFF", sizeof(FOURCC), 1, file);

    for (i=0; i<4; i++)
		fputc(0, file);

	fwrite("WAVE", sizeof(FOURCC), 1, file);

	fwrite("fmt ", sizeof(FOURCC), 1, file);

	i = sizeof(WAVEFORMATEX) - sizeof(WORD);
	fwrite(&i, sizeof(DWORD), 1, file);

	fwrite(&wfx, i, 1, file);

	fwrite("data", sizeof(FOURCC), 1, file);

    for (i=0; i<4; i++)
		fputc(0, file);
}

void CloseWAV(FILE *file, int size)
{
	fseek(file, 40, SEEK_SET);
	fwrite(&size, sizeof(int), 1, file);
	size += 36;
	fseek(file, 4, SEEK_SET);
	fwrite(&size, sizeof(int), 1, file);
}

void DownWAV(FILE *file)
{
	int position = ftell(file);

	WAVEFORMATEX wfx;

	wfx.wFormatTag = WAVE_FORMAT_PCM;
	wfx.nChannels = 2;
	wfx.nSamplesPerSec = 44100;
	wfx.wBitsPerSample = 16;
	wfx.nBlockAlign = wfx.nChannels * wfx.wBitsPerSample / 8;
	wfx.nAvgBytesPerSec = wfx.nSamplesPerSec * wfx.nBlockAlign;

	fseek(file, 20, SEEK_SET);
	fwrite(&wfx, sizeof(WAVEFORMATEX) - sizeof(WORD), 1, file);
	fseek(file, position, SEEK_SET);
}

static void DownKernel(FILE *file)
{
	int i, j, l;

	iptr += WINSIZ;

	while (iptr > eptr)
	{
		suml = 0;
		sumr = 0;
		j = -firodrv-sptrr;

		for (i=sptr; i<=eptr; i++, j+=ISRM)
		{
			l = j > 0 ? j : -j;

			spp = &smpld[i & ((WINSIZ<<1)-1)];
			hgain = hfir[l];
			suml += hgain * spp->l;
			sumr += hgain * spp->r;
		}

		smplo.l = SaturateRound(suml);
		smplo.r = SaturateRound(sumr);

		if (Sound_Max < abs(smplo.l))
			Sound_Max = abs(smplo.l);

		if (Sound_Max < abs(smplo.r))
			Sound_Max = abs(smplo.r);

		fwrite(&smplo, sizeof(SmplData), 1, file);

		sptr += ismd;
		sptrr += ismr;
		if (sptrr>0)
		{
			sptrr -= ISRM;
			sptr++;
		}

		eptr += ismd;
		eptrr += ismr;
		if (eptrr>=ISRM)
		{
			eptrr -= ISRM;
			eptr++;
		}
	}
}

static double bessel0(double x)
{
	double value = 1.0, step = 1.0, part = x/2;

	while (part > 1.0E-10)
	{
		value += part*part;
		step += 1.0;
		part = (part*x)/(2.0*step);
	}

	return value;
}
