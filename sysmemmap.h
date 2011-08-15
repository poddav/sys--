// -*- C++ -*-
//! \file       sysmemmap.h
//! \date       Mon Dec 11 22:49:52 2006
//! \brief      memory mapped objects interface.
//
// Copyright (C) 2006 by poddav
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

#ifndef SYSMEMMAP_H
#define SYSMEMMAP_H

#include "sysmmdetail.h"
#include "sysio.h"
#include "syserror.h"
#include <stdexcept>
#include <cassert>

namespace sys { namespace mapping {

typedef detail::map_impl::off_type	off_type;
typedef detail::map_impl::size_type	size_type;

/// \class sys::mapping::map_base
///
/// base class for memory mapped object.
/// map_base maintains reference counted implementation of mapped object.
/// mapped object is destroyed when there's no views on top of it.

class DLLIMPORT map_base
{
    refcount_ptr<detail::map_impl>	impl;

public:
    typedef detail::map_impl::off_type	off_type;
    typedef detail::map_impl::size_type	size_type;

    template <class T> class view;

    /// is_open()
    ///
    /// Returns: true if THIS references initialized memory mapped object.

    bool is_open () const { return impl; }

    /// size()
    ///
    /// Returns: size of memory mapped object (size of underlying file).

    off_type size () const { return impl? impl->get_size(): 0; }

    /// close()
    ///
    /// Effects: removes reference to underlying memory mapped object, if any.

    void close () { impl.reset(); }

    /// writeable()
    ///
    /// Returns: true if memory mapped object is open with writing enabled, false
    /// otherwise.
    bool writeable () const { return impl? impl->writeable(): false; }

    /// page_size() and page_mask()
    ///
    /// Return system-dependent virtual page size and corresponding bitwise mask.

    static size_type page_size () { return detail::map_impl::page_size(); }
    static size_type page_mask () { return detail::map_impl::page_mask(); }

protected:
    /// map_base is a base class for memory mapped objects and can be constructed by
    /// ancestors only.

    map_base () : impl() { }

    /// map_base (FILENAME, MODE, SIZE)
    ///
    /// Effects: create memory mapped object on top of the file FILENAME with access
    /// mode MODE.
    /// Throws: sys::generic_error if map cannot be created,
    ///         std::bad_alloc if memory allocation for map failed.

    template <typename CharT>
    map_base (const CharT* filename, mode_t mode, off_type size = 0) : impl()
	{ open (filename, mode, size); }

    /// map_base (HANDLE, MODE, SIZE)
    ///
    /// Effects: create memory mapped object with access mode MODE on top of file
    /// referenced by system handle HANDLE.  HANDLE could be safely used for any i/o
    /// operations (read, write, close etc) after map_base object is created.
    /// Throws: sys::generic_error if map cannot be created,
    ///         std::bad_alloc if memory allocation for map failed.

    map_base (sys::raw_handle handle, mode_t mode, off_type size = 0) : impl()
	{ open (handle, mode, size); }

    ~map_base () { }

    template <typename CharT>
    void open (const CharT* filename, mode_t mode, off_type size = 0);
    void open (sys::raw_handle handle, mode_t mode, off_type size = 0);

private:
    /// map_base object cannot be copied

    map_base (const map_base&);		// not defined
    map_base& operator= (const map_base&);

    /// open_mode (MODE)
    ///
    /// Returns: system-dependent file open mode corresponding to specified map access
    /// mode.

