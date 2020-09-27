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
#include "WMAencoder.h"
#pragma comment(lib, "Wmvcore.lib")

static inline void TEST(bool a) { if (a==false) throw myexc(_T("WMA encoder returns error"), ERROR_ENCODER); }
static inline void TEST(bool a, const TCHAR* msg) { if (a==false) throw myexc(msg, ERROR_ENCODER); }

static inline void TESTB(bool a) { if (a==false) throw myexc(_T("WMA encoder returns error"), ERROR_ENCODER); }
static inline void TESTB(bool a, const TCHAR* msg) { if (a==false) throw myexc(msg, ERROR_ENCODER); }

static inline void TEST(HRESULT hr) { if (FAILED(hr)) throw myexc(_T("WMA encoder returns error"), ERROR_ENCODER); }
static inline void TEST(HRESULT hr, const TCHAR* msg) { if (FAILED(hr)) throw myexc(msg, ERROR_ENCODER); }

const DWORD INVALID_INDEX = 0x7FFFFFFF;

WMAencoder::WMAencoder() : hr(S_OK)
{
	convert1chto2 = false;
	hr = WMCreateProfileManager(&pProfileManager); TEST(hr);
}

bool WMAencoder::open(WMAcodec codec, bool isVBR, int numPasses, int qb, const Wformat& wav, Wformat& wma)
{
	DWORD dwSize;
	DWORD targetCodec = INVALID_INDEX, targetFormat = INVALID_INDEX;
	bool ret;

/**/CComPtr<IWMCodecInfo3> pCodecInfo3;
	hr = pProfileManager->QueryInterface<IWMCodecInfo3>(&pCodecInfo3); TEST(hr);

	ret = getCodecFormat(pCodecInfo3, codec, isVBR, numPasses, qb, wma, targetCodec, targetFormat);
	//TESTB(ret, _T("Cannot find proper format for the input file"));
	if (ret == false) return false;

	// Wformat:wma contains info about real format of WMA file. Wformat:wav contains info about real format of input WAV file.
	if (wav.nChannels == 1 && wma.nChannels == 2)
		convert1chto2 = true;

/**/CComPtr<IWMStreamConfig> pStreamConfig;
	hr = pCodecInfo3->GetCodecFormat(WMMEDIATYPE_Audio, targetCodec, targetFormat, &pStreamConfig); TEST(hr);
	hr = pStreamConfig->SetStreamNumber(1); TEST(hr);

/**/CComPtr<IWMProfile> pProfile;
	hr = pProfileManager->CreateEmptyProfile(WMT_VER_9_0, &pProfile); TEST(hr);
	hr = pProfile->AddStream(pStreamConfig); TEST(hr);

	hr = WMCreateWriter(NULL, &pWriter); TEST(hr);
	hr = pWriter->SetProfile(pProfile); TEST(hr);

	{ DWORD dwInputCount = 0; hr = pWriter->GetInputCount(&dwInputCount); TEST(hr); TESTB(dwInputCount > 0); }

/**/CComPtr<IWMInputMediaProps> pInputProps;
	hr = pWriter->GetInputProps(0, &pInputProps); TEST(hr);
	hr = pInputProps->GetMediaType(NULL, &dwSize); TEST(hr);
	XWmMediaType pType(dwSize);
	hr = pInputProps->GetMediaType(pType, &dwSize); TEST(hr);

	WAVEFORMATEX* pWave = NULL;
	if (pType->cbFormat >= sizeof(WAVEFORMATEX))
		pWave = (WAVEFORMATEX*)pType->pbFormat;
	TESTB(pWave != NULL);

	bool chg = false;
	if (pWave->nSamplesPerSec != wav.sampleRate || pWave->wBitsPerSample != wav.bitsPerSample || pWave->nChannels != channelsAfterUpconvert(wav.nChannels))
	{
		pWave->nSamplesPerSec = (DWORD)wav.sampleRate;
		pWave->wBitsPerSample = (WORD)wav.bitsPerSample;
		pWave->nChannels = (WORD)channelsAfterUpconvert(wav.nChannels);

		pWave->nBlockAlign = (pWave->wBitsPerSample/8) * pWave->nChannels;
		pWave->nAvgBytesPerSec = pWave->nBlockAlign * pWave->nSamplesPerSec;
		chg=true;
	}
	if (pWave->wFormatTag == WAVE_FORMAT_EXTENSIBLE && pWave->cbSize >= 22)
	{
		WAVEFORMATEXTENSIBLE * pWaveEx = (WAVEFORMATEXTENSIBLE*)pWave;
		pWaveEx->dwChannelMask = guessChannelMask(pWave->nChannels);
		pWaveEx->Samples.wValidBitsPerSample = pWave->wBitsPerSample;
		/*pWave->wFormatTag = (WORD) pWaveEx->SubFormat.Data1;*/
		chg=true;
	}
	if (chg)
	{
		hr = pInputProps->SetMediaType(pType); TEST(hr);
		hr = pWriter->SetInputProps(0, pInputProps); TEST(hr);
	}

	{ DWORD dwFormatCount = 0; hr = pWriter->GetInputFormatCount(0, &dwFormatCount); TEST(hr); TESTB(dwFormatCount > 0); }
	// GetInputFormatCount failed previously for multichannel formats, before ...mask = guessChannelMask() added. Leave this check j.i.c.

	sampleRate = wav.sampleRate;
	sampleSize = wav.nChannels * (wav.bitsPerSample/8);
	return true;
}

