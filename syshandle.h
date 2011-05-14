/*!-*- C++ -*-
 * \file        syshandle.h
 * \date        Tue Nov 28 07:28:39 2006
 * \brief       generic handle class.
 *
 * $Id$
 */

#ifndef SYSHANDLE_H
#define SYSHANDLE_H

#include "sysdef.h"
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#endif

namespace sys {

//! \class generic_handle
//! \brief exception-safe wrapper for handle objects

enum {
    win_invalid_handle		= 0,
    win_invalid_file		= -1,
    posix_invalid_handle	= -1,
};

#ifdef _WIN32
typedef HANDLE	raw_handle;
#else
typedef int	raw_handle;
#endif

template <long invalid_handle_value>
class DLLIMPORT generic_handle
{
public:
    typedef raw_handle	handle_type;

    generic_handle (handle_type h = invalid_handle())
       	: handle (h)
       	{ }

    generic_handle (generic_handle& other)
	: handle (other.release())
	{ }

    ~generic_handle () { close(); }

    generic_handle& operator= (handle_type h)
	{ reset (h); return *this; }

    generic_handle& operator= (generic_handle& other)
	{
	    if (&other != this) reset (other.release());
	    return *this;
	}

    template <long ihv>
    generic_handle& operator= (generic_handle<ihv>& other)
	{
	    if (&other != this)
	    {
		if (other.valid())
		    reset (other.release());
		else
		    close();
	    }
	    return *this;
	}

    bool close ()
	{
	    bool rc = false;
	    if (handle != invalid_handle())
	    {
		rc = close_handle (handle);
		handle = invalid_handle();
	    }
	    return rc;
	}

    void reset (handle_type new_h = invalid_handle())
	{
	    if (handle != new_h)
	    {
		close();
		handle = new_h;
	    }
	}

    bool valid () const { return handle != invalid_handle(); }

    bool operator! () const { return !valid(); }

    // implicit conversion operator
    //
    operator handle_type () const { return handle; }

    operator bool () const { return valid(); }

    handle_type get () const { return handle; }

    handle_type release ()
	{
	    handle_type h = handle;
	    handle = invalid_handle();
	    return h;
	}

    static handle_type invalid_handle ()
       	{
#ifdef _WIN32
	    return reinterpret_cast<handle_type> (invalid_handle_value);
#else
	    return static_cast<handle_type> (invalid_handle_value);
#endif
       	}

private:
    handle_type		handle;

    static bool close_handle (handle_type h)
	{
#ifdef _WIN32
	    return ::CloseHandle (h);
#else
	    return ::close (h) != -1;
#endif
	}
};

#ifdef _WIN32
typedef generic_handle<win_invalid_handle>	handle;
typedef generic_handle<win_invalid_file>	file_handle;
#else
typedef generic_handle<posix_invalid_handle>	handle;
typedef generic_handle<posix_invalid_handle>	file_handle;
#endif

}

#endif /* SYSHANDLE_H */
