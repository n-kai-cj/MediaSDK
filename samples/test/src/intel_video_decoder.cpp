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

#include "intel_video_decoder.h"
#include "YuvRgbConvert.h"

IntelQsvH264Decoder::IntelQsvH264Decoder()
{
}

IntelQsvH264Decoder::~IntelQsvH264Decoder()
{
}

int IntelQsvH264Decoder::initialize()
{
    mfxStatus sts = MFX_ERR_NONE;

    // Initialize Intel Media SDK session
    // - MFX_IMPL_AUTO_ANY selects HW acceleration if available (on any adapter)
    // - Version 1.0 is selected for greatest backwards compatibility.
    // OS specific notes
    // - On Windows both SW and HW libraries may present
    // - On Linux only HW library only is available
    //   If more recent API features are needed, change the version accordingly
    mfxIMPL impl = MFX_IMPL_HARDWARE;
    mfxVersion ver = { {0, 1} };

    sts = session.Init(impl, &ver);

    if (MFX_ERR_NONE != sts)
    {
        return sts;
    }

    // Create Media SDK decoder
    mfxDEC = new MFXVideoDECODE(session);

    // Set required video parameters for decode
    memset(&mfxVideoParams, 0, sizeof(mfxVideoParams));
    mfxVideoParams.mfx.CodecId = MFX_CODEC_AVC;
    mfxVideoParams.IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;
    mfxVideoParams.AsyncDepth = 0;

    // Prepare Media SDK bit stream buffer
    // - Arbitrary buffer size for this example
    memset(&mfxBS, 0, sizeof(mfxBS));
    mfxBS.MaxLength = MAX_BUFFER_BYTES;
    mfxBS.Data = (mfxU8*)malloc(sizeof(mfxU8) * mfxBS.MaxLength);
    mfxBS.DataOffset = 0;
    mfxBS.DataLength = 0;

    return sts;
}

void IntelQsvH264Decoder::uninitialize()
{
    // ===================================================================
    // Clean up resources
    //  - It is recommended to close Media SDK components first, before releasing allocated surfaces, since
    //    some surfaces may still be locked by internal Media SDK resources.

    mfxDEC->Close();
    // session closed automatically on destruction

#if defined(DX9_D3D) || defined(DX11_D3D)
    CleanupHWDevice();
#endif

    if (mfxBS.Data)
    {
        free(mfxBS.Data);
    }

    pmfxSurfaces.clear();
    pmfxOutputSurfaces.clear();
    syncPoints.clear();
    nIndex = 0;
    width = -1;
    height = -1;
    isInitialized = false;
    if (raw_for_decoder)
    {
        free(raw_for_decoder);
    }
}

