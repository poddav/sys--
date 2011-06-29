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
#include <iterator>	// for std::back_inserter
#include <cassert>
#include "sysdef.h"
#include "bindata.h"

namespace sys {

#ifndef SYSPP_USE_EXT_STRING
using std::string;
using std::basic_string;
using std::basic_istringstream;
using std::basic_ostringstream;
using std::basic_stringstream;
#else
using ext::string;
using ext::basic_string;
using ext::basic_istringstream;
using ext::basic_ostringstream;
using ext::basic_stringstream;
#endif // SYSPP_USE_EXT_STRING

using bin::uint8_t;
using bin::uint16_t;
using bin::uint32_t;
using std::size_t;

#if defined(_WIN32) || SYSPP_SIZEOF_WCHAR_T == 2
typedef wchar_t  UChar16;
#define _U16(STR)	L##STR
#elif HAS_CHAR16_T
typedef char16_t UChar16;
#define _U16(STR)	u##STR
#elif defined(__CHAR16_TYPE__)
typedef __CHAR16_TYPE__ UChar16;
#define _U16(STR)	u##STR
#else
typedef uint16_t UChar16;
#define _U16(STR)	STR
#endif

typedef uint8_t  UChar8;
typedef uint32_t UChar32;
typedef UChar16  UChar;

typedef basic_string<UChar> UString; // UTF-16 string
typedef UString  wstring;

static const UChar32 replacement_code_point = 0xfffd; // code point for invalid characters

// ---------------------------------------------------------------------------

// wcstombs (SRC, DST)
/// Effects: converts wide character string SRC to multibyte character string DST in
///	     current locale encoding.
/// Returns: number of characters successfully converted.
///    Note: wide characters represented in UTF-16 encoding.
int wcstombs (const wstring& wstr, string& cstr);

// wcstombs (SRC, DST)
/// Effects: converts multibyte character string SRC in current locale encoding to wide
///	     character string DST.
/// Returns: number of characters successfully converted.
///    Note: wide characters represented in UTF-16 encoding.
int mbstowcs (const string& cstr, wstring& wstr);

// utf8towcs (SRC, DST)
/// Effects: converts UTF-8 multibyte character string SRC to wide character string DST.
/// Returns: number of characters successfully converted.
///    Note: wide characters represented in UTF-16 encoding.
int utf8towcs (const string& ustr, wstring& wstr);
int u8tou16 (const string& src, wstring& dst);

// wcstoutf8 (SRC, DST)
/// Effects: converts wide character string SRC to UTF-8 multibyte character string DST.
/// Returns: number of characters successfully converted.
///    Note: wide characters represented in UTF-16 encoding.
int wcstoutf8 (const wstring& wstr, string& ustr);
int u16tou8 (const wstring& src, string& dst);

// u16len (STR)
/// \return length, in characters, of the null-terminated UTF-16 character sequence STR.
inline size_t u16len (const UChar* str)
    { return std::char_traits<UChar>::length (str); }

// mbslen (STR)
/// \return length, in characters, of the null-terminated UTF-8 multibyte character
///	    sequence STR.
DLLIMPORT size_t mbslen (const char* str);

// mbslen (STR, SIZE)
//! \return number of characters in the UTF-8 multibyte character sequence STR.
//!	    Sequence is limited to SIZE bytes.
DLLIMPORT size_t mbslen (const char* str, size_t byte_len);

#ifdef _WIN32

namespace detail {

DLLIMPORT int wcstombs (const wstring& wstr, string& cstr, unsigned codepage);
DLLIMPORT int mbstowcs (const string& cstr, wstring& wstr, unsigned codepage);

} // namespace detail

inline int wcstombs (const wstring& wstr, string& cstr)
    { return detail::wcstombs (wstr, cstr, 0 /* CP_ACP */); }

inline int mbstowcs (const string& cstr, wstring& wstr)
    { return detail::mbstowcs (cstr, wstr, 0 /* CP_ACP */); }

inline int utf8towcs (const string& ustr, wstring& wstr)
    { return detail::mbstowcs (ustr, wstr, 65001 /* CP_UTF8 */); }

inline int wcstoutf8 (const wstring& wstr, string& ustr)
    { return detail::wcstombs (wstr, ustr, 65001 /* CP_UTF8 */); }

#else

// assume default encoding for multibyte strings is UTF-8 on non-win32 platforms

inline int wcstombs (const wstring& wstr, string& cstr)
    { return u16tou8 (wstr, cstr); }

inline int mbstowcs (const string& cstr, wstring& wstr)
    { return u8tou16 (cstr, wstr); }

inline int utf8towcs (const string& ustr, wstring& wstr)
    { return u8tou16 (ustr, wstr); }

inline int wcstoutf8 (const wstring& wstr, string& ustr)
    { return u16tou8 (wstr, ustr); }

#endif // _WIN32

namespace detail {

// u8tou32 (FIRST, LAST)
// convert single UTF-8 character into Unicode code point.
// Requires: FIRST != LAST
// Returns: converted unicode character or -1 if invalid UTF-8 sequence was
//          encountered.
// Posteffects: iterator FIRST points to the byte past the end of the converted
//              sequence.

template <class Iterator>
UChar32 u8tou32 (Iterator& first, Iterator last)
{
    assert (first != last);

    UChar8 ch = *first++;
    if (ch <= 0x7f) // ASCII character
	return ch;

    if ((ch & 0xc0) != 0xc0) // invalid UTF-8 sequence
	return ch;

    UChar32 code_point = 0;
    int code_length = 0;
    char mask = 0x40;
    for (;;)
    {
	if (first == last) // incomplete UTF-8 sequence
	{
	    code_point = -1;
	    break;
	}
	UChar8 next = *first;
	if ((next & 0xc0) != 0x80)
	{
	    code_point = -1;
	    break;
	}
	++first;
	code_point = (code_point << 6) | (next & 0x3f);
	++code_length;
	if ((mask >>= 1) == 1) // too long UTF-8 sequence, discard it
	{
	    code_point = -1;
	    break;
	}
	if (0 == (ch & mask))
	{
	    code_point |= static_cast<UChar32> (ch & (mask - 1)) << (6 * code_length);
	    break;
	}
    }
    return code_point;
}

// u32tou8 (CODE, OUT)
// convert Unicode code point into UTF-8 sequence and put it into OUT iterator
// Returns: number of bytes in resulting UTF-8 sequence

template <class Iterator>
int u32tou8 (UChar32 code, Iterator out)
{
    if (code > 0x10ffff)
	code = replacement_code_point;
    if (code <= 0x7f)
    {
	*out++ = static_cast<char> (code);
	return 1;
    }
    else if (code <= 0x7ff)
    {
	*out++ = static_cast<char> (0xc0 | (code >> 6));
	*out++ = static_cast<char> (0x80 | (code & 0x3f));
	return 2;
    }
    else if (code <= 0xffff)
    {
	*out++ = static_cast<char> (0xe0 | (code >> 12));
	*out++ = static_cast<char> (0x80 | ((code >> 6) & 0x3f));
	*out++ = static_cast<char> (0x80 | (code & 0x3f));
	return 3;
    }
    else
    {
	*out++ = static_cast<char> (0xf0 | (code >> 18));
	*out++ = static_cast<char> (0x80 | ((code >> 12) & 0x3f));
	*out++ = static_cast<char> (0x80 | ((code >> 6)  & 0x3f));
	*out++ = static_cast<char> (0x80 | (code & 0x3f));
	return 4;
    }
}

} // namespace detail

template <class InIterator, class OutIterator>
int u8tou16 (InIterator first, InIterator last, OutIterator out)
{
    int count = 0;
    while (first != last)
    {
	UChar32 code_point = detail::u8tou32 (first, last);
	// first is updated by u8tou32

	if (code_point > 0x10ffff)
	    code_point = replacement_code_point;
	if (code_point < 0x10000)
	{
	    *out++ = static_cast<UChar> (code_point);
	}
	else
	{
	    code_point -= 0x10000;
	    *out++ = (static_cast<UChar> (0xd800 + (code_point >> 10)));
	    *out++ = (static_cast<UChar> (0xdc00 + (code_point & 0x3ff)));
	}
	++count;
    }
    return count;
}

inline int u8tou16 (const string& src, wstring& dst)
{
    dst.clear();
    if (dst.capacity() < src.size())
	dst.reserve (src.size());
    return u8tou16 (src.begin(), src.end(), std::back_inserter (dst));
}

template <class InIterator, class OutIterator>
int u16tou8 (InIterator first, InIterator last, OutIterator out)
{
    int count = 0;
    while (first != last)
    {
	UChar32 code_point = *first++;
	if (code_point >= 0xd800 && code_point <= 0xdbff)
	{
	    if (first != last)
	    {
		UChar32 next = *first;
		if (next >= 0xdc00 && next <= 0xdfff)
		{
		    ++first;
		    code_point = (code_point - 0xd800) << 10;
		    code_point |= next - 0xdc00;
		    code_point += 0x10000;
		}
	    }
	}
	detail::u32tou8 (code_point, out);
	++count;
    }
    return count;
}

inline int u16tou8 (const wstring& src, string& dst)
{
    dst.clear();
    if (dst.capacity() < src.size())
	dst.reserve (src.size());
    return u16tou8 (src.begin(), src.end(), std::back_inserter (dst));
}

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

