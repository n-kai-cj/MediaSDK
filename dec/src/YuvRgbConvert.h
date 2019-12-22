// Copyright 2016 Adrien Descamps
// Distributed under BSD 3-Clause License

// Provide optimized functions to convert images from 8bits yuv420 to rgb24 format

// There are a few slightly different variations of the YCbCr color space with different parameters that 
// change the conversion matrix.
// The three most common YCbCr color space, defined by BT.601, BT.709 and JPEG standard are implemented here.
// See the respective standards for details
// The matrix values used are derived from http://www.equasys.de/colorconversion.html

// YUV420 is stored as three separate channels, with U and V (Cb and Cr) subsampled by a 2 factor
// For conversion from yuv to rgb, no interpolation is done, and the same UV value are used for 4 rgb pixels. This 
// is suboptimal for image quality, but by far the fastest method.

// For all methods, width and height should be even, if not, the last row/column of the result image won't be affected.
// For sse methods, if the width if not divisable by 32, the last (width%32) pixels of each line won't be affected.

#ifndef __yuvrgbconvert__
#define __yuvrgbconvert__

#include <stdio.h>
#include <stdint.h>
#include <cstring>
#ifdef _WIN32
#include <intrin.h> // for SSE
#elif __linux__
#include <x86intrin.h> // all intrinsic
#endif

#ifdef HAVE_IPP
#include <ipp.h> // for IPP
#include <ippi.h> // for IPP
#endif

typedef enum
{
	YCBCR_JPEG,
	YCBCR_601,
	YCBCR_709
}YCbCrType;

class YuvRgbConvert
{
public:

#ifdef HAVE_IPP
	// yuv to rgb with Intel IPP, yuv in nv12 semi palnar format
	static void nv12_rgb24_ipp(unsigned char I[],
		const int image_width, const int image_height, unsigned char J[]);

	// yuv to rgb with Intel IPP, yuv in nv21 semi palnar format
	static void nv21_rgb24_ipp(unsigned char I[],
		const int image_width, const int image_height, unsigned char J[]);
#endif

	// yuv to rgb, standard c implementation
	static void yuv420_rgb24_std(
		uint32_t width, uint32_t height,
		const uint8_t* y, const uint8_t* u, const uint8_t* v, uint32_t y_stride, uint32_t uv_stride,
		uint8_t* rgb, uint32_t rgb_stride,
		YCbCrType yuv_type);

	// yuv to rgb, yuv in nv12 semi planar format
	static void nv12_rgb24_std(
		uint32_t width, uint32_t height,
		const uint8_t* y, const uint8_t* uv, uint32_t y_stride, uint32_t uv_stride,
		uint8_t* rgb, uint32_t rgb_stride,
		YCbCrType yuv_type);

	// yuv to rgb, yuv in nv12 semi planar format
	static void nv21_rgb24_std(
		uint32_t width, uint32_t height,
		const uint8_t* y, const uint8_t* uv, uint32_t y_stride, uint32_t uv_stride,
		uint8_t* rgb, uint32_t rgb_stride,
		YCbCrType yuv_type);

	// yuv to rgb, sse implementation
	// pointers must be 16 byte aligned, and strides must be divisable by 16
	static void yuv420_rgb24_sse(
		uint32_t width, uint32_t height,
		const uint8_t* y, const uint8_t* u, const uint8_t* v, uint32_t y_stride, uint32_t uv_stride,
		uint8_t* rgb, uint32_t rgb_stride,
		YCbCrType yuv_type);

	// yuv to rgb, sse implementation
	// pointers do not need to be 16 byte aligned
	static void yuv420_rgb24_sseu(
		uint32_t width, uint32_t height,
		const uint8_t* y, const uint8_t* u, const uint8_t* v, uint32_t y_stride, uint32_t uv_stride,
		uint8_t* rgb, uint32_t rgb_stride,
		YCbCrType yuv_type);