int IntelQsvH264Decoder::decodeHeader(uint8_t *frame, size_t length)
{

    mfxStatus sts = MFX_ERR_NONE;

    // header process already done
    if (isInitialized)
    {
        return sts;
    }

    // store video frame data into mfxBS
    sts = StoreBitstreamData(frame, length);
    if (MFX_ERR_NONE != sts)
    {
        fprintf(stderr, "error: decoder header data store failed. len=%d, sts=%d\n", (int)length, sts);
        return sts;
    }

    sts = mfxDEC->DecodeHeader(&mfxBS, &mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    if (MFX_ERR_NONE != sts)
    {
        fprintf(stderr, "error: decode header failed. %d\n", sts);
        return sts;
    }

    // Validate video decode parameters (optional)
    sts = mfxDEC->Query(&mfxVideoParams, &mfxVideoParams);
    if (MFX_ERR_NONE != sts)
    {
        return sts;
    }

    // Query number of required surfaces for decoder
    mfxFrameAllocRequest Request;
    memset(&Request, 0, sizeof(Request));
    sts = mfxDEC->QueryIOSurf(&mfxVideoParams, &Request);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    if (MFX_ERR_NONE != sts)
    {
        return sts;
    }

    mfxU16 numSurfaces = Request.NumFrameSuggested;

    // Allocate surfaces for decoder
    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    mfxU16 width = (mfxU16)MSDK_ALIGN32(Request.Info.Width);
    mfxU16 height = (mfxU16)MSDK_ALIGN32(Request.Info.Height);
    mfxU8 bitsPerPixel = 12; // NV12 format is a 12 bits per pixel format
    mfxU32 surfaceSize = width * height * bitsPerPixel / 8;
    mfxU8* surfaceBuffers = new mfxU8[(unsigned long long)surfaceSize * (unsigned long long)numSurfaces];

    pmfxSurfaces.clear();
    pmfxOutputSurfaces.clear();
    syncPoints.clear();
    for (int i = 0; i < numSurfaces; i++)
    {
        mfxFrameSurface1 mfxSurf;
        memset(&mfxSurf, 0, sizeof(mfxFrameSurface1));
        mfxSurf.Info = mfxVideoParams.mfx.FrameInfo;
        mfxSurf.Data.Y = &surfaceBuffers[surfaceSize * i];
        mfxSurf.Data.U = mfxSurf.Data.Y + (unsigned long long)width * (unsigned long long)height;
        mfxSurf.Data.V = mfxSurf.Data.U + 1;
        mfxSurf.Data.Pitch = width;
        pmfxSurfaces.push_back(mfxSurf);
    }
    raw_for_decoder = (uint8_t*)malloc(sizeof(uint8_t)*(unsigned long long)width * (unsigned long long)height * 3);

    sts = mfxDEC->Init(&mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    if (MFX_ERR_NONE != sts)
    {
        fprintf(stderr, "error: decoder Init() error sts = %d\n", sts);
        return sts;
    }

    isInitialized = true;

    return sts;
}

int IntelQsvH264Decoder::decode(uint8_t* in, size_t in_length)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (decodeHeader(in, in_length) < 0)
    {
        fprintf(stderr, "error: decode header error\n");
        return -1;
    }

    // avoid MFX_ERR_INCOMPATIBLE_VIDEO_PARAM
    if (in_length > 5 && in_length < 50 &&
        ((in[0] == 0 && in[1] == 0 && in[2] == 0 && in[3] == 1 && ((in[4] & 0x0f) == 7 || (in[4] & 0x0f) == 8)) ||
        (in[0] == 0 && in[1] == 0 && in[2] == 1 && ((in[3] & 0x0f) == 7 || (in[3] & 0x0f) == 8))))
    {
        // may be SPS/PPS only packet
        // MFX Decode is imcompatible for only them
        return -1;
    }

    // store video frame data into mfxBS
    sts = StoreBitstreamData(in, in_length);
    if (MFX_ERR_NONE != sts)
    {
        fprintf(stderr, "error: decoder data store failed. length=%d, sts=%d\n", (int)in_length, sts);
        return sts;
    }

    if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts)
    {
        nIndex = GetFreeSurfaceIndex(pmfxSurfaces); // Find free frame surface
        if (MFX_ERR_NOT_FOUND == nIndex)
        {
            fprintf(stderr, "error: decoder surface not found\n");
            fprintf(stderr, "       list size=%d %d\n", (int)pmfxSurfaces.size(), (int)pmfxOutputSurfaces.size());
            mfxDEC->Reset(&mfxVideoParams);
            return nIndex;
        }
    }

    mfxSyncPoint syncp;
    mfxFrameSurface1* pmfxOutSurface = NULL;

    // Decode a frame asychronously (returns immediately)
    //  - If input bitstream contains multiple frames DecodeFrameAsync will start decoding multiple frames, and remove them from bitstream
    sts = mfxDEC->DecodeFrameAsync(&mfxBS, &pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);

    if (MFX_WRN_VIDEO_PARAM_CHANGED == sts)
    {
        // MFX_WRN_VIDEO_PARAM_CHANGED if the SDK decoder parsed a new sequence header in the bitstream
        // and decoding can continue with existing frame buffers.
        // The application can optionally retrieve new video parameters by calling MFXVideoDECODE_GetVideoParam.
        sts = mfxDEC->DecodeFrameAsync(&mfxBS, &pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);
    }

    // Ignore warnings if output is available,
    // if no output and no action required just repeat the DecodeFrameAsync call
    if (MFX_ERR_NONE < sts && syncp)
    {
        sts = MFX_ERR_NONE;
    }

    if (MFX_ERR_NONE == sts)
    {
        // Aligned Width and Height is not actual Resolution
        getResolution(&pmfxOutSurface->Info, width, height);
        syncPoints.push_back(syncp);
        pmfxOutputSurfaces.push_back(pmfxOutSurface);
    }

    if (MFX_ERR_NONE == sts && 
        (syncPoints.size() == 0 || pmfxOutputSurfaces.size() == 0 || syncPoints.size() != pmfxOutputSurfaces.size()))
    {
        return MFX_ERR_MORE_DATA;
    }

    return sts;
}

