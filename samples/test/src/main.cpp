#include "intel_video_decoder.h"



int main(int argc, char **argv)
{

    printf("---- start ----\n");

    std::string windowname = "window";
    cv::namedWindow(windowname, cv::WINDOW_NORMAL);

    IntelQsvH264Decoder *decoder = new IntelQsvH264Decoder();
    decoder->initialize();

    FILE *in = fopen("../out.264", "rb");
    if (!in) {
        printf("file not open\n");
        return -1;
    }

    int frameLen = 10 * 1024 * 1024;
    int headerProbeLen = 1024;
    uint8_t *frame = new uint8_t[frameLen];
    int width = -1;
    int height = -1;
    int count = 0;
    int totalReadBytes = 0;
    int totalCount = 0;
    while (true)
    {
        int nReadBytes = fread(frame, 1, 1*1024, in);
        totalReadBytes += nReadBytes;
        totalCount++;
        if (0 == nReadBytes)
        {
            printf("reach end of file\n");
            break;
        }

        if (!decoder->isInit())
        {
            if (decoder->decodeHeader(frame, nReadBytes) >= 0)
            {
                printf("decode header succeed.\n");
            }
            else
            {
                continue;
            }
        }
        int sts = decoder->decode(frame, nReadBytes);
        if (sts == 0)
        {
            if (width != decoder->getWidth() || height != decoder->getHeight())
            {
                printf("main: param changed %dx%d -> %dx%d\n", width, height, decoder->getWidth(), decoder->getHeight());
            }
            width = decoder->getWidth();
            height = decoder->getHeight();
            uint8_t* rawData = new uint8_t[(int)(width * height * 3)];
            int size = decoder->getFrame(rawData, 2);
            while (size > 0)
            {
                cv::Mat mat(height, width, CV_8UC3, rawData);
                cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
                cv::imshow(windowname, mat);
                cv::waitKey(1);
                size = decoder->getFrame(rawData, 2);
                printf("main: while end size=%d\n", size);
            }
        }
        else
        {
            printf("main: decode failed %d\n", sts);
        }
    }

    printf("main: whie loop exit\n");

    uint8_t* rawData = new uint8_t[(int)(width * height * 3)];
    int size = decoder->drainFrame(rawData, 1);
    while (size > 0)
    {
        cv::Mat mat(height, width, CV_8UC3, rawData);
        cv::imshow("window", mat);
        cv::waitKey(1);
        size = decoder->drainFrame(rawData, 1);
        printf("main: drain while end size=%d\n", size);
    }

    printf("totalCount = %d, totalReadBytes = %d\n", totalCount, totalReadBytes);

    decoder->uninitialize();

    printf("---- finish ---- \n");

    return 0;
}

