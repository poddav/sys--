// -*- C++ -*-
//! \file        sysfs.cc
//! \date        Tue Aug 31 05:51:19 2010
//! \brief       filesystem manipulation functions definitions.
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

#include "sysfs.h"

namespace sys {

#if 0
string getcwd ()
{
    local_buffer<char> buf;
#ifdef _WIN32
    size_t ret = ::GetCurrentDirectoryA (buf.get(), buf.size());
    if (ret <= buf.size())
	return buf.get();

    buf.reserve (ret);
    if (::GetCurrentDirectoryA (buf.get(), buf.size()))
	return buf.get();
#else
    if (::getcwd (buf.get(), buf.size()))
	return buf.get();

    if (errno == ERANGE)
    {
	buf.reserve (1024);
	if (::getcwd (buf.get(), buf.size()))
	    return buf.get();
    }
#endif
    return string();
}
#endif

#ifndef _WIN32

namespace detail {

bool wgetcwd (wchar_t* buf, size_t buf_size)
{
    if (!buf_size)
	return false;

    local_buffer<char> cwd (buf_size * MB_LEN_MAX);
    if (!::getcwd (cwd.get(), cwd.size()))
	return false;

    wstring wcwd;
    if (!mbstowcs (cwd.get(), wcwd))
	return false;

    if (wcwd.size() >= buf_size)
	return false;

    ::wmemcpy (buf, wcwd.c_str(), wcwd.size()+1);
    return true;
}

} // namespace detail

#endif // _WIN32

} // namespace sys

