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

#ifndef UTILS_H
#define UTILS_H

#include <WinDef.h>
#include <tchar.h>
#include <exception>

enum {
	SUCCESS = 0,
	ERROR_INSUFFICIENT_ARGS,
	ERROR_INCORRECT_ARGS,
	ERROR_ALLOCATION,
	ERROR_ENCODER,
	ERROR_SYSTEM,
	ERROR_CANNOT_OPEN_INFILE,
	ERROR_CANNOT_OPEN_OUTFILE,
};

class myexc : public std::exception
{
	const _TCHAR* message;
	int retcode;
public:
	myexc(const _TCHAR* msg, int ret):message(msg), retcode(ret) {}
	const _TCHAR* const msg() const { return message; }
	int ret() const { return retcode; }
	~myexc() {};
};

class progress
{
	_TCHAR* message;
	size_t samplerate;
	__int64 totalSamples;
	_TCHAR buf[80];
	int seconds;
	int percent;

public:
	void init(_TCHAR* Message, size_t Samplerate, __int64 TotalSamples) { message = Message;  samplerate = Samplerate; totalSamples = TotalSamples; seconds = percent = 0; }

	void begin();
	void advance(__int64 currSamples);
	void done();
};

DWORD guessChannelMask(int channels);

#define STDINFILE "-"


#endif