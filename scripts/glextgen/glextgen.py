#!/usr/bin/python
# Utility to create Crystal Space's glextmanager.h compatibility header.
# The header is constructed from the database metaglext.xml, and the templates
# in the glext subdirectory.

from xml.dom import minidom
from string import *

xmldoc = minidom.parse ("metaglext.xml")

templates = {}

def getTemplate (template):
  if template in templates:
    return templates[template];
  else:
    templateFile = file ("glext/" + template, "r");
    lines = templateFile.readlines();
    templates[template] = lines;
    return lines;   

def concat(list):
  result = "";
  for word in list:
    result += word;
  return result;

def getTemplateK (template, kind):
  return getTemplate (template + "_" + kind);

def extensionUrl (ext):
  urlPrefix = "http://www.opengl.org/registry/specs/";
  if ext.startswith("GL_") or ext.startswith("GLX_"):
    ext = ext[3:];
    extsplit = ext.split ("_", 1);
    return urlPrefix + extsplit[0] + "/" + extsplit[1] + ".txt";
  elif ext.startswith("WGL_"):
    extsplit = ext.split ("_", 2);
    return urlPrefix + extsplit[1] + "/" + extsplit[0].lower() + "_" + extsplit[2] + ".txt";

def interpolate (strings, values):
  resStrings = list();
  for string in strings:
    words = string.split ("%");
    result = "";
    i = 0;
    if len (words) >= 2:
      while i < (len (words) - 1):
	result = result + words[i];
	key = words[i + 1];
	if key == "":
	  result = result + "%"
	else:
	  ksplit = key.split (":");
	  value = values [ksplit[0].lower()];
	  if key.islower():
	    value = value.lower();
	  if key.isupper():
	    value = value.upper();
	  if len (ksplit) > 1:
	    if ksplit[1] == "url":
	      value = extensionUrl (value);
	    else:
	      width = int (ksplit[1]);
	      if width > 0:
		value = value.ljust (width);
	      if width < 0:
		value = value.rjust (-width);
	  result = result + value;  
	i = i + 2;
    while i < len (words):
      result = result + words[i];
      i = i + 1;
    resStrings.append (result);
  return resStrings;

def getConstant (const):
    values = { "name" : const.getAttribute ("name"), 
      "value" : const.getAttribute ("value") };
    return interpolate (getTemplate ("constant"), values);

def getFunctionType (func):
    values = { "name" : func.getAttribute ("name"), 
      "return" : func.getAttribute ("return") };
    if (func.getAttribute ("pointer") == "true"):
      values["pointer"] = "*";
    else:
      values["pointer"] = "";
    prevarg = 0;
    arguments = "";
    for arg in func.getElementsByTagName ("arg"):
      arguments = arguments + join (getArgument (arg, prevarg), "");
      prevarg = arg;
    values["arglist"] = arguments;
    return interpolate (getTemplate ("func_type"), values);

def getArgument (arg, prevarg):
    values = { "name" : arg.getAttribute ("name") };
    if prevarg != 0:
      values["comma"] = ", ";
    else:
      values["comma"] = "";
    if (arg.getAttribute ("const") == "true"):
      values["const"] = "const ";
    else:
      values["const"] = "";
    values["type"] = arg.getAttribute("type");
    if (arg.getAttribute ("pointer") == "true"):
      values["pointer"] = "*";
    else:
      values["pointer"] = "";
    return interpolate (getTemplate ("func_arg"), values);


class Extensions(object):
    def __init__(self):
        self.writtenFuncs = list()
        self.writtenFuncTypes = list()
        self.tflags = list()
        self.dflags = list()
        self.initflags = list()
        self.defs = list()
        self.funcs = list()
        self.initext = list()

    def getDefinitions (self, ext, template_key):
        name = ext.getAttribute ("name");
        #type = ((name.split ("_"))[0]).lower();
        values = { "name" : name, "tokens" : "", "functiontypes" : "" };
        for const in ext.getElementsByTagName ("token"):
          values["tokens"] = values["tokens"] + join (getConstant (const), "");
        for func in ext.getElementsByTagName ("function"):
          if func.getAttribute("name") not in self.writtenFuncTypes:
            values["functiontypes"] = values["functiontypes"] + \
              join (getFunctionType (func), "");
            self.writtenFuncTypes.append (func.getAttribute("name"))
        return interpolate (getTemplateK ("defs", template_key), values);

    def getFunctions (self, ext, template_key):
      funcs = list();
      name = ext.getAttribute ("name");
      #type = ((name.split ("_"))[0]).lower();
      for func in ext.getElementsByTagName ("function"):
        values = { "name" : func.getAttribute("name") };
        if func.getAttribute("name") not in self.writtenFuncs:
          funcs = funcs + interpolate (getTemplate ("func"), values);
          self.writtenFuncs.append (func.getAttribute("name"))
      values = {"name" : name, "functions" : join (funcs, "") };
      return interpolate (getTemplateK ("funcs", template_key), values);

