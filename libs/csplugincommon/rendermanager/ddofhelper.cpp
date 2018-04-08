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

#include "cssysdef.h"
#include "csplugincommon/rendermanager/ddofhelper.h"
#include "iengine/rendermanager.h"
#include "iutil/cfgfile.h"
#include "ivaria/reporter.h"


namespace CS
{
  namespace RenderManager
  {

#define REDUCE_X     0
#define REDUCE_Y     1
#define RESOLVE_X    2
#define RESOLVE_Y    3
#define RESOLVE_X2   4
#define RESOLVE_Y2   5
#define RESOLVE_X3   6
#define RESOLVE_Y3   7
#define BUILD_ABC_X  8
#define BUILD_ABC_Y  9


    const char * const shaderPath[] = {
      "/shader/postproc/DDOF/reduce_x.xml",
      "/shader/postproc/DDOF/reduce_y.xml",
      "/shader/postproc/DDOF/resolve_x.xml",
      "/shader/postproc/DDOF/resolve_y.xml",
      "/shader/postproc/DDOF/resolve_x2.xml",
      "/shader/postproc/DDOF/resolve_y2.xml",
      "/shader/postproc/DDOF/resolve_x3.xml",
      "/shader/postproc/DDOF/resolve_y3.xml",
      "/shader/postproc/DDOF/build_abc_x.xml",
      "/shader/postproc/DDOF/build_abc_y.xml"
    };

    const char * DDOFHelper::messageID = "crystalspace.rendermanager.posteffect.ddofhelper";

    
    class DDOFSetupView : public scfImplementation1<DDOFSetupView, iSetupViewCallback>
    {
      csRef<DDOFHelper> helper;
      csWeakRef<iPostEffect> effect;
      int w, h;
    public:
      DDOFSetupView (DDOFHelper *pHelper, iPostEffect * pEffect) : scfImplementationType (this), 
        helper (pHelper), effect (pEffect), w(-1), h(-1)
      {
      }

      bool operator () (int width, int height)
      {
        if (w == width && h == height) return true;
        w = width;
        h = height;
        if (helper.IsValid() && effect.IsValid())
        {
          bool ret = helper->Setup(effect, width, height);
          ret &= effect->Construct(true);
          return ret;
        }
        return false;
      }
    };


    bool DDOFHelper::MakeDesc(LayerDesc * desc, const char * shader, const char * name)
    {
      desc->layerShader = shaders.Get (shader, 0);
      if (!desc->layerShader.IsValid())
      {
        desc->layerShader = loader->LoadShader (shader);
        if (!desc->layerShader.IsValid()) return false;
        shaders.Put (shader, desc->layerShader);
      }
      desc->name = name;
      return true;
    }

    bool DDOFHelper::AddInput(LayerDesc * desc, const char * name, const char * texname, bool genPixSize)
    {
      PostEffectLayerInputMap inp;
      inp.type = AUTO;
      inp.sourceName = name;
      inp.svTextureName = texname;
      inp.svTexcoordName = "texture coordinate 0";
      inp.svPixelSizeName =  genPixSize ? "pixel size" : "";

      desc->inputs.Push(inp);
      return true;
    }
    bool DDOFHelper::AddOutput(LayerDesc * desc, const char * name, const char * fmt, int downsample, DownsampleAxis axis)
    {
      PostEffectLayerOptions opt;
      opt.name = name;
      opt.info.format = fmt;
      opt.info.reusable = true;
      opt.info.mipmap = false;
      opt.info.maxMipmap = 0;
      opt.info.axis = axis;
      opt.info.downsample = downsample;

      desc->outputs.Push(opt);
      return true;
    }

    bool DDOFHelper::ConstructReduce(LayerDesc * desc, DownsampleAxis axis, int num, const char * color, const char * abc)
    {
      *desc = LayerDesc();

      csString name,
        colorName,
        abcName;
      const char * name_fmt = axis == AXIS_X ? "reduce_%d_x" : "reduce_%d_y";
      const char * shaderName = axis == AXIS_X ? shaderPath[REDUCE_X] : shaderPath[REDUCE_Y];

      name.Format(name_fmt, num);

      if (color) colorName = color;
      else {colorName.Format(name_fmt, num-1); colorName.Append(".color");}

      if (abc) abcName = abc;
      else {abcName.Format(name_fmt, num-1); abcName.Append(".abc");}


      if (!MakeDesc(desc, shaderName, name)) return false;

      AddInput(desc, colorName, "tex color", true);
      AddInput(desc, abcName, "tex coef");

      AddOutput(desc, "color", "argb32_f", num, axis);
      AddOutput(desc, "abc", "argb32_f", num, axis);

      return true;
    }

