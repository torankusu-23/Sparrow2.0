#include "../public/Image.h"

#include <ctime>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>

Image::Image() : buffer(NULL), width(0), height(0), bpp(0) {}

Image::~Image() {if (buffer) delete[] buffer;}

Image::Image(int w, int h, int bpp) : buffer(NULL), width(w), height(h), bpp(bpp)
{
	unsigned long nbytes = width * height * bpp;
	buffer = new unsigned char[width * height * bpp];
	memset(buffer, 0, nbytes);
}
//���ƹ���
Image::Image(const Image& img) : buffer(NULL), width(img.width), height(img.height), bpp(img.bpp)
{
	unsigned long nbytes = width * height * bpp;
	buffer = new unsigned char[nbytes];
	memcpy(buffer, img.buffer, nbytes);
}

Image& Image::operator =(const Image& img)
{
	if (this != &img)
	{
		if (buffer) delete[] buffer;
		width = img.width;
		height = img.height;
		bpp = img.bpp;
		unsigned long nbytes = width * height * bpp;
		buffer = new unsigned char[nbytes];
		memcpy(buffer, img.buffer, nbytes);
	}
	return *this;
}

bool Image::load_rle_data(std::ifstream& in)	//RLE�㷨ѹ��ͼ��Ķ�ȡ
{
	unsigned long pixelcount = width * height;
	unsigned long currentpixel = 0;
	unsigned long currentbyte = 0;
	Color colorbuffer;	//Ĭ�Ϲ���Ҷ�ͼ
	do  //��0��ʼ����ÿһ������
	{
		unsigned char chunkheader = 0;
		chunkheader = in.get();	//��ȡrle����chunk�ĸ���
		if (!in.good())
		{
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
		if (chunkheader < 128)
		{
			chunkheader++;
			for (int i = 0; i < chunkheader; i++)	//����chunk
			{
				in.read((char*)colorbuffer.rgba, bpp);
				if (!in.good())
				{
					std::cerr << "an error occured while reading the header\n";
					return false;
				}
				for (int t = 0; t < bpp; t++)
					buffer[currentbyte++] = colorbuffer.rgba[t];
				currentpixel++;
				if (currentpixel > pixelcount)
				{
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
		else //chunk>=128
		{
			chunkheader -= 127;
			in.read((char*)colorbuffer.rgba, bpp);
			if (!in.good())
			{
				std::cerr << "an error occured while reading the header\n";
				return false;
			}
			for (int i = 0; i < chunkheader; i++)
			{
				for (int t = 0; t < bpp; t++)
					buffer[currentbyte++] = colorbuffer.rgba[t];
				currentpixel++;
				if (currentpixel > pixelcount)
				{
					std::cerr << "Too many pixels read\n";
					return false;
				}
			}
		}
	} while (currentpixel < pixelcount);
	return true;
}

bool Image::unload_rle_data(std::ofstream& out)	//RLE�㷨ѹ��ͼ�����
{
	const unsigned char max_chunk_length = 128;
	unsigned long npixels = width * height;
	unsigned long curpix = 0;
	while (curpix < npixels)
	{
		unsigned long chunkstart = curpix * bpp;
		unsigned long curbyte = curpix * bpp;
		unsigned char run_length = 1;
		bool raw = true;
		while (curpix + run_length < npixels && run_length < max_chunk_length)
		{
			bool succ_eq = true;
			for (int t = 0; succ_eq && t < bpp; t++)	//�ж�������������������ͬ����ѹ��
			{
				succ_eq = (buffer[curbyte + t] == buffer[curbyte + t + bpp]);
			}
			curbyte += bpp;	//������һ������
			if (1 == run_length)
			{
				raw = !succ_eq;
			}
			if (raw && succ_eq)	
			{
				run_length--;
				break;
			}
			if (!raw && !succ_eq)	//����ͬ���������ǰ�������
			{
				break;
			}
			run_length++;	//��ͬ��������+1
		}
		curpix += run_length;
		out.put(raw ? run_length - 1 : run_length + 127);
		if (!out.good())
		{
			std::cerr << "can't dump the tga file\n";
			return false;
		}
		out.write((char*)(buffer + chunkstart), (raw ? run_length * bpp : bpp));
		if (!out.good())
		{
			std::cerr << "can't dump the tga file\n";
			return false;
		}
	}
	return true;
}

bool Image::read_tga_file(const char* filename)
{
	if (buffer) delete[] buffer;	// a:����buffer�Ͷ�ȡfile��buffer���ܲ����ݣ���Ҫ��������ռ�
	buffer = NULL;
	std::ifstream in;
	in.open(filename, std::ios::binary);
	if (!in.is_open())
	{
		std::cerr << "can't open file " << filename << "\n";
		in.close();
		return false;
	}
	TGA_Attr header;
	in.read((char*)&header, sizeof(header));	//��ȡtgaͼ��header��Ϣ
	if (!in.good())
	{
		in.close();
		std::cerr << "an error occured while reading the header\n";
		return false;
	}
	width = header.width;
	height = header.height;
	bpp = header.bitsperpixel >> 3;	//����3λ��������8����bits per pixel ת���� bytes per pixel
	if (width <= 0 || height <= 0 || (bpp != GRAYSCALE && bpp != RGB && bpp != RGBA))
	{
		in.close();
		std::cerr << "bad bpp (or width/height) value\n";
		return false;
	}
	unsigned long nbytes = bpp * width * height;
	buffer = new unsigned char[nbytes];
	if (3 == header.datatypecode || 2 == header.datatypecode)	//δѹ������ʻ�ڰ�ͼ��
	{
		in.read((char*)buffer, nbytes);
		if (!in.good())
		{
			in.close();
			std::cerr << "an error occured while reading the buffer\n";
			return false;
		}
	}
	else if (10 == header.datatypecode || 11 == header.datatypecode)	//RLEѹ������ʻ�ڰ�ͼ��
	{
		if (!load_rle_data(in))		//��ȡRLE����
		{
			in.close();
			std::cerr << "an error occured while reading the data\n";
			return false;
		}
	}
	else
	{
		in.close();
		std::cerr << "unknown file format " << (int)header.datatypecode << "\n";
		return false;
	}
	if (!(header.imagedescriptor & 0x20))	//ԭ��������
	{
		flip_vertically();
	}
	if (header.imagedescriptor & 0x10)		//ԭ�����
	{
		flip_horizontally();
	}
	in.close();
	return true;
}

bool Image::write_tga_file(const char* filename, bool rle)	//дtga�ļ��������Ƿ�ѹ��,Ĭ��ѹ��
{
	unsigned char developer_area_ref[4] = { 0, 0, 0, 0 };	//tga��ʽ
	unsigned char extension_area_ref[4] = { 0, 0, 0, 0 };
	unsigned char footer[18] = "TRUEVISION-XFILE.";
	//unsigned char footer[18] = { 'T','R','U','E','V','I','S','I','O','N','-','X','F','I','L','E','.','\0' };
	std::ofstream out;
	out.open(filename, std::ios::binary);
	if (!out.is_open())
	{ 
		std::cerr << "can't open file " << filename << "\n";
		out.close();
		return false;
	}
	TGA_Attr header;
	memset((void*)&header, 0, sizeof(header));
	header.bitsperpixel = bpp << 3;
	header.width = width;
	header.height = height;
	header.datatypecode = (bpp == GRAYSCALE ? (rle ? 11 : 3) : (rle ? 10 : 2));
	header.imagedescriptor = 0x20; // ���Ͻ�ԭ����Ϣ
	out.write((char*)&header, sizeof(header));
	if (!out.good())
	{
		out.close();
		std::cerr << "can't dump the tga file\n";
		return false;
	}
	if (!rle)
	{

		out.write((char*)buffer, width * height * bpp);
		if (!out.good())
		{
			std::cerr << "can't unload raw data\n";
			out.close();
			return false;
		}
	}
	else
	{
		if (!unload_rle_data(out))	//RLEѹ���洢
		{
			out.close();
			std::cerr << "can't unload rle data\n";
			return false;
		}
	}
	out.write((char*)developer_area_ref, sizeof(developer_area_ref));
	if (!out.good())
	{
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char*)extension_area_ref, sizeof(extension_area_ref));
	if (!out.good())
	{
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.write((char*)footer, sizeof(footer));
	if (!out.good())
	{
		std::cerr << "can't dump the tga file\n";
		out.close();
		return false;
	}
	out.close();
	return true;
}

Color Image::get_color(int x, int y)
{
	if (!buffer|| x < 0 || y < 0 || x >= width || y >= height)	return Color();
	return Color(buffer + (x + y * width) * bpp, bpp);
}

bool Image::set_color(int x, int y, const Color& c)
{
	if (!buffer || x < 0 || y < 0 || x >= width || y >= height)	return false;
	memcpy(buffer + (x + y * width) * bpp, c.rgba, bpp);
	return true;
}


int Image::get_bpp()
{
	return bpp;
}

int Image::get_width()
{
	return width;
}

int Image::get_height()
{
	return height;
}

unsigned char* Image::get_buffer()
{
	return buffer;
}

bool Image::flip_horizontally()
{
	if (!buffer) return false;
	int half = width >> 1;
	for (int i = 0; i < half; i++)
	{
		for (int j = 0; j < height; j++)	//������ؽ�����ɫ
		{
			Color c1 = get_color(i, j);
			Color c2 = get_color(width - 1 - i, j);
			set_color(i, j, c2);
			set_color(width - 1 - i, j, c1);
		}
	}
	return true;
}

bool Image::flip_vertically()
{
	if (!buffer) return false;
	unsigned long bytes_per_line = width * bpp;
	unsigned char* line = new unsigned char[bytes_per_line];
	int half = height >> 1;
	for (int j = 0; j < half; j++)
	{
		unsigned long l1 = j * bytes_per_line;
		unsigned long l2 = (height - 1 - j) * bytes_per_line;
		memmove((void*)line, (void*)(buffer + l1), bytes_per_line);	//ֱ�ӽ�����������
		memmove((void*)(buffer + l1), (void*)(buffer + l2), bytes_per_line);
		memmove((void*)(buffer + l2), (void*)line, bytes_per_line);
	}
	delete[] line;
	return true;
}

void Image::clear()
{
	memset((void*)buffer, 0, width * height * bpp);
}

bool Image::scale(int w, int h)	//��
{
	if (w <= 0 || h <= 0 || !buffer) return false;
	unsigned char* tdata = new unsigned char[w * h * bpp];
	int nscanline = 0;
	int oscanline = 0;
	int erry = 0;
	unsigned long nlinebytes = w * bpp;
	unsigned long olinebytes = width * bpp;
	for (int j = 0; j < height; j++)
	{
		int errx = width - w;
		int nx = -bpp;
		int ox = -bpp;
		for (int i = 0; i < width; i++)
		{
			ox += bpp;
			errx += w;
			while (errx >= (int)width)
			{
				errx -= width;
				nx += bpp;
				memcpy(tdata + nscanline + nx, buffer + oscanline + ox, bpp);
			}
		}
		erry += h;
		oscanline += olinebytes;
		while (erry >= (int)height)
		{
			if (erry >= (int)height << 1) // it means we jump over a scanline
				memcpy(tdata + nscanline + nlinebytes, tdata + nscanline, nlinebytes);
			erry -= height;
			nscanline += nlinebytes;
		}
	}
	delete[] buffer;
	buffer = tdata;
	width = w;
	height = h;
	return true;
}


