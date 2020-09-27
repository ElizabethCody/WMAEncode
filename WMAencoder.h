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

#ifndef WMAENCODER_H
#define WMAENCODER_H

#include <atlbase.h>
#include "utils.h"

class XInit
{
public:
	XInit()
	{
		HRESULT hr = CoInitialize(NULL);
		if (FAILED(hr)) throw myexc(_T("COM initialization failed"), ERROR_ENCODER);
	}
	~XInit()
	{
		CoUninitialize();
	}
};

class WMAencoder : public XInit
{
	class XWmMediaType
	{
		WM_MEDIA_TYPE* p;
	public:
		XWmMediaType(size_t n)
		{
			if (n < sizeof(WM_MEDIA_TYPE))
				n = sizeof(WM_MEDIA_TYPE);
			p = (WM_MEDIA_TYPE*) new BYTE[n];
		}
		~XWmMediaType() { Release(); }
	
		operator WM_MEDIA_TYPE* () const { return (WM_MEDIA_TYPE*) p; }
		WM_MEDIA_TYPE* operator ->() const { return (WM_MEDIA_TYPE*) p; }

		void Release() { if (p) { delete[] ((BYTE*)p); p = 0; } }
	};

public:
	enum WMAcodec
	{
		WMAvoice        = 0xA,
		WMAstandard     = 0x161,
		WMAprofessional = 0x162,
		WMAlossless     = 0x163,
	};

	struct Wformat
	{
		unsigned int nChannels;
		unsigned int sampleRate;
		unsigned int bitsPerSample;
	};

private:
	HRESULT hr;

	CComPtr<IWMProfileManager> pProfileManager;
	CComPtr<IWMWriter> pWriter;
	CComPtr<IWMWriterPreprocess> pWriterPreprocess;

	double sampleTime; // accumulating. "Running total" - MSDN.

	int sampleRate;
	DWORD sampleSize; // = channels * bitdepth/8;

	bool getCodecFormat(CComPtr<IWMCodecInfo3> pCI, WMAcodec cdc, bool vbr, int npass, int qb, Wformat& wma, DWORD& cdcIdx, DWORD& fmtIdx);

	bool convert1chto2;
	int channelsAfterUpconvert(int ch)
	{
		if (convert1chto2) { assert(ch==1); return 2; }
		return ch;
	}
	
	void xwrite(const void* arr, DWORD nSamples, bool preproc);

public:
	WMAencoder();
	~WMAencoder() {}

	bool open(WMAcodec codec, bool isVBR, int numPasses, int qb, const Wformat& wav, Wformat& wma);

	void start(const WCHAR * filename);
	void write(const void* arr, DWORD nSamples) { xwrite(arr, nSamples, false); }
	void close();

	void startPreprocess();
	void preprocess(const void * arr, DWORD nSamples) { xwrite(arr, nSamples, true); }
	void endPreprocess();

	bool addTag(const WCHAR* name, const WCHAR* value);
	bool addTag(const char*  name, const char*  value); // not implemented.

	static void printFormats();
};

#endif
