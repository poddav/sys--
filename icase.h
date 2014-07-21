// -*- C++ -*-
/// \file       icase.h
/// \date       Sat Feb 10 02:13:52 2007
/// \brief      case insensitive string comparison.
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

#ifndef SYS_ICASE_H
#define SYS_ICASE_H

#include <cctype>	// for std::toupper/tolower
#include <string>	// for std::string
#include <algorithm>	// for std::transform
#include <locale>	// for std::locale
#ifdef _WIN32
#include <windows.h>
#else
#include <strings.h>
#endif

#if defined(__MINGW32__) && !defined(LOCALE_INVARIANT)
#define LOCALE_INVARIANT                                                      \
          (MAKELCID(MAKELANGID(LANG_INVARIANT, SUBLANG_NEUTRAL), SORT_DEFAULT))
#endif

namespace icase {

typedef std::string::traits_type	traits_type;

/// strcmp/strncmp
/// \brief case-insensitive character strings comparison

inline int strcmp (const char* lhs, const char* rhs)
{
    using namespace std;
#ifdef _WIN32
    return CompareStringA (LOCALE_INVARIANT, SORT_STRINGSORT|NORM_IGNORECASE,
                           lhs, -1, rhs, -1) - 2;
#elif defined(__unix__) || defined(__CYGWIN__)
    return strcasecmp (lhs, rhs);
#elif defined(_MSC_VER) || (defined(__MINGW32__) && !defined(__STRICT_ANSI__))
    return _stricmp (lhs, rhs);
#else
    return stricmp (lhs, rhs);
#endif
}

inline int strncmp (const char* lhs, const char* rhs, size_t length)
{
    using namespace std;
#ifdef _WIN32
    return CompareStringA (LOCALE_INVARIANT, SORT_STRINGSORT|NORM_IGNORECASE,
                           lhs, length, rhs, length) - 2;
#elif defined(__unix__) || defined(__CYGWIN__)
    return strncasecmp (lhs, rhs, length);
#elif defined(_MSC_VER) || (defined(__MINGW32__) && !defined(__STRICT_ANSI__))
    return _strnicmp (lhs, rhs, length);
#else
    return strnicmp (lhs, rhs, length);
#endif
}

/// eqchar
/// \brief case-insensitive character equality

struct eqchar
{
    bool operator () (char lhs, char rhs) const
    {
	return std::toupper (lhs) == std::toupper (rhs);
    }
};

/// ltstr
/// \brief case-insensitive "less-than" comparison functor

struct ltstr
{
    bool operator() (const std::string& lhs, const std::string& rhs) const
    {
	if (&lhs != &rhs)
	{
	    std::string::const_iterator l = lhs.begin(), l_end = lhs.end();
	    std::string::const_iterator r = rhs.begin(), r_end = rhs.end();
	    while (l != l_end && r != r_end)
	    {
		int cl = std::toupper (traits_type::to_int_type (*l));
		int cr = std::toupper (traits_type::to_int_type (*r));
		if (!traits_type::eq (cl, cr))
		    return traits_type::lt (cl, cr);
		++l;
		++r;
	    }
	    return lhs.size() < rhs.size();
	}
	else
	    return false;
    }
};

/// eqstr
/// \brief case-insensitive equality comparison functor

struct eqstr
{
    bool operator() (const std::string& lhs, const std::string& rhs) const
    {
	if (lhs.size() == rhs.size())
	{
	    std::string::const_iterator l = lhs.begin(), l_end = lhs.end();
	    std::string::const_iterator r = rhs.begin();
	    while (l != l_end)
	    {
		int cl = std::toupper (traits_type::to_int_type (*l));
		int cr = std::toupper (traits_type::to_int_type (*r));
		if (!traits_type::eq (cl, cr))
		    return false;
		++l;
		++r;
	    }
	    return true;
	}
	else
	    return false;
    }

    bool operator() (const std::string& lhs, const char* rhs) const
    {
	std::string::const_iterator l = lhs.begin(), l_end = lhs.end();
	while (l != l_end && *rhs)
	{
	    int cl = std::toupper (traits_type::to_int_type (*l));
	    int cr = std::toupper (traits_type::to_int_type (*rhs));
	    if (!traits_type::eq (cl, cr))
		return false;
	    ++l;
	    ++rhs;
	}
	return l == l_end && !*rhs;
    }

    bool operator() (const char* lhs, const std::string& rhs) const
    {
	return operator() (rhs, lhs);
    }