    value_type* begin () { return m_ptr; }
    value_type* end   () { return m_ptr + m_size; }

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
    uni_string (const UChar* str) : m_wstr (str? str: (const UChar*)L"") { }

    template <typename CharT>
    const basic_string<CharT>& get_string ();

    template <typename CharT>
    const CharT* get () { return get_string<CharT>().c_str(); }

    const char* get_cstr();
    const UChar* get_wstr();

    bool empty () const { return m_cstr.empty() && m_wstr.empty(); }

    void assign (const char* str, size_t len)
	{ m_cstr.assign (str, len); m_wstr.clear(); }
    void assign (const UChar* str, size_t len)
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

template <> inline const wstring& uni_string::get_string<UChar> ()
{
    if (m_wstr.empty() && !m_cstr.empty())
	mbstowcs (m_cstr, m_wstr);
    return m_wstr;
}

inline const char* uni_string::get_cstr()
{ return get_string<char>().c_str(); }

inline const UChar* uni_string::get_wstr()
{ return get_string<UChar>().c_str(); }

// ---------------------------------------------------------------------------
namespace detail
{
    // mb_len_max()
    // Returns: max length of the UTF-8 character, in bytes.
    inline size_t mb_len_max ()	
    {
#ifdef MB_LEN_MAX
	return std::max (MB_LEN_MAX, 4);
#else
	return 4;
#endif
    }

    // mb_len_max<char_type> (N)
    // Returns: returns maximum number of bytes required to hold N characters of
    // type char_type.

    template <typename char_type>
    size_t mb_len_max (size_t n);

    template<>
    inline size_t mb_len_max<char> (size_t n) { return n * MB_LEN_MAX; }

    template<>
    inline size_t mb_len_max<UChar> (size_t n) { return n * sizeof(UChar); }

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
	typedef basic_string<CharT>	string_type;
    };

    template <>
    struct opp_type<char> : char_def_type<UChar>
    { };

    template <>
    struct opp_type<UChar> : char_def_type<char>
    { };
} // namespace detail
} // namespace sys

#endif /* SYSSTRING_H */
