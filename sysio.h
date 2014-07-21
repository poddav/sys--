// -*- C++ -*-
//! \file       sysio.h
//! \date       Sat Jun 09 13:29:28 2007
//! \brief      low level system i/o wrappers.
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

#ifndef SYSPP_SYSIO_H
#define SYSPP_SYSIO_H

#include "syshandle.h"
#include "sysstring.h"
#include <ios>		// for std::ios
#include <utility>	// for std::pair
#include <fcntl.h>	// for POSIX io flags

#ifndef _WIN32

// define Win32 I/O flags

#define GENERIC_READ	0x80000000
#define GENERIC_WRITE	0x40000000

#define CREATE_NEW	1
#define CREATE_ALWAYS	2
#define OPEN_EXISTING	3
#define OPEN_ALWAYS	4
#define TRUNCATE_EXISTING	5

#define FILE_SHARE_READ		0x00000001
#define FILE_SHARE_WRITE	0x00000002
#define FILE_SHARE_DELETE	0x00000004

#endif

namespace sys {

size_t write_file (raw_handle file, const char* buf, size_t size);
size_t read_file (raw_handle file, char* buf, size_t size);
std::streamoff seek_file (raw_handle file, std::streamoff off, std::ios::seekdir dir);

namespace io {

    inline raw_handle in ()
    {
#ifdef _WIN32
	return ::GetStdHandle (STD_INPUT_HANDLE);
#else
	return STDIN_FILENO;
#endif
    }

    inline raw_handle out ()
    {
#ifdef _WIN32
	return ::GetStdHandle (STD_OUTPUT_HANDLE);
#else
	return STDOUT_FILENO;
#endif
    }

    inline raw_handle err ()
    {
#ifdef _WIN32
	return ::GetStdHandle (STD_ERROR_HANDLE);
#else
	return STDERR_FILENO;
#endif
    }

    // namespace for i/o mode constants and mode conversion functions

    enum win_iomode {
	generic_null	= 0,
	generic_read	= GENERIC_READ,
	generic_write	= GENERIC_WRITE,
	read_write	= GENERIC_READ|GENERIC_WRITE,
    };

    enum win_createmode {
	open_default	= 0,
	create_new	= CREATE_NEW,
	create_always	= CREATE_ALWAYS,
	open_existing	= OPEN_EXISTING,
	open_always	= OPEN_ALWAYS,
	truncate_existing = TRUNCATE_EXISTING,
    };

    enum win_sharemode {
	share_none	= 0,
	share_read	= FILE_SHARE_READ,
	share_write	= FILE_SHARE_WRITE,
	share_delete	= FILE_SHARE_DELETE,
	share_all	= share_read|share_write|share_delete,
	share_default	= share_all,
    };

    inline win_iomode operator| (win_iomode lhs, win_iomode rhs)
	{ return static_cast<win_iomode> (int(lhs)|int(rhs)); }
    inline win_iomode& operator|= (win_iomode& lhs, win_iomode rhs)
	{ return lhs = lhs|rhs; }

    inline win_sharemode operator| (win_sharemode lhs, win_sharemode rhs)
	{ return static_cast<win_sharemode> (int(lhs)|int(rhs)); }
    inline win_sharemode operator& (win_sharemode lhs, win_sharemode rhs)
	{ return static_cast<win_sharemode> (int(lhs)&int(rhs)); }
    inline win_sharemode operator^ (win_sharemode lhs, win_sharemode rhs)
	{ return static_cast<win_sharemode> (int(lhs)^int(rhs)); }
    inline win_sharemode operator~ (win_sharemode lhs)
	{ return static_cast<win_sharemode> (~int(lhs)); }
    inline win_sharemode& operator|= (win_sharemode& lhs, win_sharemode rhs)
	{ return lhs = lhs|rhs; }
    inline win_sharemode& operator&= (win_sharemode& lhs, win_sharemode rhs)
	{ return lhs = lhs&rhs; }
    inline win_sharemode& operator^= (win_sharemode& lhs, win_sharemode rhs)
	{ return lhs = lhs^rhs; }

