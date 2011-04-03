/***********************************************************************/
/*                                                                     */
/* bmp_helper.cpp: Source file for utility routine for Win32 BMP       */
/*                                                                     */
/*     Copyright (C) 2011 Yak! / Yasutaka ATARASHI                     */
/*                                                                     */
/*     This software is distributed under the terms of a zlib/libpng   */
/*     License.                                                        */
/*                                                                     */
/*     $Id$                 */
/*                                                                     */
/***********************************************************************/

#include "bmp_helper.hpp"

#include <iostream>
#include <fstream>

namespace {

	inline void* copy(const void* from, std::size_t size, void *to)
	{
		const char *from_ = static_cast<const char*>(from);
		char *to_ = static_cast<char*>(to);
		return std::copy(from_, from_ + size, to_);
	}

	inline void mywrite(std::ostream &os, const void* p, std::size_t size)
	{
		os.write(static_cast<const char*>(p), size);
	}

}

namespace yak { namespace windows {

	// NOTE: Assuming bit per components is 8
	void BMPHelper::set_pixels_rgb(const void* p)
	{
		const int stride_from = header.biWidth * header.biBitCount / 8, stride_to = stride;
		const int height = header.biHeight;
		const int width = header.biWidth;
		const unsigned char *from = static_cast<const unsigned char*>(p);
		unsigned char *to = &v[0];
		for(int h = height - 1; h >= 0; --h) {
			const unsigned char *from_ = from + stride_from * h;
			unsigned char *to_ = to + stride_to * (height - 1 - h);
			for(int w = 0; w < width; ++w) {
				to_[w*3 + 2] = from_[w*3 + 0]; // R
				to_[w*3 + 1] = from_[w*3 + 1]; // G
				to_[w*3 + 0] = from_[w*3 + 2]; // B
			}
			std::fill(to + stride_to * (height - 1 - h) + stride_from, to + stride_to * (height - h), 0);
		}
	}

	void BMPHelper::set_pixels_bw(const void* p)
	{
		const int stride_from = normalize(header.biWidth * header.biBitCount, 8) / 8, stride_to = stride;
		const int height = header.biHeight;
		const unsigned char *from = static_cast<const unsigned char*>(p);
		unsigned char *to = &v[0];
		for(int h = height - 1; h >= 0; --h) {
			// NOTE: In pedantic, copying padding bits from source to destination might be problem (because padding in BMP should be 0)
			std::copy(from + stride_from * h, from + stride_from * (h + 1), to + stride_to * (height - 1 - h));
			std::fill(to + stride_to * (height - 1 - h) + stride_from, to + stride_to * (height - h), 0);
		}
	}

	void BMPHelper::write(const std::string &s)
	{
		fixup();
		BITMAPFILEHEADER fileheader = {
			0x4d42, // WORD bfType : Magic
			sizeof(BITMAPFILEHEADER) + sizeof(header) + sizeof(RGBQUAD) * palette.size() + v.size(), // DWORD bfSize : All of BMP
			0, // WORD bfReserved1
			0, // WORD bfReserved2
			sizeof(BITMAPFILEHEADER) + sizeof(header) + sizeof(RGBQUAD) * palette.size() // DWORD bfOffBits : Offset to actual bits
		};
		std::ofstream ofs(s.c_str(), std::ios::out | std::ios::binary);
		mywrite(ofs, &fileheader, sizeof(fileheader));
		mywrite(ofs, &header, sizeof(header));
		if(palette.size() != 0) {
			mywrite(ofs, &palette[0], sizeof(RGBQUAD) * palette.size());
		}
		mywrite(ofs, &v[0], v.size());
	}

	void BMPHelper::write(void *p, std::size_t size_)
	{
		if(size_ < size()) throw 1; // TODO: appropriate exception
		fixup();
		BITMAPFILEHEADER fileheader = {
			0x4d42, // WORD bfType : Magic
			sizeof(BITMAPFILEHEADER) + sizeof(header) + sizeof(RGBQUAD) * palette.size() + v.size(), // DWORD bfSize : All of BMP
			0, // WORD bfReserved1
			0, // WORD bfReserved2
			sizeof(BITMAPFILEHEADER) + sizeof(header) + sizeof(RGBQUAD) * palette.size() // DWORD bfOffBits : Offset to actual bits
		};
		p = copy(&fileheader, sizeof(fileheader), p);
		p = copy(&header, sizeof(header), p);
		if(palette.size() != 0) {
			p = copy(&palette[0], sizeof(RGBQUAD) * palette.size(), p);
		}
		copy(&v[0], v.size(), p);
	}

}}