    static io::sys_mode open_mode (mode_t mode)
	{
#ifdef _WIN32
	    return io::sys_mode (mode == read? io::generic_read: io::read_write,
				 io::open_existing);
#else
	    return io::sys_mode (mode == read? O_RDONLY: O_RDWR);
#endif
	}
};

enum write_mode_t {
    writeshare	= write,	// normal read-write mode
    writecopy	= copy,		// copy-on-write mode
};

/// \class sys::mapping::readonly
///
/// class referring to readonly memory mapped object.

class readonly : public map_base
{
public:
    readonly () { }
    template <typename CharT>
    explicit readonly (const CharT* filename, off_type size = 0)
       	: map_base (filename, read, size) { }
    template <typename Ch, typename Tr, typename Al>
    explicit readonly (const basic_string<Ch,Tr,Al>& filename, off_type size = 0)
	: map_base (filename.c_str(), read, size) { }
    explicit readonly (sys::raw_handle handle, off_type size = 0)
       	: map_base (handle, read, size) { }

    template <typename CharT>
    void open (const CharT* filename, off_type size = 0)
       	{ map_base::open (filename, read, size); }

    template <typename Ch, typename Tr, typename Al>
    void open (const basic_string<Ch,Tr,Al>& filename, off_type size = 0)
	{ map_base::open (filename.c_str(), read, size); }

    void open (sys::raw_handle handle, off_type size = 0)
       	{ map_base::open (handle, read, size); }
};

/// \class sys::mapping::readwrite
///
/// class referring to read-write memory mapped object.

class readwrite : public map_base
{
public:
    readwrite () { }

    template <typename CharT>
    explicit readwrite (const CharT* filename, write_mode_t mode = writeshare,
			off_type size = 0)
       	: map_base (filename, mode == writeshare? write: copy, size) { }

    template <typename Ch, typename Tr, typename Al>
    explicit readwrite (const basic_string<Ch,Tr,Al>& filename, write_mode_t mode = writeshare,
		       	off_type size = 0)
       	: map_base (filename.c_str(), mode == writeshare? write: copy, size) { }

    explicit readwrite (sys::raw_handle handle, write_mode_t mode = writeshare,
			off_type size = 0)
       	: map_base (handle, mode == writeshare? write: copy, size) { }

    template <typename CharT>
    void open (const CharT* filename, write_mode_t mode = writeshare, off_type size = 0)
       	{ map_base::open (filename, mode == writeshare? write: copy, size); }

    template <typename Ch, typename Tr, typename Al>
    void open (const basic_string<Ch,Tr,Al>& filename, write_mode_t mode = writeshare,
	       off_type size = 0)
	{ map_base::open (filename.c_str(), mode == writeshare? write: copy, size); }

    void open (sys::raw_handle handle, write_mode_t mode = writeshare, off_type size = 0)
       	{ map_base::open (handle, mode == writeshare? write: copy, size); }
};

/// \class sys::mapping::map_base::view
///
/// base class that maps views of memory mapped object into the address space of the
/// calling process.

template<class T>
class map_base::view
{
public:
    typedef map_base::off_type	off_type;
    typedef map_base::size_type	size_type;

protected:
    /// view (MAP, OFFSET, N)
    ///
    /// Effects: creates a view of memory mapped object MAP, starting at offset
    /// OFFSET, containing N objects of type T.  if N is zero, tries to map all
    /// available area.
    /// Throws: std::invalid_argument if MAP does not refer to initialized memory
    ///                               mapped object.
    ///         std::range_error if OFFSET is greater than size of MAP.
    ///         sys::generic_error if some system errors occurs.

    explicit view (map_base& mf, off_type offset = 0, size_type n = 0)
	: map (mf.impl), area (0)
	{ remap (offset, n); }

    view () : map (0), area (0) { }

    ~view () { if (area) map->unmap ((void*)area, msize*sizeof(T)); }

public:
    T* get () const { return area; }
    size_type size () const { return msize; }

    T* operator-> () const { return area; }
    T& operator* () const { return *area; }
    T& operator[] (size_t n) const { return area[n]; }

    T* begin () const { return area; }
    T* end   () const { return area+size(); }

    bool sync () { return area? map->sync (area, msize*sizeof(T)): false; }

