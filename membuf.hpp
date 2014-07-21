// -*- C++ -*-
//! \file       membuf.hpp
//! \date       Mon May 16 13:07:42 2011
//! \brief      memory mapped streams.
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

#ifndef SYS_MEMBUF_HPP
#define SYS_MEMBUF_HPP

#include <ios>
#include <streambuf>
#include "sysmemmap.h"

namespace sys {

template <typename CharT, typename Traits = std::char_traits<CharT> >
class basic_memory_buf;

template <typename CharT, typename Traits = std::char_traits<CharT>,
	  typename Alloc = std::allocator<CharT> >
class dynamic_memory_buf;

typedef basic_memory_buf<char>	    memory_buf;
typedef dynamic_memory_buf<char>    dynamic_buf;

// ---------------------------------------------------------------------------
/// \class basic_memory_buf
/// \brief Stream buffer on top of the user-supplied byte sequence.

template <typename CharT, typename Traits>
class basic_memory_buf : public std::basic_streambuf<CharT, Traits>
{
public: // types

    typedef CharT				char_type;
    typedef Traits				traits_type;
    typedef typename traits_type::pos_type 	pos_type;
    typedef typename traits_type::off_type	off_type;
    typedef size_t				size_type;

public: // methods

    explicit basic_memory_buf (std::ios::openmode mode)
       	: m_mode (mode)
	{ }
    basic_memory_buf (const char_type* in, size_type sz)
	: m_mode (std::ios::in)
	{ this->setbuf (const_cast<char_type*> (in), sz); }
    basic_memory_buf (char_type* in, size_type sz, std::ios::openmode mode)
	: m_mode (mode)
	{ this->setbuf (in, sz); }

    template <size_t N>
    explicit basic_memory_buf (const char_type (&ary)[N])
	: m_mode (std::ios::in)
	{ this->setbuf (const_cast<char_type*> (ary), N); }

    template <size_t N>
    explicit basic_memory_buf (char_type (&ary)[N], std::ios::openmode mode)
	: m_mode (mode)
	{ this->setbuf (ary, N); }

    /// gdata() and gsize() return remaining portion of the GET sequence
    const char_type* gdata () const { return this->gptr(); }
    size_type	     gsize () const { return this->egptr() - this->gptr(); }

    /// pdata() and psize() return written portion of the PUT sequence
    const char_type* pdata () const { return this->pbase(); }
    size_type	     psize () const { return this->pptr() - this->pbase(); }

    /// goffset()
    /// \return offset of the current GET position within underlying sequence
    off_type goffset () const { return (this->gptr() - this->eback()); }

    /// poffset()
    /// \return offset of the current PUT position within underlying sequence
    off_type poffset () const { return (this->pptr() - this->pbase()); }

protected: // virtual methods

    pos_type seekoff (off_type off, std::ios::seekdir way, std::ios::openmode mode)
	{ return m_seek (off, way, mode); }
    pos_type seekpos (pos_type pos, std::ios::openmode mode)
	{ return m_seek (off_type (pos), std::ios::beg, mode); }
    std::streambuf* setbuf (char_type* s, std::streamsize n)
	{
	    // eback() always points to the beginning of underlying sequence and
	    // epptr() always points to its end, no matter what i/o mode
	    // specified for the buffer.
	    if (m_mode & std::ios::in)
		this->setg (s, s, s+n);
	    else
		this->setg (s, s, s);
	    if (m_mode & std::ios::out)
		this->setp (s, s+n);
	    else
		this->setp (s+n, s+n);
	    return this;
       	}

private: // methods

    // m_seek (OFFSET, WAY)
    off_type m_seek (off_type offset, std::ios::seekdir way, std::ios::openmode);

protected: // data

