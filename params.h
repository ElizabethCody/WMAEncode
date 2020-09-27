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
#ifndef PARAMS_H
#define PARAMS_H
#include <tchar.h>

#include <utility>
#include <list>
#include <string>

class Params
{
	Params(const Params&);
	Params& operator= (const Params&);

public:
	enum wma_codec
	{
		c_undef = 0,
		voice = 1,
		standard,
		professional,
		lossless,
	} codec;

	enum wma_mode
	{
		m_undef = 0,
		cbr     = 1,
		cbr2pass,
		vbr,
		abr2pass,
	} mode;

	int out_samplerate;
	int out_channels;
	int out_bitdepth;

	int quality;
	int bitrate;

	bool ignorelength;
	bool bufferstdin;

	bool raw_input;
	int raw_samplerate;
	int raw_channels;
	int raw_bitdepth;


	int priority;
	bool help;
	bool print_formats;
	bool silent;

	int blocksize; //write samples to WMA encoder by 'blocksize'

	_TCHAR *sourceName, *destName; //replace with std::wstring

	typedef std::pair<std::wstring, std::wstring> wma_tag;
	typedef std::list<wma_tag> TagCollection;

	TagCollection Tags;

	Params();
	~Params();

	void parse(int argc, _TCHAR* argv[]);
};

#endif
