/******************************************************************************\
Copyright (c) 2019 Intel Corporation

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
\**********************************************************************************/

#pragma once

#ifndef __INTELVIDEODECODER_H__
#define __INTELVIDEODECODER_H__

#include <stdio.h>
#include <memory>
#include <vector>
#include <algorithm>
#include <mutex>

#include "mfxvideo++.h"

#include "YuvRgbConvert.h"

// =================================================================
// OS-specific definitions of types, macro, etc...
// The following should be defined:
//  - mfxTime
//  - MSDK_FOPEN
//  - MSDK_SLEEP
#if defined(_WIN32) || defined(_WIN64)

// reduces the size of the Win32 header files by excluding some of the less frequently used APIs
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define MSDK_FOPEN(FH, FN, M)           { fopen_s(&FH, FN, M); }
#define MSDK_SLEEP(X)                   { Sleep(X); }
#define msdk_sscanf sscanf_s
#define msdk_strcopy strcpy_s

typedef LARGE_INTEGER mfxTime;
#elif defined(__linux__)
#pragma once

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define MSDK_FOPEN(FH, FN, M)           { FH=fopen(FN,M); }
#define MSDK_SLEEP(X)                   { usleep(1000*(X)); }
#define msdk_sscanf sscanf
#define msdk_strcopy strcpy

typedef timespec mfxTime;
#endif

// =================================================================
// Helper macro definitions...
#define MSDK_IGNORE_MFX_STS(P, X)       {if ((X) == (P)) {P = MFX_ERR_NONE;}}
#define MSDK_BREAK_ON_ERROR(P)          {if (MFX_ERR_NONE != (P)) break;}
#define MSDK_SAFE_DELETE_ARRAY(P)       {if (P) {delete[] P; P = NULL;}}
#define MSDK_ALIGN32(X)                 (((mfxU32)((X)+31)) & (~ (mfxU32)31))
#define MSDK_ALIGN16(value)             (((value + 15) >> 4) << 4)
#define MSDK_SAFE_RELEASE(X)            {if (X) { X->Release(); X = NULL; }}
#define MSDK_MAX(A, B)                  (((A) > (B)) ? (A) : (B))

#define MSDK_DEC_WAIT_INTERVAL 300000
#define MAX_BUFFER_BYTES 100*1024*1024

// =================================================================
class IntelQsvH264Decoder
{
public:
    IntelQsvH264Decoder();
    ~IntelQsvH264Decoder();

    int initialize();
    void uninitialize();
    int decodeHeader(uint8_t *frame, size_t length);
    int decode(uint8_t* in, size_t in_length);
    int decode_get(uint8_t *in, size_t in_length, uint8_t* out, int conv_opt);
    int getFrame(uint8_t* out, int conv_opt);
    int drainFrame(uint8_t *out, int conv_opt);
    int getWidth() { return this->width; }
    int getHeight() { return this->height; }
    bool isInit() { return isInitialized; }

private:
    // Create Media SDK decoder
    MFXVideoDECODE *mfxDEC;
    // Set required video parameters for decode
    mfxVideoParam mfxVideoParams;
    // Prepare Media SDK bit stream buffer
    // - Arbitrary buffer size for this example
    mfxBitstream mfxBS;
    MFXVideoSession session;
    // Allocate surface headers (mfxFrameSurface1) for decoder
    std::vector<mfxFrameSurface1> pmfxSurfaces;
    std::vector<mfxFrameSurface1*> pmfxOutputSurfaces;
    std::vector<mfxSyncPoint> syncPoints;

    int nIndex = 0;
    int width = -1;
    int height = -1;
    bool isInitialized = false;
    uint8_t* raw_for_decoder;

    int GetFreeSurfaceIndex(const std::vector<mfxFrameSurface1> &pSurfacesPool);
    mfxStatus StoreBitstreamData(uint8_t *data, size_t len);
    void getResolution(mfxFrameInfo* pInfo, int& w, int& h);
    int GetRawFrame(mfxFrameSurface1* pSurface, uint8_t* rgb, int conv_opt);
};

#endif /* __INTELVIDEODECODER_H__ */
