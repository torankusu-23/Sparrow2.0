//ͼ�εĶ�ȡ���洢����ʾ��ʽ��.tga�ļ���ʽ
//*------------------------------------------------------bpp��غܶ�ṹ,δ�����ã���ɾ
#pragma once
#include "fstream"

#pragma pack(push, 1)	//Ĭ�϶���ϵ��(8?)ѹ��ջ�������õ�ǰ����ϵ��Ϊ1���ֽ�8λ����ʾ256������
struct TGA_Attr		//TGA�ļ����� refer:https://blog.csdn.net/m0_46293129/article/details/105330102
{
	char idlength;			//ָ��ͼ����Ϣ�ֶγ��ȣ�ȡֵ��Χ0~255
	char colormaptype;		//0����ʹ����ɫ�� 1��ʹ����ɫ��
	char datatypecode;		//0��û��ͼ������ 1��δѹ������ɫ��ͼ�� 2��δѹ�������ͼ�� 3��δѹ���ĺڰ�ͼ�� 9��RLEѹ������ɫ��ͼ�� 10��RLEѹ�������ͼ�� 11��RLEѹ���ĺڰ�ͼ��
	short colormaporigin;	//��ɫ����ַ
	short colormaplength;	//��ɫ�����
	char colormapdepth;		//��ɫ����λ��
	short x_origin;			//ͼ��X������ʼλ��
	short y_origin;			//ͼ��Y������ʼλ��
	short width;			//ͼ����
	short height;			//ͼ��߶�
	char  bitsperpixel;		//ͼ��ÿ���ش洢ռ��λ��,ֵΪ8��16��24��32��
	char  imagedescriptor;	
};
#pragma pack(pop)	//Ĭ��ϵ����ջ

struct Color		//��ɫ���ݽṹ
{
	unsigned char rgba[4];	//unsigned char ռ��1���ֽڣ�0~255
	unsigned char bpp;		//bytes per pixel��ÿ������������ֽڸ������Ҷ�ͼ1��rgbͼ3��rgbaͼ4

	Color() : rgba(), bpp(1){for (int i = 0; i < 4; i++) rgba[i] = 0;}

	Color(unsigned char r, unsigned char g, unsigned char b, unsigned char a = 255) : rgba(), bpp(4)
	{
		rgba[0] = r;
		rgba[1] = g;
		rgba[2] = b;
		rgba[3] = a;
	}

	Color(const unsigned char* color, unsigned char _bpp) : rgba(), bpp(_bpp)	//�����������ɫ�ṹ
	{
		for (int i = 0; i < int(bpp); i++) rgba[i] = color[i];
		for (int i = int(bpp); i < 4; i++) rgba[i] = 0;
	}

	unsigned char& operator[](const int i) { return rgba[i]; }	//ez for use

	Color operator*(float coef)	
	{
		coef = (coef > 1.f ? 1.f : (coef < 0.f ? 0.f : coef));
		for (int i = 0; i < 3; i++) this->rgba[i] *= coef;	//alpha���Բ���
		return *this;
	}
};

class Image	//TGAͼ����
{
private:
	unsigned char* buffer;
	int width;
	int height;
	int bpp;
	bool load_rle_data(std::ifstream& in);		//��ȡrleѹ��tga�ļ�
	bool unload_rle_data(std::ofstream& out);	//���rleѹ��tga�ļ�

public:
	enum FORMAT {GRAYSCALE = 1, RGB = 3, RGBA = 4};	//

	Image();
	~Image();
	Image(int width, int height, int bpp);
	Image(const Image& image);
	Image& operator=(const Image& image);

	bool read_tga_file(const char* filename);
	bool write_tga_file(const char* filename, bool rle = true);	//? what coef
	bool flip_horizontally();	//ˮƽ��ת
	bool flip_vertically();		//��ֱ��ת
	bool scale(int width, int height);

	Color get_color(int x, int y);
	bool set_color(int x, int y, const Color& c);

	unsigned char* get_buffer();
	int get_width();
	int get_height();
	int get_bpp();
	void clear();
};