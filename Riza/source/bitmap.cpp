//	VirtualDub - Video processing and capture application
//	A/V interface library
//	Copyright (C) 1998-2004 Avery Lee
//
//	This program is free software; you can redistribute it and/or modify
//	it under the terms of the GNU General Public License as published by
//	the Free Software Foundation; either version 2 of the License, or
//	(at your option) any later version.
//
//	This program is distributed in the hope that it will be useful,
//	but WITHOUT ANY WARRANTY; without even the implied warranty of
//	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//	GNU General Public License for more details.
//
//	You should have received a copy of the GNU General Public License
//	along with this program; if not, write to the Free Software
//	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

#include <windows.h>
#include <vd2/Riza/bitmap.h>
#include <vd2/Kasumi/pixmap.h>
#include <vd2/Kasumi/pixmaputils.h>

int VDBitmapFormatToPixmapFormat(const VDAVIBitmapInfoHeader& hdr) {
	int variant;

	return VDBitmapFormatToPixmapFormat(hdr, variant);
}

int VDBitmapFormatToPixmapFormat(const VDAVIBitmapInfoHeader& hdr, int& variant) {
	using namespace nsVDPixmap;

	variant = 1;

	switch(hdr.biCompression) {
	case VDAVIBitmapInfoHeader::kCompressionRGB:
		if (hdr.biPlanes == 1) {
			if (hdr.biBitCount == 1)
				return kPixFormat_Pal1;
			else if (hdr.biBitCount == 2)
				return kPixFormat_Pal2;
			else if (hdr.biBitCount == 4)
				return kPixFormat_Pal4;
			else if (hdr.biBitCount == 8)
				return kPixFormat_Pal8;
			else if (hdr.biBitCount == 16)
				return kPixFormat_XRGB1555;
			else if (hdr.biBitCount == 24)
				return kPixFormat_RGB888;
			else if (hdr.biBitCount == 32)
				return kPixFormat_XRGB8888;
		}
		break;
	case VDAVIBitmapInfoHeader::kCompressionBitfields:
		{
			const BITMAPV4HEADER& v4hdr = (const BITMAPV4HEADER&)hdr;
			const int bits = v4hdr.bV4BitCount;
			const uint32 r = v4hdr.bV4RedMask;
			const uint32 g = v4hdr.bV4GreenMask;
			const uint32 b = v4hdr.bV4BlueMask;

			if (bits == 16 && r == 0x7c00 && g == 0x03e0 && b == 0x001f)
				return kPixFormat_XRGB1555;
			else if (bits == 16 && r == 0xf800 && g == 0x07e0 && b == 0x001f)
				return kPixFormat_RGB565;
			else if (bits == 32 && r == 0xff0000 && g == 0x00ff00 && b == 0x0000ff)
				return kPixFormat_XRGB8888;
		}
		break;

	case VDMAKEFOURCC('U', 'Y', 'V', 'Y'):
		return kPixFormat_YUV422_UYVY;

	case VDMAKEFOURCC('Y', 'U', 'Y', 'V'):
	case VDMAKEFOURCC('Y', 'U', 'Y', '2'):
		return kPixFormat_YUV422_YUYV;

	case VDMAKEFOURCC('Y', 'V', '2', '4'):	// Avisynth format
		return kPixFormat_YUV444_Planar;

	case VDMAKEFOURCC('Y', 'V', '1', '6'):
		return kPixFormat_YUV422_Planar;

	case VDMAKEFOURCC('Y', 'V', '1', '2'):
		return kPixFormat_YUV420_Planar;

	case VDMAKEFOURCC('I', '4', '2', '0'):
		variant = 2;
		return kPixFormat_YUV420_Planar;

	case VDMAKEFOURCC('I', 'Y', 'U', 'V'):
		variant = 3;
		return kPixFormat_YUV420_Planar;

	case VDMAKEFOURCC('Y', 'V', 'U', '9'):
		return kPixFormat_YUV410_Planar;

	case VDMAKEFOURCC('Y', '8', ' ', ' '):
	case VDMAKEFOURCC('Y', '8', '0', '0'):
		return kPixFormat_Y8;
	}
	return 0;
}

