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

#include "intel_qsv_decoder_api.h"

// =================================================================
int initialize()
{
	dec = new IntelQsvH264Decoder();
	return dec->initialize();
}

void uninitialize()
{
	dec->uninitialize();
	delete dec;
}

int decodeHeader(uint8_t* frame, size_t length)
{
	return dec->decodeHeader(frame, length);
}

int decode(uint8_t* in, size_t in_length)
{
	return dec->decode(in, in_length);
}

int decode_get(uint8_t* in, size_t in_length, uint8_t* out, int conv_opt)
{
	return dec->decode_get(in, in_length, out, conv_opt);
}

int getFrame(uint8_t* out, int conv_opt)
{
	return dec->getFrame(out, conv_opt);
}

int drainFrame(uint8_t* out, int conv_opt)
{
	return dec->drainFrame(out, conv_opt);
}

int getWidth()
{
	return dec->getWidth();
}

int getHeight()
{
	return dec->getHeight();
}

bool isInit()
{
	return dec->isInit();
}
// =================================================================



// =================================================================
int newInstance()
{
	mutex.lock();
	decodes.push_back(new IntelQsvH264Decoder());
	int ret = decodes.size() - 1;
	mutex.unlock();
	return ret;
}

int m_initialize(int i)
{
	auto dec = decodes[i];
	return dec->initialize();
}

void m_uninitialize(int i)
{
	auto dec = decodes[i];
	dec->uninitialize();
	delete dec;
}

int m_decodeHeader(int i, uint8_t* frame, size_t length)
{
	auto dec = decodes[i];
	return dec->decodeHeader(frame, length);
}

int m_decode(int i, uint8_t* in, size_t in_length)
{
	auto dec = decodes[i];
	return dec->decode(in, in_length);
}

int m_decode_get(int i, uint8_t* in, size_t in_length, uint8_t* out, int conv_opt)
{
	auto dec = decodes[i];
	return dec->decode_get(in, in_length, out, conv_opt);
}

int m_getFrame(int i, uint8_t* out, int conv_opt)
{
	auto dec = decodes[i];
	return dec->getFrame(out, conv_opt);
}

int m_drainFrame(int i, uint8_t* out, int conv_opt)
{
	auto dec = decodes[i];
	return dec->drainFrame(out, conv_opt);
}

int m_getWidth(int i)
{
	auto dec = decodes[i];
	return dec->getWidth();
}

int m_getHeight(int i)
{
	auto dec = decodes[i];
	return dec->getHeight();
}

bool m_isInit(int i)
{
	auto dec = decodes[i];
	return dec->isInit();
}
// =================================================================