#define TESTR(hr) if (FAILED(hr)) return false

bool WMAencoder::getCodecFormat(CComPtr<IWMCodecInfo3> pCodecInfo3, WMAcodec codec, bool isVBR, int numPasses, int qb, Wformat& req, DWORD& targetCodec, DWORD& targetFormat)
{
	DWORD dwSize;
	Wformat bestMatch = {0, 0, 0};
	Wformat curr = bestMatch;
	targetCodec = targetFormat = INVALID_INDEX;

	BOOL fIsVBR = isVBR ? TRUE : FALSE;  DWORD dwNumPasses = (DWORD)numPasses;

	DWORD dwCodecCount;
	hr = pCodecInfo3->GetCodecInfoCount(WMMEDIATYPE_Audio, &dwCodecCount); TESTR(hr);

	for(DWORD dwCodecIndex = 0; dwCodecIndex < dwCodecCount; dwCodecIndex++)
	{
		if (codec != WMAvoice)
		{
			hr = pCodecInfo3->SetCodecEnumerationSetting(WMMEDIATYPE_Audio, dwCodecIndex, g_wszVBREnabled, WMT_TYPE_BOOL, (const BYTE*) &fIsVBR, sizeof(BOOL));
			if (FAILED(hr)) continue;
			hr = pCodecInfo3->SetCodecEnumerationSetting(WMMEDIATYPE_Audio, dwCodecIndex, g_wszNumPasses, WMT_TYPE_DWORD, (const BYTE*) &dwNumPasses, sizeof(DWORD));
			if (FAILED(hr)) continue;
		}
		
		DWORD dwCodecFormatCount;
		hr = pCodecInfo3->GetCodecFormatCount(WMMEDIATYPE_Audio, dwCodecIndex, &dwCodecFormatCount); TESTR(hr);

		for(DWORD dwFormatIndex = 0; dwFormatIndex < dwCodecFormatCount; dwFormatIndex++)
		{
/**/		CComPtr<IWMStreamConfig> pStreamConfig;
			hr = pCodecInfo3->GetCodecFormat(WMMEDIATYPE_Audio, dwCodecIndex, dwFormatIndex, &pStreamConfig); TESTR(hr);
/**/		CComPtr<IWMMediaProps> pProps;
			hr = pStreamConfig->QueryInterface<IWMMediaProps>(&pProps); TESTR(hr);
			hr = pProps->GetMediaType(NULL, &dwSize); TESTR(hr);
			XWmMediaType pType(dwSize);
			hr = pProps->GetMediaType(pType, &dwSize); TESTR(hr);
			WAVEFORMATEX * pWave = NULL;
			if (pType->cbFormat >= sizeof(WAVEFORMATEX))
				pWave = (WAVEFORMATEX*)pType->pbFormat;
			if (pWave == NULL) break;

			if (pWave->wFormatTag != (WORD)codec) break;

			/* We found proper codec. Let's find format */
			targetCodec = dwCodecIndex;

			int formatQ = 0;
			if ((pWave->nAvgBytesPerSec & 0x7FFFFF00) == 0x7FFFFF00)
				formatQ = pWave->nAvgBytesPerSec & 0x000000FF;
			else
				formatQ = (pWave->nAvgBytesPerSec*8 + 100)/1000;

			if (qb != formatQ) // bitrate/quality is not equal to requested
				continue;

			curr.sampleRate = pWave->nSamplesPerSec;
			curr.nChannels = pWave->nChannels;
			curr.bitsPerSample = pWave->wBitsPerSample;

			if (curr.sampleRate == req.sampleRate && curr.nChannels == req.nChannels && curr.bitsPerSample == req.bitsPerSample)
			{
				targetFormat = dwFormatIndex; bestMatch = curr;
				break; // found
			}

			if (codec == WMAlossless)
			{
				if (curr.sampleRate != req.sampleRate || curr.bitsPerSample < req.bitsPerSample)
					continue;
				if (curr.nChannels != req.nChannels && (curr.nChannels != 2 || req.nChannels != 1)) //allow 1ch->2ch upconvert
					continue;
			}
					
			if (bestMatch.sampleRate == req.sampleRate && curr.sampleRate != req.sampleRate)
				continue;				
			else if (bestMatch.sampleRate < req.sampleRate)
			{
				if (curr.sampleRate < bestMatch.sampleRate) continue;
				if (curr.sampleRate > bestMatch.sampleRate)
					{ targetFormat = dwFormatIndex; bestMatch = curr; }
			}
			else // bestMatch.sampleRate > req.sampleRate
			{
				if (curr.sampleRate < req.sampleRate) continue;
				if (curr.sampleRate < bestMatch.sampleRate)
					{ targetFormat = dwFormatIndex; bestMatch = curr; }
			}

			if (bestMatch.nChannels == req.nChannels && curr.nChannels != req.nChannels)
				continue;
			else if (bestMatch.nChannels < req.nChannels)
			{
				if (curr.nChannels < bestMatch.nChannels) continue;
				if (curr.nChannels > bestMatch.nChannels)
				{
					if (curr.nChannels > req.nChannels && (curr.nChannels != 2 || req.nChannels != 1))
						continue;
					{ targetFormat = dwFormatIndex; bestMatch = curr; }
				}
			}
			else // bestMatch.nChannels > req.nChannels : m=2, req=1
			{
				assert(bestMatch.nChannels == 2 && req.nChannels == 1);
				if (curr.nChannels == 1)
					{ targetFormat = dwFormatIndex; bestMatch = curr; }
				else if (curr.nChannels != 2)
					continue;
			}

			if (bestMatch.bitsPerSample == req.bitsPerSample && curr.bitsPerSample != req.bitsPerSample)
				continue;
			else if (bestMatch.bitsPerSample < req.bitsPerSample)
			{
				if (curr.bitsPerSample < bestMatch.bitsPerSample) continue;
				if (curr.bitsPerSample > bestMatch.bitsPerSample)
					{ targetFormat = dwFormatIndex; bestMatch = curr; }
			}
			else // bestMatch.bitsPerSample < req.bitsPerSample
			{
				if (curr.bitsPerSample < req.bitsPerSample) continue;
				if (curr.bitsPerSample < bestMatch.bitsPerSample)
					{ targetFormat = dwFormatIndex; bestMatch = curr; }
			}

		}//iterate dwFormatIndex
		
		if (targetCodec != INVALID_INDEX) break; // already found requested codec, no need to see others.
	}//iterate dwCodecIndex
	
	if (targetCodec == INVALID_INDEX || targetFormat == INVALID_INDEX)
		return false;

	req = bestMatch;
	return true;
}