int VDGetPixmapToBitmapVariants(int format) {
	if (format == nsVDPixmap::kPixFormat_YUV420_Planar)
		return 3;

	if (format == nsVDPixmap::kPixFormat_Y8)
		return 2;

	return 1;
}

bool VDMakeBitmapFormatFromPixmapFormat(vdstructex<VDAVIBitmapInfoHeader>& dst, const vdstructex<VDAVIBitmapInfoHeader>& src, int format, int variant) {
	return VDMakeBitmapFormatFromPixmapFormat(dst, src, format, variant, src->biWidth, src->biHeight);
}

bool VDMakeBitmapFormatFromPixmapFormat(vdstructex<VDAVIBitmapInfoHeader>& dst, const vdstructex<VDAVIBitmapInfoHeader>& src, int format, int variant, uint32 w, uint32 h) {
	if (format == nsVDPixmap::kPixFormat_Pal8) {
		if (src->biBitCount > 8)
			return false;

		uint32 clrUsed = src->biClrUsed;
		uint32 clrImportant = src->biClrImportant;

		if (clrUsed == 0) {
			if (src->biCompression != BI_RGB && src->biCompression != BI_RLE4 && src->biCompression != BI_RLE8)
				return false;

			clrUsed = 1 << src->biBitCount;
		}
		if (clrUsed >= 256)
			clrUsed = 0;				// means 'max for type'
		if (clrImportant >= clrUsed)
			clrImportant = 0;			// means 'all required'

		uint32 clrEntries = clrUsed;
		if (!clrEntries)
			clrEntries = 256;

		dst.resize(sizeof(VDAVIBitmapInfoHeader) + sizeof(RGBQUAD) * clrEntries);
		dst->biSize				= sizeof(VDAVIBitmapInfoHeader);
		dst->biWidth			= w;
		dst->biHeight			= h;
		dst->biPlanes			= 1;
		dst->biBitCount			= 8;
		dst->biCompression		= BI_RGB;
		dst->biSizeImage		= ((w+3)&~3)*h;
		dst->biXPelsPerMeter	= src->biXPelsPerMeter;
		dst->biYPelsPerMeter	= src->biYPelsPerMeter;
		dst->biClrUsed			= src->biClrUsed;
		dst->biClrImportant		= src->biClrImportant;

		uint32 clrTableSize = sizeof(RGBQUAD)*clrEntries;
		memcpy((char *)dst.data() + sizeof(VDAVIBitmapInfoHeader), (const char *)src.data() + src->biSize, clrTableSize);

		return true;
	}

	if (!VDMakeBitmapFormatFromPixmapFormat(dst, format, variant, w, h))
		return false;

	dst->biXPelsPerMeter = src->biXPelsPerMeter;
	dst->biYPelsPerMeter = src->biYPelsPerMeter;

	return true;
}