int IntelQsvH264Decoder::decode_get(uint8_t *in, size_t in_length, uint8_t* out, int conv_opt)
{
    mfxStatus sts = MFX_ERR_NONE;

    sts = (mfxStatus)decode(in, in_length);
    if (MFX_ERR_NONE == sts)
    {
        sts = (mfxStatus)getFrame(out, conv_opt);
    }

    return sts;
}

int IntelQsvH264Decoder::getFrame(uint8_t* out, int conv_opt)
{
    mfxStatus sts = MFX_ERR_NONE;

    if (syncPoints.size() == 0 || pmfxOutputSurfaces.size() == 0 || syncPoints.size() != pmfxOutputSurfaces.size())
    {
        return MFX_ERR_MORE_DATA;
    }

    mfxSyncPoint syncp = syncPoints[0];
    mfxFrameSurface1* pmfxOutSurface = pmfxOutputSurfaces[0];
    syncPoints.erase(syncPoints.begin());
    pmfxOutputSurfaces.erase(pmfxOutputSurfaces.begin());

    sts = session.SyncOperation(syncp, MSDK_DEC_WAIT_INTERVAL); // Synchronize. Wait until decoded frame is ready

    if (MFX_WRN_DEVICE_BUSY == sts)
    {
        fprintf(stderr, "error: device busy %d\n", sts);
        return MFX_ERR_ABORTED;
    }

    if (MFX_ERR_NONE == sts)
    {
        // decode complete
        int size = GetRawFrame(pmfxOutSurface, out, conv_opt);
        return size;
    }

    return sts;
}

int IntelQsvH264Decoder::drainFrame(uint8_t *out, int conv_opt)
{
    mfxStatus sts = MFX_ERR_NONE;
    mfxSyncPoint syncp;
    mfxFrameSurface1 *outSurf = NULL;

    int nIndex = GetFreeSurfaceIndex(pmfxSurfaces); // Find free frame surface
    if (MFX_ERR_NOT_FOUND == nIndex)
    {
        fprintf(stderr, "error: decoder surface not found\n");
        return nIndex;
    }

    // Decode a frame asychronously (returns immediately)
    //  - If input bitstream contains multiple frames DecodeFrameAsync will start decoding multiple frames, and remove them from bitstream
    sts = mfxDEC->DecodeFrameAsync(NULL, &pmfxSurfaces[nIndex], &outSurf, &syncp);

    // Ignore warnings if output is available,
    // if no output and no action required just repeat the DecodeFrameAsync call
    if (MFX_ERR_NONE < sts && syncp)
    {
        sts = MFX_ERR_NONE;
    }
    if (MFX_ERR_NONE == sts)
    {
        sts = session.SyncOperation(syncp, MSDK_DEC_WAIT_INTERVAL); // Synchronize. Wait until decoded frame is ready
    }

    if (MFX_WRN_DEVICE_BUSY == sts)
    {
        fprintf(stderr, "error: device busy %d\n", sts);
        return MFX_ERR_ABORTED;
    }

    if (MFX_ERR_NONE == sts)
    {
        // decode complete
        int size = GetRawFrame(outSurf, out, conv_opt);
        return size;
    }

    return sts;
}