    typedef int					    posix_mode;
    typedef std::pair<win_iomode, win_createmode>   win_mode;
    typedef std::ios::openmode		    ios_mode;

    inline win_iomode		win_io_mode (const win_mode& mode) { return mode.first; }
    inline win_createmode	win_create_mode (const win_mode& mode) { return mode.second; }

    // run-time mode conversion

    SYSPP_DLLIMPORT win_mode		posix_to_win (posix_mode);
    SYSPP_DLLIMPORT posix_mode	win_to_posix (win_mode);

    // compile-time mode conversion

    template<posix_mode mode>
    inline win_mode		posix_to_win ()
	{
	    enum {
		acc_mode = O_RDONLY|O_WRONLY|O_RDWR,
		io_mode = (mode & acc_mode) == O_WRONLY? generic_write
		        : (mode & acc_mode) == O_RDWR? read_write
			: generic_read,
		create_mode = ( mode & O_CREAT?
			        ( mode & O_EXCL? create_new
			        : mode & O_TRUNC? create_always
			        : open_always )
			      : mode & O_TRUNC? truncate_existing
			      : open_existing ),
	    };
	    return win_mode (win_iomode(io_mode), win_createmode(create_mode));
	}

    template<win_iomode io_mode, win_createmode create_mode>
    inline posix_mode		win_to_posix ()
	{
	    enum {
		mode = ( io_mode == generic_read? O_RDONLY
		       : io_mode == generic_write? O_WRONLY
		       : O_RDWR )
		     | ( create_mode == create_new? O_CREAT | O_EXCL
		       : create_mode == create_always? O_CREAT | O_TRUNC
		       : create_mode == open_always? O_CREAT
		       : create_mode == truncate_existing? O_TRUNC
		       : 0 )
	    };
	    return mode;
	}

#ifdef _WIN32
    typedef win_mode		sys_mode;

    inline sys_mode		posix_to_sys (posix_mode mode) { return posix_to_win (mode); }
    inline sys_mode		win_to_sys (win_iomode io_mode, win_createmode create_mode)
       	{ return win_mode (io_mode, create_mode); }

    template<posix_mode mode>
    inline sys_mode		posix_to_sys () { return posix_to_win<mode>(); }

    template<int mode>
    inline sys_mode		ios_to_sys ()
	{
	    using namespace std;
	    enum {
		rw_mode = int(ios::in)|int(ios::out),
		write_mode = int(ios::out)|int(ios::app)|int(ios::trunc),

		io_mode = ( (mode & rw_mode) == ios::in? generic_read
			  : (mode & rw_mode) == ios::out? generic_write
			  : read_write ),
		create_mode = ( mode & int(ios::trunc)? create_always
			      :	mode & write_mode? open_always
			      : open_existing )
	    };
	    return win_mode (win_iomode(io_mode), win_createmode(create_mode));
	}
#else
    typedef posix_mode		sys_mode;

    inline sys_mode		posix_to_sys (posix_mode mode) { return mode; }
    inline sys_mode		win_to_sys (win_iomode io_mode, win_createmode create_mode)
       	{ return win_to_posix (win_mode (io_mode, create_mode)); }

    template<posix_mode mode>
    inline sys_mode		posix_to_sys () { return mode; }

    template<int mode>
    inline sys_mode		ios_to_sys ()
	{
	    using namespace std;
	    enum {
		rw_mode = int(ios::in)|int(ios::out),
		write_mode = int(ios::out)|int(ios::app)|int(ios::trunc),

		pmode = ( (mode & rw_mode) == ios::in? O_RDONLY
			: (mode & rw_mode) == ios::out? O_WRONLY
			: O_RDWR )
		      | ( mode & ios::trunc? O_TRUNC
			: mode & write_mode? O_CREAT: 0 )
		      | ( mode & std::ios::app? O_APPEND: 0 )
	    };
	    return pmode;
	}
#endif

    SYSPP_DLLIMPORT sys_mode		ios_to_sys (ios_mode);

    // --- input/output functors ---------------------------------------------

    class SYSPP_DLLIMPORT writer
    {
    public:
	explicit writer (raw_handle handle) : m_handle (handle) { }

