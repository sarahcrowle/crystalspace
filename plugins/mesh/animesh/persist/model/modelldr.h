/*
  Copyright (C) 2010-2012 Christian Van Brussel, Institute of Information
      and Communication Technologies, Electronics and Applied Mathematics
      at Universite catholique de Louvain, Belgium
      http://www.uclouvain.be/en-icteam.html

  This library is free software; you can redistribute it and/or
  modify it under the terms of the GNU Library General Public
  License as published by the Free Software Foundation; either
  version 2 of the License, or (at your option) any later version.

  This library is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Library General Public License for more details.

  You should have received a copy of the GNU Library General Public
  License along with this library; if not, write to the Free
  Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/
#ifndef __CS_MODELLDR_H__
#define __CS_MODELLDR_H__

#include "csutil/scf_implementation.h"
#include "imap/reader.h"
#include "imap/writer.h"
#include "iutil/comp.h"
#include "cstool/collisionhelper.h"
#include "csutil/csstring.h"
#include "imesh/skeletonmodel.h"

CS_PLUGIN_NAMESPACE_BEGIN (ModelLoader)
{

  class ModelLoader :
    public scfImplementation2<ModelLoader,
                              iLoaderPlugin,
                              iComponent>
  {
  public:
    ModelLoader (iBase* parent);

    //-- iComponent
    virtual bool Initialize (iObjectRegistry*);

    //-- iLoaderPlugin
    virtual csPtr<iBase> Parse (iDocumentNode* node,
      iStreamSource* ssource, iLoaderContext* ldr_context,
      iBase* context);

    virtual bool IsThreadSafe() { return true; }

  private:
    bool ParseModel (iDocumentNode* node, iLoaderContext* ldr_context);
    bool ParseBone (iDocumentNode* node, iLoaderContext* ldr_context);
    bool ParseChain (iDocumentNode* node);

    iObjectRegistry* object_reg;
    csRef<iSyntaxService> synldr;
    csRef<CS::Animation::iSkeletonModelManager> modelManager;
    csRef<CS::Animation::iSkeletonModel> model;
    CS::Collisions::CollisionHelper collisionHelper;

    csStringHash xmltokens;
#define CS_TOKEN_ITEM_FILE \
  "plugins/mesh/animesh/persist/model/modelldr.tok"
#include "cstool/tokenlist.h"
#undef CS_TOKEN_ITEM_FILE
  };

}
CS_PLUGIN_NAMESPACE_END (ModelLoader)

#endif
