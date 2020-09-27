/*
 * Copyright (c) 2011 lvqcl
 *
 * Permission is hereby granted, free of charge, to any person obtaining
 * a copy of this software and associated documentation files (the
 * "Software"), to deal in the Software without restriction, including
 * without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to
 * permit persons to whom the Software is furnished to do so, subject to
 * the following conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE
 * LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION
 * OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION
 * WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// wavIO.cpp : WAV parsing and reading module.
//

#include "stdafx.h"
#include "wavIO.h"
#include "utils.h"

// KSDATAFORMAT_SUBTYPE_PCM
static XGUID pcm_guid =
{	/* (00000001-0000-0010-8000-00aa00389b71) */
	0x01, 0x00, 0x00, 0x00,   0x00, 0x00,   0x10, 0x00,   0x80, 0x00,   0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
};

// KSDATAFORMAT_SUBTYPE_IEEE_FLOAT
static XGUID ieee_float_guid =
{	/* (00000003-0000-0010-8000-00aa00389b71) */
	0x03, 0x00, 0x00, 0x00,   0x00, 0x00,   0x10, 0x00,   0x80, 0x00,   0x00, 0xaa, 0x00, 0x38, 0x9b, 0x71
};

#ifndef	WAVE_FORMAT_PCM
#define	WAVE_FORMAT_PCM				0x0001
#define	WAVE_FORMAT_IEEE_FLOAT		0x0003
#define	WAVE_FORMAT_EXTENSIBLE		0xFFFE
#endif

#ifndef	SPEAKER_FRONT_LEFT
#define SPEAKER_FRONT_LEFT			0x1
#define SPEAKER_FRONT_RIGHT			0x2
#define SPEAKER_FRONT_CENTER		0x4
#define SPEAKER_ALL					0x80000000
#endif

#define U32(buf) (*(DWORD*)(buf))
#define U16(buf) (*(WORD*)(buf))

#define WRITE_U32(buf, x) (*(DWORD*)(buf) = (DWORD)(x))
#define WRITE_U16(buf, x) (*(WORD*)(buf) = (WORD)(x))


static bool seek_forward(FILE *in, size_t length)
{
	size_t seeked;
	while(length > 0)
	{
		static unsigned char buf[4096];
		seeked = fread(buf, 1, (length>4096) ? 4096 : length, in);
		if (!seeked)
			return false; /* Couldn't read more, can't read file */
		else
			length -= seeked;
	}
	return true;
}

static bool read_wave_chunk(FILE *in, unsigned char type[4], unsigned int *len)
{
	unsigned char buf[8];

	if (fread(buf, 1, 8, in) < 8)
		return false;

	memcpy(type, buf, 4);
	*len = U32(buf+4);
	return true;
}

bool is_wave(const unsigned char *buf, unsigned int* len)
{
	if (memcmp(buf, "RIFF", 4) != 0)
		return false;

	*len = U32(buf+4);

	if (memcmp(buf+8, "WAVE", 4) != 0)
		return false;

	return true;
}


void WavReader::init()
{
	in = NULL;
	samplesRead = 0;
	memset(&format, 0, sizeof(format));
	sampleSize_ = 0;
	riffLength = dataLength = 0;
	ignoreHeaderSize = false;
	samplesLen = 0;
	dataOffset = 0;
}

