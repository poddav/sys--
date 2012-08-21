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

#include <cstddef>	// for std::size_t
#include <windows.h>

namespace sys { namespace mem {

using std::size_t;

/// \class sys::mem::global
/// \brief wrapper for memory managed by GlobalAlloc/GlobalFree system calls.

class global
{
public:
    explicit global (HGLOBAL handle) : m_handle (handle) {}
    global (unsigned flags, size_t bytes) : m_handle (::GlobalAlloc (flags, bytes))
       	{ if (!m_handle) throw std::bad_alloc(); }

    ~global () { if (m_handle) ::GlobalFree (m_handle); }

    HGLOBAL handle () const { return m_handle; }
    size_t size () const { return ::GlobalSize (m_handle); }
    unsigned flags () const { return ::GlobalFlags (m_handle); }
    unsigned lock_count () const { return ::GlobalFlags (m_handle) & GMEM_LOCKCOUNT; }

    void realloc (size_t bytes, unsigned flags)
	{
	    HGLOBAL hmem = ::GlobalReAlloc (m_handle, bytes, flags);
	    if (!hmem) throw std::bad_alloc();
	    m_handle = hmem;
	}

    HGLOBAL release ()
	{
	    HGLOBAL hmem = m_handle;
	    m_handle = 0;
	    return hmem;
	}

private:
    global (const global&); // not defined
    global& operator= (const global&);

    HGLOBAL	m_handle;
};

/// \class sys::mem::local
/// \brief wrapper for memory managed by LocalAlloc/LocalFree system calls.

class local
{
public:
    explicit local (HLOCAL handle) : m_handle (handle) {}
    local (unsigned flags, size_t bytes) : m_handle (::LocalAlloc (flags, bytes))
	{ if (!m_handle) throw std::bad_alloc(); }

    ~local () { if (m_handle) ::LocalFree (m_handle); }

    HLOCAL handle () const { return m_handle; }
    size_t size () const { return ::LocalSize (m_handle); }
    unsigned flags () const { return ::LocalFlags (m_handle); }
    unsigned lock_count () const { return ::LocalFlags (m_handle) & LMEM_LOCKCOUNT; }

    void realloc (size_t bytes, unsigned flags)
	{
	    HLOCAL hmem = ::LocalReAlloc (m_handle, bytes, flags);
	    if (!hmem) throw std::bad_alloc();
	    m_handle = hmem;
	}

    HLOCAL release ()
	{
	    HLOCAL hmem = m_handle;
	    m_handle = 0;
	    return hmem;
	}

private:
    local (const local&); // not defined
    local& operator= (const local&);

    HLOCAL	m_handle;
};

namespace detail {

template <class mem_type> class lock {};

template <> class lock<global>
{
public:
    static void* Lock   (HANDLE hmem) { return ::GlobalLock (hmem); }
    static bool  Unlock (HANDLE hmem) { return ::GlobalUnlock (hmem); }
};

template <> class lock<local>
{
public:
    static void* Lock   (HANDLE hmem) { return ::LocalLock (hmem); }
    static bool  Unlock (HANDLE hmem) { return ::LocalUnlock (hmem); }
};

} // namespace detail

/// \class sys::mem::lock
/// \brief wrapper for locks over sys::mem::global objects.

template <typename T, class mem_type = global>
class lock : private detail::lock<mem_type>
{
public:
    explicit lock (mem_type& hmem)
	: m_handle (hmem.handle())
       	, m_ptr (static_cast<T*> (this->Lock (m_handle)))
	{ if (!m_ptr) throw sys::generic_error(); }

    explicit lock (HANDLE hmem)
	: m_handle (hmem)
       	, m_ptr (static_cast<T*> (this->Lock (m_handle)))
	{ if (!m_ptr) throw sys::generic_error(); }

    ~lock () { this->Unlock (m_handle); }

    T* get () const         { return m_ptr; }

    T& operator* () const   { return *m_ptr; }
    T* operator-> () const  { return m_ptr; }
    T& operator[] (unsigned index) const { return m_ptr[index]; }

private:
    HANDLE	m_handle;
    T*		m_ptr;
};

} } // namespace sys::mem

#endif /* SYS_WINMEM_HPP */
