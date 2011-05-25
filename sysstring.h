// -*- C++ -*-
//! \file       sysstring.h
//! \date       Wed Sep 19 06:58:57 2007
//! \brief      forward declaration of string class for 'sys' library.
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

#ifndef SYSSTRING_H
#define SYSSTRING_H

#ifndef SYSPP_USE_EXT_STRING
#include <string>	// for std::basic_string
#include <iosfwd>
#else
#include "string.hpp"	// for ext::basic_string
#include "iosfwd.hpp"
#endif // SYSPP_USE_EXT_STRING
#include <climits>	// for MB_LEN_MAX
#include "sysdef.h"

namespace sys {

#ifndef SYSPP_USE_EXT_STRING
using std::string;
using std::wstring;
using std::basic_string;
using std::basic_istringstream;
using std::basic_ostringstream;
using std::basic_stringstream;
#else
using ext::string;
using ext::wstring;
using ext::basic_string;
using ext::basic_istringstream;
using ext::basic_ostringstream;
using ext::basic_stringstream;
#endif // SYSPP_USE_EXT_STRING

#ifdef _WIN32
namespace detail {

DLLIMPORT int wcstombs (const wstring& wstr, string& cstr, unsigned codepage);
DLLIMPORT int mbstowcs (const string& cstr, wstring& wstr, unsigned codepage);

} // namespace detail
#endif

// wcstombs (SRC, DST)
/// Effects: converts wide character string SRC to multibyte character string DST in
///	     current locale encoding.
/// Returns: number of characters successfully converted.
///    Note: wide characters are either UTF-16 or UTF-32 depending on sizeof(wchar_t)
int wcstombs (const wstring& wstr, string& cstr);

// wcstombs (SRC, DST)
/// Effects: converts multibyte character string SRC in current locale encoding to wide
///	     character string DST.
/// Returns: number of characters successfully converted.
///    Note: wide characters are either UTF-16 or UTF-32 depending on sizeof(wchar_t)
int mbstowcs (const string& cstr, wstring& wstr);

// utf8towcs (SRC, DST)
/// Effects: converts UTF-8 multibyte character string SRC to wide character string DST.
/// Returns: number of characters successfully converted.
///    Note: wide characters are either UTF-16 or UTF-32 depending on sizeof(wchar_t)
int utf8towcs (const string& ustr, wstring& wstr);

// wcstoutf8 (SRC, DST)
/// Effects: converts wide character string SRC to UTF-8 multibyte character string DST.
/// Returns: number of characters successfully converted.
///    Note: wide characters are either UTF-16 or UTF-32 depending on sizeof(wchar_t)
int wcstoutf8 (const wstring& wstr, string& ustr);

#ifdef _WIN32

inline int wcstombs (const wstring& wstr, string& cstr)
    { return detail::wcstombs (wstr, cstr, 0 /* CP_ACP */); }

inline int mbstowcs (const string& cstr, wstring& wstr)
    { return detail::mbstowcs (cstr, wstr, 0 /* CP_ACP */); }

inline int utf8towcs (const string& ustr, wstring& wstr)
    { return detail::mbstowcs (ustr, wstr, 65001u); }

inline int wcstoutf8 (const wstring& wstr, string& ustr)
    { return detail::wcstombs (wstr, ustr, 65001u); }

#else

// assume default encoding for multibyte strings is UTF-8 on non-win32 platforms

inline int utf8towcs (const string& ustr, wstring& wstr)
    { return mbstowcs (ustr, wstr); }

inline int wcstoutf8 (const wstring& wstr, string& ustr)
    { return wcstombs (wstr, ustr); }

#endif // _WIN32

// mbslen (STR)
/// \return length, in characters, of the null-terminated UTF-8 multibyte character
///	    sequence STR.
DLLIMPORT size_t mbslen (const char* str);

// mbslen (STR, SIZE)
//! \return length, in characters, of the UTF-8 multibyte character sequence STR.
//!	    Sequence is limited to SIZE bytes.
DLLIMPORT size_t mbslen (const char* str, size_t byte_len);

// ---------------------------------------------------------------------------
/// \class local_buffer
/// \brief Buffer that uses stack for small allocations and dynamic memory for larger ones.

template <class T>
class local_buffer
{
public:
    typedef T		value_type;
    typedef std::size_t	size_type;