    std::ios::openmode		m_mode;
};

// ---------------------------------------------------------------------------
/// \class dynamic_memory_buf
/// \brief Stream buffer on top of the character sequence
//         with dynamic memory management.

template <typename CharT, typename Traits, typename Alloc>
class dynamic_memory_buf : public basic_memory_buf<CharT, Traits>
		         , private Alloc
{
public: // types

    typedef basic_memory_buf<CharT, Traits>	base_type;
    typedef CharT				char_type;
    typedef Traits				traits_type;
    typedef Alloc				allocator_type;
    typedef typename traits_type::int_type 	int_type;
    typedef typename traits_type::pos_type 	pos_type;
    typedef typename traits_type::off_type	off_type;
    typedef typename base_type::size_type	size_type;

public: // methods

    explicit dynamic_memory_buf (std::ios::openmode mode = 0)
       	: base_type (mode), m_allocated (false)
      	{}
    dynamic_memory_buf (const char_type* in, size_type sz, std::ios::openmode mode)
	: base_type (mode), m_allocated (false)
	{ this->setbuf (const_cast<char_type*> (in), sz); }
    template <size_t N>
    dynamic_memory_buf (const char_type (&ary)[N], std::ios::openmode mode)
	: base_type (mode), m_allocated (false)
	{ this->setbuf (const_cast<char_type*> (ary), N); }

    allocator_type get_allocator () const
       	{ return static_cast<allocator_type> (*this); }

protected: // virtual methods

    int_type overflow (int_type c);
    std::streamsize xsputn (const char_type* buf, std::streamsize size);

    std::streambuf* setbuf (char_type* s, std::streamsize n)
	{
	    if (m_allocated)
	    {
		this->deallocate (this->eback(), this->epptr() - this->eback());
		m_allocated = false;
	    }
	    return base_type::setbuf (s, n);
       	}

private: // methods

    // m_grow (NEW_SIZE)
    void m_grow (size_type new_size);

protected: // data

    enum { GROW_SIZE = 1024 };

    bool			m_allocated;
};

// ---------------------------------------------------------------------------
/// \class mapped_buf
/// \brief stream buffer on top of the memory mapped file.

class SYSPP_DLLIMPORT mapped_buf : public std::streambuf
{
public: // types

    typedef char				char_type;
    typedef std::char_traits<char_type>		traits_type;
    typedef traits_type::int_type 		int_type;
    typedef traits_type::pos_type 		pos_type;
    typedef traits_type::off_type 		off_type;
    typedef mapping::size_type			size_type;
    typedef mapping::view<char_type>            view_type;

public: // methods

    mapped_buf () : m_offset (0) { }
    mapped_buf (const mapped_buf& other) : std::streambuf(), m_offset (other.m_offset)
        { m_view.bind (other.m_view); }
    explicit mapped_buf (const mapping::map_base& map) : m_offset (0)
        { m_view.bind (map); }
    template <typename T>
    explicit mapped_buf (const mapping::map_base::view<T>& view) : m_offset (0)
        { m_view.bind (view); }

    virtual ~mapped_buf () { close(); }

    mapped_buf& operator= (const mapped_buf& other)
        {
            if (&other != this)
            {
                setg (0, 0, 0);
                setp (0, 0);
                m_view.bind (other.m_view);
                m_offset = other.m_offset;
            }
            return *this;
        }

    bool is_open () const { return m_view.is_bound(); }

    template<typename CharT>
    mapped_buf* open (const CharT* filename, std::ios::openmode mode,
		      bool private_mode = false);
    mapped_buf* close ();

    static size_type page_size () { return mapped_file::page_size(); }

protected: // virtual methods

    int_type underflow ();
    int_type overflow (int_type c);

    std::streamsize xsgetn (char_type* buf, std::streamsize size);
    std::streamsize xsputn (const char_type* buf, std::streamsize size);

    pos_type seekoff (off_type off, std::ios::seekdir way, std::ios::openmode);
    pos_type seekpos (pos_type pos, std::ios::openmode mode);

public: // additional convenient methods

    /// reserve (SIZE)
    /// \brief  tries to remap underlying view to map on at least SIZE bytes,
    ///         starting from current position.
    ///         Syncronizes get/put positions.
    /// \return number of bytes available in buffer. could be less than requested
    ///         SIZE.
    size_type greserve (size_type sz);
    size_type preserve (size_type sz);

    // pointers returned by gdata() and pdata() are valid until next
    // stream seek operation or stream overflow/underflow.

    const char_type* gdata () const { return gptr(); }
    size_type	     gsize () const { return egptr() - gptr(); }

    const char_type* pdata () const { return pbase(); }
    size_type	     psize () const { return pptr() - pbase(); }

    /// offset()
    /// \return current offset from the beginning of underlying mapped file.
    off_type goffset () const { return m_offset + (gptr() - eback()); }
    off_type poffset () const { return m_offset + (pptr() - pbase()); }

    /// map_size()
    /// \return size of the underlying memory map object.
    size_type map_size () const { return m_view.max_offset(); }

private: // methods

    // m_seek (OFFSET, WAY)
    off_type m_seek (off_type offset, std::ios::seekdir way, std::ios::openmode mode);

