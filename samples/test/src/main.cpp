#include "intel_video_decoder.h"



int main(int argc, char **argv)
{

    printf("---- start ----\n");

    std::string windowname = "first";
    cv::namedWindow(windowname, cv::WINDOW_NORMAL);
    std::string windowname2 = "second";
    cv::namedWindow(windowname2, cv::WINDOW_NORMAL);

    IntelQsvH264Decoder *decoder = new IntelQsvH264Decoder();
    IntelQsvH264Decoder* dec2 = new IntelQsvH264Decoder();
    decoder->initialize();
    dec2->initialize();

    FILE *in = fopen("../test1.264", "rb");
    if (!in)
    {
        printf("file not open\n");
        return -1;
    }
    FILE* in2 = fopen("../test2.264", "rb");
    if (!in2)
    {
        printf("file not open\n");
        return -1;
    }

    int frameLen = 10 * 1024 * 1024;
    int headerProbeLen = 1024;
    uint8_t *frame = new uint8_t[frameLen];
    uint8_t* frame2 = new uint8_t[frameLen];
    int width = decoder->getWidth();
    int height = decoder->getHeight();
    int width2 = dec2->getWidth();
    int height2 = dec2->getHeight();
    int count = 0;
    int totalReadBytes = 0;
    int totalCount = 0;
    while (true)
    {
        int nReadBytes = fread(frame, 1, 1 * 1024, in);
        int nReadBytes2 = fread(frame2, 1, 1 * 1024, in2);
        totalReadBytes += nReadBytes;
        totalCount++;
        if (0 == nReadBytes && 0 == nReadBytes2)
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
        if (!dec2->isInit())
        {
            if (dec2->decodeHeader(frame2, nReadBytes2) >= 0)
            {
                printf("dec2 header succeed.\n");
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

        sts = dec2->decode(frame2, nReadBytes2);
        if (sts == 0)
        {
            width2 = dec2->getWidth();
            height2 = dec2->getHeight();
            uint8_t* rawData = new uint8_t[width2 * height2 * 3];
            int size = dec2->getFrame(rawData, 1);
            while (size > 0)
            {
                cv::Mat mat(height2, width2, CV_8UC3, rawData);
                cv::cvtColor(mat, mat, cv::COLOR_RGB2BGR);
                cv::imshow(windowname2, mat);
                cv::waitKey(1);
                size = dec2->getFrame(rawData, 2);
                printf("main: dec2 while end size=%d\n", size);
            }
        }
        else
        {
            printf("main: dec2 failed %d\n", sts);
        }

    }

    printf("main: whie loop exit\n");

    uint8_t* rawData = new uint8_t[(int)(width * height * 3)];
    int size = decoder->drainFrame(rawData, 1);
    while (size > 0)
    {
        cv::Mat mat(height, width, CV_8UC3, rawData);
        cv::imshow(windowname, mat);
        cv::waitKey(1);
        size = decoder->drainFrame(rawData, 1);
        printf("main: drain while end size=%d\n", size);
    }
    size = dec2->drainFrame(rawData, 1);
    while (size > 0)
    {
        cv::Mat mat(height2, width2, CV_8UC3, rawData);
        cv::imshow(windowname2, mat);
        cv::waitKey(1);
        size = dec2->drainFrame(rawData, 1);
        printf("main: dec2 drain while end size=%d\n", size);
    }

    printf("totalCount = %d, totalReadBytes = %d\n", totalCount, totalReadBytes);

    decoder->uninitialize();
    dec2->uninitialize();

    printf("---- finish ---- \n");

    return 0;
}

