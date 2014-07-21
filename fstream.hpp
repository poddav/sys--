// -*- C++ -*-
//! \file       fstream.hpp
//! \date       Wed May 23 14:41:42 2007
//! \brief      system file streams i/o interface.
//
// C++ streams interface to low level system i/o functions.
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

#ifndef SYS_FSTREAM_HPP
#define SYS_FSTREAM_HPP

#include "sysdef.h"
#include "sysio.h"	// for sys::io constants
#include "sysstring.h"
#include <streambuf>	// for std::streambuf
#include <iostream>	// for std::istream and std::ostream
#include <cstdio>	// for BUFSIZ

#if defined(_WIN32) && !defined(SYSPP_FSTREAM_TEXT_MODE)
#define SYSPP_FSTREAM_TEXT_MODE	1
#endif

namespace sys {

// ---------------------------------------------------------------------------
// sys::filebuf class

class SYSPP_DLLIMPORT filebuf : public std::streambuf
{
public: // types

    typedef char				char_type;
    typedef std::char_traits<char_type>		traits_type;
    typedef traits_type::int_type 		int_type;
    typedef traits_type::pos_type 		pos_type;
    typedef traits_type::off_type 		off_type;

    typedef std::streambuf			streambuf_type;

    static const size_t		default_bufsize = BUFSIZ;

public: // methods

    filebuf () : m_handle (), m_mode (std::ios::openmode(0)),
		 m_buf (0), m_buf_size (0), m_cur_gsize (), m_buf_allocated (false)
	{ }
    virtual ~filebuf ();

    bool is_open () const { return m_handle.valid(); }

    template<typename CharT>
    filebuf* open (const CharT* filename, std::ios::openmode mode,
		   sys::io::win_createmode ex_mode = sys::io::open_default,
		   sys::io::win_sharemode share = sys::io::share_default);

    template<typename CharT>
    filebuf* open (const basic_string<CharT>& filename, std::ios::openmode mode,
		   sys::io::win_createmode ex_mode = sys::io::open_default,
		   sys::io::win_sharemode share = sys::io::share_default)
	{ return open (filename.c_str(), mode, ex_mode, share); }

    filebuf* close ();

    // handle()
    //
    // Returns: underlying system file handle.

    sys::raw_handle handle () const { return m_handle; }

protected: // virtual methods

    virtual int_type overflow (int_type c);
    virtual int_type pbackfail (int_type c);
    virtual int_type underflow ();

    virtual streambuf_type* setbuf (char_type* buf, std::streamsize n);

    virtual pos_type seekoff (off_type off, std::ios::seekdir way,
			      std::ios::openmode);
    virtual pos_type seekpos (pos_type pos, std::ios::openmode mode);

    virtual std::streamsize xsgetn (char_type* buf, std::streamsize size);
    virtual std::streamsize xsputn (const char_type* buf, std::streamsize size);

    virtual int sync () { return m_sync(); }

private: // methods

    // initialize buffer pointers
    //
    void m_init ()
	{
	    m_cur_gsize = 0;
	    setg (m_buf, m_buf, m_buf);

	    if (m_mode & std::ios::out)
		setp (m_buf, m_buf + m_buf_size);
	    else
		setp (0, 0);
	}

    // flush internal buffer
    // RETURNS: zero on success, -1 otherwise
    //
    int m_sync ()
	{
	    int rc = 0;
	    if (std::streamsize out_buffered = pptr() - pbase())
	    {
		if (m_writefile (pbase(), out_buffered) != out_buffered)
		    rc = -1;
		m_init();
	    }
	    else if (m_mode & std::ios::in)
		m_flush_input();
	    return rc;
	}

    void m_flush_input ()
	{
	    if (std::streamsize buffered = m_input_size())
	    {
#if SYSPP_FSTREAM_TEXT_MODE
		if (m_mode & std::ios::binary)
		    m_seek (-buffered, std::ios::cur);
		else
		    m_flush_text();
#else
		m_seek (-buffered, std::ios::cur);
#endif
	    }
	    m_cur_gsize = 0;
	    setg (m_buf, m_buf, m_buf);
	}

    // return size of unread input buffer
    //
    std::streamsize m_input_size () const
	{
	    std::streamsize buffered = egptr() - gptr();
	    if (eback() == &m_putback)
		buffered += m_cur_gsize;
	    return buffered;
	}

    // read characters from file
    //
    std::streamsize m_readfile (char* buf, std::streamsize size);

    // write characters to file
    //
    std::streamsize m_writefile (const char* buf, std::streamsize size);

#if SYSPP_FSTREAM_TEXT_MODE
    // text mode read/write methods
    //
    std::streamsize m_read_text (char* buf, std::streamsize size);
    std::streamsize m_write_text (const char* buf, std::streamsize size);
    void m_flush_text ();
#endif