    bool DDOFHelper::ConstructResolve(LayerDesc * desc, DownsampleAxis axis, int num, const char * color, const char * abc)
    {
      *desc = LayerDesc();

      csString name,
        colorName,
        abcName,
        solution;
      const char * name_fmt = axis == AXIS_X ? "resolve_%d_x" : "resolve_%d_y";
      const char * shaderName = axis == AXIS_X ? shaderPath[RESOLVE_X] : shaderPath[RESOLVE_Y];

      name.Format(name_fmt, num);
      solution.Format(name_fmt, num+1);

      name_fmt = axis == AXIS_X ? "reduce_%d_x" : "reduce_%d_y";
      if (color) colorName = color;
      else {colorName.Format(name_fmt, num); colorName.Append(".color");}

      if (abc) abcName = abc;
      else {abcName.Format(name_fmt, num); abcName.Append(".abc");}


      if (!MakeDesc(desc, shaderName, name)) return false;

      AddInput(desc, colorName, "tex color");
      AddInput(desc, abcName, "tex coef");
      AddInput(desc, solution, "tex solution", true);

      AddOutput(desc, "color", "argb32_f", num, axis);

      return true;
    }

    bool DDOFHelper::ConstructResolve(LayerDesc * desc, DownsampleAxis axis, int num, int dim)
    {
      *desc = LayerDesc();

      csString name,
        inputColor,
        inputAbc;

      const char * shader;
      const char * name_fmt = axis == AXIS_X ? "resolve_%d_x" : "resolve_%d_y";
      const char * input_fmt = axis == AXIS_X ? "reduce_%d_x" : "reduce_%d_y";
      name.Format(name_fmt, num);
      inputColor.Format(input_fmt, num); inputColor.Append(".color");
      inputAbc.Format(input_fmt, num); inputAbc.Append(".abc");

      if (dim == 2) shader = axis == AXIS_X ?  shaderPath[RESOLVE_X2] : shaderPath[RESOLVE_Y2];
      else if (dim == 3) shader = axis == AXIS_X ?  shaderPath[RESOLVE_X3] : shaderPath[RESOLVE_Y3];
      else return false;

      if (!MakeDesc(desc, shader, name)) return false;

      AddInput(desc, inputColor, "tex color");
      AddInput(desc, inputAbc, "tex coef");

      AddOutput(desc, "out", "argb32_f", num, axis);
      return true;
    }

    bool DDOFHelper::ConstructBuild(LayerDesc * desc, DownsampleAxis axis)
    {
      *desc = LayerDesc();

      const char * name = axis == AXIS_X ? "build_abc_x" : "build_abc_y";
      const char * shader = axis == AXIS_X ? shaderPath[BUILD_ABC_X] : shaderPath[BUILD_ABC_Y];
      if (!MakeDesc(desc, shader, name)) return false;
      AddOutput(desc, "out", "argb32_f");

      return true;
    }


    bool DDOFHelper::Setup(iPostEffect * pEffect, int width, int height)
    {
      LayerDesc desc;
      csRef<csShaderVariable> cocScale;

      csRef<iShaderVarStringSet> svStrings = csQueryRegistryTagInterface<iShaderVarStringSet> (objReg,
        "crystalspace.shader.variablenameset");
      if (!svStrings.IsValid()) return false;

      loader = csQueryRegistry<iLoader> (objReg);
      if (!loader.IsValid()) return false;

      pEffect->ClearLayers();

      cocScale.AttachNew(new csShaderVariable(svStrings->Request("coc scale")));
      cocScale->SetValue(15.0f);
      iPostEffectLayer *pLayer;

      int count = 0;
      int dim = width;


      if (!ConstructBuild(&desc, AXIS_X))
      {
        csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
          "Failed to construct DDOF build_abc layer!");
        return false;
      }
      pLayer = pEffect->AddLayer(desc);
      pLayer->AddDefaultVar(cocScale);


