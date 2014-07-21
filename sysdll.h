// -*- C++ -*-
//! \file       sysdll.h
//! \date       Mon May 14 17:16:40 2007
//! \brief      system dynamic-loading library interface.
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

#ifndef SYS_LIBRARY_H
#define SYS_LIBRARY_H

#ifdef _WIN32
#include <windows.h>
#else
#include <dlfcn.h>
#include "sysstring.h"	// for wcstombs()
#endif

namespace sys {

namespace detail {

#ifdef _WIN32
typedef HMODULE module_type;
typedef FARPROC proc_type;
#else
typedef void*	module_type;
typedef void*	proc_type;
#endif

module_type load_library (const char* name);
module_type load_library (const WChar* name);

} // namespace detail

class library
{
public:
    typedef detail::module_type module_type;
    typedef detail::proc_type	proc_type;

    template <typename char_type>
    explicit library (const char_type* name) : lib (detail::load_library (name)) { }

    ~library () { close(); }

    operator void*() const { return lib; }
    bool operator! () const { return !lib; }

    proc_type	get_proc (const char* symbol) const;
    
    module_type	get_handle () const { return lib; }
    void	close ();

protected:
    library () { }

private:
    library (const library&); // not defined
    library& operator= (const library&); 

    module_type		lib;
};

class library_throw : private library
{
public:
    typedef library::module_type module_type;
    typedef library::proc_type   proc_type;

    template <typename char_type>
    explicit library_throw (const char_type* name);

    operator void*() const { return this->lib; }
    bool operator! () const { return !this->lib; }

    proc_type	get_proc (const char* symbol) const;

    using library::get_handle;
    using library::close;
};

// ---------------------------------------------------------------------------
// implementation

#ifdef _WIN32

inline detail::module_type detail::
load_library (const char* name)
{
    return ::LoadLibraryA (name);
}

inline detail::module_type detail::
load_library (const WChar* name)
{
    return ::LoadLibraryW (name);
}

inline void library::
close ()
{
    if (lib)
    {
       	::FreeLibrary (lib);
	lib = 0;
    }
}

inline proc_type library::
get_proc (const char* symbol)
{
    return lib? ::GetProcAddress (lib, symbol): 0;
}

template <typename char_type> inline library_throw::
library_throw (const char_type* name)
    : library (name)
{
    if (!lib) SYS_THROW_GENERIC_ERROR (name);
}

template <typename char_type>
inline proc_type library_throw::
get_proc (const char_type* symbol)
{
    proc_type proc = library::get_proc (symbol);
    if (!proc)
	SYS_THROW_GENERIC_ERROR (symbol);
    return proc;
}

#else

inline detail::module_type detail::
load_library (const char* name)
{
    return ::dlopen (name, RTLD_LAZY);
}

inline detail::module_type detail::
load_library (const WChar* name)
{
    string cname;
    return wcstombs (name, cname) && ::dlopen (cname.c_str(), RTLD_LAZY);
}

inline void library::
close ()
{
    if (lib)
    {
       	::dlclose (lib);
	lib = 0;
    }
}

inline proc_type library::
get_proc (const char* symbol)
{
    return lib? ::dlsym (lib, symbol): 0;
}

template <typename char_type> inline library_throw::
library_throw (const char_type* name)
    : library (name)
{
    if (!this->lib)
       	SYS_THROW_GENERIC_ERROR (name, ::dlerror());
}

inline proc_type library_throw::
get_proc (const char* symbol)
{
    proc_type proc = library::get_proc (symbol);
    if (!proc)
	SYS_THROW_GENERIC_ERROR (symbol, ::dlerror());
}

#endif

} // namespace sys

#endif /* SYS_LIBRARY_H */