def getExtensions (extensions):
    by_type = dict();
    for ext in extensions:
        name = ext.getAttribute ("name");
        #type = ((name.split ("_"))[0]).lower();
        type = ext.getAttribute ("type")
        print name + "...";
        template_key = type
        if type.startswith("ver"):
            type = type[3:]
            template_key = type + "ver"
        elif type.startswith("pseudo"):
            type = type[6:]
            template_key = "pseudo"
        if type in by_type:
            ext_data = by_type[type]
        else:
            ext_data = Extensions()
            by_type[type] = ext_data
        values = { "name" : name }
        print " flags..."
        ext_data.tflags += interpolate (getTemplate ("ext_tested"), values)
        ext_data.dflags += interpolate (getTemplateK ("ext_flag", template_key), values)
        ext_data.initflags += interpolate (getTemplate ("ext_init"), values)
        print " tokens..."
        ext_data.defs += ext_data.getDefinitions (ext, template_key)
        print " funcs..."
        ext_data.funcs += ext_data.getFunctions (ext, template_key)
        ext_data.initext += getInitExtensions (ext, template_key)
    interpolated = dict()
    for (type, ext_dat) in by_type.iteritems():
        values = {
          "definitions" : concat (ext_dat.defs),
          "initflags" : concat (ext_dat.initflags),
          "functions" : concat (ext_dat.funcs),
          "extflagsdetected" : concat (ext_dat.dflags),
          "extflagstested" : concat (ext_dat.tflags),
          "initextensions" : concat (ext_dat.initext),
          "tech": type,
          "footer": join (getTemplateK ("footer", type), "")
        };
        print "assembling " + type + "...";
        interpolated[type] = interpolate (getTemplate ("headerfiletemplate"), values);
    return interpolated

def getInitExtensions (ext, template_key):
    cfgprefix = "Video.OpenGL.UseExtension.";
    name = ext.getAttribute ("name")
    values = { 
      "name" : name, 
      "namelen" : str (len (name)),
      "cfgprefix" : cfgprefix, 
      "cfglen" : str (len (cfgprefix)),
      "functionsinit" : "",
      "depcheck" : "" };
    #type = ((name.split ("_"))[0]).lower();
    specials = ( 
      "WGL_ARB_extensions_string"
      );
    if name in specials:
      ettype = "special";
    values["extcheck"] = join (interpolate (getTemplateK ("extcheck", 
      template_key), values), "");
    for func in ext.getElementsByTagName ("function"):
      values["functionsinit"] += join (getFunctionInit (func), "");
    for dep in ext.getElementsByTagName ("depends"):
      values["depcheck"] += join (getDependency (dep), "");
    return interpolate (getTemplateK ("initext", 
      template_key), values);

def getFunctionInit (func):
    name = func.getAttribute ("name")
    values = { 
      "name" : name};
    return interpolate (getTemplate ("funcinit"), values);

def getDependency (dep):
    ext = dep.getAttribute ("ext")
    values = { 
      "ext" : ext};
    return interpolate (getTemplate ("depends"), values);

by_type = getExtensions (xmldoc.getElementsByTagName ("extension"))
for (type, ext_header) in by_type.iteritems():
    stuff = join (ext_header, "")
    output = file (type.lower() + "extmanager.h", "w")
    output.write ("/**\n")
    output.write (" * WARNING - This file is automagically generated from scripts/glextgen/glextgen.py\n")
    output.write (" */\n\n")
    output.write (stuff)
    output.close ()
