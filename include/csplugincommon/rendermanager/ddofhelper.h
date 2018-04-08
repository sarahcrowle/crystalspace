/*
    Copyright (C) 2013 by Pedro SOUZA

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

#ifndef __CS_CSPLUGINCOMMON_RENDERMANAGER_DDOFHELPER_H__
#define __CS_CSPLUGINCOMMON_RENDERMANAGER_DDOFHELPER_H__

#include "iutil/objreg.h"
#include "imap/loader.h"

#include "csplugincommon/rendermanager/posteffects.h"



namespace CS
{
  namespace RenderManager
  {

    /*
     * Helper class to properly construct the diffusion dof post effect.
     * As the number of layers of the effect is dependant of the screen
     * resolution one can not use xml to define the effect.
     * 
     * The DDOF effect uses the heat difusion equation to perform a 
     * variable width blur based on the Circle of Confusion of each
     * pixel. This method generates a tridiagonal system, so resolving
     * the system the final pixel color is obtained.
     *
     * The DDOF effect consists of 3 kinds of layers:
     *    build_abc - this layer calculates the coefficients (a, b, c) of
     * the tridiagonal system. Each row(or column) of the output texture is
     * a whole system.
     *    reduce - this layer reduces the number of variables in the system
     * by half leaving only variables with odd indices.
     *    resolve - given a solution of a reduced system, that is, the odd
     * variables, this layer resolves the even variables by using backsubstitution
     *
     * Thus the system is recursively reduced until is reached a 2 or 3 variable
     * system that can be solved fast. Then we recursively perform the
     * backsubtitution. This algorithm is known as Cyclic Reduction.
     *
		 * Notice that the DDOFHelper must be created on heap due to it being
		 * referenced by the PostEffect (if used the setup view callback).
     */
    class CS_CRYSTALSPACE_EXPORT DDOFHelper : public scfImplementation0<DDOFHelper>
    {
      csRef<iLoader> loader;
      csHash<csRef<iShader>, csString> shaders;
      static const char * messageID;

      iObjectRegistry* objReg;
      /**
       * Load the given shader and sets the layer name
       */
      bool MakeDesc(LayerDesc * desc, const char * shader, const char * name);

      /**
       * Add an input to desc with the given parameters
       */
      bool AddInput(LayerDesc * desc, const char * name, const char * texname = "tex diffuse", bool genPixSize = false);

      /**
       * Add an output to desc with the given parameters
       */
      bool AddOutput(LayerDesc * desc, const char * name, const char * fmt, int downsample = 0, DownsampleAxis axis = AXIS_XY);

      /**
       * Setup a reduction LayerDesc based on the given parameters
       */
      bool ConstructReduce(LayerDesc * desc, DownsampleAxis axis, int num, const char * color = NULL, const char * abc = NULL);

      /**
       * Setup a resolve LayerDesc based on the given parameters
       */
      bool ConstructResolve(LayerDesc * desc, DownsampleAxis axis, int num, const char * color = NULL, const char * abc = NULL);

      /**
       * Setup a resolve LayerDesc based on the given parameters.
       * This function is used to setup only the first resolve layer
       * based on its dimension (only 2 or 3 are allowed).
       */
      bool ConstructResolve(LayerDesc * desc, DownsampleAxis axis, int num, int dim);

      /**
       * Setup the build_abc layer, that is, the tridiagonal system
       * and add a default CoCScale of 15.0f
       */
      bool ConstructBuild(LayerDesc * desc, DownsampleAxis axis);

    public:

      /**
       * Constructor
       */
      DDOFHelper(iObjectRegistry* reg): scfImplementationType (this), objReg(reg) {}

      /*
       * Adds a setup view callback to handle resolution changes.
       * The setup is done when iPostEffect->SetupView is called.
       */
      bool Setup(iPostEffect * pEffect);

      /*
       * Setup the DDOF effect
       * This method does not adds a callback to handle resolution changes!
       */
      bool Setup(iPostEffect * pEffect, int width, int height);

      /**
       * Change the CoC Scale, given a DDOF effect.
       */
      bool SetCoCScale(iPostEffect * pEffect, float scale);
    };
  }
}

#endif