	std::streamsize operator() (const char* buf, size_t size)
	    { return write_file (m_handle, buf, size); }

    private:
	raw_handle		m_handle;
    };

    class SYSPP_DLLIMPORT reader
    {
    public:
	explicit reader (raw_handle handle) : m_handle (handle) { }

	std::streamsize operator() (char* buf, size_t size)
	    { return read_file (m_handle, buf, size); }

    private:
	raw_handle		m_handle;
    };

#ifdef _WIN32
    // --- 64-bit seek wrapper -----------------------------------------------

    namespace detail {

    template<bool streamoff_is_wide> inline std::streamoff
    seek (raw_handle handle, std::streamoff offset, int whence)
    {
	LARGE_INTEGER pos;
	pos.QuadPart = offset;
	pos.LowPart = ::SetFilePointer (handle, pos.LowPart, &pos.HighPart, whence);

	if (pos.LowPart == 0xffffffff && ::GetLastError() != NO_ERROR)
	    return std::streamoff (-1);

	return static_cast<std::streamoff> (pos.QuadPart);
    }

    template <> inline std::streamoff
    seek<false> (raw_handle handle, std::streamoff offset, int whence)
    {
	long pos = ::SetFilePointer (handle, static_cast<long> (offset), NULL, whence);
	return static_cast<std::streamoff> (pos);
    }

    } // namespace detail
#endif

} // namespace io

// --------------------------------------------------------------------------

// isatty (HANDLE)

inline bool isatty (raw_handle handle)
{
#ifdef _WIN32
    return ::GetFileType (handle) == FILE_TYPE_CHAR;
#else
    return ::isatty (handle);
#endif
}

raw_handle create_file (const char* filename, io::sys_mode mode,
	   		io::win_sharemode share = io::share_default);

raw_handle create_file (const WChar* filename, io::sys_mode mode,
	   		io::win_sharemode share = io::share_default);

// open_file -- alias for sys::create_file

template <typename CharT> inline raw_handle
open_file (const CharT* filename, io::sys_mode mode,
	   io::win_sharemode share = io::share_default)
{ return create_file (filename, mode, share); }

inline bool close_file (raw_handle file)
{ return detail::base_handle::close_handle (file); }

#ifdef _WIN32

inline raw_handle
create_file (const char* name, io::sys_mode flags, io::win_sharemode share)
{
    return ::CreateFileA (name, io::win_io_mode (flags), share, NULL,
			  io::win_create_mode (flags), FILE_ATTRIBUTE_NORMAL, NULL);
}

inline raw_handle
create_file (const WChar* name, io::sys_mode flags, io::win_sharemode share)
{
    return ::CreateFileW (name, io::win_io_mode (flags), share, NULL,
			  io::win_create_mode (flags), FILE_ATTRIBUTE_NORMAL, NULL);
}

inline size_t write_file (raw_handle file, const char* buf, size_t size)
{
    DWORD written;
    ::WriteFile (file, buf, size, &written, 0);
    return written;
}

inline size_t read_file (raw_handle file, char* buf, size_t size)
{
    DWORD read_bytes;
    ::ReadFile (file, buf, size, &read_bytes, 0);
    return read_bytes;
}

inline std::streamoff
seek_file (raw_handle file, std::streamoff off, std::ios::seekdir dir)
{
    return io::detail::seek<(sizeof(std::streamoff) > sizeof(DWORD))> (file, off, dir);
}

#else

inline raw_handle
create_file (const char* name, io::sys_mode flags, io::win_sharemode)
{
    return ::open (name, flags, 0666);
}

inline size_t write_file (raw_handle file, const char* buf, size_t size)
{
    int written = ::write (file, buf, size);
    return written > 0? written: 0;
}

inline size_t read_file (raw_handle file, char* buf, size_t size)
{
    int read_bytes = ::read (file, buf, size);
    return read_bytes > 0? read_bytes: 0;
}

inline std::streamoff
seek_file (raw_handle file, std::streamoff offset, std::ios::seekdir dir)
{
    return ::lseek (file, static_cast<off_t> (offset), static_cast<int> (dir));
}

#endif

} // namespace sys

#endif /* SYSPP_SYSIO_H */
