// -*- C++ -*-
//! \file       winmem.hpp
//! \date       Mon Sep 17 16:09:04 2007
//! \brief      windows memory management wrappers.
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

#ifndef SYS_WINMEM_HPP
#define SYS_WINMEM_HPP

#include <windows.h>

namespace sys { namespace mem {

using std::size_t;

/// \class sys::mem::global
/// \brief wrapper for memory managed by GlobalAlloc/GlobalFree system calls.

class global
{
public:
    global (unsigned flags, size_t bytes) : m_handle (::GlobalAlloc (flags, bytes))
       	{ if (!m_handle) throw sys::generic_error(); }

    ~global () { if (m_handle) ::GlobalFree (m_handle); }

    HGLOBAL handle () const { return m_handle; }
    unsigned flags () const { return ::GlobalFlags (m_handle); }
    unsigned lock_count () const { return ::GlobalFlags (m_handle) & GMEM_LOCKCOUNT; }

    void realloc (size_t bytes, unsigned flags)
	{
	    HGLOBAL hmem = ::GlobalReAlloc (m_handle, bytes, flags);
	    if (!hmem) throw sys::generic_error();
	    m_handle = hmem;
	}

    HGLOBAL release ()
	{
	    HGLOBAL hmem = m_handle;
	    m_handle = 0;
	    return hmem;
	}

private:

    HGLOBAL	m_handle;
};

/// \class sys::mem::lock
/// \brief wrapper for locks over sys::mem::global objects.

template <typename T>
class lock
{
public:
    explicit lock (global& gmem)
	: m_handle (gmem.handle())
       	, m_ptr (static_cast<T*> (::GlobalLock (m_handle)))
	{ if (!m_ptr) throw sys::generic_error(); }

    explicit lock (HGLOBAL gmem)
	: m_handle (gmem)
       	, m_ptr (static_cast<T*> (::GlobalLock (m_handle)))
	{ if (!m_ptr) throw sys::generic_error(); }

    ~lock () { ::GlobalUnlock (m_handle); }

    T* get () const         { return m_ptr; }

    T& operator* () const   { return *m_ptr; }
    T* operator-> () const  { return m_ptr; }
    T& operator[] (unsigned index) const { return m_ptr[index]; }

private:
    HGLOBAL	m_handle;
    T*		m_ptr;
};

/// \class sys::mem::local_free_on_leave
/// \brief RAII wrapper for the handle obtained through GlobalAlloc.

struct local_free_on_leave
{
    local_free_on_leave (void *p) : m_ptr (p) { }
    ~local_free_on_leave () { ::LocalFree (m_ptr); }
private:
    void *m_ptr;
};

} } // namespace sys::mem

#endif /* SYS_WINMEM_HPP */