    // seek file
    //
    off_type m_seek (off_type offset, std::ios::seekdir way)
	{ return sys::seek_file (m_handle, offset, way); }

private: // data

    file_handle			m_handle;
    std::ios::openmode		m_mode;
    char_type*			m_buf;
    size_t			m_buf_size;	// allocated buffer size
    std::streamsize		m_cur_gsize;	// size of input buffer area
    bool			m_buf_allocated;
    char_type			m_putback;
};

namespace detail {

// ---------------------------------------------------------------------------
// base_from_member wrapper

class fstream_base
{
protected:
    filebuf 	m_filebuf;

    fstream_base () : m_filebuf () { }

    bool is_open () const   { return m_filebuf.is_open(); }
    filebuf* rdbuf () const { return const_cast<filebuf*>(&m_filebuf); }
    sys::raw_handle handle () const { return m_filebuf.handle(); }
};

// ---------------------------------------------------------------------------
/// \class text_writer
///
/// \brief this class implements translation of new-lines into \r\n pairs in text
///        stream.

class SYSPP_DLLIMPORT text_writer
{
public:
    typedef char			char_type;
    typedef std::char_traits<char>	traits_type;

    explicit text_writer (raw_handle handle)
	: sys_write (handle)
	, out_end (text_buf + text_buf_size)
	{ }

    std::streamsize operator() (const char_type* buf, std::streamsize size);

private: // methods

    bool flush ();
    bool append (char_type chr)
	{
	    if (out_ptr == out_end)
		if (!flush()) return false;
	    *out_ptr++ = chr;
	    return true;
	}
    bool append (const char_type* str, size_t len);

private: // data

    static const size_t	text_buf_size = filebuf::default_bufsize*2;

    io::writer		sys_write;
    char_type		text_buf[text_buf_size]; // text translation buffer
    char_type* const	out_end;
    char_type*		out_ptr; // current position within translation buffer
    std::streamsize	written; // total bytes written
    int			newline_count; // counter for newlines in translation buffer
};

} // namespace detail

// ---------------------------------------------------------------------------
// fstream classes

class ifstream : private detail::fstream_base, public std::istream
{
public:
    typedef detail::fstream_base	private_base;

    ifstream () : private_base(), std::istream (&m_filebuf) { }

