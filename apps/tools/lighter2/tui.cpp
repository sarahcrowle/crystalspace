/*
  Copyright (C) 2006 by Marten Svanfeldt

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

#include "common.h"

#include "config.h"
#include "statistics.h"
#include "lighter.h"
#include "swappable.h"
#include "tui.h"


namespace lighter
{
  TUI globalTUI;

  TUI::TUI ()
    : scfImplementationType (this), messageBufferEnd (0),
    simpleMode (false), prevWasReporter (false), 
    kdLastNumNodes (0), lastTaskProgress (0.0f)
  {
  }

  void TUI::Initialize (iObjectRegistry* objReg)
  {
    object_reg = objReg;
    simpleMode = globalLighter->configMgr->GetBool ("lighter2.simpletui", false);
  }

  void TUI::Redraw (int drawFlags)
  {
    if (!drawMutex.TryLock())
      return;

    // Redraw the "static" background
    if (simpleMode)
    {
      DrawSimple ();
    }
    else
    {
      // Clear
      if (drawFlags & TUI_DRAW_CLEAR)
        csPrintf (CS_ANSI_CLEAR_SCREEN);

      // Screen layout (keep to 80x25...)
      if (drawFlags & TUI_DRAW_STATIC)
        DrawStatic ();

      // Draw progress
      if (drawFlags & TUI_DRAW_PROGRESS)
        DrawProgress ();

      // Draw messages
      if (drawFlags & TUI_DRAW_MESSAGES)
        DrawMessage ();

      // Draw RayCore stats
      if (drawFlags & TUI_DRAW_RAYCORE)
        DrawRayCore ();

      // Draw photonmapper stats
      if (drawFlags & TUI_DRAW_PMCORE)
        DrawPMCore ();

      // Draw global settings
      if (drawFlags & TUI_DRAW_SETTINGS)
        DrawSettings ();
      
      // Draw global stats
      if (drawFlags & TUI_DRAW_STATS)
        DrawStats ();
      
      if (drawFlags & TUI_DRAW_SWAPCACHE)
        DrawSwapCacheStats ();

      /* Linux: output is buffered, and the UI may appear "incomplete" if not 
       * flushed */
      fflush (stdout);
    }

    drawMutex.Unlock();
  }

  void TUI::FinishDraw ()
  {
    if (simpleMode)
    {
      DrawSimpleEnd ();
    }

    else
    {
      for(size_t i=0; i<25; i++)
        csPrintf("\n");
    }
  }

  static const char* TUI_SEVERITY_TEXT[] = 
  {
    CS_ANSI_FK CS_ANSI_BW "B" CS_ANSI_RST " ",
    CS_ANSI_FW CS_ANSI_BR "X" CS_ANSI_RST " ",
    CS_ANSI_FK CS_ANSI_BY "!" CS_ANSI_RST " ",
    CS_ANSI_FW CS_ANSI_BB "i" CS_ANSI_RST " ",
    CS_ANSI_FW CS_ANSI_BW "D" CS_ANSI_RST " "
  };

  THREADED_CALLABLE_IMPL4(TUI, Report, iReporter* reporter, int severity,
    const char* msgId, const char* description)
  {
    csStringArray descrSplit;
    descrSplit.SplitString (description, "\n");

    for (size_t i = descrSplit.GetSize(); i-- > 0;)
    {
      if (simpleMode)
      {
        if (!prevWasReporter)
          csPrintf ("\n");
        csPrintf ("%s\n", descrSplit[i]);
        prevWasReporter = true;
      }
      else
      {
        csString& buf = messageBuffer[messageBufferEnd];
        buf.Clear ();

        buf.Append (TUI_SEVERITY_TEXT[severity]);
        csString tmp(descrSplit[i]);
        buf.Append (tmp.Slice (0,74).PadRight (74));
      }

      messageBufferEnd = (messageBufferEnd + 1) % messageBufferSize;
    }

    // Print them
    Redraw (TUI::TUI_DRAW_MESSAGES);

    // Also hand it off to the standard listener
    return false;
  }

  void TUI::DrawStatic () const
  {
    csPrintf ("/------------------------    Crystal Space Lighter    ------------------------\\\n");
    csPrintf ("| Total progress:                                                             |\n");
    csPrintf ("|                                                                             |\n");
    csPrintf ("| Part progress:                                                              |\n");
    csPrintf ("|                                                                             |\n");
    csPrintf ("|-----------------------------------------------------------------------------|\n");
    csPrintf ("| Raytracer Stats    | Photon Map Stats   | Settings   | Scene Stats          |\n");
    csPrintf ("|                    |                    | [ ] Raytrc |   Sectors:           |\n");
    csPrintf ("|   Direct:          |   Photns:          | [ ] PtnMap |   Meshes:            |\n");
    csPrintf ("|   Photns:          |   Lookps:          | [ ] LMs    |   Lights:            |\n");
    csPrintf ("|   Reflec:          |   KD-Dth:          | [ ] AO     |   LM:                |\n");
    csPrintf ("|   Refrac:          |                    |-----------------------------------|\n");
    csPrintf ("|   FinalG:          | Irradiance Cache   | KD-Tree Stats                     |\n");
    csPrintf ("|                    |                    |   Nodes:                          |\n");
    csPrintf ("| Total:             |   Prime:           |   Depth:                          |\n");
    csPrintf ("|                    |   Secnd:           |   Prims:                          |\n");
    csPrintf ("|                    |   Lookps:          |-----------------------------------|\n");
    csPrintf ("|                    |   Splits:          | SwapCache                         |\n");
    csPrintf ("|                    |                    |                                   |\n");
    csPrintf ("|- CS Messages ---------------------------------------------------------------|\n");
    csPrintf ("|                                                                             |\n");
    csPrintf ("|                                                                             |\n");
    csPrintf ("|                                                                             |\n");
    csPrintf ("|                                                                             |\n");
    csPrintf ("\\-----------------------------------------------------------------------------/\n");
    csPrintf (CS_ANSI_CURSOR(1,1));
  }

  void TUI::DrawStats () const
  {
    // Scene stats
    csPrintf (CS_ANSI_CURSOR(67,8) "%8zu", globalStats.scene.numSectors);
    csPrintf (CS_ANSI_CURSOR(67,9) "%8zu", globalStats.scene.numObjects);
    csPrintf (CS_ANSI_CURSOR(67,10) "%8zu", globalStats.scene.numLights);

    // KD-Tree stats
    csPrintf (CS_ANSI_CURSOR(54,14) "%8zu / %8zu", globalStats.kdtree.numNodes, globalStats.kdtree.leafNodes);
    csPrintf (CS_ANSI_CURSOR(54,15) "%8zu / %8.03f", globalStats.kdtree.maxDepth, 
      (float)globalStats.kdtree.sumDepth / (float)globalStats.kdtree.leafNodes);
    csPrintf (CS_ANSI_CURSOR(54,16) "%8zu / %8.03f", globalStats.kdtree.numPrimitives, 
      (float)globalStats.kdtree.numPrimitives / (float)globalStats.kdtree.leafNodes);
    csPrintf (CS_ANSI_CURSOR(1,1));
  }
  
  csString TUI::FormatByteSize (uint64 size)
  {
    static const char* const units[] = {"KB", "MB", "GB"};
    const int numUnits = sizeof(units)/sizeof(const char*);
    const uint64 unitThreshold = CONST_UINT64(2048);
    
    if (size <= unitThreshold)
    {
      return csString().Format ("%" PRIu64 "B", size);
    }
    
    int unit = 0;
    while ((size > unitThreshold * CONST_UINT64(1024)) && (unit < numUnits-1))
    {
      size /= CONST_UINT64(1024);
      unit++;
    }
    return csString().Format ("%.1f%s",
      double (size) / 1024.0, units[unit]);
  }
  
  void TUI::DrawSwapCacheStats () const
  {
    csPrintf (CS_ANSI_CURSOR(45,19) 
      "                                 ");
    if (globalLighter->swapManager)
    {
      uint64 swappedIn, swappedOut, maxSize;
      globalLighter->swapManager->GetSizes (swappedIn, swappedOut, maxSize);
      csPrintf (CS_ANSI_CURSOR(47,19) "%s/%s in, %s out",
	                FormatByteSize (swappedIn).GetData(),
	                FormatByteSize (maxSize).GetData(),
	                FormatByteSize (swappedOut).GetData());
    }
    csPrintf (CS_ANSI_CURSOR(1,1));
  }

  void TUI::DrawSettings () const
  {
    csPrintf (CS_ANSI_CURSOR(46,8) "%s",
      globalConfig.GetLighterProperties ().directLightEngine == LIGHT_ENGINE_RAYTRACER
      ? "X" : "");

    csPrintf (CS_ANSI_CURSOR(46,9) "%s",
      (globalConfig.GetLighterProperties ().directLightEngine == LIGHT_ENGINE_PHOTONMAPPER ||
       globalConfig.GetLighterProperties ().indirectLightEngine == LIGHT_ENGINE_PHOTONMAPPER)
      ? "X" : "");

    csPrintf (CS_ANSI_CURSOR(46,10) "%s", true ? "X" : "");
    csPrintf (CS_ANSI_CURSOR(46,11) "%s", false ? "X" : "");
  
//    csPrintf (CS_ANSI_CURSOR(46,13) "%#4.2g", globalConfig.GetDIProperties ().pointLightMultiplier);
//    csPrintf (CS_ANSI_CURSOR(46,15) "%#4.2g", globalConfig.GetDIProperties ().areaLightMultiplier);

//    csPrintf (CS_ANSI_CURSOR(46,17) "%#4.2g", globalConfig.GetLMProperties ().lmDensity);

    csPrintf (CS_ANSI_CURSOR(1,1));
  }

  csString TUI::FormatAmount (uint64 value, int width)
  {
    // Suffix for ray units (none, thousands, millions, billions, trillions; quadrillion)
    const char* siConv[] = {" ", "K", "M", "G", "T", "P"};
    size_t suffix (0);

    while ((value > CONST_UINT64(99999))
      && (suffix < sizeof(siConv)/sizeof(siConv[0])))
    {
      value /= CONST_UINT64(1000);
      suffix++;
    }

    return csString().Format ("%*" CS_PRIu64 " %s", width, value, siConv[suffix]);
  }

  void TUI::DrawRayCore () const
  {
    // Make local copies of counters
    uint64 directRays = globalStats.raytracer.numShadowRays;
    uint64 lightRays = globalStats.raytracer.numLightRays;
    uint64 reflectRays = globalStats.raytracer.numReflectionRays;
    uint64 refractRays = globalStats.raytracer.numRefractionRays;
    uint64 finalGatherRays = globalStats.raytracer.numFinalGatherRays;
    uint64 totalRays = globalStats.raytracer.numRays;

    // Output ray counters with suffix
    csPrintf (CS_ANSI_CURSOR(12,9) "%s", FormatAmount (directRays).GetData());
    csPrintf (CS_ANSI_CURSOR(12,10) "%s", FormatAmount (lightRays).GetData());
    csPrintf (CS_ANSI_CURSOR(12,11) "%s", FormatAmount (reflectRays).GetData());
    csPrintf (CS_ANSI_CURSOR(12,12) "%s", FormatAmount (refractRays).GetData());
    csPrintf (CS_ANSI_CURSOR(12,13) "%s", FormatAmount (finalGatherRays).GetData());
    csPrintf (CS_ANSI_CURSOR(10,15) "%s", FormatAmount (totalRays).GetData());

    csPrintf (CS_ANSI_CURSOR(1,1));
  }

  void TUI::DrawPMCore () const
  {
    // Make local copies of counters
    uint64 photons = globalStats.photonmapping.numStoredPhotons;
    uint64 lookups = globalStats.photonmapping.numKDLookups;
    uint64 KDdepth = globalStats.photonmapping.KDTreeDepth;
    uint64 irPrim = globalStats.photonmapping.irCachePrimary;
    uint64 irSecnd = globalStats.photonmapping.irCacheSecondary;
    uint64 irLookups = globalStats.photonmapping.irCacheLookups;
    uint64 irSplits = globalStats.photonmapping.irCacheSplits;

    // Output photon counters with suffix
    csPrintf (CS_ANSI_CURSOR(33,9) "%s", FormatAmount (photons).GetData());
    csPrintf (CS_ANSI_CURSOR(33,10) "%s", FormatAmount (lookups).GetData());
    csPrintf (CS_ANSI_CURSOR(33,11) "%s", FormatAmount (KDdepth).GetData());

    csPrintf (CS_ANSI_CURSOR(32,15) "%s", FormatAmount (irPrim, 7).GetData());
    csPrintf (CS_ANSI_CURSOR(32,16) "%s", FormatAmount (irSecnd, 7).GetData());
    csPrintf (CS_ANSI_CURSOR(33,17) "%s", FormatAmount (irLookups).GetData());
    csPrintf (CS_ANSI_CURSOR(33,18) "%s", FormatAmount (irSplits).GetData());

    csPrintf (CS_ANSI_CURSOR(1,1));
  }

  void TUI::DrawProgress () const
  {
    const uint overallProg = globalStats.progress.GetOverallProgress();
    const uint taskProg = globalStats.progress.GetTaskProgress();

    csString taskName = globalStats.progress.GetTaskName();
    static const size_t maxTaskNameLen = 40;
    if (taskName.Length() > maxTaskNameLen)
      taskName = taskName.Slice (0, maxTaskNameLen-3) + "...";

    csPrintf (CS_ANSI_CURSOR(71, 2));
    csPrintf ("%3d%%", overallProg);
    
    csPrintf (CS_ANSI_CURSOR(3, 3));
    csPrintf ("%s", GetProgressBar (overallProg).GetDataSafe ());

    csPrintf (CS_ANSI_CURSOR(18, 4));
    csPrintf ("%s", taskName.GetDataSafe ());

    csPrintf (CS_ANSI_CURSOR(71, 4));
    csPrintf ("%3d%%", taskProg);

    csPrintf (CS_ANSI_CURSOR(3, 5));
    csPrintf ("%s", GetProgressBar (taskProg).GetDataSafe ());
    csPrintf (CS_ANSI_CURSOR(1,1));
  }

  void TUI::DrawMessage () const
  {
    csPrintf (CS_ANSI_CURSOR(3, 21));

    // Draw the four buffers, starting with messageBufferEnd
    int row = messageBufferEnd-1;
    if(row <  0) row += messageBufferSize;

    for(uint i = 0; i < 4; i++)
    {
      csPrintf ("%s", messageBuffer[row].GetDataSafe ());
      
      row--;
      if(row <  0) row += messageBufferSize;
      csPrintf (CS_ANSI_CURSOR_DOWN(1));
      csPrintf (CS_ANSI_CURSOR_BWD(100) CS_ANSI_CURSOR_FWD(2));
    }
    csPrintf (CS_ANSI_CURSOR(1,1));
  }

  csString TUI::GetProgressBar (uint percent) const
  {
    csString result;

    percent = csClamp(percent, 100u, 0u);

    result.SetCapacity (73);
    //Draw a progess bar being in total 73+2 characters wide
    //|========== =--------- ---------- ---------- ---------- ---------- ---------- ---|
    
    result.Append ('|');

    uint numDone = (73*percent) / 100;
    uint numToDo = (73*(100-percent)) / 100;

    if(numDone + numToDo < 73) numToDo++;

    for(uint i = 0; i < numDone; ++i)
    {
      result.Append ('=');
    }

    for(uint i = 0; i < numToDo; ++i)
    {
      result.Append ('-');
    }

    result.Append ('|');

    return result;
  }


  void TUI::DrawSimple ()
  {
    bool doFlush = false;
    const char* lt = (const char*)lastTask;
    const char* tn = globalStats.progress.GetTaskName ();
    if ((lt == 0 && tn != 0) || (lt != 0 && lastTask != tn))
    {
      lastTask = globalStats.progress.GetTaskName();

      prevWasReporter = false;
      csPrintf ("\n% 4d %% - %s ", 
        globalStats.progress.GetOverallProgress(),
        lastTask.GetDataSafe());
      doFlush = true;

      // Print new task and global progress
      lastTaskProgress = 0;
    }
    else
    {
      while (lastTaskProgress + 10 < globalStats.progress.GetTaskProgress())
      {
        prevWasReporter = false;
        csPrintf (".");
        lastTaskProgress += 10;
        doFlush = true;
      }
    }
    if (doFlush) fflush (stdout);
  }

  void TUI::DrawSimpleEnd ()
  {
    // Print KD-tree stats
    csPrintf ("\nKD-tree: \n");
    csPrintf ("N: %8zu / %8zu\n", globalStats.kdtree.numNodes, globalStats.kdtree.leafNodes);
    csPrintf ("D: %8zu / %8.03f\n", globalStats.kdtree.maxDepth, 
      (float)globalStats.kdtree.sumDepth / (float)globalStats.kdtree.leafNodes);
    csPrintf ("P: %8zu / %8.03f\n", globalStats.kdtree.numPrimitives, 
      (float)globalStats.kdtree.numPrimitives / (float)globalStats.kdtree.leafNodes);

    kdLastNumNodes = globalStats.kdtree.numNodes;
  }

   void TUI::DrawPhotonNumber(csString sectorName, int &photons,bool interactive)
   {
     if (interactive)
     {
       int redrawFlags = TUI::TUI_DRAW_ALL ^ TUI::TUI_DRAW_MESSAGES;
       TUI::Redraw(redrawFlags);
       csPrintf (CS_ANSI_CURSOR(3, 21));
       csPrintf("%d photons advised for %s sector",photons,sectorName.GetData());
       csPrintf (CS_ANSI_CURSOR(3, 22));
       int userPhotonsNbr;
       csPrintf("Number to emit (type -1 to use the advised number) : ");
       scanf("%d",&userPhotonsNbr);
       if (userPhotonsNbr >= 0)  photons = userPhotonsNbr;
     }
     globalLighter->Notify("%d photons for %s sector",photons,sectorName.GetData());
   }

}