void WMAencoder::start(const WCHAR * filename)
{
	hr = pWriter->SetOutputFilename(filename); TEST(hr);
	hr = pWriter->BeginWriting(); TEST(hr);
	sampleTime = 0;
}

void WMAencoder::close()
{
	hr = pWriter->EndWriting(); TEST(hr);
	pWriter.Release();
}

void WMAencoder::startPreprocess()
{
	hr = pWriter->QueryInterface<IWMWriterPreprocess>(&pWriterPreprocess); TEST(hr);
	DWORD maxPasses=0;
	hr = pWriterPreprocess->GetMaxPreprocessingPasses(0, 0, &maxPasses); TEST(hr);
	hr = pWriterPreprocess->SetNumPreprocessingPasses(0, 0, 1); TEST(hr);
	hr = pWriterPreprocess->BeginPreprocessingPass(0, 0); TEST(hr);
	sampleTime = 0;
}

void WMAencoder::endPreprocess()
{
	hr = pWriterPreprocess->EndPreprocessingPass(0, 0); TEST(hr);
	pWriterPreprocess.Release();
	sampleTime = 0;
}

void WMAencoder::xwrite(const void* arr, DWORD nSamples, bool preproc)
{
/**/CComPtr<INSSBuffer> pBuff;
	BYTE* buff = NULL;
	DWORD len = nSamples*sampleSize * (convert1chto2?2:1);

	hr = pWriter->AllocateSample(len, &pBuff); TEST(hr);
	hr = pBuff->GetBuffer(&buff); TEST(hr);
	if (convert1chto2)
	{
		for(size_t i=0; i<nSamples; i++)
		{
			for(size_t b=0; b<sampleSize; b++)
				buff[b+2*sampleSize*i] = buff[b+sampleSize+2*sampleSize*i] = ((BYTE*)arr)[b+sampleSize*i];
		}
	}
	else
		memcpy(buff, arr, len);
	hr = pBuff->SetLength(len); TEST(hr);
	
	if (preproc)
		hr = pWriterPreprocess->PreprocessSample(0, (QWORD)(sampleTime+0.5), 0, pBuff);
	else
		hr = pWriter          ->     WriteSample(0, (QWORD)(sampleTime+0.5), 0, pBuff);
	TEST(hr);
	
	sampleTime += nSamples*10000000.0/sampleRate;
}