    template<typename CharT>
    explicit ifstream (const CharT* filename,
	       	std::ios::openmode mode = std::ios::in,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
	: private_base(), std::istream (&m_filebuf)
	{ open (filename, mode, ex_mode, share); }

    template<typename CharT>
    explicit ifstream (const basic_string<CharT>& filename,
	       	std::ios::openmode mode = std::ios::in,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
	: private_base(), std::istream (&m_filebuf)
	{ open (filename.c_str(), mode, ex_mode, share); }

    template<typename CharT>
    void open (const CharT* filename, std::ios::openmode mode = std::ios::in,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
    {
	if (m_filebuf.open (filename, mode|std::ios::in, ex_mode, share))
	    clear();
	else
	    setstate (std::ios::failbit);
    }

    template<typename CharT>
    void open (const basic_string<CharT>& filename, std::ios::openmode mode = std::ios::in,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
	{ open (filename.c_str(), mode, ex_mode, share); }

    void close ()
    {
	if (!m_filebuf.close())
	    setstate (std::ios::failbit);
    }

    using private_base::is_open;
    using private_base::rdbuf;
    using private_base::handle;
};

class ofstream : private detail::fstream_base, public std::ostream
{
public:
    typedef detail::fstream_base	private_base;

    ofstream () : private_base(), std::ostream (&m_filebuf) { }

    template<typename CharT>
    explicit ofstream (const CharT* filename,
	       	std::ios::openmode mode = std::ios::out,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
       	: private_base(), std::ostream (&m_filebuf)
	{ open (filename, mode, ex_mode, share); }

    template<typename CharT>
    explicit ofstream (const basic_string<CharT>& filename,
	       	std::ios::openmode mode = std::ios::out,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
       	: private_base(), std::ostream (&m_filebuf)
	{ open (filename.c_str(), mode, ex_mode, share); }

    template<typename CharT>
    void open (const CharT* filename, std::ios::openmode mode = std::ios::out,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
    {
	if (m_filebuf.open (filename, mode|std::ios::out, ex_mode, share))
	    clear();
	else
	    setstate (std::ios::failbit);
    }

    template<typename CharT>
    void open (const basic_string<CharT>& filename, std::ios::openmode mode = std::ios::out,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
	{ open (filename.c_str(), mode, ex_mode, share); }

    void close ()
    {
	if (!m_filebuf.close())
	    setstate (std::ios::failbit);
    }

    using private_base::is_open;
    using private_base::rdbuf;
    using private_base::handle;
};

class fstream : private detail::fstream_base, public std::iostream
{
public:
    typedef detail::fstream_base	private_base;

    fstream () : private_base(), std::iostream (&m_filebuf) { }

    template<typename CharT>
    explicit fstream (const CharT* filename,
	        std::ios::openmode mode = std::ios::in|std::ios::out,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
       	: private_base(), std::iostream (&m_filebuf)
	{ open (filename, mode, ex_mode, share); }

    template<typename CharT>
    explicit fstream (const basic_string<CharT>& filename,
	        std::ios::openmode mode = std::ios::in|std::ios::out,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
       	: private_base(), std::iostream (&m_filebuf)
	{ open (filename.c_str(), mode, ex_mode, share); }

    template<typename CharT>
    void open (const CharT* filename,
	        std::ios::openmode mode = std::ios::in|std::ios::out,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
    {
	if (m_filebuf.open (filename, mode|std::ios::in|std::ios::out, ex_mode, share))
	    clear();
	else
	    setstate (std::ios::failbit);
    }

    template<typename CharT>
    void open (const basic_string<CharT>& filename,
	        std::ios::openmode mode = std::ios::in|std::ios::out,
		sys::io::win_createmode ex_mode = sys::io::open_default,
		sys::io::win_sharemode share = sys::io::share_default)
	{ open (filename.c_str(), mode, ex_mode, share); }

    void close ()
    {
	if (!m_filebuf.close())
	    setstate (std::ios::failbit);
    }

    using private_base::is_open;
    using private_base::rdbuf;
    using private_base::handle;
};


// --- template and inline members implementation ----------------------------

template<typename CharT> filebuf* filebuf::
open (const CharT* filename, std::ios::openmode mode,
      sys::io::win_createmode ex_mode, sys::io::win_sharemode share)
{
    if (is_open()) return NULL;

    sys::io::sys_mode sys_mode;
    if (ex_mode)
    {
	sys::io::win_iomode io_mode (sys::io::generic_null);
	if (mode & std::ios::in) io_mode |= sys::io::generic_read;
	if (mode & std::ios::out) io_mode |= sys::io::generic_write;
	sys_mode = win_to_sys (io_mode, ex_mode);
    }
    else
	sys_mode = sys::io::ios_to_sys (mode);

    m_handle = create_file (filename, sys_mode, share);
    if (!m_handle)
	return NULL;

    if (!m_buf)
    {
	m_buf = new char_type[default_bufsize];
	m_buf_allocated = true;
	m_buf_size = default_bufsize;
    }
    m_mode = mode;
    m_init();

    if (mode & (std::ios::ate))
	m_seek (0, std::ios::end);

    return this;
}

#ifdef _WIN32

inline std::streamsize filebuf::
m_readfile (char_type* buf, std::streamsize size)
{
#if SYSPP_FSTREAM_TEXT_MODE
    if (!(m_mode & std::ios::binary))
	return m_read_text (buf, size);
#endif
    return sys::read_file (m_handle, buf, size);
}

inline std::streamsize filebuf::
m_writefile (const char_type* buf, std::streamsize size)
{
    if (m_mode & std::ios::app)
	m_seek (0, std::ios::end);
#if SYSPP_FSTREAM_TEXT_MODE
    if (!(m_mode & std::ios::binary))
	return m_write_text (buf, size);
#endif
    return sys::write_file (m_handle, buf, size);
}

#if SYSPP_FSTREAM_TEXT_MODE

// m_write_text (buf, size)
//
// Effects: translates all newline characters '\n' within input buffer 'buf'
// into character pairs "\r\n" and writes translation results into file.
// Returns: number of source characters successfully written.

inline std::streamsize filebuf::
m_write_text (const char_type* buf, std::streamsize size)
{
    detail::text_writer text_write (m_handle);
    return text_write (buf, size);
};

#endif /* SYSPP_FSTREAM_TEXT_MODE */

#else /* _WIN32 */

inline std::streamsize filebuf::
m_readfile (char_type* buf, std::streamsize size)
{
    return sys::read_file (m_handle, buf, size);
}

inline std::streamsize filebuf::
m_writefile (const char_type* buf, std::streamsize size)
{
    return sys::write_file (m_handle, buf, size);
}

#endif /* _WIN32 */

inline std::ostream& operator<< (std::ostream& lhs, const wstring& rhs)
{
    string cstr;
    if (wcstombs (rhs, cstr))
    	lhs.write (cstr.data(), cstr.size());
    else
	lhs.setstate (std::ios::failbit);
    return lhs;
}

inline std::istream& operator>> (std::istream& lhs, wstring& rhs)
{
    string cstr;
    if (lhs >> cstr)
	if (!mbstowcs (cstr, rhs))
	    lhs.setstate (std::ios::failbit);
    return lhs;
}

} // namespace sys

#endif /* SYS_FSTREAM_HPP */