    bool operator() (const char* lhs, const char* rhs) const
    {
        return 0 == strcmp (lhs, rhs);
    }
};

// ---------------------------------------------------------------------------

/// ctype<CharT>
/// \brief ctype facet of locale.

template <typename CharT>
struct ctype
{
    typedef std::ctype<CharT>	facet_type;

    explicit ctype (const std::locale& loc = std::locale())
	: m_loc (loc)
	, m_ctype (std::use_facet<facet_type> (m_loc))
	// [22.1.2.4]
	// The reference returned by use_facet() remains valid at least as long
	// as any copy of loc exists.
	{ }

    const facet_type& facet () const { return m_ctype; }

protected:
    std::locale		m_loc;
    const facet_type&	m_ctype;
};

// ---------------------------------------------------------------------------
// XXX consider which templatization is better, either by string class or by
// XXX character type.
// ---------------------------------------------------------------------------

/// less<String>
/// \brief std::locale-based "less-than" comparison functor,
///        templatized by string class.

template <class String>
struct less : private ctype<typename String::value_type>
{
    typedef String				string_type;
    typedef typename string_type::value_type	char_type;
    typedef typename string_type::traits_type	traits_type;

    explicit less (const std::locale& loc = std::locale())
       	: ctype<char_type> (loc)
       	{ }

    bool operator() (const string_type& lhs, const string_type& rhs) const
    {
	typename string_type::const_iterator l = lhs.begin(), l_end = lhs.end();
	typename string_type::const_iterator r = rhs.begin(), r_end = rhs.end();
	while (l != l_end && r != r_end)
	{
	    char_type cl = this->facet().toupper (*l);
	    char_type cr = this->facet().toupper (*r);
	    if (!traits_type::eq (cl, cr))
		return traits_type::lt (cl, cr);
	    ++l;
	    ++r;
	}
	return lhs.size() < rhs.size();
    }
};

/// equal_to<CharT, Traits>
/// \brief std::locale-based "equal-to" comparison functor,
///        templatized by character type and character traits.

template <typename CharT, typename Traits = std::char_traits<CharT> >
struct equal_to : private ctype<CharT>
{
    typedef CharT		char_type;
    typedef Traits		traits_type;

    explicit equal_to (const std::locale& loc = std::locale())
	: ctype<char_type> (loc)
	{ }

    template <class String>
    bool operator() (const String& lhs, const String& rhs) const
    {
	if (lhs.size() == rhs.size())
	{
	    typename String::const_iterator l = lhs.begin(), l_end = lhs.end();
	    typename String::const_iterator r = rhs.begin();
	    while (l != l_end)
	    {
		char_type cl = this->facet().toupper (*l);
		char_type cr = this->facet().toupper (*r);
		if (!traits_type::eq (cl, cr))
		    return false;
		++l;
		++r;
	    }
	    return true;
	}
	else
	    return false;
    }
};

// ---------------------------------------------------------------------------

/// hash functor

struct hash
{
    static const size_t init_value = 5381;

    size_t operator () (const std::string& s) const
    {
	size_t h = init_value;
	std::string::const_iterator p = s.begin(), s_end = s.end();
	for ( ; p != s_end; ++p)
	    h = 33 * h + std::toupper (traits_type::to_int_type (*p));
	return h;
    }
    size_t operator () (const char* s) const
    {
	size_t h = init_value;
	int c;
        while ((c = traits_type::to_int_type (*s++)))
            h = 33 * h + std::toupper (c);
	return h;
    }
};

/// upcase
/// \brief functor providing conversion to upper case

struct upcase
{
    char operator() (char s) const {
       	return traits_type::to_char_type (std::toupper (traits_type::to_int_type (s)));
    }
};

/// toupper
/// \brief overloaded functions for upper-case conversion

template <class Iterator>
inline Iterator toupper (Iterator first, Iterator last)
{
    return std::transform (first, last, first, icase::upcase());
}

inline std::string& toupper (std::string& s)
{
    toupper (s.begin(), s.end());
    return s;
}

/// locase
/// \brief functor providing conversion to lower case

struct locase
{
    char operator() (char s) const {
       	return traits_type::to_char_type (std::tolower (traits_type::to_int_type (s)));
    }
};

/// tolower
/// \brief overloaded functions for lower-case conversion

template <class Iterator>
inline Iterator tolower (Iterator first, Iterator last)
{
    return std::transform (first, last, first, icase::locase());
}

inline std::string& tolower (std::string& s)
{
    tolower (s.begin(), s.end());
    return s;
}

} // namespace icase

#endif /* SYS_ICASE_H */
