/***********************************************************************/
/*                                                                     */
/* bmp_helper.hpp: Header file for utility routine for Win32 BMP       */
/*                                                                     */
/*     Copyright (C) 2011 Yak! / Yasutaka ATARASHI                     */
/*                                                                     */
/*     This software is distributed under the terms of a zlib/libpng   */
/*     License.                                                        */
/*                                                                     */
/*     $Id$                 */
/*                                                                     */
/***********************************************************************/
#ifndef YAK_BMP_HELPER_HPP
#define YAK_BMP_HELPER_HPP

#include <windows.h>

#include <iosfwd>
#include <vector>

namespace yak { namespace windows {

class BMPHelper
{
public:
	BMPHelper() {
		header.biSize = sizeof(BITMAPINFOHEADER);
		header.biPlanes = 1;
		header.biCompression = BI_RGB;
		header.biXPelsPerMeter = 0; // 3780 for 96dpi
		header.biYPelsPerMeter = 0; // 3780 for 96dpi
	}
	// NOTE: Assuming bit per components is 8
	void init_rgb(int bpc, int width, int height)
	{
		stride = normalize(normalize(width * bpc * 3, 8) / 8, 4); // 8 is bits/byte, 4 is alignment
		v.resize(stride * height);

		header.biWidth = width;
		header.biHeight = height;
		header.biBitCount = bpc * 3; // TODO: assertion
		header.biSizeImage = stride * height;
	}
	void init_index(int bits, int width, int height)
	{
		stride = normalize(normalize(width * bits, 8) / 8, 4); // 8 is bits/byte, 4 is alignment
		v.resize(stride * height);

		header.biWidth = width;
		header.biHeight = height;
		header.biBitCount = bits; // TODO: assertion
		header.biSizeImage = stride * height;
	}
	void init_bw(int bits, int width, int height)
	{
		init_index(bits, width, height);
		const unsigned int m = (1U << bits) - 1;
		palette.resize(m + 1);
		for(unsigned int i = 0; i <= m; ++i) {
			palette[i].rgbRed = palette[i].rgbGreen = palette[i].rgbBlue = conv(i, 0, m, 0, 255);
			palette[i].rgbReserved = 0;
		}
	}
	// NOTE: Assuming bit per components is 8
	void set_palette_rgb(const void* p, int num_colors)
	{
		palette.resize(num_colors);
		const unsigned char (*rgb)[3] = static_cast<const unsigned char(*)[3]>(p);
		for(int i = 0; i < num_colors; ++i) {
			palette[i].rgbRed      = rgb[i][0];
			palette[i].rgbGreen    = rgb[i][1];
			palette[i].rgbBlue     = rgb[i][2];
			palette[i].rgbReserved = 0;
		}
	}
	void set_palette_bw(const void* p, int num_colors)
	{
		palette.resize(num_colors);
		const unsigned char *v = static_cast<const unsigned char*>(p);
		for(int i = 0; i < num_colors; ++i) {
			palette[i].rgbRed      = v[i];
			palette[i].rgbGreen    = v[i];
			palette[i].rgbBlue     = v[i];
			palette[i].rgbReserved = 0;
		}
	}
	// NOTE: Assuming bit per components is 8
	void set_pixels_rgb(const void* p);
	void set_pixels_bw(const void* p);
	std::size_t size() const {
		return sizeof(BITMAPFILEHEADER) + sizeof(header) + sizeof(RGBQUAD) * palette.size() + v.size();
	}
	void write(const std::string &s);
	void write(void *p, std::size_t size_);
private:
	inline int conv(int x, int xmin, int xmax, int ymin, int ymax) const
	{
		return ymin + (x - xmin) * (ymax - ymin) / (xmax - xmin);
	}
	int normalize(int n, int d) const
	{
		return (n+d-1)/d*d;
	}
	void fixup()
	{
		if(header.biBitCount == 24U || header.biBitCount == 32U || (1U << header.biBitCount) == palette.size()) {
			header.biClrUsed = header.biClrImportant = 0;
		} else {
			header.biClrUsed = header.biClrImportant = palette.size();
		}
	}
	BITMAPINFOHEADER header;
	std::vector<RGBQUAD> palette;
	std::vector<unsigned char> v;
	int stride;
};

}}

#endif
