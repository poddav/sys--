// -*- C++ -*-
//! \file      fstream.cc
//! \date      Wed May 23 15:34:17 2007
//! \brief     system file streams i/o implementation.
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

#include "fstream.hpp"

#if SYSPP_FSTREAM_TEXT_MODE
#include <algorithm>	// for std::count
#endif

#ifndef _WIN32
#include <sys/types.h>
#endif

namespace sys {

filebuf::
~filebuf ()
{
    close();
    if (m_buf_allocated) delete[] m_buf;
}

filebuf* filebuf::
close ()
{
    if (!is_open())
	return NULL;

    m_sync();
    bool rc = m_handle.close();
    m_mode = std::ios::openmode(0);

    return (rc ? this : NULL);
}

filebuf::streambuf_type* filebuf::
setbuf (char_type* buf, std::streamsize size)
{
    if (0 != m_input_size())
	return 0;

    m_sync();
    if (buf == 0 && size == 0 || buf != 0 && size > 0)
    {
	if (m_buf_allocated)
	{
	    delete[] m_buf;
	    m_buf_allocated = false;
	}
	m_buf = buf;
	m_buf_size = size;
    }
    m_init();
    return this;
}

std::streamsize filebuf::
xsgetn (char_type* buf, std::streamsize size)
{
    std::streamsize ret = 0;
    if (size && (m_mode & std::ios::in))
    {
	if (std::streamsize buffered = pptr() - pbase())
	    m_writefile (pbase(), buffered);
	setp (m_buf, m_buf);    // next put will cause overflow

	if (eback() == &m_putback)
	{
	    if (gptr() != egptr())
	    {
		*buf++ = *gptr();
		--size;
		++ret;
	    }
	    setg (m_buf, m_buf, m_buf + m_cur_gsize);
	}
	if (std::streamsize buffered = egptr() - gptr())
	{
	    if (size < buffered) buffered = size;
	    traits_type::copy (buf, gptr(), buffered);
	    gbump (buffered);
	    buf += buffered;
	    size -= buffered;
	    ret += buffered;
	}
	if (size > 0)
	{
	    if ((size_t)size < m_buf_size)
	    {
		m_cur_gsize = m_readfile (m_buf, m_buf_size);
		if (size > m_cur_gsize)
		    size = m_cur_gsize;
		setg (m_buf, m_buf+size, m_buf+m_cur_gsize);
		if (size)
		{
		    traits_type::copy (buf, eback(), size);
		    ret += size;
		}
	    }
	    else
	    {
		ret += m_readfile (buf, size);
		setg (m_buf, m_buf, m_buf);
		m_cur_gsize = 0;
	    }
	}
    }
    return ret;
}

filebuf::int_type filebuf::
underflow ()
{
    if (!(m_mode & std::ios::in))
	return traits_type::eof();

    if (gptr() < egptr())
	return traits_type::to_int_type (*gptr());

    if (eback() == &m_putback && m_cur_gsize)
    {
	setg (m_buf, m_buf, m_buf + m_cur_gsize);
	return traits_type::to_int_type (*gptr());
    }

    if (m_buf_size)
    {
	if (std::streamsize buffered = pptr() - pbase())
	    m_writefile (pbase(), buffered);
	setp (m_buf, m_buf);	// next put will cause overflow
	m_cur_gsize = m_readfile (m_buf, m_buf_size);
	setg (m_buf, m_buf, m_buf+m_cur_gsize);
	if (m_cur_gsize)
	    return traits_type::to_int_type (*gptr());
    }
    else
    {
	m_cur_gsize = 0;
	std::streamsize len = m_readfile (&m_putback, 1);
	setg (&m_putback, &m_putback, &m_putback + len);
	if (len)
	    return traits_type::to_int_type (m_putback);
    }
    return traits_type::eof();
}

filebuf::int_type filebuf::
pbackfail (int_type c)
{
    if ((m_mode & std::ios::in)
       	&& !traits_type::eq_int_type (c, traits_type::eof())
	&& pbase() == epptr())	// make sure last buffer operation wasn't output
    {
	if (gptr() > eback())
	{
	    gbump (-1);
	    *gptr() = traits_type::to_char_type (c);
	    return c;
	}
	else if (gptr() != &m_putback)
	{
	    m_putback = traits_type::to_char_type (c);
	    setg (&m_putback, &m_putback, &m_putback + 1);
	    return c;
	}
    }
    return traits_type::eof();
}

std::streamsize filebuf::
xsputn (const char_type* buf, std::streamsize size)
{
    std::streamsize ret = 0; // return value
    if (m_mode & std::ios::out)
    {
	// if sequence is too large for internal buffer, write it directly
	//
	if ((size_t)size > m_buf_size)
	{
	    if (m_sync() == 0)
		ret = m_writefile (buf, size);
	}
	else
	{
	    if (m_mode & std::ios::in)
		m_flush_input();
	    if (m_buf_size && pbase() == epptr())
		setp (m_buf, m_buf+m_buf_size);
	    ret = streambuf_type::xsputn (buf, size);
	}
    }
    return ret;
}

filebuf::int_type filebuf::
overflow (int_type c)
{
    if (!(m_mode & std::ios::out))
	return traits_type::eof();

    if (traits_type::eq_int_type (c, traits_type::eof()))
	return traits_type::not_eof (c);

    char_type chr = traits_type::to_char_type (c);

    // if stream is unbuffered, write character directly
    //
    if (0 == m_buf_size)
    {
	if (m_mode & std::ios::in)
	    m_flush_input();
	if (m_writefile (&chr, 1) == 1)
	    return (c);
	else
	    return traits_type::eof();
    }
    else if (pbase() == epptr())
	setp (m_buf, m_buf+m_buf_size);

    // otherwise flush buffer
    //
    if (m_sync() != 0)
	return traits_type::eof();

    *pptr() = chr;
    pbump (1);

    return (c);
}

filebuf::pos_type filebuf::
seekoff (off_type offset, std::ios::seekdir way, std::ios::openmode)
{
    if (is_open() && m_sync() == 0)
	return m_seek (offset, way);
    else
	return pos_type(-1);
}

filebuf::pos_type filebuf::
seekpos (pos_type p, std::ios::openmode)
{
    if (is_open() && m_sync() == 0)
	return m_seek (off_type(p), std::ios::beg);
    else
	return pos_type(-1);
}

#if SYSPP_FSTREAM_TEXT_MODE

// m_flush_text()
//
// Effects: discard remaining input buffer and adjust file position.

void filebuf::
m_flush_text ()
{
    std::streamsize seek_value = egptr() - gptr();
    // XXX single '\n' in input stream counted as '\r\n' also
    if (seek_value)
	seek_value += std::count (gptr(), egptr(), '\n');
    if (eback() == &m_putback && m_cur_gsize)
    {
	seek_value += m_cur_gsize;
	seek_value += std::count (m_buf, m_buf+m_cur_gsize, '\n');
    }
    m_seek (-seek_value, std::ios::cur);
}

// m_read_text (buf, size)
//
// Effects: read characters into supplied buffer and translate all "\r\n"
// character sequences into '\n' characters.
// Returns: number of characters successfully read (not counting '\r'
// characters that were removed by translation).

std::streamsize filebuf::
m_read_text (char_type* buf, std::streamsize buf_size)
{
    DWORD bytes_read;
    BOOL rc = ::ReadFile (m_handle, buf, buf_size, &bytes_read, NULL);
    bool eof_reached = !rc || (size_t)buf_size != bytes_read;

    buf_size = bytes_read;
    char_type* end = buf + buf_size;
    DWORD tail_size = 0;
    while (buf != end)
    {
	buf = const_cast<char_type*> (traits_type::find (buf, end - buf, '\r'));
	if (!buf)
	{
	    if (eof_reached || tail_size == 0)
		break;
	    if (!::ReadFile (m_handle, end, tail_size, &bytes_read, 0) || !bytes_read)
		break;
	    buf = end;
	    buf_size += bytes_read;
	    end += bytes_read;
	    eof_reached = bytes_read != tail_size;
	    tail_size = 0;
	    continue;
	}
	if (buf+1 == end)
	{
	    if (eof_reached)
		break;
	    if (!tail_size)
	    {
		char_type next_char;
		if (::ReadFile (m_handle, &next_char, 1, &tail_size, 0) && tail_size)
		    if (traits_type::eq (next_char, '\n'))
			*buf = '\n';
		    else
			m_seek (-1, std::ios::cur);
		break;
	    }
	    else
	    {
		if (!::ReadFile (m_handle, end, tail_size, &bytes_read, 0) || !bytes_read)
		    break;
		buf_size += bytes_read;
		end += bytes_read;
		eof_reached = bytes_read != tail_size;
		tail_size = 0;
	    }
	}
	++buf;
	if (traits_type::eq (*buf, '\n'))
	{
	    traits_type::move (buf-1, buf, end - buf);
	    --end;
	    --buf_size;
	    ++tail_size;
	}
    }
    return buf_size;
}

#endif

bool detail::text_writer::
flush ()
{
    size_t text_size = out_ptr-text_buf;
    size_t bytes_written = sys_write (text_buf, text_size);
    written += bytes_written;
    out_ptr = text_buf;
    bool success = bytes_written == text_size;
    if (success)
	written -= newline_count;
    else
	written -= std::count (text_buf, text_buf+bytes_written, '\n');
    newline_count = 0;
    return success;
}

bool detail::text_writer::
append (const char_type* str, size_t len)
{
    if (len >= text_buf_size)
    {
	// text is too large for translation buffer,
	// write it directly
	if (out_ptr != text_buf)
	    if (!flush()) return false;
	size_t bytes_written = sys_write (str, len);
	written += bytes_written;
	if (len != bytes_written)
	    return false;
    }
    else
    {
	if (size_t avail_size = out_end-out_ptr)
	{
	    if (len > avail_size)
	    {
		traits_type::copy (out_ptr, str, avail_size);
		out_ptr += avail_size;
		if (!flush()) return false;
		str += avail_size;
		len -= avail_size;
	    }
	}
	else
	    if (!flush()) return false;
	traits_type::copy (out_ptr, str, len);
	out_ptr += len;
    }
    return true;
}

std::streamsize detail::text_writer::
operator() (const char_type* buf, std::streamsize size)
{
    written = 0;
    newline_count = 0;
    out_ptr = text_buf;

    while (size)
    {
	const char_type* new_line = traits_type::find (buf, size, '\n');
	if (!new_line) break; // no newline found
	if (size_t prior = new_line - buf)
	{
	    // some text precedes newline
	    if (!append (buf, prior)) return written;
	    size -= prior;
	}
	if (!append ('\r') || !append ('\n')) return written;
	++newline_count;
	buf = new_line+1;
	--size;
    }
    if (size)
    {
	// write remaining translation buffer
	if (out_ptr == text_buf) // translation buffer empty
	{
	    written += sys_write (buf, size);
	}
	else if (size > out_end-out_ptr)
	{
	    if (flush())
		written += sys_write (buf, size);
	}
	else
	{
	    traits_type::copy (out_ptr, buf, size);
	    out_ptr += size;
	    flush();
	}
    }
    else if (out_ptr != text_buf)
	flush();
    return written;
}

} // namespace sys