void WavReader::open(const _TCHAR* name, bool ignorelength)
{
	errno_t err;
	if (_tcscmp(name, _T(STDINFILE)) == 0)
	{
		setbinmode(stdin);
		in = stdin;
	}
	else
	{
		err = _tfopen_s(&in, name, _T("rb"));
		if (err) throw myexc(_T("Cannot open input file"), ERROR_CANNOT_OPEN_INFILE);
	}
	samplesRead = 0;

	unsigned char buf[41];
	unsigned char fcc[4];
	unsigned int riffLen, dataLen;
	unsigned int chunkLen;

	size_t ret;

	ret = fread(buf, 1, 12, in);
	if (ret < 12) throw myexc(_T("Input file is truncated"), ERROR_CANNOT_OPEN_INFILE);

	if (!is_wave(buf, &riffLen)) throw myexc(_T("Not a valid WAV file"), ERROR_CANNOT_OPEN_INFILE);

	for(;;)
	{
		if (!read_wave_chunk(in, fcc, &chunkLen)) throw myexc(_T("Unexpected EOF"), ERROR_CANNOT_OPEN_INFILE);

		if (memcmp(fcc, "fmt ", 4) == 0)
		{
			if (chunkLen < 16)
				throw myexc(_T("Unrecognised format chunk in WAV header"), ERROR_CANNOT_OPEN_INFILE);
			if (fread(buf, 1, chunkLen, in) < chunkLen)
				throw myexc(_T("Unexpected EOF in reading WAV header"), ERROR_CANNOT_OPEN_INFILE);
			if (chunkLen % 2)
				fgetc(in); /* read padding byte */

			/* now we have at least format.bitsPerSample and all fields before */
			/* we probably don't have format.bSize */
			
			format.formatTag = U16(buf+0);
			format.nChannels = U16(buf+2);
			format.sampleRate = U32(buf+4);
			format.avgBytesPerSec = U32(buf+8);	// should be equal to sampleRate * blockAlign.
			format.blockAlign = U16(buf+12);		// should be equal to nChannels * bitsPerSample/8. 
			format.bitsPerSample = U16(buf+14);		// should be multiple of 8
			sampleSize_ = format.nChannels * ((format.bitsPerSample+7)/8);
			
			if (format.formatTag == WAVE_FORMAT_PCM)
			{
				format.validBitsPerSample = format.bitsPerSample;
				
				if (format.blockAlign * 8 != format.nChannels * format.bitsPerSample)
				{
					//e.g. Adobe Audition 1.5 creates files that have blockAlign = 6 or 3 (24bit) and bitsPerSample = 20bit (padded).
					format.bitsPerSample = format.blockAlign * 8 / format.nChannels;
					format.formatTag = WAVE_FORMAT_EXTENSIBLE; //???
				}
				format.channelMask = guessChannelMask(format.nChannels);
			}
			else if (format.formatTag == WAVE_FORMAT_IEEE_FLOAT)
				 throw myexc(_T("WAV file has unsupported type (IEEE float)"), ERROR_CANNOT_OPEN_INFILE);
			else if (format.formatTag == WAVE_FORMAT_EXTENSIBLE)
			{
				if (chunkLen < 40 || U16(buf+16) < 22)
					throw myexc(_T("WAV file with invalid WAVEFORMATEXTENSIBLE struct"), ERROR_CANNOT_OPEN_INFILE);
				if (memcmp(buf+24, pcm_guid, 16) == 0)
				{
					format.validBitsPerSample = U16(buf+18);
					format.channelMask = U32(buf+20);
					if (format.channelMask == 0) //sox creates such files, and foobar2000 doesn't like them.
						format.channelMask = guessChannelMask(format.nChannels);
				}
				else if (memcmp(buf+24, ieee_float_guid, 16) == 0)
					 throw myexc(_T("WAV file has unsupported type (IEEE float)"), ERROR_CANNOT_OPEN_INFILE);
				else throw myexc(_T("WAV file has unsupported type (must be standard PCM)"), ERROR_CANNOT_OPEN_INFILE);
			}
			else throw myexc(_T("WAV file has unsupported type (must be standard PCM)"), ERROR_CANNOT_OPEN_INFILE);
		}

		else if (memcmp(fcc, "data", 4) == 0)
		{
			dataOffset = ftell(in);
			dataLen = chunkLen; /* length of "data" chunk */
			/* if (dataLen >= 0x7fffffff or 0xffffffff or 0xffffff00 or 0xfffff000 or 0x0) ... */

			riffLength = riffLen;
			dataLength = dataLen;
			if (dataLength == 0xFFFFFFFF || ignorelength == true)
				{ ignoreHeaderSize = true;  samplesLen = 0; }
			else
				{ ignoreHeaderSize = false; samplesLen = dataLength/sampleSize(); }
			return;
		}

		else
		{
			if (chunkLen % 2) chunkLen++; /* padding byte */
			if (!seek_forward(in, chunkLen)) throw myexc(_T("Unexpected EOF in reading WAV header"), ERROR_CANNOT_OPEN_INFILE);
		}
	} //end for(;;)
}

