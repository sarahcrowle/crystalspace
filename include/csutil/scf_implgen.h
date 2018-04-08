/*
  Crystal Space Shared Class Facility (SCF)
  This header contains the parts of SCF that is needed when creating
  new classes which implements SCF interfaces.

  Copyright (C) 2005 by Marten Svanfeldt
            (C) 2005 by Michael Adams

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

/**\file
 * SCF implementation base classes
 */

/*
 * NOTICE! This file is statically preprocessed into scf_implgen_p.h.
 * Read comment in top of that file if doing any changes here!
 */

#if !defined(SCF_IN_IMPLEMENTATION_H) && !defined(DOXYGEN_RUN)
#error Do not include this file directly. Included from scf_implementation.h
#endif

#if 0
/* WTF... seems without a class here doxygen will ignore all the stuff 
 * generated below. Whatever, take it, keep it. */
class __Doxygen_Workaround__ {};
#endif

#define SCF_IN_IMPLGEN1_H 1

// Generate classes with direct interface inheritance
#include "scf_implgen2.h"

// Generate classes with virtual interface inheritance
#define SCF_IMPL_VIRT
#include "scf_implgen2.h"
#undef SCF_IMPL_VIRT

#undef SCF_IN_IMPLGEN_H1

