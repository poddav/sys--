// -*- C++ -*-
//! \file        sysstring.cc
//! \date        Tue Sep 07 08:54:57 2010
//! \brief       multibyte/wide string conversion functions implementation.
//
// Copyright (C) 2010 by poddav
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to
// deal in the Software without restriction, including without limitation the
// rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
// sell copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
// FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
// IN THE SOFTWARE.
//

#include <cstdlib>
#include "sysstring.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace sys {

#ifdef _WIN32

namespace detail {

DLLIMPORT int wcstombs (const wstring& wstr, string& cstr, unsigned codepage)
{
    cstr.clear();
    if (wstr.empty()) return 0;

    // loosely attempt to predict output size
    local_buffer<char> cbuf (wstr.size() * 2);
    int count = ::WideCharToMultiByte (codepage, 0, wstr.data(), wstr.size(),
				       cbuf.get(), cbuf.size(), 0, 0);
    if (!count)
    {
	int err = ::GetLastError();
	if (err != ERROR_INSUFFICIENT_BUFFER)
	    return 0;
	count = ::WideCharToMultiByte (codepage, 0, wstr.data(), wstr.size(),
				       cbuf.get(), 0, 0, 0);
	cbuf.reserve (count);
	count = ::WideCharToMultiByte (codepage, 0, wstr.data(), wstr.size(),
				       cbuf.get(), cbuf.size(), 0, 0);
	if (!count) return 0;
    }
    cstr.assign (cbuf.get(), count);
    return std::min (wstr.size(), cstr.size());
}

DLLIMPORT int mbstowcs (const string& cstr, wstring& wstr, unsigned codepage)
{
    wstr.clear();
    if (cstr.empty()) return 0;

    // loosely attempt to predict output size
    local_buffer<wchar_t> wbuf (cstr.size());
    int count = ::MultiByteToWideChar (codepage, 0, cstr.data(), cstr.size(),
				       wbuf.get(), wbuf.size());
    if (!count)
    {
	int err = ::GetLastError();
	if (err != ERROR_INSUFFICIENT_BUFFER)
	    return 0;
	count = ::MultiByteToWideChar (codepage, 0, cstr.data(), cstr.size(),
				       wbuf.get(), 0);
	wbuf.reserve (count);
	count = ::MultiByteToWideChar (codepage, 0, cstr.data(), cstr.size(),
				       wbuf.get(), wbuf.size());
	if (!count) return 0;
    }
    wstr.assign (wbuf.get(), count);
    return wstr.size();
}

} // namespace detail

#else // _WIN32

DLLIMPORT int wcstombs (const wstring& wstr, string& cstr)
{
    mbstate_t mbstate;
    std::memset (&mbstate, 0, sizeof(mbstate));

    local_buffer<char> chbuf (MB_CUR_MAX);
    cstr.clear();
    int count = 0;

    for (wstring::const_iterator in = wstr.begin(); in != wstr.end(); ++in)
    {
	int len = wcrtomb (chbuf.get(), *in, &mbstate);
	if (!len) break;
	if (len > 0)
	{
	    cstr.append (chbuf.get(), len);
	    ++count;
	}
    }
    return count;
}

DLLIMPORT int mbstowcs (const string& cstr, wstring& wstr)
{
    mbstate_t mbstate;
    std::memset (&mbstate, 0, sizeof(mbstate));

    wstr.clear();
    int count = 0;
    const char* in = cstr.c_str();

    for (int insize = cstr.size(); insize > 0; )
    {
	wchar_t wch;
	int len = mbrtowc (&wch, in, insize, &mbstate);
	if (!len) break;
	if (len > 0)
	{
	    wstr.push_back (wch);
	    ++count;
	}
	else
	    len = 1;
	insize -= len;
	in += len;
    }
    return count;
}

#endif // _WIN32

DLLIMPORT size_t mbslen (const char* str)
{
    size_t len = 0;
    for (; *str; ++str)
    {
	if ((*str & 0xc0) != 0x80) ++len;
    }
    return len;
}

DLLIMPORT size_t mbslen (const char* str, size_t byte_len)
{
    size_t len = 0;
    for (; byte_len; --byte_len)
    {
	if ((*str++ & 0xc0) != 0x80) ++len;
    }
    return len;
}

} // namespace sys
