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

// enc_wma.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "params.h"
#include "utils.h"
#include "wavIO.h"
#include "WMAencoder.h"

const wchar_t Program[] = L"WmaEncode";
const wchar_t Version[] = L"0.2.9c";

void printUsage(const _TCHAR* exeName);
void printHelp(const _TCHAR* exeName);
void setIdlePriority();

static inline size_t chooseSampleBlockSize(size_t rate)
{
	return rate;
}

static unsigned int samplerate_kHz(unsigned int freq);

using namespace std;

int _tmain(int argc, _TCHAR* argv[])
{
	if (argc == 1)
	{
		printUsage(argv[0]);
		return ERROR_INSUFFICIENT_ARGS;
	}

	try{
		Params par;
		par.parse(argc, argv);

		if (par.help) { printHelp(argv[0]); return SUCCESS; }
		if (par.print_formats) { WMAencoder::printFormats(); return SUCCESS; }
		if (par.priority != -1) { setIdlePriority(); }

		if (par.sourceName == NULL || par.destName == NULL)
		{
			printUsage(argv[0]);
			return ERROR_INSUFFICIENT_ARGS;
		}
		bool inputStdin = (_tcscmp(par.sourceName, _T(STDINFILE)) == 0) ? true : false;

		WMAencoder::WMAcodec codec;
		switch(par.codec)
		{
		case Params::voice:
			codec = WMAencoder::WMAvoice; break;
		default:
		case Params::standard:
			codec = WMAencoder::WMAstandard; break;
		case Params::professional:
			codec = WMAencoder::WMAprofessional; break;
		case Params::lossless:
			codec = WMAencoder::WMAlossless; break;
		}
		
		if (par.mode == Params::m_undef)
		{
			if (codec == WMAencoder::WMAlossless)
				par.mode = Params::vbr;
			else if (codec == WMAencoder::WMAvoice)
				par.mode = Params::cbr;
			else // WMA standard, WMA professional
			{
				if (par.quality < 0)
					par.mode = Params::cbr;
				else
					par.mode = Params::vbr;
			}
		}

		bool isVBR; int numPasses;
		switch(par.mode)
		{
		default:
		case Params::cbr:
			isVBR = false; numPasses = 1; break;
		case Params::cbr2pass:
			isVBR = false; numPasses = 2; break;
		case Params::vbr:
			isVBR = true; numPasses = 1; break;
		case Params::abr2pass:
			isVBR = true; numPasses = 2; break;
		}

		int qb; // quality (VBR mode) or bitrate (other modes)
		if (par.mode == Params::vbr)
		{
			qb = par.quality;
			if (codec == WMAencoder::WMAlossless) { qb = 100; }
			else if (qb <= 0)
			{
				wcerr << L"Quality must be set for VBR mode" << endl;
				return ERROR_INCORRECT_ARGS;
			}
		}	
		else
		{
			qb = par.bitrate;
			if (qb <= 0) qb = 128;
		}

		WavReaderBuffered::buf_mode bufmode = WavReaderBuffered::file_buffer;
		if (par.bufferstdin == true) bufmode = WavReaderBuffered::mem_buffer;

		if (codec == WMAencoder::WMAvoice || codec == WMAencoder::WMAlossless) bufmode = WavReaderBuffered::no_buffer;
		else if (numPasses == 1) bufmode = WavReaderBuffered::no_buffer;
		else if (!inputStdin) bufmode = WavReaderBuffered::no_buffer;

		WavReaderBuffered source;
		if (par.raw_input)
			source.openRaw(par.sourceName, par.raw_channels, par.raw_bitdepth, par.raw_samplerate, bufmode);
		else
			source.open(par.sourceName, par.ignorelength, bufmode);

		WMAencoder::Wformat wav;
		wav.sampleRate    = source.fmt()->sampleRate;
		wav.bitsPerSample = source.fmt()->bitsPerSample;
		wav.nChannels     = source.fmt()->nChannels;

		if (par.out_samplerate < 1000) par.out_samplerate = samplerate_kHz(par.out_samplerate);

		WMAencoder::Wformat wma;
		wma.sampleRate    = par.out_samplerate;
		wma.bitsPerSample = par.out_bitdepth;
		wma.nChannels     = par.out_channels;
		if (wma.sampleRate    <= 0)  wma.sampleRate    = wav.sampleRate;
		if (wma.bitsPerSample <= 0)  wma.bitsPerSample = wav.bitsPerSample;
		if (wma.nChannels     <= 0)  wma.nChannels     = wav.nChannels;
		// Wformat:wma contains info about requested format of WMA file.

		WMAencoder encoder;
		if (false == encoder.open(codec, isVBR, numPasses, qb, wav, wma))
		{
			wcerr << L"Requested encoder not found" << endl;
			return ERROR_INCORRECT_ARGS;
		}
		// Wformat:wma contains info about real format of WMA file.
		// Windows encoder runtime can resample, change bit depth or decrease number of channels.
		// (e.g. it is possible to encode 44100 24-bit WAV to 32000 16-bit WMA)
		// But it cannot increase number of channels. It is not possible to convert mono WAV to lossless WMA or to 128 kbps WMA standard.
		// Of course, we can do it ourselves, if we want. Allow different chan.numbers in open()/getCodecFormat(),
		// then convert mono->stereo in preprocess() and write() functions...

		Params::TagCollection::iterator i;
		for(i = par.Tags.begin(); i!= par.Tags.end(); i++)
			encoder.addTag(i->first.c_str(), i->second.c_str());

		//encoder.addTag(L"WM/ToolName",    Program);
		//encoder.addTag(L"WM/ToolVersion", Version);

		size_t toReadSamples = chooseSampleBlockSize(wav.sampleRate);
		if (par.blocksize > 0)
			toReadSamples = par.blocksize;
		else if (par.blocksize == -1 && inputStdin == false)
		{
			__int64 sl = source.lengthInSamples();
			if (sl <= 100000000L && sl > 0L) // 100 000 000 samples = 400 MBytes (16-bit, stereo) or 600 MBytes (24-bit, stereo)
				toReadSamples = (size_t)sl;
		}
		
		size_t sampleSize = wav.nChannels * (wav.bitsPerSample/8);
		std::vector<char> arr(toReadSamples * sampleSize);

		__int64 totalSamples = source.lengthInSamples();
		progress indicator;

		encoder.start(par.destName);
		if (numPasses == 2)
		{
			DWORD samplesRead = 0;
			if (!par.silent) { indicator.init(L"Start preprocess", wav.sampleRate, totalSamples); indicator.begin(); }
			encoder.startPreprocess();
			while((samplesRead = (DWORD)source.readSamples(arr.data(), toReadSamples)) != 0)
			{
				encoder.preprocess(arr.data(), samplesRead);
				if (!par.silent) indicator.advance(source.samplesRD());
			}
			encoder.endPreprocess();
			totalSamples = source.actLengthInSamples(); //useful for stdin input
			if (!par.silent) indicator.done();
			source.rewind();
		}

		if (!par.silent) { indicator.init(L"Start encoding", wav.sampleRate, totalSamples); indicator.begin(); }
		DWORD samplesRead = 0;
		while((samplesRead = (DWORD)source.readSamples(arr.data(), toReadSamples)) != 0)
		{
			encoder.write(arr.data(), samplesRead);
			if (!par.silent) indicator.advance(source.samplesRD());
		}
		if (!par.silent) indicator.done();
		encoder.close();
		source.close();

		return SUCCESS;
	}

	catch(const myexc& e)
	{
		wcerr << e.msg() << endl;
		return e.ret();
	}

	catch(const exception& e)
	{
		wcerr << e.what() << endl;
		return -1;
	}
}