void WavReader::openRaw(const _TCHAR* name, int nChannels, unsigned int bitsPerSample, int sampleRate)
{
	errno_t err;
	if (_tcscmp(name, _T(STDINFILE)) == 0)
	{
		setbinmode(stdin);
		in = stdin;
	}
	else
	{
		err = _tfopen_s(&in, name, _T("rb"));
		if (err) throw myexc(_T("Cannot open input file"), ERROR_CANNOT_OPEN_INFILE);
	}
	samplesRead = 0;

	bitsPerSample = ((bitsPerSample+7)/8)*8;
	format.formatTag = WAVE_FORMAT_PCM;
	format.nChannels = nChannels;
	format.sampleRate = sampleRate;
	format.blockAlign = nChannels * bitsPerSample/8;
	format.avgBytesPerSec = sampleRate * format.blockAlign;
	format.bitsPerSample = bitsPerSample;		// should be multiple of 8
	sampleSize_ = nChannels * bitsPerSample/8;

	format.validBitsPerSample = format.bitsPerSample;
	format.channelMask = guessChannelMask(format.nChannels);

	dataOffset = 0;
	riffLength = 0xFFFFFFFF;
	dataLength = 0xFFFFFFFF;
	ignoreHeaderSize = true;
	samplesLen = 0;
}

size_t WavReader::readSamples(void* arr, size_t nsamples)
{
	size_t n;

	if ( (ignoreHeaderSize == false) && (nsamples > (size_t)(samplesLen - samplesRead)) )
	{
		nsamples = (size_t)(samplesLen - samplesRead);
		if (nsamples == 0) return 0;
	}

	n = fread(arr, sampleSize(), nsamples, in);
	//if (n==0 && feof(in)) { }
	//if (n==0 && ferror(in)) { }
	samplesRead += n;

	return n;
}

void WavReader::close()
{
	if (in) fclose(in);
	init();
}

void WavReader::rewind()
{
	::rewind(in);
	fseek(in, dataOffset, SEEK_SET);
	
	samplesRead = 0;
}




void WavReaderBuffered::init()
{
	is_stdin = false;
	read_from_buf = false;
	xbuf = NULL;
}

void WavReaderBuffered::open(const _TCHAR* name, bool ignorelen,  buf_mode mode)
{
	if (_tcscmp(name, _T(STDINFILE)) == 0)
		is_stdin = true;
	if (!is_stdin) mode = no_buffer;

	rd.open(name, ignorelen);

	if (mode == file_buffer)
		xbuf = new FileBuffer();
	else if (mode == mem_buffer)
		xbuf = new MemoryBuffer();
}

void WavReaderBuffered::openRaw(const _TCHAR* name, int nChannels, unsigned int bitsPerSample, int sampleRate, buf_mode mode)
{
	if (_tcscmp(name, _T(STDINFILE)) == 0)
		is_stdin = true;
	if (!is_stdin) mode = no_buffer;

	rd.openRaw(name, nChannels, bitsPerSample, sampleRate);

	if (mode == file_buffer)
		xbuf = new FileBuffer();
	else if (mode == mem_buffer)
		xbuf = new MemoryBuffer();
}

size_t WavReaderBuffered::readSamples(void* arr, size_t nsamples)
{
	if (read_from_buf == false)
	{
		size_t n = rd.readSamples(arr, nsamples);
		if (is_stdin && xbuf)
			xbuf->write(arr, rd.sampleSize(), n);
		return n;
	}
	else
		return xbuf->read(arr, rd.sampleSize(), nsamples);
}

void WavReaderBuffered::rewind()
{
	if (!is_stdin)
	{
		rd.rewind();
	}
	else
	{
		assert(xbuf!=NULL);
		xbuf->read_rewind();
		read_from_buf = true;
	}
}

void WavReaderBuffered::close()
{
	rd.close();
	delete xbuf;
	init();
}
