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

#ifndef XBUFFER_H
#define XBUFFER_H

#include <crtdefs.h>

class XBuffer
{
protected:
	__int64 written_samples;
	__int64 read_samples;
	
public:
	XBuffer() : written_samples(0), read_samples(0) {};
	virtual ~XBuffer() {};

	virtual void write(const void* src, size_t elementSize, size_t count) = 0;
	virtual size_t read(void* dst, size_t elementSize, size_t count) = 0;
	virtual void read_rewind() = 0;

	inline __int64 wr_samples() { return written_samples; }
	inline __int64 rd_samples() { return read_samples; }
};

//TODO: replace this  code with some generic container class. Maybe just std::list<void*>?
class MemoryBuffer : public XBuffer
{
	static const size_t N = 10*1024*1024;
	
	struct buf
	{
		buf* next;
		char data[N];
	};

	buf* head;

	buf* currWR;
	buf* currRD;

	size_t availWR;
	size_t availRD;

	void init();
	void free();
public:
	MemoryBuffer() { init(); }
	~MemoryBuffer() { free(); }

	void write(const void* src, size_t elementSize, size_t count);
	size_t read(void* dst, size_t elementSize, size_t count);
	void read_rewind() { availRD = 0; currRD = NULL; read_samples = 0; }
};

class FileBuffer : public XBuffer
{
	FILE* buf;
	void init();
	void free() { if (buf) { fclose(buf); buf = NULL; } }
public:
	FileBuffer() { init(); }
	~FileBuffer() { free(); }

	void write(const void* src, size_t elementSize, size_t count)
	{
		size_t n = ::fwrite(src, elementSize, count, buf);
		written_samples += n;
	}
	size_t read(void* dst, size_t elementSize, size_t count)
	{
		size_t n = ::fread(dst, elementSize, count, buf);
		read_samples += n;
		return n;
	}
	void read_rewind() { ::rewind(buf); read_samples = 0; }
};

#endif