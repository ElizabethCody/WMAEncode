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
#include "xbuffer.h"
#include "utils.h"

void MemoryBuffer::init()
{
	head = NULL;

	currWR = NULL;
	currRD = NULL;

	availWR = 0;
	availRD = 0;
}

void MemoryBuffer::free()
{
	while(head)
	{
		buf* next = head->next;
		delete head;
		head = next;
	}
}

void MemoryBuffer::write(const void* src1, const size_t elementSize, const size_t count)
{
	const char* src = (const char*)src1;
	size_t nbytes = elementSize * count;

	while(nbytes)
	{
		if (availWR == 0)
		{
			buf* next = new buf;
			if (head == NULL)
			{
				head = next;
				currWR = head;
				currWR->next = NULL;
			}
			else
			{
				currWR->next = next;
				currWR = next;
				currWR->next = NULL;
			}
			availWR = N;
		}

		size_t toCurBuf = min(nbytes, availWR);

		memcpy(currWR->data + (N-availWR), src, toCurBuf);

		src += toCurBuf;
		nbytes -= toCurBuf;
		availWR -= toCurBuf;
	}
	written_samples += count;
}

size_t MemoryBuffer::read(void* dst1, const size_t elementSize, size_t count)
{
	char* dst = (char*)dst1;
	assert(read_samples <= written_samples);
	if (written_samples - read_samples < (__int64)count)
		count = (size_t) (written_samples - read_samples);
	size_t nbytes = elementSize * count;
	
	while(nbytes)
	{
		if (availRD == 0)
		{
			if (currRD == NULL)
				currRD = head;
			else
			{
#if 1
				currRD = head->next;
				delete head;
				head = currRD;
#else
				currRD = currRD->next;
#endif
			}
			availRD = N; // it is safe: we never read past written data.
		}

		size_t fromCurBuf = min(nbytes, availRD);
		memcpy(dst, currRD->data + (N-availRD), fromCurBuf);

		dst += fromCurBuf;
		nbytes -= fromCurBuf;
		availRD -= fromCurBuf;
	}
	read_samples += count;

	return count;
}


static _TCHAR tempName[MAX_PATH];
static _TCHAR tempPath[MAX_PATH];

static FILE* mytmpfile()
{
	DWORD ret;

	memset(tempPath, 0, sizeof(tempPath));
	memset(tempName, 0, sizeof(tempName));

	ret = ::GetTempPath(MAX_PATH-14, tempPath);
	if (ret == 0 || ret > MAX_PATH-14) return NULL;

	ret = ::GetTempFileName(tempPath, _T("wme"), 0, tempName);
	if (ret == 0) return NULL;

	HANDLE fh = CreateFile(tempName, GENERIC_READ|GENERIC_WRITE, 0, 0, CREATE_ALWAYS, FILE_ATTRIBUTE_TEMPORARY|FILE_FLAG_DELETE_ON_CLOSE, 0);
	if (fh == INVALID_HANDLE_VALUE) return NULL;

	int fd = _open_osfhandle(intptr_t(fh), _O_BINARY|_O_RDWR);
	if (fd == -1) { CloseHandle(fh); return NULL; }

	FILE* fp = _fdopen(fd, "w+");
	if (!fp) { _close(fd); return NULL; }

	return fp;
}

void FileBuffer::init()
{
	buf = mytmpfile();
	if(!buf) throw myexc(_T("Cannot create temporary file"), ERROR_SYSTEM);
}