    enum {
	// for large types, reduce size of stack-allocated array
	default_size = (255 - sizeof(value_type*)*2) / sizeof(T) + 1
    };

    local_buffer () : m_size (default_size) { m_ptr = m_buf; }

    explicit local_buffer (size_t initial_size)
	{
	    if (initial_size > default_size)
	    {
		m_ptr = new value_type[initial_size];
		m_size = initial_size;
	    }
	    else
	    {
		m_ptr = m_buf;
		m_size = default_size;
	    }
	}

    ~local_buffer () { if (m_ptr != m_buf) delete[] m_ptr; }

    /// make sure buffer is enough to hold SIZE objects, reallocating if necessary.
    /// old contents is lost after reallocation. 
    void reserve (size_t size)
	{
	    if (size > m_size)
	    {
		value_type* new_ptr = new value_type[size];
		if (m_ptr != m_buf) delete[] m_ptr;
		m_ptr = new_ptr;
		m_size = size;
	    }
	}

    size_type size () const { return m_size; }

    value_type* get () { return m_ptr; }
    const value_type* get () const { return m_ptr; }

    value_type& operator[] (std::ptrdiff_t i) { return m_ptr[i]; }
    const value_type& operator[] (std::ptrdiff_t i) const { return m_ptr[i]; }

private:

    local_buffer (const local_buffer&);		// not defined
    local_buffer& operator= (const local_buffer&);

    size_type		m_size;
    value_type*		m_ptr;
    value_type		m_buf[default_size];
};

// ---------------------------------------------------------------------------
/// \class uni_string

class uni_string
{
    string	m_cstr;
    wstring	m_wstr;

public:
    uni_string () { }
    uni_string (const char* str) : m_cstr (str? str: "") { }
    uni_string (const wchar_t* str) : m_wstr (str? str: L"") { }

    template <typename CharT>
    const basic_string<CharT>& get_string ();

    template <typename CharT>
    const CharT* get () { return get_string<CharT>().c_str(); }

    const char* get_cstr();
    const wchar_t* get_wstr();

    bool empty () const { return m_cstr.empty() && m_wstr.empty(); }

    void assign (const char* str, size_t len)
	{ m_cstr.assign (str, len); m_wstr.clear(); }
    void assign (const wchar_t* str, size_t len)
       	{ m_wstr.assign (str, len); m_cstr.clear(); }

    void assign (const string& str) { m_cstr.assign (str); m_wstr.clear(); }
    void assign (const wstring& str) { m_wstr.assign (str); m_cstr.clear(); }
};

template <> inline const string& uni_string::get_string<char> ()
{
    if (m_cstr.empty() && !m_wstr.empty())
	wcstombs (m_wstr, m_cstr);
    return m_cstr;
}

template <> inline const wstring& uni_string::get_string<wchar_t> ()
{
    if (m_wstr.empty() && !m_cstr.empty())
	mbstowcs (m_cstr, m_wstr);
    return m_wstr;
}

inline const char* uni_string::get_cstr()
{ return get_string<char>().c_str(); }

inline const wchar_t* uni_string::get_wstr()
{ return get_string<wchar_t>().c_str(); }

// ---------------------------------------------------------------------------
namespace unicode
{
    namespace detail
    {
	// mb_len_max<char_type> (N)
	// Returns: returns maximum number of bytes required to hold N characters of
	// type char_type.

	template <typename char_type>
	size_t mb_len_max (size_t n);

	template<>
	inline size_t mb_len_max<char> (size_t n) { return n * MB_LEN_MAX; }

	template<>
	inline size_t mb_len_max<wchar_t> (size_t n) { return n * sizeof(wchar_t); }

	// template opp_type<CharT>
	// defines typedefs for character type opposite to CharT
	// i.e. for 'char' defines 'wchar_t' and vice versa.

	template <class CharT>
	struct opp_type { };

	template <class CharT>
	struct char_def_type
	{
	    typedef CharT			char_type;
	    typedef std::char_traits<CharT>	traits_type;
	    typedef basic_string<CharT>		string_type;
	};

	template <>
	struct opp_type<char> : char_def_type<wchar_t>
	{ };

	template <>
	struct opp_type<wchar_t> : char_def_type<char>
	{ };
    } // namespace detail
} // namespace unicode
} // namespace sys

#endif /* SYSSTRING_H */