bool WMAencoder::addTag(const WCHAR* name, const WCHAR* value)
{
	if (name==NULL || value == NULL) return false;
	{
		CComPtr<IWMHeaderInfo3> pHeaderInfo3;
		hr = pWriter->QueryInterface<IWMHeaderInfo3>(&pHeaderInfo3); TEST(hr);
		WORD index;
		hr = pHeaderInfo3->AddAttribute(0, name, &index, WMT_TYPE_STRING, 0, (const BYTE*)value, (DWORD)(sizeof(value[0])*wcslen(value)) );
		if (FAILED(hr)) return false;
	}
	return true;
}

#include <iostream>
using namespace std;

void WMAencoder::printFormats()
{
	HRESULT hr = S_OK;
	DWORD dwSize;

	XInit XCom;
	
/**/CComPtr<IWMProfileManager> pProfileManager;
	hr = WMCreateProfileManager(&pProfileManager); TEST(hr);
/**/CComPtr<IWMCodecInfo3> pCodecInfo3;
	hr = pProfileManager->QueryInterface<IWMCodecInfo3>(&pCodecInfo3); TEST(hr);

	DWORD dwCodecCount;
	hr = pCodecInfo3->GetCodecInfoCount(WMMEDIATYPE_Audio, &dwCodecCount); TEST(hr);
	
	for(DWORD dwCodecIndex = 0; dwCodecIndex < dwCodecCount; dwCodecIndex++)
	{
		hr = pCodecInfo3->GetCodecName(WMMEDIATYPE_Audio, dwCodecIndex, NULL, &dwSize); TEST(hr);
		std::vector<WCHAR> codecName(dwSize);
		hr = pCodecInfo3->GetCodecName(WMMEDIATYPE_Audio, dwCodecIndex, codecName.data(), &dwSize); TEST(hr);
		wcout << L"Formats for codec " << codecName.data() << ":" << endl;

		for(int i=0; i<=1; i++)
		{
			BOOL fIsVBR = (i==0) ? FALSE : TRUE;
			for(DWORD dwNumPasses=1; dwNumPasses<=2; dwNumPasses++)
			{
				hr = pCodecInfo3->SetCodecEnumerationSetting(WMMEDIATYPE_Audio, dwCodecIndex, g_wszVBREnabled, WMT_TYPE_BOOL, (BYTE*) &fIsVBR, sizeof(BOOL));
				//if (FAILED(hr)) then it's WMA Voice;
				hr = pCodecInfo3->SetCodecEnumerationSetting(WMMEDIATYPE_Audio, dwCodecIndex, g_wszNumPasses, WMT_TYPE_DWORD, (BYTE*) &dwNumPasses, sizeof(DWORD));
				//if (FAILED(hr)) then it's WMA Voice;
				DWORD dwCodecFormatCount;
				hr = pCodecInfo3->GetCodecFormatCount(WMMEDIATYPE_Audio, dwCodecIndex, &dwCodecFormatCount); TEST(hr);
				if (dwCodecFormatCount == 0) continue;
			
/**/			CComPtr<IWMStreamConfig> pStreamConfig;
				hr = pCodecInfo3->GetCodecFormat(WMMEDIATYPE_Audio, dwCodecIndex, 0, &pStreamConfig); TEST(hr);
/**/			CComPtr<IWMMediaProps> pProps;
				hr = pStreamConfig->QueryInterface<IWMMediaProps>(&pProps); TEST(hr);

				hr = pProps->GetMediaType(NULL, &dwSize); TEST(hr);
				XWmMediaType pType(dwSize);
				hr = pProps->GetMediaType(pType, &dwSize); TEST(hr);
				WAVEFORMATEX * pWave = NULL;
				if (pType->cbFormat >= sizeof(WAVEFORMATEX))
					pWave = (WAVEFORMATEX*)pType->pbFormat;

				if (pWave && pWave->wFormatTag == 0x0a && (fIsVBR || dwNumPasses > 1)) continue;

				wcout << (fIsVBR?L"VBR ":L"CBR ") << dwNumPasses << L"-pass mode" << endl;
				for(DWORD dwFormatIndex = 0; dwFormatIndex < dwCodecFormatCount; dwFormatIndex++)
				{
					hr = pCodecInfo3->GetCodecFormatDesc(WMMEDIATYPE_Audio, dwCodecIndex, dwFormatIndex, NULL, NULL, &dwSize); TEST(hr);
					std::vector<WCHAR> formatName(dwSize);
					hr = pCodecInfo3->GetCodecFormatDesc(WMMEDIATYPE_Audio, dwCodecIndex, dwFormatIndex, NULL, formatName.data(), &dwSize); TEST(hr);
					if (wcsstr(formatName.data(), L"(A/V)") == NULL) //we cannot encode to them anyway: they are masked by their counterparts
						wcout << "\t" << formatName.data() << endl;
				}// formatIndex
			}// numPasses
		}// isVBR
		wcout << endl;
	}// codecIndex
}
