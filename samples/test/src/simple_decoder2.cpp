#include <stdio.h>
#include <memory>
#include <vector>
#include <algorithm>

#include "mfxvideo++.h"
#include "mfxjpeg.h"
#include "mfxvp8.h"
#include "mfxplugin.h"

#include <opencv2/opencv.hpp>


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



// =================================================================
class Decoder {
public:
    Decoder();
    ~Decoder();
    int initialize();
    int decodeHeader(uint8_t* frame, size_t length);
//    int decode(uint8_t* in, size_t in_length);
    int decodeTest1();


//private:
    FILE* in = fopen("out2.264", "rb");
    FILE* out = fopen("out.yuv", "wb");

    uint8_t* esData = new uint8_t[10 * 1024 * 1024];

    mfxBitstream* mfxBS;
    MFXVideoSession* session;
    MFXVideoDECODE* mfxDEC;
    mfxVideoParam* mfxVideoParams;
    std::vector<mfxFrameSurface1*> pmfxSurfaces;

    mfxStatus StoreBitstreamData(uint8_t* data, size_t len);
    int GetFreeSurfaceIndex(const std::vector<mfxFrameSurface1*>& pSurfacesPool);
    mfxStatus WriteSection(mfxU8* plane, mfxU16 factor, mfxU16 chunksize,
        mfxFrameInfo* pInfo, mfxFrameData* pData, mfxU32 i,
        mfxU32 j, FILE* fSink);
    mfxStatus WriteRawFrame(mfxFrameSurface1* pSurface, FILE* fSink);

};

Decoder::Decoder()
{
}

Decoder::~Decoder()
{
}

int Decoder::initialize()
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

    session = new MFXVideoSession();
    sts = session->Init(impl, &ver);
    //MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    if (MFX_ERR_NONE != sts)
    {
        printf("error: session.init() %d\n", sts);
        return sts;
    }

    // Set required video parameters for decode
    mfxVideoParams = new mfxVideoParam();
//    memset(mfxVideoParams, 0, sizeof(mfxVideoParams));
    mfxVideoParams->mfx.CodecId = MFX_CODEC_AVC;
    mfxVideoParams->IOPattern = MFX_IOPATTERN_OUT_SYSTEM_MEMORY;

    // Prepare Media SDK bit stream buffer
    // - Arbitrary buffer size for this example
//    memset(&mfxBS, 0, sizeof(mfxBS));
    mfxBS = new mfxBitstream();
    mfxBS->MaxLength = 1024 * 1024;
    mfxBS->Data = new uint8_t[mfxBS->MaxLength];

    return sts;
}

int Decoder::decodeHeader(uint8_t* frame, size_t length)
{
    mfxStatus sts = MFX_ERR_NONE;

    // Read a chunk of data from stream file into bit stream buffer
    // - Parse bit stream, searching for header and fill video parameters structure
    // - Abort if bit stream header is not found in the first bit stream buffer chunk
    sts = StoreBitstreamData(frame, length);
    //MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // Create Media SDK decoder
    mfxDEC = new MFXVideoDECODE(session[0]);

    sts = mfxDEC->DecodeHeader(mfxBS, mfxVideoParams);
    printf("DecodeHeader finish sts=%d\n", sts);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    //MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    if (MFX_ERR_NONE != sts)
    {
        printf("error: decode header %d\n", sts);
        return sts;
    }

    // Validate video decode parameters (optional)
    // sts = mfxDEC.Query(&mfxVideoParams, &mfxVideoParams);

    // Query number of required surfaces for decoder
    mfxFrameAllocRequest Request;
    memset(&Request, 0, sizeof(Request));
    sts = mfxDEC->QueryIOSurf(mfxVideoParams, &Request);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    //MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    if (MFX_ERR_NONE != sts)
    {
        printf("error query iosurf %d\n", sts);
        return sts;
    }

    mfxU16 numSurfaces = Request.NumFrameSuggested;

    // Allocate surfaces for decoder
    // - Width and height of buffer must be aligned, a multiple of 32
    // - Frame surface array keeps pointers all surface planes and general frame info
    mfxU16 width = (mfxU16)MSDK_ALIGN32(Request.Info.Width);
    mfxU16 height = (mfxU16)MSDK_ALIGN32(Request.Info.Height);
    mfxU8 bitsPerPixel = 12;        // NV12 format is a 12 bits per pixel format
    mfxU32 surfaceSize = width * height * bitsPerPixel / 8;
//    std::vector<mfxU8> surfaceBuffersData(surfaceSize * numSurfaces);
//    mfxU8* surfaceBuffers = surfaceBuffersData.data();
    mfxU8* surfaceBuffers = new mfxU8[surfaceSize * numSurfaces];

    // Allocate surface headers (mfxFrameSurface1) for decoder
//    std::vector<mfxFrameSurface1> pmfxSurfaces(numSurfaces);
    for (int i = 0; i < numSurfaces; i++)
    {
        mfxFrameSurface1 *mfxSurf = new mfxFrameSurface1();
//        memset(&mfxSurf, 0, sizeof(mfxFrameSurface1));
        mfxSurf->Info = mfxVideoParams->mfx.FrameInfo;
        mfxSurf->Data.Y = &surfaceBuffers[surfaceSize * i];
        mfxSurf->Data.U = mfxSurf->Data.Y + (unsigned long long)width * (unsigned long long)height;
        mfxSurf->Data.V = mfxSurf->Data.U + 1;
        mfxSurf->Data.Pitch = width;
        pmfxSurfaces.push_back(mfxSurf);
    }

    // Initialize the Media SDK decoder
    sts = mfxDEC->Init(mfxVideoParams);
    MSDK_IGNORE_MFX_STS(sts, MFX_WRN_PARTIAL_ACCELERATION);
    //MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);
    if (MFX_ERR_NONE != sts)
    {
        printf("error dec->init sts=%d\n", sts);
        return sts;
    }

    printf("aligned wxs = %dx%d, (%dx%d)\n", width, height, Request.Info.Width, Request.Info.Height);


    // ==================================================
//    decodeTest1();
    // ==================================================

    return sts;
}

