// -*- C++ -*-
//! \file       clipboard.hpp
//! \date       Mon Sep 17 13:10:29 2007
//! \brief      windows clipboard access wrappers.
//
// Copyright (C) 2007 by poddav
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

#ifndef SYS_CLIPBOARD_HPP
#define SYS_CLIPBOARD_HPP

#include "syserror.h"
#include "winmem.hpp"
#include <cstring>	// for std::memcpy
#include <cwchar>   	// for std::wmemcpy

namespace sys {

class clipboard
{
public:

    template <class T> struct traits { };

    explicit clipboard (HWND new_owner = 0)
	{
	    if (!::OpenClipboard (new_owner))
		throw sys::generic_error();
	}

    ~clipboard () { ::CloseClipboard(); }

    void clear () { ::EmptyClipboard(); }

    HANDLE get_data (unsigned format) { return ::GetClipboardData (format); }

    bool set_data (unsigned format, mem::global& gmem)
	{
	    bool rc = ::SetClipboardData (format, gmem.handle());
	    if (rc) gmem.release();
	    return rc;
	}

    bool set_locale (LCID locid = LOCALE_USER_DEFAULT)
	{
	    mem::global gmem (GMEM_MOVEABLE|GMEM_DDESHARE, sizeof(LCID));
	    mem::lock<LCID> ptr (gmem);
	    *ptr = locid;
	    return set_data (CF_LOCALE, gmem);
       	}

    template <typename char_type>
    bool set_text (const char_type* data, size_t size);

    template <typename string_type>
    bool set_text (const string_type& str)
	{ return set_text (str.data(), str.size()); }

    template <class string_type>
    bool get_text (string_type& str);
};

template<> struct clipboard::traits<char>
{
    static unsigned format () { return CF_TEXT; }

    static char* copy (char* dst, const char* src, size_t n)
	{ return static_cast<char*> (std::memcpy (dst, src, n)); }
};

template<> struct clipboard::traits<wchar_t>
{
    static unsigned format () { return CF_UNICODETEXT; }

    static wchar_t* copy (wchar_t* dst, const wchar_t* src, size_t n)
	{ return std::wmemcpy (dst, src, n); }
};

template <typename char_type> bool clipboard::
set_text (const char_type* data, size_t size)
{
    mem::global gmem (GMEM_MOVEABLE|GMEM_DDESHARE, (size+1)*sizeof(char_type));
    mem::lock<char_type> ptr (gmem);
    traits<char_type>::copy (ptr.get(), data, size);
    ptr[size] = 0;
    return this->set_data (traits<char_type>::format(), gmem);
}

template <class string_type> bool clipboard::
get_text (string_type& str)
{
    typedef typename string_type::value_type	char_type;
    str.clear();
    bool ret = false;
    if (HANDLE hdata = this->get_data (traits<char_type>::format()))
    {
	if (size_t size = ::GlobalSize (hdata) / sizeof(char_type))
	{
	    mem::lock<char_type> ptr (hdata);
	    if (ptr[size-1] == 0)
	       	--size; // do not copy terminating null character
	    str.assign (ptr.get(), size);
	}
	ret = true;
    }
    return ret;
}

} // namespace sys

#endif /* SYS_CLIPBOARD_HPP */
