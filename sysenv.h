// -*- C++ -*-
//! \file       sysenv.h
//! \date       Wed Sep 19 06:51:08 2007
//! \brief      system environment manipulation functions.
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

#ifndef SYSENV_H
#define SYSENV_H

#ifdef _WIN32
#include <windows.h>
#else
#include <stdlib.h>
#endif

#include "sysstring.h"

namespace sys {

// get_env (VAR, VALUE)
// Effects: places value of specified environment variable VAR into VALUE.
// Returns: true if variable exists, false otherwise
// Note: if no variable VAR exists, VALUE will be emptied.

template <typename char_type>
bool get_env (const char_type* varname, basic_string<char_type>& value);

template <typename char_type>
inline bool get_env (const basic_string<char_type>& varname, basic_string<char_type>& value)
{ return get_env (varname.c_str(), value); }

// set_env (VAR, VALUE)
// Effects: set value of environment variable VAR to VALUE.
// Returns: true on success, false otherwise

template <typename char_type>
bool set_env (const char_type* varname, const char_type* value);

template <typename char_type>
inline bool set_env (const char_type* varname,
		     const basic_string<char_type>& value)
{ return set_env (varname, value.c_str()); }

template <typename char_type>
inline bool set_env (const basic_string<char_type>& varname,
		     const basic_string<char_type>& value)
{ return set_env (varname.c_str(), value.c_str()); }

// unset_env (VAR)
// Effects: delete variable VAR from current process's environment.
// Returns: true on success, false otherwise

template <typename char_type>
bool unset_env (const char_type* varname);

template <typename char_type>
inline bool unset_env (const basic_string<char_type>& varname)
{ return unset_env (varname.c_str()); }

// expand_env (SRC, DST)
// Effects: copies SRC into DST, expanding all references to environment variables
// in the form %VARNAME%.
// Returns: true on success, false otherwise.
// Note: if expansion fails, exact copy of SRC will be placed into DST.

template <typename char_type>
bool expand_env (const char_type* src, basic_string<char_type>& dst);

// expand_env (STR)
// Effects: expand environment variables inplace.

template <typename char_type>
bool expand_env (basic_string<char_type>& str);

namespace detail {

#ifdef _WIN32

inline DWORD get_env (const char* name, char* value, DWORD value_size)
{ return ::GetEnvironmentVariableA (name, value, value_size); }

inline DWORD get_env (const wchar_t* name, wchar_t* value, DWORD value_size)
{ return ::GetEnvironmentVariableW (name, value, value_size); }

inline int set_env (const char* name, const char* value)
{ return ::SetEnvironmentVariableA (name, value); }

inline int set_env (const wchar_t* name, const wchar_t* value)
{ return ::SetEnvironmentVariableW (name, value); }

inline DWORD expand_env_strings (const char* src, char* dst, DWORD dst_size)
{ return ::ExpandEnvironmentStringsA (src, dst, dst_size); }

inline DWORD expand_env_strings (const wchar_t* src, wchar_t* dst, DWORD dst_size)
{ return ::ExpandEnvironmentStringsW (src, dst, dst_size); }

#else

inline int set_env (const char* name, const char* value)
{ return ::setenv (name, value, 1) != -1; }

#endif

} // namespace detail

// ---------------------------------------------------------------------------
// template functions implementation

template <typename char_type>
inline bool set_env (const char_type* varname, const char_type* value)
{ return detail::set_env (varname, value); }

#ifdef _WIN32

template <typename char_type>
bool get_env (const char_type* var, basic_string<char_type>& value)
{
    bool success = false;
    local_buffer<char_type> buf;

    if (DWORD size = detail::get_env (var, buf.get(), buf.size()))
    {
	if (size > buf.size())
	{
	    buf.reserve (size);
	    size = detail::get_env (var, buf.get(), size);
	}
	if (size)
	{
	    value.assign (buf.get(), size);
	    success = true;
	}
    }
    if (!success)
	value.clear();

    return success;
}

template <typename char_type>
inline bool unset_env (const char_type* varname)
{ return detail::set_env (varname, 0); }

template <typename char_type>
bool expand_env (const char_type* src, basic_string<char_type>& dst)
{
    bool success = false;
    local_buffer<char_type> buf;

    if (DWORD size = detail::expand_env_strings (src, buf.get(), buf.size()))
    {
	if (size > buf.size())
	{
	    buf.reserve (size);
	    size = detail::expand_env_strings (src, buf.get(), size);
	}
	if (size)
	{
	    dst.assign (buf.get(), size);
	    success = true;
	}
    }
    if (!success)
	dst = src;

    return success;
}

template <typename char_type>
inline bool expand_env (basic_string<char_type>& str)
{
    basic_string<char_type> dst;
    bool rc = expand_env (str, dst);
    if (rc) str = dst;
    return rc;
}

#else /* _WIN32 */

template<>
inline bool get_env (const char* var, basic_string<char>& value)
{
    const char* env = ::getenv (var);
    if (env) value = env;
    else     value.clear();
    return env != 0;
}

template<>
bool get_env (const UChar* var, basic_string<UChar>& value)
{
    string varname;
    if (!wcstombs (var, varname))
	return false;

    const char* env = ::getenv (varname.c_str());
    if (!env)
    {
	value.clear();
	return false;
    }
    return mbstowcs (env, value) != 0;
}

template<>
inline bool unset_env (const char* varname)
{ return ::unsetenv (varname) != -1; }

template<>
inline bool unset_env (const UChar* varname)
{
    string cvar;
    if (!wcstombs (varname, cvar))
	return false;
    return ::unsetenv (cvar.c_str()) != -1;
}

#endif /* _WIN32 */

} // namespace sys

#endif /* SYSENV_H */
