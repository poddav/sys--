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
#ifndef _WIN32
#include <boost/timer.hpp>
#else
#include <windows.h>
#endif
#include "syserror.h"

namespace sys {

#ifdef _WIN32

class DLLIMPORT timer
{
public:
    typedef boost::uint64_t clock_t;

    // default ctor
    // Postconditions: elapsed() == 0
    timer ()
	{
	    if (!sys_info.sys_freq)
		throw generic_error ("high-resolution timer not available");
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
    struct DLLIMPORT detail
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

#else

using boost::timer;

#endif

} // namespace sys

#endif /* SYS_TIMER_HPP */