int IntelQsvH264Decoder::GetFreeSurfaceIndex(const std::vector<mfxFrameSurface1> &pSurfacesPool)
{
    auto it = std::find_if(pSurfacesPool.begin(), pSurfacesPool.end(), [](const mfxFrameSurface1 &surface) {
        return 0 == surface.Data.Locked;
    });

    if (it == pSurfacesPool.end())
    {
        return MFX_ERR_NOT_FOUND;
    }
    else
    {
        return it - pSurfacesPool.begin();
    }
}

mfxStatus IntelQsvH264Decoder::StoreBitstreamData(uint8_t *data, size_t len)
{
    memmove(mfxBS.Data, mfxBS.Data + mfxBS.DataOffset, mfxBS.DataLength);
    mfxBS.DataOffset = 0;

    std::memcpy(mfxBS.Data + mfxBS.DataLength, (mfxU8 *)data, len);

    mfxBS.DataLength += len;

    return MFX_ERR_NONE;
}

void IntelQsvH264Decoder::getResolution(mfxFrameInfo* pInfo, int& w, int& h)
{
    if (pInfo->CropH > 0 && pInfo->CropW > 0)
    {
        w = pInfo->CropW;
        h = pInfo->CropH;
    }
    else
    {
        w = pInfo->Width;
        h = pInfo->Height;
    }
}

int IntelQsvH264Decoder::GetRawFrame(mfxFrameSurface1 *pSurface, uint8_t *rgb, int conv_opt)
{
    mfxFrameInfo *pInfo = &pSurface->Info;
    mfxFrameData *pData = &pSurface->Data;

    if (pInfo->FourCC == MFX_FOURCC_RGB4 || pInfo->FourCC == MFX_FOURCC_A2RGB10)
    {
        // unsupport rgb format for now...
        fprintf(stderr, "error: video format seems RGB that is invalid.\n");
        return -1;
    }

    // NV12 format maybe
    int w = pInfo->Width;
    int h = pInfo->Height;

    uint8_t* nv12;
    const int nv12Size = w * h + w * h / 2;

    if (rgb == nullptr)
    {
        fprintf(stderr, "output buffer is invalid\n");
        return w * h * 3;
    }

    // convert nv12 to rgb in convert options
    switch (conv_opt)
    {
    case 0:
        YuvRgbConvert::nv12_rgb24_std(w, h, pData->Y, pData->UV, w, w, raw_for_decoder, w * 3, YCBCR_JPEG);
        break;
    case 1:
        YuvRgbConvert::nv12_rgb24_sse(w, h, pData->Y, pData->UV, w, w, raw_for_decoder, w * 3, YCBCR_JPEG);
        break;
#ifdef HAVE_IPP
    case 2:
        nv12 = (uint8_t*)malloc(sizeof(uint8_t) * nv12Size);
        std::memcpy(nv12, pData->Y, (unsigned long long)w * (unsigned long long)h);
        std::memcpy(nv12 + (unsigned long long)w * (unsigned long long)h, pData->UV, w * h / 2);
        YuvRgbConvert::nv12_rgb24_ipp(nv12, w, h, raw_for_decoder);
        free(nv12);
        break;
#endif
    default:
        YuvRgbConvert::nv12_rgb24_sse(w, h, pData->Y, pData->UV, w, w, raw_for_decoder, w * 3, YCBCR_JPEG);
        break;
    }

    int dstSize = width * height * 3;
    std::memcpy(rgb, raw_for_decoder, dstSize);

    return dstSize;
}

