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

#ifndef WAVIO_H
#define WAVIO_H

#include <tchar.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>

typedef unsigned long  DWORD;
typedef unsigned short WORD;
typedef unsigned char  XGUID[16];

static inline void setbinmode(FILE *f)
{
	_setmode(_fileno(f), _O_BINARY);
}

struct wavFormat
{
	WORD formatTag;					// WAVE_FORMAT_PCM or WAVE_FORMAT_EXTENSIBLE
	unsigned int nChannels;
	unsigned int sampleRate;
	unsigned int avgBytesPerSec;	// should be equal to sampleRate * blockAlign.
	unsigned int blockAlign;		// should be equal to nChannels * bitsPerSample / 8. 
	unsigned int bitsPerSample;		// should be multiple of 8

	unsigned int validBitsPerSample;// is <= bitsPerSample; e.g. bitsPerSample=24 and validBitsPerSample=20;
	DWORD		channelMask;		// bit mask
};

class WavReader
{
	WavReader(const WavReader&);
	WavReader& operator=(const WavReader&);
	void init();

	FILE* in;
	__int64 samplesRead;
	long dataOffset; // = 44 bytes most of the time...

	wavFormat format;
	unsigned int sampleSize_; // = nChannels * bitsPerSample/8
	unsigned int riffLength;
	unsigned int dataLength;
	bool ignoreHeaderSize;
	__int64 samplesLen;

public:
	WavReader() { init(); }
	void open(const _TCHAR* name, bool ignorelen);
	void openRaw(const _TCHAR* name, int nChannels, unsigned int bitsPerSample, int sampleRate);
	size_t readSamples(void* arr, size_t nsamples);
	size_t readBytes(void* arr, size_t bytes)
	{
		size_t nsamples = bytes / sampleSize_;
		return readSamples(arr, nsamples) * sampleSize_;
	}
	void rewind();
	void close();
	~WavReader() { close(); }

	bool fail() const { return (ferror(in)!=0); }
	const wavFormat* fmt() { return &format; }
	unsigned int sampleSize() { return sampleSize_; }
	__int64 lengthInSamples() { return samplesLen; }
	__int64 actLengthInSamples() { if (samplesLen) return samplesLen; return samplesRead; }
	__int64 samplesRD() { return samplesRead; }
};

#include "xbuffer.h"

class WavReaderBuffered
{
public:
	enum buf_mode{
		no_buffer,
		file_buffer,
		mem_buffer,
	};

private:
	WavReaderBuffered(const WavReaderBuffered&);
	WavReaderBuffered& operator=(const WavReaderBuffered&);
	void init();
	
	bool is_stdin;
	bool read_from_buf;
	XBuffer* xbuf;
	WavReader rd;

public:
	WavReaderBuffered() { init(); }
	void open(const _TCHAR* name, bool ignorelen, buf_mode mode);
	void openRaw(const _TCHAR* name, int nChannels, unsigned int bitsPerSample, int sampleRate, buf_mode mode);
	size_t readSamples(void* arr, size_t nsamples);
	void rewind();
	void close();
	~WavReaderBuffered() { close(); }

	bool fail() const { return rd.fail(); }

	const wavFormat* fmt() { return rd.fmt(); }
	__int64 lengthInSamples() { return rd.lengthInSamples(); }
	__int64 actLengthInSamples() { return rd.actLengthInSamples(); }
	__int64 samplesRD() { return read_from_buf ? xbuf->rd_samples() : rd.samplesRD(); }
};

#endif