	// yuv nv12 to rgb, sse implementation
	// pointers must be 16 byte aligned, and strides must be divisable by 16
	static void nv12_rgb24_sse(
		uint32_t width, uint32_t height,
		const uint8_t* y, const uint8_t* uv, uint32_t y_stride, uint32_t uv_stride,
		uint8_t* rgb, uint32_t rgb_stride,
		YCbCrType yuv_type);

	// yuv nv12 to rgb, sse implementation
	// pointers do not need to be 16 byte aligned
	static void nv12_rgb24_sseu(
		uint32_t width, uint32_t height,
		const uint8_t* y, const uint8_t* uv, uint32_t y_stride, uint32_t uv_stride,
		uint8_t* rgb, uint32_t rgb_stride,
		YCbCrType yuv_type);

	// yuv nv21 to rgb, sse implementation
	// pointers must be 16 byte aligned, and strides must be divisable by 16
	static void nv21_rgb24_sse(
		uint32_t width, uint32_t height,
		const uint8_t* y, const uint8_t* uv, uint32_t y_stride, uint32_t uv_stride,
		uint8_t* rgb, uint32_t rgb_stride,
		YCbCrType yuv_type);

	// yuv nv21 to rgb, sse implementation
	// pointers do not need to be 16 byte aligned
	static void nv21_rgb24_sseu(
		uint32_t width, uint32_t height,
		const uint8_t* y, const uint8_t* uv, uint32_t y_stride, uint32_t uv_stride,
		uint8_t* rgb, uint32_t rgb_stride,
		YCbCrType yuv_type);



	// rgb to yuv, standard c implementation
	static void rgb24_yuv420_std(
		uint32_t width, uint32_t height,
		const uint8_t* rgb, uint32_t rgb_stride,
		uint8_t* y, uint8_t* u, uint8_t* v, uint32_t y_stride, uint32_t uv_stride,
		YCbCrType yuv_type);

	// rgb to yuv, sse implementation
	// pointers must be 16 byte aligned, and strides must be divisible by 16
	static void rgb24_yuv420_sse(
		uint32_t width, uint32_t height,
		const uint8_t* rgb, uint32_t rgb_stride,
		uint8_t* y, uint8_t* u, uint8_t* v, uint32_t y_stride, uint32_t uv_stride,
		YCbCrType yuv_type);

	// rgb to yuv, sse implementation
	// pointers do not need to be 16 byte aligned
	static void rgb24_yuv420_sseu(
		uint32_t width, uint32_t height,
		const uint8_t* rgb, uint32_t rgb_stride,
		uint8_t* y, uint8_t* u, uint8_t* v, uint32_t y_stride, uint32_t uv_stride,
		YCbCrType yuv_type);

	// rgba to yuv, standard c implementation
	// alpha channel is ignored
	static void rgb32_yuv420_std(
		uint32_t width, uint32_t height,
		const uint8_t* rgba, uint32_t rgba_stride,
		uint8_t* y, uint8_t* u, uint8_t* v, uint32_t y_stride, uint32_t uv_stride,
		YCbCrType yuv_type);

	// rgba to yuv, sse implementation
	// pointers must be 16 byte aligned, and strides must be divisible by 16
	// alpha channel is ignored
	static void rgb32_yuv420_sse(
		uint32_t width, uint32_t height,
		const uint8_t* rgba, uint32_t rgba_stride,
		uint8_t* y, uint8_t* u, uint8_t* v, uint32_t y_stride, uint32_t uv_stride,
		YCbCrType yuv_type);

	// rgba to yuv, sse implementation
	// pointers do not need to be 16 byte aligned
	// alpha channel is ignored
	static void rgb32_yuv420_sseu(
		uint32_t width, uint32_t height,
		const uint8_t* rgba, uint32_t rgba_stride,
		uint8_t* y, uint8_t* u, uint8_t* v, uint32_t y_stride, uint32_t uv_stride,
		YCbCrType yuv_type);

};

#endif /* __yuvrgbconvert__ */