      {
        ++count;
        dim >>= 1;
        if (!ConstructReduce(&desc, AXIS_X, count, "*screen", "build_abc_x"))
        {
          csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
            "Failed to construct DDOF reduce layer!");
          return false;
        }
        pEffect->AddLayer(desc);

      }
      while (dim > 3)
      {
        ++count;
        dim >>= 1;
        if (!ConstructReduce(&desc, AXIS_X, count))
        {
          csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
            "Failed to construct DDOF reduce layer!");
          return false;
        }
        pEffect->AddLayer(desc);

      }

      if (!ConstructResolve(&desc, AXIS_X, count, dim))
      {
        csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
          "Failed to construct DDOF resolve layer!");
        return false;
      }
      pEffect->AddLayer(desc);


      while (--count > 0)
      {
        if (!ConstructResolve(&desc, AXIS_X, count))
        {
          csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
            "Failed to construct DDOF resolve layer!");
          return false;
        }
        pEffect->AddLayer(desc);

      }

      if (!ConstructResolve(&desc, AXIS_X, count, "*screen", "build_abc_x"))
      {
        csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
          "Failed to construct DDOF resolve layer!");
        return false;
      }
      pEffect->AddLayer(desc);



      if (!ConstructBuild(&desc, AXIS_Y))
      {
        csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
          "Failed to construct DDOF build_abc layer!");
        return false;
      }
      pLayer = pEffect->AddLayer(desc);
      pLayer->AddDefaultVar(cocScale);


      count = 0;
      dim = height;
      {
        ++count;
        dim >>= 1;
        if (!ConstructReduce(&desc, AXIS_Y, count, "resolve_0_x", "build_abc_y"))
        {
          csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
            "Failed to construct DDOF reduce layer!");
          return false;
        }

        pEffect->AddLayer(desc);

      }
      while (dim > 3)
      {
        ++count;
        dim >>= 1;
        if (!ConstructReduce(&desc, AXIS_Y, count))
        {
          csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
            "Failed to construct DDOF reduce layer!");
          return false;
        }
        pEffect->AddLayer(desc);

      }

      if (!ConstructResolve(&desc, AXIS_Y, count, dim))
      {
        csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
          "Failed to construct DDOF resolve layer!");
        return false;
      }
      pEffect->AddLayer(desc);


      while (--count > 0)
      {
        if (!ConstructResolve(&desc, AXIS_Y, count))
        {
          csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
            "Failed to construct DDOF resolve layer!");
          return false;
        }
        pEffect->AddLayer(desc);

      }

      if (!ConstructResolve(&desc, AXIS_Y, count, "resolve_0_x", "build_abc_y"))
      {
        csReport (objReg, CS_REPORTER_SEVERITY_ERROR, messageID,
          "Failed to construct DDOF reduce layer!");
        return false;
      }

      pEffect->AddLayer(desc);

      return true;
    }

    bool DDOFHelper::SetCoCScale(iPostEffect * pEffect, float scale)
    {
      iPostEffectLayer * pLayer = pEffect->GetLayer("build_abc_x");
      if (!pLayer) return false;
      iShaderVariableContext *pContext = pLayer->GetSVContext();
      if (!pContext) return false;


      csRef<iShaderVarStringSet> svStrings = csQueryRegistryTagInterface<iShaderVarStringSet> (objReg,
        "crystalspace.shader.variablenameset");
      if (!svStrings.IsValid()) return false;


      csRef<csShaderVariable> cocScale;
      cocScale.AttachNew(new csShaderVariable(svStrings->Request("coc scale")));
      cocScale->SetValue(scale);

      pContext->AddVariable(cocScale);


      pLayer = pEffect->GetLayer("build_abc_y");
      if (!pLayer) return false;
      pContext = pLayer->GetSVContext();
      if (!pContext) return false;

      pContext->AddVariable(cocScale);

      return true;
    }

    bool DDOFHelper::Setup (iPostEffect * pEffect)
    {
      if (pEffect)
      {
				csRef<DDOFSetupView> setupView;
				setupView.AttachNew (new DDOFSetupView (this, pEffect));
        pEffect->SetSetupViewCallback (setupView);
        return true;
      }
      return false;
    }
    
  }
}