int Decoder::decodeTest1()
{
    mfxStatus sts = MFX_ERR_NONE;

    mfxSyncPoint syncp;
//    mfxFrameSurface1* pmfxOutSurface = NULL;
    mfxFrameSurface1* pmfxOutSurface = new mfxFrameSurface1();
    int nIndex = 0;
    mfxU32 nFrame = 0;

    //
    // Stage 1: Main decoding loop
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_DATA == sts || MFX_ERR_MORE_SURFACE == sts) {
//        if (MFX_WRN_DEVICE_BUSY == sts)
//            MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call to DecodeFrameAsync

        if (MFX_ERR_MORE_DATA == sts) {
            int size = fread(esData, 1, 1024, in);
            if (size <= 0)
            {
                break;
            }
            sts = StoreBitstreamData(esData, size);
//            sts = ReadBitStreamData(&mfxBS, fSource.get());       // Read more data into input bit stream
            MSDK_BREAK_ON_ERROR(sts);
        }

        if (MFX_ERR_MORE_SURFACE == sts || MFX_ERR_NONE == sts) {
            nIndex = GetFreeSurfaceIndex(pmfxSurfaces);        // Find free frame surface
//            MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);
            if (MFX_ERR_NOT_FOUND == nIndex)
            {
                printf("error getfreesurfaceindex sts=%d, nIndex=%d\n", sts, nIndex);
            }
        }
        // Decode a frame asychronously (returns immediately)
        //  - If input bitstream contains multiple frames DecodeFrameAsync will start decoding multiple frames, and remove them from bitstream
        sts = mfxDEC->DecodeFrameAsync(mfxBS, pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncp)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts)
        {
            sts = session->SyncOperation(syncp, 60000);      // Synchronize. Wait until decoded frame is ready
            printf("1loop syncoperation sts=%d, syncp=%d\n", sts, syncp);
        }

        if (MFX_ERR_NONE == sts) {
            ++nFrame;
            WriteRawFrame(pmfxOutSurface, out);
            // if (bEnableOutput) {
            //     sts = WriteRawFrame(pmfxOutSurface, fSink.get());
            //     MSDK_BREAK_ON_ERROR(sts);

            //     printf("Frame number: %d\r", nFrame);
            //     fflush(stdout);
            // }
        }
        printf("1loop %d sts=%d\n", nFrame, sts);
    }
    printf("1loop exit\n\n");

    // MFX_ERR_MORE_DATA means that file has ended, need to go to buffering loop, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
//    MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    //
    // Stage 2: Retrieve the buffered decoded frames
    //
    while (MFX_ERR_NONE <= sts || MFX_ERR_MORE_SURFACE == sts) {
        // if (MFX_WRN_DEVICE_BUSY == sts)
        //     MSDK_SLEEP(1);  // Wait if device is busy, then repeat the same call to DecodeFrameAsync

        nIndex = GetFreeSurfaceIndex(pmfxSurfaces);        // Find free frame surface
        // MSDK_CHECK_ERROR(MFX_ERR_NOT_FOUND, nIndex, MFX_ERR_MEMORY_ALLOC);

        // Decode a frame asychronously (returns immediately)
        sts = mfxDEC->DecodeFrameAsync(NULL, pmfxSurfaces[nIndex], &pmfxOutSurface, &syncp);

        // Ignore warnings if output is available,
        // if no output and no action required just repeat the DecodeFrameAsync call
        if (MFX_ERR_NONE < sts && syncp)
            sts = MFX_ERR_NONE;

        if (MFX_ERR_NONE == sts)
            sts = session->SyncOperation(syncp, 60000);      // Synchronize. Waits until decoded frame is ready

        if (MFX_ERR_NONE == sts) {
            ++nFrame;
            WriteRawFrame(pmfxOutSurface, out);
            // if (bEnableOutput) {
            //     sts = WriteRawFrame(pmfxOutSurface, fSink.get());
            //     MSDK_BREAK_ON_ERROR(sts);

            //     printf("Frame number: %d\r", nFrame);
            //     fflush(stdout);
            // }
            printf("2loop %d sts=%d\n", nFrame, sts);
        }
    }

    printf("2loop exit nFrame=%d, sts=%d\n", nFrame, sts);

    return sts;
}

