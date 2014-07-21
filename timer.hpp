// -*- C++ -*-
//! \file       timer.hpp
//! \date       Sat Jul 21 20:07:38 2007
//! \brief      high-performance timer that measures elapsed time.
//
// Uses same interface as boost::timer.
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

#ifndef SYS_TIMER_HPP
#define SYS_TIMER_HPP

#include "sysdef.h"
#include <limits>		// for std::numeric_limits
#include <boost/cstdint.hpp>	// for boost::uint64_t
#ifdef _WIN32
#include <windows.h>
#elif defined(HAS_POSIX_CLOCK)
#include <time.h>
#else
#include <boost/timer.hpp>
#endif
#include "syserror.h"

namespace sys {

#ifdef _WIN32

class SYSPP_DLLIMPORT timer
{
public:
    typedef boost::uint64_t clock_t;

    // default ctor
    // Postconditions: elapsed() == 0
    timer ()
	{
	    if (!sys_info.sys_freq)
		SYS_THROW_GENERIC_ERROR ("high-resolution timer not available");
	    detail::query (_start_time);
	}

    // restart()
    // Postconditions: elapsed() == 0
    void restart () { detail::query (_start_time); }

    double elapsed () const                  // return elapsed time in seconds
	{
	    clock_t current;
	    detail::query (current);
	    return double(current - _start_time) / sys_info.sys_freq;
       	}

    double elapsed_max() const // return estimated maximum value for elapsed()
    // Portability warning: elapsed_max() may return too high a value on systems
    // where clock_t overflows or resets at surprising values.
	{
	    return (double (std::numeric_limits<clock_t>::max())
		    - double(_start_time)) / double(sys_info.sys_freq); 
	}

    static double elapsed_min() // return minimum value for elapsed()
	{ return 1.0/double(sys_info.sys_freq); }

private:
    struct SYSPP_DLLIMPORT detail
    {
	clock_t sys_freq;

	detail () : sys_freq (0)
       	{ ::QueryPerformanceFrequency ((LARGE_INTEGER*)&sys_freq); }

	static void query (clock_t& t)
	{ ::QueryPerformanceCounter ((LARGE_INTEGER*)&t); }
    };

    static const detail sys_info;

    clock_t	_start_time;
}; // timer

#elif HAS_POSIX_CLOCK

class SYSPP_DLLIMPORT timer
{
public:
    typedef struct timespec clock_t;

    // default ctor
    // Postconditions: elapsed() == 0
    timer ()
	{
	    if (!sys_info.sys_res.tv_nsec)
		SYS_THROW_GENERIC_ERROR ("high-resolution timer not available");
	    detail::query (_start_time);
	}

    // restart()
    // Postconditions: elapsed() == 0
    void restart () { detail::query (_start_time); }

    double elapsed () const                  // return elapsed time in seconds
	{
	    clock_t current;
	    detail::query (current);
	    double diff_nsec = double (current.tv_nsec - _start_time.tv_nsec) / 1e9;
	    if (current.tv_sec == _start_time.tv_sec)
    		return diff_nsec;
	    else if (current.tv_sec > _start_time.tv_sec)
		return diff_nsec + 1 + current.tv_sec - _start_time.tv_sec;
	    else
		return double (std::numeric_limits<time_t>::max() - _start_time.tv_sec)
		    + current.tv_sec + diff_nsec + 2;
       	}

    double elapsed_max() const // return estimated maximum value for elapsed()
	{ return double (std::numeric_limits<time_t>::max()); }

    static double elapsed_min() // return minimum value for elapsed()
	{ return sys_info.sys_res.tv_nsec / 1e9; }

private:
    struct SYSPP_DLLIMPORT detail
    {
	clock_t	    sys_res;

	detail () : sys_res {0,0}
       	{ ::clock_getres (CLOCK_MONOTONIC, &sys_res); }

	static void query (clock_t& t)
	{ ::clock_gettime (CLOCK_MONOTONIC, &t); }
    };

    static const detail sys_info;

    clock_t	_start_time;
}; // timer

#else

using boost::timer;

#endif

} // namespace sys

#endif /* SYS_TIMER_HPP */
