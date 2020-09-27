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

// Parse commandline

#include "stdafx.h"
#include "params.h"
#include "utils.h"


Params::Params()
{
	codec = c_undef;

	mode = m_undef;

	out_samplerate = 0;
	out_channels = 0;
	out_bitdepth = 0;

	quality = -1;
	bitrate = -1;

	ignorelength = false;
	bufferstdin = false;

	raw_input = false;
	raw_samplerate = 44100;
	raw_channels = 2;
	raw_bitdepth = 16;

	priority = -1;
	help = false;
	print_formats = false;
	silent = false;

	sourceName = destName = NULL;
	
	blocksize = 0;
}

Params::~Params()
{
	free(sourceName); free(destName);
}

enum options_tags
{
	tag_begin_unused = 500,

	//tag_codec,
	//tag_mode,
	tag_outsamplerate,
	tag_outchannels,
	tag_outbitdepth,
	//tag_ignorelength,
	tag_bufferstdin,
	tag_priority,
	tag_printformats,
	//tag_silent,

	tag_rawinput,
	tag_rawsamplerate,
	tag_rawchannels,
	tag_rawbitdepth,

	tag_blocksize,

	tag_meta_Title,
	tag_meta_TrackNumber,
	tag_meta_Author,
	tag_meta_AlbumTitle,
	tag_meta_Year,
	tag_meta_Genre,
	tag_meta_Composer,
	tag_meta_AlbumArtist,
	tag_meta_DiscNumber,
	tag_meta_Comment,
	tag_meta_Tag,
};


static const struct option long_options[] =
{
	{_T("codec"),			required_argument,	0,	_T('c')},
	{_T("mode"),			required_argument,	0,	_T('m')},

	{_T("out-samplerate"),	required_argument,	0,	tag_outsamplerate},
	{_T("out-channels"),	required_argument,	0,	tag_outchannels},
	{_T("out-bitdepth"),	required_argument,	0,	tag_outbitdepth},

	{_T("quality"),			required_argument,	0,	_T('q')},
	{_T("bitrate"),			required_argument,	0,	_T('b')},

	{_T("ignorelength"),	no_argument,		0,	_T('i')},
	{_T("bufferstdin"),		no_argument,		0,	tag_bufferstdin},

	{_T("priority"),		no_argument,		0,	tag_priority},
	{_T("nice"),			no_argument,		0,	tag_priority},
	{_T("help"),			no_argument,		0,	_T('h')},
	{_T("print-formats"),	no_argument,		0,	tag_printformats},
	{_T("formats"),			no_argument,		0,	tag_printformats},
	{_T("silent"),			no_argument,		0,	_T('s')},

	{_T("raw"),				no_argument,		0,	tag_rawinput},
	{_T("raw-samplerate"),	required_argument,	0,	tag_rawsamplerate},
	{_T("raw-channels"),	required_argument,	0,	tag_rawchannels},
	{_T("raw-bitdepth"),	required_argument,	0,	tag_rawbitdepth},

	{_T("blocksize"),		required_argument,	0,	tag_blocksize},

	{_T("title"),			required_argument,	0,	tag_meta_Title},
	{_T("tracknumber"),		required_argument,	0,	tag_meta_TrackNumber},
	{_T("artist"),			required_argument,	0,	tag_meta_Author},
	{_T("album"),			required_argument,	0,	tag_meta_AlbumTitle},
	{_T("year"),			required_argument,	0,	tag_meta_Year},
	{_T("genre"),			required_argument,	0,	tag_meta_Genre},
	{_T("composer"),		required_argument,	0,	tag_meta_Composer},
	{_T("albumartist"),		required_argument,	0,	tag_meta_AlbumArtist},
	{_T("discnumber"),		required_argument,	0,	tag_meta_DiscNumber},
	{_T("comment"),			required_argument,	0,	tag_meta_Comment},
	{_T("tag"),				required_argument,	0,	tag_meta_Tag},

	{0,						0,					0,	0}
};

const _TCHAR* options =   _T("hisq:b:c:m:");


