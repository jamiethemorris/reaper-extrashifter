/***************************************
*** REAPER Plug-in API
**
** Copyright (C) 2006-2015, Cockos Incorporated
**
**    This software is provided 'as-is', without any express or implied
**    warranty.  In no event will the authors be held liable for any damages
**    arising from the use of this software.
**
**    Permission is granted to anyone to use this software for any purpose,
**    including commercial applications, and to alter it and redistribute it
**    freely, subject to the following restrictions:
**
**    1. The origin of this software must not be misrepresented; you must not
**       claim that you wrote the original software. If you use this software
**       in a product, an acknowledgment in the product documentation would be
**       appreciated but is not required.
**    2. Altered source versions must be plainly marked as such, and must not be
**       misrepresented as being the original software.
**    3. This notice may not be removed or altered from any source distribution.
**
** Notes: the C++ interfaces used require MSVC on win32, or at least the MSVC-compatible C++ ABI. Sorry, mingw users :(
**
*/

// ALTERED: focused non-Windows ABI excerpt; class and constants unmodified.
#pragma once
typedef double ReaSample;
#define REAPER_PITCHSHIFT_API_VER 0x14

class IReaperPitchShift
{
  public:
    virtual ~IReaperPitchShift() { };
    virtual void set_srate(double srate)=0;
    virtual void set_nch(int nch)=0;
    virtual void set_shift(double shift)=0;
    virtual void set_formant_shift(double shift)=0; // shift can be <0 for "only shift when in formant preserve mode", so that you can use it for effective rate changes etc in that mode
    virtual void set_tempo(double tempo)=0;

    virtual void Reset()=0;  // reset all buffers/latency
    virtual ReaSample *GetBuffer(int size)=0;
    virtual void BufferDone(int input_filled)=0;

    virtual void FlushSamples()=0; // make sure all output is available

    virtual bool IsReset()=0;

    virtual int GetSamples(int requested_output, ReaSample *buffer)=0; // returns number of samplepairs returned

    virtual void SetQualityParameter(int parm)=0; // set to: (mode<<16)+(submode), or -1 for "project default" (default)
    virtual int Extended(int call, void *parm1, void *parm2, void *parm3) { return 0; } // return 0 if unsupported
};
#define REAPER_PITCHSHIFT_EXT_GETMINMAXPRODUCTS 0x1
#define REAPER_PITCHSHIFT_EXT_GET_CHAN_LIMIT 0x100 // return channel limit if any
#define REAPER_PITCHSHIFT_EXT_SET_OUTPUT_BPM 0x200 // parm1 = *(double *)bpm
#define REAPER_PITCHSHIFT_EXT_WANT_OUTPUT_BPM 0x201 // returns 1 if desired


