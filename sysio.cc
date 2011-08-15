// -*- C++ -*-
//! \file        sysio.cc
//! \date        Thu May 24 23:20:44 2007
//! \brief       low level system i/o wrappers implementation.
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

#include "sysio.h"
#include "sysstring.h"

#ifdef _WIN32
#include <windows.h>
#else
#include <cerrno>
#endif /* _WIN32 */

namespace sys {

io::posix_mode
io::win_to_posix (io::win_mode mode)
{
    posix_mode posix_mode;

    switch (win_io_mode (mode))
    {
    case generic_read:	posix_mode = O_RDONLY; break;
    case generic_write: posix_mode = O_WRONLY; break;
    default:		posix_mode = O_RDWR; break;
    }

    switch (win_create_mode (mode))
    {
    case create_new:		posix_mode |= O_CREAT | O_EXCL; break;
    case create_always:		posix_mode |= O_CREAT | O_TRUNC; break;
    case open_always:		posix_mode |= O_CREAT; break;
    case truncate_existing:	posix_mode |= O_TRUNC; break;
    default:
    case open_existing:		break;
    }
    
    return posix_mode;
}

io::win_mode
io::posix_to_win (io::posix_mode flags)
{
    const int acc_mode = O_RDONLY|O_WRONLY|O_RDWR;
    win_iomode io_mode;
    if ((flags & acc_mode) == O_WRONLY)
	io_mode = generic_write;
    else if ((flags & acc_mode) == O_RDWR)
	io_mode = generic_read|generic_write;
    else
	io_mode = generic_read;

    win_createmode create_mode;
    if (flags & O_CREAT)
	if (flags & O_EXCL)
	    create_mode = io::create_new;
	else if (flags & O_TRUNC)
	    create_mode = io::create_always;
	else
	    create_mode = io::open_always;
    else if (flags & O_TRUNC)
	create_mode = io::truncate_existing;
    else
	create_mode = io::open_existing;

    return win_mode (io_mode, create_mode);
}

#ifdef _WIN32

io::sys_mode
io::ios_to_sys (io::ios_mode ios_mode)
{
    const io::ios_mode write_mode = (std::ios::out|std::ios::app|std::ios::trunc);
   
    win_iomode desired_access (generic_null);

    if (ios_mode & std::ios::in)
	desired_access = generic_read;

    if (ios_mode & write_mode)
	desired_access |= generic_write;

    win_createmode create_mode;

    if (ios_mode & std::ios::trunc)
	create_mode = create_always;
    else if (ios_mode & write_mode)
	create_mode = open_always;
    else
	create_mode = open_existing;

    return win_mode (desired_access, create_mode);
}

#else

io::sys_mode
io::ios_to_sys (io::ios_mode ios_mode)
{
    const io::ios_mode write_mode = (std::ios::out|std::ios::app|std::ios::trunc);
    const io::ios_mode rw_mode = std::ios::in|std::ios::out;

    posix_mode posix_mode;
   
    if ((ios_mode & rw_mode) == std::ios::in)
	posix_mode = O_RDONLY;
    else if ((ios_mode & rw_mode) == std::ios::out)
	posix_mode = O_WRONLY;
    else if (ios_mode & rw_mode)
	posix_mode = O_RDWR;
    else
	posix_mode = 0;

    if (ios_mode & std::ios::trunc)
	posix_mode |= O_TRUNC;
    if (ios_mode & write_mode)
	posix_mode |= O_CREAT;

    if (ios_mode & std::ios::app)
	posix_mode |= O_APPEND;

    return posix_mode;
}

raw_handle
create_file (const WChar* name, io::sys_mode flags, io::win_sharemode)
{
    string cname;
    if (!wcstombs (name, cname))
    {
	errno = ENOENT;
	return file_handle::invalid_handle();
    }
    return ::open (cname.c_str(), flags, 0666);
}

#endif /* _WIN32 */

} // namespace sys
