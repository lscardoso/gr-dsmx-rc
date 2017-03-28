/* -*- c++ -*- */

#define DSMX_API

%include "gnuradio.i"			// the common stuff

//load generated python docstrings
%include "dsmx_swig_doc.i"

%{
#include "dsmx/preambleDetection.h"
#include "dsmx/Despreader.h"
#include "dsmx/bindListener.h"
%}


%include "dsmx/preambleDetection.h"
GR_SWIG_BLOCK_MAGIC2(dsmx, preambleDetection);
%include "dsmx/Despreader.h"
GR_SWIG_BLOCK_MAGIC2(dsmx, Despreader);
%include "dsmx/bindListener.h"
GR_SWIG_BLOCK_MAGIC2(dsmx, bindListener);