void printUsage(const _TCHAR* exeName)
{
	static TCHAR exeName2[_MAX_FNAME];
	_tsplitpath_s(exeName, NULL, 0, NULL, 0, exeName2, _MAX_FNAME, NULL, 0);
	
	wcout << exeName << L" ver." << Version << L": command-line interface to MS WMA encoder";
	wcout << L" (uses Windows Media Format runtime)\n";

	wcout << L"\nUsage:\n";
	wcout << L"\t" << exeName2 << " <input> [output] [options]\n";
	wcout << L"\t<input>: .WAV PCM file; \"-\" means stdin\n"
			 L"\toutput: output file (stdout not allowed)\n"
			 L"Options:\n"
			 L"\t--quality <n>, -q <n>: set VBR quality; n = (10,25,50,75,90,98)\n"
			 L"\t--bitrate <n>, -b <n>: set bitrate to <n> kbps\n"
			 L"\ntype " << exeName2 << L" --help for a complete list of options\n";
}

void printHelp(const _TCHAR* exeName)
{
	static TCHAR exeName2[_MAX_FNAME];
	_tsplitpath_s(exeName, NULL, 0, NULL, 0, exeName2, _MAX_FNAME, NULL, 0);

	wcout << L"Usage:\n";
	wcout << L"\t" << exeName2 << " <input> [output] [options]\n";
	wcout << L"\t<input>: .WAV PCM file; \"-\" means stdin\n"
			 L"\toutput: output file (stdout not allowed)\n"
			 L"Options:\n"
			 L"\t--quality <n>, -q <n>: set VBR quality; n = (10,25,50,75,90,98)\n"
			 L"\t--bitrate <n>, -b <n>: set bitrate to <n> kbps (default = 128)\n"
			 L"\t--codec, -c:           select codec\n"
			 L"\t\tstandard,     std - WMA standard (default)\n"
			 L"\t\tprofessional, pro - WMA professional\n"
			 L"\t\tlossless,     lsl - WMA lossless\n"
			 L"\t\tvoice             - WMA voice\n"
			 L"\t--mode, -m:            select encoding mode\n"
			 L"\t\tcbr      - CBR (default)\n"
			 L"\t\tcbr2pass - CBR two-pass\n"
			 L"\t\tvbr      - VBR (quality-based)\n"
			 L"\t\tvbr2pass - VBR two-pass (bitrate-based)\n"
			 L"\t--out-samplerate n:    choose output format with n samples per second\n"
			 L"\t--out-channels n:      choose output format with n channels\n"
			 L"\t--out-bitdepth n:      choose output format with n bits per sample\n"
			 L"\t--ignorelength, -i:    ignore length in WAV header\n"
			 L"\t--bufferstdin:         buffer data from stdin for 2pass encoding in RAM\n"
			 L"\t\t(the encoder buffers all input data in RAM. Not recommended!)\n"
			 L"Options for raw PCM input:\n"
			 L"\t--raw:                 input is raw PCM data (headerless)\n"
 			 L"\t--raw-samplerate:      sample rate (default = 44100)\n"
 			 L"\t--raw-channels:        number of channels (default = 2)\n"
 			 L"\t--raw-bitdepth:        bit depth (default = 16)\n"
			 L"Other options:\n"
			 L"\t--priority, --nice:    set low priority\n"
			 L"\t--silent, -s:          suppress progress messages\n"
			 L"\t--print-formats:       print all available codecs and formats\n"
			 L"\t--help, -h:            show this text\n";
	wcout << L"Tagging options:\n"
			 L"\t--title <x>, --tracknumber <x>, --artist <x>, --album <x>,\n"
			 L"\t--year <x>, --genre <x>, --composer <x>, --albumartist <x>,\n"
			 L"\t--discnumber <x>, --comment <x>  or --tag <name>=<value>\n";
}

void setIdlePriority()
{
	::SetPriorityClass(::GetCurrentProcess(), IDLE_PRIORITY_CLASS);
}

static unsigned int samplerate_kHz(unsigned int freq)
{
	switch (freq)
	{
	case 11:
		return 11025;
	case 22:
		return 22050;
	case 44:
		return 44100;
	case 88:
		return 88200;
	case 176:
		return 176400;
	default:
		return freq*1000;
	}
} 