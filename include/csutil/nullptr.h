/*
  Copyright (C) 2010 by Mike Gist

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/

#ifndef __CSUTIL_NULLPTR_H__
#define __CSUTIL_NULLPTR_H__

#if !defined(CS_NO_PROVIDE_NULLPTR)
#if !defined(CS_HAS_NULLPTR) \
    || (defined(__STRICT_ANSI__) && !defined(CS_HAS_NULLPTR_STRICT_ANSI))

/*
  Based on "library implementation" in C++ Standards Committee paper 2431
  http://www.open-std.org/jtc1/sc22/wg21/docs/papers/2007/n2431.pdf
*/
namespace std
{ 
  class nullptr_t
  {
  private:
    void operator&() const;

  public:
    template<typename T>
    operator T*() const
    {
      return 0;
    }

    template<class C, typename T>
    operator T C::*() const
    {
      return 0;
    }
  };
}

#ifndef CS_NO_NULLPTR_RENAME_HACK
/* Hack around the "identifier 'nullptr' will become a keyword in C++0x"
 * warnings gcc 4.6 emits.
 * The "proper" way - using a #pragma GCC diagnostic - is, unfortunately,
 * broken: http://gcc.gnu.org/bugzilla/show_bug.cgi?id=48914
 * Instead, to avoid use of that identifier, rename that identifier.
 */
#define nullptr CS_nullptr
#endif

using ::std::nullptr_t;
const nullptr_t nullptr = {};

/*
  some gcc versions (e.g. 4.5) fail to implicitly perform the above casts
  in comparisons (e.g. pointer == nullptr) - to reduce the clutter around
  the engine, overload operator== and operator!=
*/

// overload operator== for comparison of non-member functions with nullptr
template<typename T>
bool operator==(T* const& p, nullptr_t const& n)
{
    return p == static_cast<T* const>(n);
}
template<typename T>
bool operator==(nullptr_t const& n, T* const& p)
{
    return p == static_cast<T* const>(n);
}

// overload operator== for comparison of member functions with nullptr
template<class C, typename T>
bool operator==(T C::* const& p, nullptr_t const& n)
{
    return p == static_cast<T C::* const>(n);
} 
template<class C, typename T>
bool operator==(nullptr_t const& n, T C::* const& p)
{
    return p == static_cast<T C::* const>(n);
}

// overload operator!= for comparison of non-member functions with nullptr
template<typename T>
bool operator!=(T* const& p, nullptr_t const& n)
{
    return p != static_cast<T* const>(n);
}
 
template<typename T>
bool operator!=(nullptr_t const& n, T* const& p)
{
    return p != static_cast<T* const>(n);
}

// overload operator!= for comparison of member functions with nullptr
template<class C, typename T>
bool operator!=(T C::* const& p, nullptr_t const& n)
{
    return p != static_cast<T C::* const>(n);
}
template<class C, typename T>
bool operator!=(nullptr_t const& n, T C::* const& p)
{
    return p != static_cast<T C::* const>(n);
}

#endif /* !defined(CS_HAS_NULLPTR) \
	  || (defined(__STRICT_ANSI__) && !defined(CS_HAS_NULLPTR_STRICT_ANSI)) */
#endif // !defined(CS_NO_PROVIDE_NULLPTR)

#endif // __CSUTIL_NULLPTR_H__