void Params::parse(int argc, _TCHAR* argv[])
{
	for(;;)
	{
		int c;
		int index = 0;
		
		c = getopt_long(argc, argv, options, long_options, &index);
		
		if (c == -1)
			break;

		switch(c)
		{
		case 0:
			/*cannot be here*/
			break;

		case 'c':
			if      (_tcscmp(optarg, _T("voice")) == 0)        codec = voice;
			else if (_tcscmp(optarg, _T("std")) == 0)          codec = standard;
			else if (_tcscmp(optarg, _T("pro")) == 0)          codec = professional;
			else if (_tcscmp(optarg, _T("lsl")) == 0)          codec = lossless;
			else if (_tcscmp(optarg, _T("standard")) == 0)     codec = standard;
			else if (_tcscmp(optarg, _T("professional")) == 0) codec = professional;
			else if (_tcscmp(optarg, _T("lossless")) == 0)     codec = lossless;
			else throw myexc(_T("Invalid codec"), ERROR_INCORRECT_ARGS);
			break;
		case 'm':
			if      (_tcscmp(optarg, _T("cbr")) == 0)      mode = cbr;
			else if (_tcscmp(optarg, _T("vbr")) == 0)      mode = vbr;
			else if (_tcscmp(optarg, _T("cbr2pass")) == 0) mode = cbr2pass;
			else if (_tcscmp(optarg, _T("vbr2pass")) == 0) mode = abr2pass;
			else if (_tcscmp(optarg, _T("cbr2")) == 0)     mode = cbr2pass;
			else if (_tcscmp(optarg, _T("vbr2")) == 0)     mode = abr2pass;
			else throw myexc(_T("Invalid mode"), ERROR_INCORRECT_ARGS);
			break;

		case tag_outsamplerate:
			out_samplerate = _ttoi(optarg);
			break;
		case tag_outchannels:
			out_channels = _ttoi(optarg);
			break;
		case tag_outbitdepth:
			out_bitdepth = _ttoi(optarg);
			break;

		case 'q':
			quality = _ttoi(optarg);
			break;
		case 'b':
			bitrate= _ttoi(optarg);
			break;

		case 'i':
			ignorelength = true;
			break;
		case tag_bufferstdin:
			bufferstdin = true;
			break;

		case tag_priority:
			priority = 0;
			break;
		case 'h':
			help = true;
			break;
		case tag_printformats:
			print_formats = true;
			break;
		case 's':
			silent = true;
			break;

		case tag_rawinput:
			raw_input = true;
			break;
		case tag_rawsamplerate:
			raw_samplerate = _ttoi(optarg);
			break;
		case tag_rawchannels:
			raw_channels = _ttoi(optarg);
			break;
		case tag_rawbitdepth:
			raw_bitdepth = _ttoi(optarg);
			break;

		case tag_blocksize:
			blocksize = _ttoi(optarg);
			break;

		case tag_meta_Title:
			Tags.push_back(wma_tag(L"Title", optarg));
			break;
		case tag_meta_TrackNumber:
			Tags.push_back(wma_tag(L"WM/TrackNumber", optarg));
			break;
		case tag_meta_AlbumTitle:
			Tags.push_back(wma_tag(L"WM/AlbumTitle", optarg));
			break;
		case tag_meta_Year:
			Tags.push_back(wma_tag(L"WM/Year", optarg));
			break;
		case tag_meta_DiscNumber:
			Tags.push_back(wma_tag(L"WM/PartOfSet", optarg));
			break;
		case tag_meta_Comment:
			Tags.push_back(wma_tag(L"Description", optarg));
			break;

		case tag_meta_Author:								// * - multiple attr. allowed
			Tags.push_back(wma_tag(L"Author", optarg));
			break;
		case tag_meta_Genre:								// *
			Tags.push_back(wma_tag(L"WM/Genre", optarg));
			break;
		case tag_meta_Composer:								// *
			Tags.push_back(wma_tag(L"WM/Composer", optarg));
			break;
		case tag_meta_AlbumArtist:							// *
			Tags.push_back(wma_tag(L"WM/AlbumArtist", optarg));
			break;

		case tag_meta_Tag:
		{
			_TCHAR* p = _tcschr(optarg, L'=');
			if (p && p != optarg)
			{
				std::wstring n(optarg, p);
				std::wstring v(p+1);
				Tags.push_back(wma_tag(n, v));
			}
			else
				Tags.push_back(wma_tag(optarg, L""));
			break;
		}

		case '?': /* For unrecognized options, or options missing arguments, `optopt' is set to the option letter, and '?' is returned. */
			break;
		default: /* printf("?? getopt returned character code 0%o ??\n", c); */
			break;
		}
	}

	if (optind < argc)
	{
		/*non-option ARGV-elements*/

		sourceName = _tcsdup(argv[optind]);
		if (sourceName == NULL) throw myexc(_T("Cannot allocate memory"), ERROR_ALLOCATION);
		if (optind+1 < argc)
		{
			destName = _tcsdup(argv[optind+1]);
			if (destName == NULL) throw myexc(_T("Cannot allocate memory"), ERROR_ALLOCATION);
		}
	}

	if (sourceName != NULL && _tcscmp(sourceName, _T(STDINFILE)) != 0 && destName == NULL)
	{
		_TCHAR* append = _T(".wma");
		size_t pos = _tcslen(sourceName);
		if (pos > 4 && _tcscmp(sourceName+pos-4, _T(".wav")) == 0)
			pos-=4;
		size_t len = pos + _tcslen(append) + 1;
		destName = (_TCHAR*) malloc(len * sizeof(*destName));
		if (destName == NULL) throw myexc(_T("Cannot allocate memory"), ERROR_ALLOCATION);

		_tcsncpy_s(destName, len, sourceName, pos);		/* out = "file" */
		_tcscpy_s(destName + pos, len - pos, append);	/* out = "file.wma" */
	}
}

