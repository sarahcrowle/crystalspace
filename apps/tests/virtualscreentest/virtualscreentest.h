/*
  Copyright (C) 2014 Christian Van Brussel, Institute of Information
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
#ifndef __VIRTUALSCREENTEST_H__
#define __VIRTUALSCREENTEST_H__

#include "cstool/demoapplication.h"
#include "ivaria/icegui.h"

class VirtualScreenTest : public CS::Utility::DemoApplication
{
private:
  csRef<iCEGUI> cegui;

  size_t screenWidth, screenHeight;
  csRef<iTextureHandle> screenTexture;
  csRef<iMeshWrapper> screenMesh;
  csRef<iGeneralMeshState> screenGenmesh;
  bool inScreen;

private:
  void DrawVirtualScreen ();
  void RemapMousePosition ();

  bool CreateRoom ();

public:
  VirtualScreenTest ();

  bool OnExitButtonClicked (const CEGUI::EventArgs&);

  //-- CS::Utility::DemoApplication
  void Frame ();

  //-- csBaseEventHandler
  bool OnMouseDown (iEvent &event);
  bool OnMouseUp (iEvent &event);

  //-- csApplicationFramework
  bool OnInitialize (int argc, char* argv[]);
  bool Application ();
};

#endif // __VIRTUALSCREENTEST_H__