// =================================================================
int main(int argc, char* argv[])
{
    
    printf("----- start -----\n");

    mfxStatus sts = MFX_ERR_NONE;

    Decoder *dec = new Decoder();

    // =====================================================================
    // Intel Media SDK decode pipeline setup
    // - In this example we are decoding an AVC (H.264) stream
    // - For simplistic memory management, system memory surfaces are used to store the decoded frames
    //   (Note that when using HW acceleration video surfaces are prefered, for better performance)
    //

    FILE* in = fopen("out2.264", "rb");
    uint8_t* esData = new uint8_t[1024];
    fread(esData, 1, 1024, in);
    dec->initialize();
    printf("after initialize\n");

    dec->decodeHeader(esData, 1024);
    printf("after decodeheader \n");
    // ===============================================================
    // Start decoding the frames
    //

    // ===============================================================
    dec->decodeTest1();
    printf("after decode\n");
    // ===============================================================

    //mfxTime tStart, tEnd;
    //mfxGetTime(&tStart);


    // MFX_ERR_MORE_DATA indicates that all buffers has been fetched, exit in case of other errors
    MSDK_IGNORE_MFX_STS(sts, MFX_ERR_MORE_DATA);
    // MSDK_CHECK_RESULT(sts, MFX_ERR_NONE, sts);

    // mfxGetTime(&tEnd);
    // double elapsed = TimeDiffMsec(tEnd, tStart) / 1000;
    // double fps = ((double)nFrame / elapsed);
    // printf("\nExecution time: %3.2f s (%3.2f fps)\n", elapsed, fps);

    // ===================================================================
    // Clean up resources
    //  - It is recommended to close Media SDK components first, before releasing allocated surfaces, since
    //    some surfaces may still be locked by internal Media SDK resources.

//    mfxDEC.Close();
    // session closed automatically on destruction

//    Release();

    printf("----- finish ----- \n");

    return 0;
}




// =================================================================

mfxStatus Decoder::StoreBitstreamData(uint8_t* data, size_t len)
{
    memmove(mfxBS->Data, mfxBS->Data + mfxBS->DataOffset, mfxBS->DataLength);
    mfxBS->DataOffset = 0;

    std::memcpy(mfxBS->Data + mfxBS->DataLength, (mfxU8*)data, len);

    mfxBS->DataLength += len;

    return MFX_ERR_NONE;
}

int Decoder::GetFreeSurfaceIndex(const std::vector<mfxFrameSurface1*>& pSurfacesPool)
{
    auto it = std::find_if(pSurfacesPool.begin(), pSurfacesPool.end(), [](const mfxFrameSurface1* surface) {
        return 0 == surface->Data.Locked;
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

mfxStatus Decoder::WriteSection(mfxU8* plane, mfxU16 factor, mfxU16 chunksize,
    mfxFrameInfo* pInfo, mfxFrameData* pData, mfxU32 i,
    mfxU32 j, FILE* fSink)
{
    if (chunksize !=
        fwrite(plane +
        (pInfo->CropY * pData->Pitch / factor + pInfo->CropX) +
            i * pData->Pitch + j, 1, chunksize, fSink))
        return MFX_ERR_UNDEFINED_BEHAVIOR;
    return MFX_ERR_NONE;
}

mfxStatus Decoder::WriteRawFrame(mfxFrameSurface1* pSurface, FILE* fSink)
{
    mfxFrameInfo* pInfo = &pSurface->Info;
    mfxFrameData* pData = &pSurface->Data;
    mfxU32 nByteWrite;
    mfxU16 i, j, h, w, pitch;
    mfxU8* ptr;
    mfxStatus sts = MFX_ERR_NONE;

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

    if (pInfo->FourCC == MFX_FOURCC_RGB4 || pInfo->FourCC == MFX_FOURCC_A2RGB10)
    {
        pitch = pData->Pitch;
        ptr = std::min(std::min(pData->R, pData->G), pData->B);

        for (i = 0; i < h; i++)
        {
            nByteWrite = (mfxU32)fwrite(ptr + i * pitch, 1, w * 4, fSink);
            if ((mfxU32)(w * 4) != nByteWrite)
            {
                return MFX_ERR_MORE_DATA;
            }
        }
    }
    else
    {
        for (i = 0; i < pInfo->CropH; i++)
            sts = WriteSection(pData->Y, 1, pInfo->CropW, pInfo, pData, i, 0, fSink);

        h = pInfo->CropH / 2;
        w = pInfo->CropW;
        for (i = 0; i < h; i++)
            for (j = 0; j < w; j += 2)
                sts = WriteSection(pData->UV, 2, 1, pInfo, pData, i, j, fSink);
        for (i = 0; i < h; i++)
            for (j = 1; j < w; j += 2)
                sts = WriteSection(pData->UV, 2, 1, pInfo, pData, i, j, fSink);
    }

    return sts;
}

// =================================================================