bool VDMakeBitmapFormatFromPixmapFormat(vdstructex<VDAVIBitmapInfoHeader>& dst, int format, int variant, uint32 w, uint32 h) {
	using namespace nsVDPixmap;

	dst.resize(sizeof(VDAVIBitmapInfoHeader));
	dst->biSize				= sizeof(VDAVIBitmapInfoHeader);
	dst->biWidth			= w;
	dst->biHeight			= h;
	dst->biPlanes			= 1;
	dst->biXPelsPerMeter	= 0;
	dst->biYPelsPerMeter	= 0;
	dst->biClrUsed			= 0;
	dst->biClrImportant		= 0;

	switch(format) {
	case kPixFormat_XRGB1555:
		dst->biCompression	= BI_RGB;
		dst->biBitCount		= 16;
		dst->biSizeImage	= ((w*2+3)&~3) * h;
		break;
	case kPixFormat_RGB565:
		dst->biCompression	= BI_BITFIELDS;
		dst->biBitCount		= 16;
		dst->biSizeImage	= ((w*2+3)&~3) * h;
		dst.resize(sizeof(VDAVIBitmapInfoHeader) + 3*sizeof(DWORD));
		{
			DWORD *fields = (DWORD *)(dst.data() + 1);
			fields[0] = 0xf800;
			fields[1] = 0x07e0;
			fields[2] = 0x001f;
		}
		break;
	case kPixFormat_RGB888:
		dst->biCompression	= BI_RGB;
		dst->biBitCount		= 24;
		dst->biSizeImage	= ((w*3+3)&~3) * h;
		break;
	case kPixFormat_XRGB8888:
		dst->biCompression	= BI_RGB;
		dst->biBitCount		= 32;
		dst->biSizeImage	= w*4 * h;
		break;
	case kPixFormat_YUV422_UYVY:
		dst->biCompression	= 'YVYU';
		dst->biBitCount		= 16;
		dst->biSizeImage	= ((w+1)&~1)*2*h;
		break;
	case kPixFormat_YUV422_YUYV:
		dst->biCompression	= '2YUY';
		dst->biBitCount		= 16;
		dst->biSizeImage	= ((w+1)&~1)*2*h;
		break;
	case kPixFormat_YUV444_Planar:
		dst->biCompression	= '42VY';
		dst->biBitCount		= 24;
		dst->biSizeImage	= w * h * 3;
		break;
	case kPixFormat_YUV422_Planar:
		dst->biCompression	= '61VY';
		dst->biBitCount		= 16;
		dst->biSizeImage	= ((w+1)>>1) * h * 4;
		break;
	case kPixFormat_YUV420_Planar:
		switch(variant) {
		case 3:
			dst->biCompression	= 'VUYI';
			break;
		case 2:
			dst->biCompression	= '024I';
			break;
		case 1:
		default:
			dst->biCompression	= '21VY';
			break;
		}
		dst->biBitCount		= 12;
		dst->biSizeImage	= w*h + (w>>1)*(h>>1)*2;
		break;
	case kPixFormat_YUV410_Planar:
		dst->biCompression	= '9UVY';
		dst->biBitCount		= 9;
		dst->biSizeImage	= ((w+2)>>2) * ((h+2)>>2) * 18;
		break;
	case kPixFormat_Y8:
		switch(variant) {
		case 2:
			dst->biCompression	= '008Y';
			break;
		case 1:
		default:
			dst->biCompression	= '  8Y';
			break;
		}
		dst->biBitCount		= 8;
		dst->biSizeImage	= ((w+3) & ~3) * h;
		break;
	default:
		return false;
	};

	return true;
}

uint32 VDMakeBitmapCompatiblePixmapLayout(VDPixmapLayout& layout, sint32 w, sint32 h, int format, int variant, const uint32 *palette) {
	using namespace nsVDPixmap;

	uint32 linspace = VDPixmapCreateLinearLayout(layout, format, w, abs(h), VDPixmapGetInfo(format).auxbufs > 1 ? 1 : 4);

	switch(format) {
	case kPixFormat_Pal8:
		layout.palette = palette;
		// fall through
	case kPixFormat_XRGB1555:
	case kPixFormat_RGB888:
	case kPixFormat_RGB565:
	case kPixFormat_XRGB8888:
		// RGB can be flipped (but YUV can't)
		if (h > 0) {
			layout.data += layout.pitch * (h-1);
			layout.pitch = -layout.pitch;
		}
		break;
	case kPixFormat_YUV420_Planar:
		if (variant < 2) {				// need to swap UV planes for YV12 (1)
			std::swap(layout.data2, layout.data3);
			std::swap(layout.pitch2, layout.pitch3);
		}
		break;
	case kPixFormat_YUV410_Planar:
		std::swap(layout.data2, layout.data3);
		std::swap(layout.pitch2, layout.pitch3);
		break;
	}

	return linspace;
}

VDPixmap VDGetPixmapForBitmap(const VDAVIBitmapInfoHeader& hdr, const void *data) {
	int variant;

	int format = VDBitmapFormatToPixmapFormat(hdr, variant);
	VDPixmapLayout layout;
	VDMakeBitmapCompatiblePixmapLayout(layout, hdr.biWidth, hdr.biHeight, format, variant);

	return VDPixmapFromLayout(layout, (void *)data);
}