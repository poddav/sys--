// -*- C++ -*-
//! \file       membuf.cc
//! \date       Mon May 16 14:56:51 2011
//! \brief      memory mapped stream buffers implementation.
//
// Copyright (C) 2011 by poddav
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

#include "membuf.hpp"

namespace sys {

mapped_buf* mapped_buf::
close ()
{
    if (!is_open())
	return NULL;

    m_view.unmap();
    m_map.close();
    setg (0, 0, 0);
    setp (0, 0);
    return this;
}

mapped_buf::size_type mapped_buf::
greserve (size_type sz)
{
    size_type result = egptr() - gptr();
    if (sz > result)
    {
	m_offset += gptr() - eback();
	m_remap (sz);
	result = egptr() - gptr();
    }
    return result;
}

mapped_buf::size_type mapped_buf::
preserve (size_type sz)
{
    size_type result = epptr() - pptr();
    if (sz > result)
    {
	m_offset += pptr() - pbase();
	m_remap (sz);
	result = epptr() - pptr();
    }
    return result;
}

mapped_buf::pos_type mapped_buf::
seekoff (off_type offset, std::ios::seekdir way, std::ios::openmode mode)
{
    if (is_open())
	return m_seek (offset, way, mode);
    else
	return pos_type(-1);
}

mapped_buf::pos_type mapped_buf::
seekpos (pos_type p, std::ios::openmode mode)
{
    if (is_open())
	return m_seek (off_type(p), std::ios::beg, mode);
    else
	return pos_type(-1);
}

mapped_buf::off_type mapped_buf::
m_seek (off_type offset, std::ios::seekdir way, std::ios::openmode mode)
{
    if (pptr() != gptr())
    {
	if (mode & std::ios::in)
	    m_setp_ex (gptr() - eback());
	else if (mode & std::ios::out)
	    m_setg_ex (pptr() - pbase());
    }
    switch (way)
    {
    default:
    case std::ios::beg: break;
    case std::ios::cur:
	{
	    off_type cur_pos = m_offset + (gptr() - eback());
	    if (!offset)
		return cur_pos;
	    offset += cur_pos;
	    break;
	}
    case std::ios::end: offset += m_map.size(); break;
    }
    if (offset < 0)
	offset = 0;
    else
	offset = std::min<off_type> (offset, m_map.size());
    off_type pos = offset - m_offset;
    if (pos >= 0 && pos <= m_view.size())
    {
	m_setg_ex (pos);
	m_setp_ex (pos);
    }
    else
    {
	m_offset = offset;
	setg (0, 0, 0);	// next get will cause underflow
	setp (0, 0);
    }
    return offset;
}

// ---------------------------------------------------------------------------
// get

std::streamsize mapped_buf::
xsgetn (char_type* buf, std::streamsize size)
{
    std::streamsize ret = 0;
    if (size_type buffered = std::min<size_type> (size, egptr() - gptr()))
    {
	traits_type::copy (buf, gptr(), buffered);
	gbump (buffered);
	buf += buffered;
	size -= buffered;
	ret += buffered;
    }
    if (size > 0)
    {
	size = std::min<size_type> (size, greserve (size));
	if (size)
	{
	    traits_type::copy (buf, gptr(), size);
	    gbump (size);
	    ret += size;
	}
    }
    m_setp_ex (gptr() - eback());
    return ret;
}

mapped_buf::int_type mapped_buf::
underflow ()
{
    if (gptr() < egptr())
	return traits_type::to_int_type (*gptr());

    off_type view_size = egptr() - eback();
    if (m_offset < m_map.size() - view_size)
    {
	m_offset += view_size;
	m_remap();
	if (gptr() != egptr())
	    return traits_type::to_int_type (*gptr());
    }
    return traits_type::eof();
}

// ---------------------------------------------------------------------------
// put

mapped_buf::int_type mapped_buf::
overflow (int_type c)
{
    if (traits_type::eq_int_type (c, traits_type::eof()))
	return traits_type::not_eof (c);

    if (pptr() == epptr())
    {
	m_offset += epptr() - pbase();
	if (m_offset >= m_map.size())
	{
	    setp (0, 0);
	    setg (0, 0, 0);
	    return traits_type::eof();
	}
	m_remap();
	if (pptr() == epptr())
	    return traits_type::eof();
    }
    *pptr() = traits_type::to_char_type (c);
    pbump (1);

    return (c);
}

std::streamsize mapped_buf::
xsputn (const char_type* buf, std::streamsize size)
{
    if (size > static_cast<std::streamsize> (epptr() - pptr()))
    {
	m_offset += pptr() - pbase();
	if (m_offset >= m_map.size())
	{
	    setp (0, 0);
	    setg (0, 0, 0);
	    return 0;
	}
	size = std::min<size_type> (size, preserve (size));
    }
    traits_type::copy (pptr(), buf, size);
    pbump (size);
    m_setg_ex (pptr() - pbase());
    return size;
}

} // namespace sys