    // m_setg_ex (OFFSET)
    void m_setg_ex (off_type offset)
	{
	    assert (offset <= m_view.size());
	    char_type* beg = m_view.begin();
	    setg (beg, beg+offset, m_view.end());
	}
    void m_setp_ex (off_type offset)
	{
	    assert (offset <= m_view.size());
	    setp (m_view.begin(), m_view.end());
	    pbump (offset);
	}
    void m_reset ()
	{
	    char_type* beg = m_view.begin();
	    char_type* end = m_view.end();
	    setp (beg, end);
	    setg (beg, beg, end);
	}
    void m_remap (size_type sz = 0)
	{
	    m_view.remap (m_offset, std::max (sz, page_size()));
	    m_reset();
	}

private: // data

    view_type           m_view;
    mapping::off_type	m_offset; // offset of eback() within m_view
};

// ---------------------------------------------------------------------------

template <typename C, typename T>
typename basic_memory_buf<C,T>::off_type basic_memory_buf<C,T>::
m_seek (off_type offset, std::ios::seekdir way, std::ios::openmode mode)
{
    off_type result = static_cast<off_type> (-1);
    if (mode & std::ios::in)
    {
	off_type goffset = offset;
	off_type seq_size = this->egptr() - this->eback();
	if (way == std::ios::cur)
	{
	    off_type cur_pos = this->gptr() - this->eback();
	    if (!goffset)
		return cur_pos;
	    goffset += cur_pos;
	}
	else if (way == std::ios::end)
	    goffset += seq_size;
	if (goffset < 0)
	    goffset = 0;
	else if (goffset > seq_size)
	    goffset = seq_size;
	this->setg (this->eback(), this->eback() + goffset, this->egptr());
	result = goffset;
    }
    if (mode & std::ios::out)
    {
	off_type seq_size = this->epptr() - this->pbase();
	if (way == std::ios::cur)
	{
	    off_type cur_pos = this->pptr() - this->pbase();
	    if (!offset)
		return cur_pos;
	    offset += cur_pos;
	}
	else if (way == std::ios::end)
	    offset += seq_size;
	if (offset < 0)
	    offset = 0;
	else if (offset > seq_size)
	    offset = seq_size;
	this->setp (this->pbase(), this->epptr());
	this->pbump (offset);
	result = offset;
    }
    return result;
}

// ---------------------------------------------------------------------------

template <typename Ch, typename Tr, typename Al>
void dynamic_memory_buf<Ch,Tr,Al>::
m_grow (size_type new_size)
{
    off_type putpos = this->poffset();
    off_type getpos = this->goffset();
    size_type alt_size = this->epptr() - this->eback();
    alt_size += alt_size/2; // grow by half
    new_size = std::max (alt_size, new_size);
    char_type* new_buf = this->allocate (new_size);
    traits_type::copy (new_buf, this->eback(), this->epptr() - this->eback());
    this->setbuf (new_buf, new_size);
    m_allocated = true;
    if (this->m_mode & std::ios::in)
	gbump (getpos);
    if (this->m_mode & std::ios::out)
	pbump (putpos);
}

template <typename Ch, typename Tr, typename Al>
typename dynamic_memory_buf<Ch,Tr,Al>::int_type dynamic_memory_buf<Ch,Tr,Al>::
overflow (int_type c)
{
    if (traits_type::eq_int_type (c, traits_type::eof()))
	return traits_type::not_eof (c);
    if (!(this->m_mode & std::ios::out))
	return traits_type::eof();

    if (this->pptr() == this->epptr())
	m_grow (this->psize() + GROW_SIZE);

    *this->pptr() = traits_type::to_char_type (c);
    this->pbump (1);

    return (c);
}

template <typename Ch, typename Tr, typename Al>
std::streamsize dynamic_memory_buf<Ch,Tr,Al>::
xsputn (const char_type* buf, std::streamsize size)
{
    if (!(this->m_mode & std::ios::out))
	return 0;
    if (size > static_cast<std::streamsize> (this->epptr() - this->pptr()))
    {
	size_type cur_size = this->epptr() - this->pbase();
	m_grow (this->psize() + size + GROW_SIZE);
    }
    traits_type::copy (this->pptr(), buf, size);
    this->pbump (size);
    return size;
}

// ---------------------------------------------------------------------------

template <typename CharT>
mapped_buf* mapped_buf::
open (const CharT* filename, std::ios::openmode mode, bool private_mode)
{
    if (is_open() || !(mode & (std::ios::out|std::ios::in)))
       	return NULL;

    mapping::mode_t mapmode =
	(mode & std::ios::out) ? private_mode
	    ? mapping::copy
	    : mapping::write
	    : mapping::read;

    mapped_file map (filename, mapmode);
    m_view.bind (map);
    
    m_offset = 0;
    setg (0, 0, 0);
    setp (0, 0);
    return this;
}

} // namespace sys

#endif /* SYS_MEMBUF_HPP */
