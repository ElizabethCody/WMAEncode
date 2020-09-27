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

#include "stdafx.h"
#include "utils.h"

DWORD guessChannelMask(int channels)
{
	DWORD dwChannelMask = 0;
	switch(channels)
	{
	case 1:
		dwChannelMask = KSAUDIO_SPEAKER_MONO; break;
	case 2:
		dwChannelMask = KSAUDIO_SPEAKER_STEREO; break;
	case 3:
		dwChannelMask = KSAUDIO_SPEAKER_STEREO | SPEAKER_LOW_FREQUENCY; break;
	case 4:
		dwChannelMask = KSAUDIO_SPEAKER_QUAD; break;
	case 5:
		dwChannelMask = KSAUDIO_SPEAKER_5POINT1_SURROUND & ~SPEAKER_LOW_FREQUENCY; break;
	case 6:
		//dwChannelMask = KSAUDIO_SPEAKER_5POINT1; break;
		dwChannelMask = KSAUDIO_SPEAKER_5POINT1_SURROUND; break;
	case 7:
		dwChannelMask = KSAUDIO_SPEAKER_5POINT1_SURROUND | SPEAKER_BACK_CENTER; break;
	case 8:
		dwChannelMask = KSAUDIO_SPEAKER_7POINT1_SURROUND; break;
	default:
		dwChannelMask = (1 << channels) - 1;
	}
	return dwChannelMask;
}


void progress::advance(__int64 currSamples)
{
	if (totalSamples)
	{
		int curr_percent = int(100.0*currSamples/totalSamples);
		if (curr_percent >= percent+5)
		{
			percent = curr_percent;
			_stprintf_s(buf, L"... %d%%", percent);
			std::wcerr << L"\r" << message << buf << std::flush;
		}
	}
	else if (samplerate)
	{
		int curr_sec = int(currSamples/samplerate);
		if (curr_sec >= seconds+10)
		{
			seconds = curr_sec;
			div_t d = div(curr_sec, 60);
			_stprintf_s(buf, L"... %d:%02d", d.quot, d.rem);
			std::wcerr << L"\r" << message << buf << std::flush;
		}
	}
}

void progress::begin()
{
	std::wcerr << L"\r" << message << L"..." << std::flush;
}

void progress::done()
{
	std::wcerr << L"\r" << message << L"... done!    " << std::endl;
}