    void remap (map_base& mf, off_type offset = 0, size_type n = 0)
	{
	    unmap();
	    map = mf.impl;
	    remap (offset, n);
	}
    void unmap ()
	{
	    if (area)
	    {
		map->unmap ((void*)area, msize*sizeof(T));
		area = 0;
		map.reset();
	    }
	}

    off_type max_offset () const { return map->get_size(); }
    
private:
    void remap (off_type offset, size_type n);

    refcount_ptr<detail::map_impl>	map;
    T*		area;	// pointer to the beginning of view address space
    size_type	msize;	// size of view in terms of T objects

    view (const view&);			// not defined
    view& operator= (const view&);	//
};

template <class T>
class const_view : public map_base::view<const T>
{
public:
    typedef map_base::off_type	off_type;
    typedef map_base::size_type	size_type;

    const_view () : map_base::view<const T>() { }
    explicit const_view (map_base& map, off_type offset = 0, size_type n = 0)
	: map_base::view<const T> (map, offset, n)
	{ }
};

template <class T>
class view : public map_base::view<T>
{
public:
    typedef map_base::off_type	off_type;
    typedef map_base::size_type	size_type;

    view () : map_base::view<T>() { }
    explicit view (readwrite& rwm, off_type offset = 0, size_type n = 0)
	: map_base::view<T> (rwm, offset, n)
	{ }
};

// --- template methods implementation ---------------------------------------

template <typename CharT> inline void map_base::
open (const CharT* filename, mode_t mode, off_type size)
{
    // this handle automatically closes itself on function exit
    sys::file_handle handle (sys::create_file (filename, open_mode (mode), io::share_read));
    if (!handle) throw file_error (filename);
    try {
	open (handle, mode, size);
    }
    catch (generic_error&)
    {
	throw file_error (filename);
    }
}

template <class T> void map_base::view<T>::
remap (off_type offset, size_type n)
{
    assert (area == 0);

    if (!map)
	throw std::invalid_argument ("map_base::view: taking view of an uninitialized map");
    if (off_type(offset+sizeof(T)) > map->get_size())
	throw std::range_error ("map_base::view: offset exceedes map size");

    size_type byte_size = n*sizeof(T);
    if (n == 0 || offset+off_type(byte_size) > map->get_size())
	byte_size = map->get_size() - offset;

    void* v = map->map (offset, byte_size);
    if (!v) throw sys::generic_error();
    area = static_cast<T*> (v);
    msize = byte_size / sizeof(T);
}

} // namespace mapping

/// \class mapped_file
///
/// the use of mapped_file class is discouraged since there's no means to prevent
/// write to readonly mapped objects.  better use sys::mapping::readwrite,
/// sys::mapping::readonly and desired view classes upon them instead.

class DLLIMPORT mapped_file : public mapping::map_base
{
public:
    mapped_file () { }

    template <typename CharT>
    explicit mapped_file (const CharT* filename, mapping::mode_t mode, off_type size = 0)
       	: map_base (filename, mode, size) { }

    explicit mapped_file (sys::raw_handle handle, mapping::mode_t mode, off_type size = 0)
       	: map_base (handle, mode, size) { }

    template <typename CharT>
    void open (const CharT* filename, mapping::mode_t mode, off_type size = 0)
       	{ map_base::open (filename, mode, size); }

    void open (sys::raw_handle handle, mapping::mode_t mode, off_type size = 0)
       	{ map_base::open (handle, mode, size); }

    template <class T>
    class view;
};

template <class T>
class DLLIMPORT mapped_file::view : public mapping::map_base::view<T>
{
public:
    typedef mapping::map_base::off_type		off_type;
    typedef mapping::map_base::size_type	size_type;

    view () : mapping::map_base::view<T>() { }
    explicit view (mapping::map_base& map, off_type offset = 0, size_type n = 0)
	: mapping::map_base::view<T> (map, offset, n)
	{ }
};

} // namespace sys

#endif /* SYSMEMMAP_H */
