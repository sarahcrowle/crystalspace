#pragma once

#include "csFrame.h"

class PostEffectApp : public wxApp
{
public:
  virtual bool OnInit (void);
  virtual int OnExit (void);
};

DECLARE_APP(PostEffectApp) 
