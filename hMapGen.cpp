#include "hMapGen.h"
#include <iostream>
#include <fstream>
#include <vector>


#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image/stb_image_write.h"

hMapGen::hMapGen()
{
}
hMapGen::~hMapGen()
{
	//stb free all shit nigger
}
std::vector<int> hMapGen::createheightMap8bit(int w, int h, int channels)
{
	std::vector<int> haits(w*h*channels);
	this->w = w;
	this->h = h;
	this->channels = channels;
	size_t size = w * h * channels;
	size_t size_row = w * channels;

	//voiko korvata vektorilla
	data = new uint8_t[size];

	//korvaa nyt hyvan saan aikana VEKTOREILLA LOL koska NEW
	pnoise1 = new uint8_t[size];
	pnoise2 = new uint8_t[size];
	pnoise3 = new uint8_t[size];


	//kaikki pikselit lapi
	for (int y = 0; y < size; y += size_row)
	{
		for (int x = 0; x < size_row; x++)
		{
			if (y == 0 || y == size - 1 || x == 0 || x == size_row - 1)
				data[y + x] = 0;
			else
				data[y + x] = (rand() % 256);
		}
	}

	int nOctaves = 4;
	float fBias = (rand() % 3000)/1000.0f;

	//perlin1
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++)
		{
			float fNoise = 0.0f;
			float fScale = 1.0f;
			float fScaleAcc = 0.0f;
			for (int o = 0; o < nOctaves; o++)
			{
				int nPitch = w >> o;
				int nSampleX1 = (x / nPitch) * nPitch;
				int nSampleY1 = (y / nPitch) * nPitch;

				int nSampleX2 = (nSampleX1 + nPitch) % w;
				int nSampleY2 = (nSampleY1 + nPitch) % w;

				float fBlendX = (float)(x - nSampleX1) / (float)(nPitch);
				float fBlendY = (float)(y - nSampleY1) / (float)(nPitch);


				float fSampleT = (1.0f - fBlendX) * ((float)data[nSampleY1 * w + nSampleX1] / 512.0) + fBlendX * (float)(data[nSampleY1 * w + nSampleX2] / 512.0);
				float fSampleB = (1.0f - fBlendX) * ((float)data[nSampleY2 * w + nSampleX1] / 512.0) + fBlendX * (float)(data[nSampleY2 * w + nSampleX2] / 512.0);

				fNoise += (fBlendY * (fSampleB - fSampleT) + fSampleT) * fScale;
				fScaleAcc += fScale;
				fScale = fScale / fBias;
			}
			pnoise1[y * (int)w + x] = (int)((fNoise / fScaleAcc) * 512.0);
		}
	}

	nOctaves = 6;
	fBias = ((rand() % 1000) / 1000.0f)+0.5f;

	//perlin2
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++)
		{
			float fNoise = 0.0f;
			float fScale = 1.0f;
			float fScaleAcc = 0.0f;
			for (int o = 0; o < nOctaves; o++)
			{
				int nPitch = w >> o;
				int nSampleX1 = (x / nPitch) * nPitch;
				int nSampleY1 = (y / nPitch) * nPitch;

				int nSampleX2 = (nSampleX1 + nPitch) % w;
				int nSampleY2 = (nSampleY1 + nPitch) % w;

				float fBlendX = (float)(x - nSampleX1) / (float)(nPitch);
				float fBlendY = (float)(y - nSampleY1) / (float)(nPitch);


				float fSampleT = (1.0f - fBlendX) * ((float)data[nSampleY1 * w + nSampleX1] / 512.0) + fBlendX * (float)(data[nSampleY1 * w + nSampleX2] / 512.0);
				float fSampleB = (1.0f - fBlendX) * ((float)data[nSampleY2 * w + nSampleX1] / 512.0) + fBlendX * (float)(data[nSampleY2 * w + nSampleX2] / 512.0);

				fNoise += (fBlendY * (fSampleB - fSampleT) + fSampleT) * fScale;
				fScaleAcc += fScale;
				fScale = fScale / fBias;
			}
			pnoise2[y * (int)w + x] = (int)((fNoise / fScaleAcc) * 512.0);
		}
	}

	nOctaves = 10;
	fBias = ((rand() % 1000) / 1000.0f) + 0.5f;

	//perlin3
	for (int x = 0; x < w; x++)
	{
		for (int y = 0; y < h; y++)
		{
			float fNoise = 0.0f;
			float fScale = 1.0f;
			float fScaleAcc = 0.0f;
			for (int o = 0; o < nOctaves; o++)
			{
				int nPitch = w >> o;
				int nSampleX1 = (x / nPitch) * nPitch;
				int nSampleY1 = (y / nPitch) * nPitch;

				int nSampleX2 = (nSampleX1 + nPitch) % w;
				int nSampleY2 = (nSampleY1 + nPitch) % w;

				float fBlendX = (float)(x - nSampleX1) / (float)(nPitch);
				float fBlendY = (float)(y - nSampleY1) / (float)(nPitch);


				float fSampleT = (1.0f - fBlendX) * ((float)data[nSampleY1 * w + nSampleX1] / 512.0) + fBlendX * (float)(data[nSampleY1 * w + nSampleX2] / 512.0);
				float fSampleB = (1.0f - fBlendX) * ((float)data[nSampleY2 * w + nSampleX1] / 512.0) + fBlendX * (float)(data[nSampleY2 * w + nSampleX2] / 512.0);

				fNoise += (fBlendY * (fSampleB - fSampleT) + fSampleT) * fScale;
				fScaleAcc += fScale;
				fScale = fScale / fBias;
			}
			pnoise3[y * (int)w + x] = (int)((fNoise / fScaleAcc) * 512.0);
		}
	}

	//combine 3
	int aihio = 0;
	for (int i = 0; i < size; i++)
	{
		aihio = (int)(((float)pnoise1[i] + (float)pnoise2[i] / 2.0 + (float)pnoise3[i] / 10.0) / 3);
		if (aihio > 255)
			aihio = 255;
		else;
		data[i] = aihio;
	}
	//data = pnoise3;

	//map data
	int pienin = 512;
	int suurin = 0;
	for (int i = 0; i < size; i++)
	{
		if ((int)data[i] < pienin)
			pienin = (int)data[i];
		else if ((int)data[i] > suurin)
			suurin = (int)data[i];
	}
	for (int i = 0; i < size; i++)
	{
		data[i] = (int)(0.0 + (255.0 - 0.0) * (((float)data[i] - (float)pienin) / ((float)suurin - (float)pienin)));
		haits[i] = data[i];
	}

	/*
	for (int y = 4; y < h - 4; y++)
	{
		for (int x = 4; x < w - 4; x++)
		{
			int c_ind = y * w + x;
			float c_col = haits[c_ind];

			c_col += haits[c_ind - 4 * w - 4];
			c_col += haits[c_ind - 4 * w - 3];
			c_col += haits[c_ind - 4 * w - 2];
			c_col += haits[c_ind - 4 * w - 1];
			c_col += haits[c_ind - 4 * w];
			c_col += haits[c_ind - 4 * w + 1];
			c_col += haits[c_ind - 4 * w + 2];
			c_col += haits[c_ind - 4 * w + 3];
			c_col += haits[c_ind - 4 * w + 4];

			c_col += haits[c_ind - 3 * w - 4];
			c_col += haits[c_ind - 3 * w - 3];
			c_col += haits[c_ind - 3 * w - 2];
			c_col += haits[c_ind - 3 * w - 1];
			c_col += haits[c_ind - 3 * w];
			c_col += haits[c_ind - 3 * w + 1];
			c_col += haits[c_ind - 3 * w + 2];
			c_col += haits[c_ind - 3 * w + 3];
			c_col += haits[c_ind - 3 * w + 4];

			c_col += haits[c_ind - 2 * w - 4];
			c_col += haits[c_ind - 2 * w - 3];
			c_col += haits[c_ind - 2 * w - 2];
			c_col += haits[c_ind - 2 * w - 1];
			c_col += haits[c_ind - 2 * w];
			c_col += haits[c_ind - 2 * w + 1];
			c_col += haits[c_ind - 2 * w + 2];
			c_col += haits[c_ind - 2 * w + 3];
			c_col += haits[c_ind - 2 * w + 4];

			c_col += haits[c_ind - w - 4];
			c_col += haits[c_ind - w - 3];
			c_col += haits[c_ind - w - 2];
			c_col += haits[c_ind - w - 1];
			c_col += haits[c_ind - w];
			c_col += haits[c_ind - w + 1];
			c_col += haits[c_ind - w + 2];
			c_col += haits[c_ind - w + 3];
			c_col += haits[c_ind - w + 4];

			c_col += haits[c_ind - 4];
			c_col += haits[c_ind - 3];
			c_col += haits[c_ind - 2];
			c_col += haits[c_ind - 1];
			c_col += haits[c_ind + 1];
			c_col += haits[c_ind + 2];
			c_col += haits[c_ind + 3];
			c_col += haits[c_ind + 4];

			c_col += haits[c_ind + 4 * w - 4];
			c_col += haits[c_ind + 4 * w - 3];
			c_col += haits[c_ind + 4 * w - 2];
			c_col += haits[c_ind + 4 * w - 1];
			c_col += haits[c_ind + 4 * w];
			c_col += haits[c_ind + 4 * w + 1];
			c_col += haits[c_ind + 4 * w + 2];
			c_col += haits[c_ind + 4 * w + 3];
			c_col += haits[c_ind + 4 * w + 4];

			c_col += haits[c_ind + 3 * w - 4];
			c_col += haits[c_ind + 3 * w - 3];
			c_col += haits[c_ind + 3 * w - 2];
			c_col += haits[c_ind + 3 * w - 1];
			c_col += haits[c_ind + 3 * w];
			c_col += haits[c_ind + 3 * w + 1];
			c_col += haits[c_ind + 3 * w + 2];
			c_col += haits[c_ind + 3 * w + 3];
			c_col += haits[c_ind + 3 * w + 4];

			c_col += haits[c_ind + 2 * w - 4];
			c_col += haits[c_ind + 2 * w - 3];
			c_col += haits[c_ind + 2 * w - 2];
			c_col += haits[c_ind + 2 * w - 1];
			c_col += haits[c_ind + 2 * w];
			c_col += haits[c_ind + 2 * w + 1];
			c_col += haits[c_ind + 2 * w + 2];
			c_col += haits[c_ind + 2 * w + 3];
			c_col += haits[c_ind + 2 * w + 4];

			c_col += haits[c_ind + w - 4];
			c_col += haits[c_ind + w - 3];
			c_col += haits[c_ind + w - 2];
			c_col += haits[c_ind + w - 1];
			c_col += haits[c_ind + w];
			c_col += haits[c_ind + w + 1];
			c_col += haits[c_ind + w + 2];
			c_col += haits[c_ind + w + 3];
			c_col += haits[c_ind + w + 4];

			haits[c_ind] = (int)(c_col / 81.0);
		}
	}
	*/
	for (int i = 0; i < 5; i++)
	{
		for (int y = 2; y < h - 2; y++)
		{
			for (int x = 2; x < w - 2; x++)
			{
				int c_ind = y * w + x;
				float c_col = data[c_ind];

				c_col += data[c_ind - 2 * w - 2];
				c_col += data[c_ind - 2 * w - 1];
				c_col += data[c_ind - 2 * w];
				c_col += data[c_ind - 2 * w + 1];
				c_col += data[c_ind - 2 * w + 2];

				c_col += data[c_ind - w - 2];
				c_col += data[c_ind - w - 1];
				c_col += data[c_ind - w];
				c_col += data[c_ind - w + 1];
				c_col += data[c_ind - w + 2];

				c_col += data[c_ind - 2];
				c_col += data[c_ind - 1];
				c_col += data[c_ind + 1];
				c_col += data[c_ind + 2];

				c_col += data[c_ind + 2 * w - 2];
				c_col += data[c_ind + 2 * w - 1];
				c_col += data[c_ind + 2 * w];
				c_col += data[c_ind + 2 * w + 1];
				c_col += data[c_ind + 2 * w + 2];

				c_col += data[c_ind + w - 2];
				c_col += data[c_ind + w - 1];
				c_col += data[c_ind + w];
				c_col += data[c_ind + w + 1];
				c_col += data[c_ind + w + 2];

				data[c_ind] = (int)(c_col / 25.0);
			}
		}
	}
	/*
	for (int i = 0; i < 2; i++)
	{
		for (int y = 4; y < h - 4; y++)
		{
			for (int x = 4; x < w - 4; x++)
			{
				int c_ind = y * w + x;
				float c_col = data[c_ind];

				c_col += data[c_ind - 4 * w - 4];
				c_col += data[c_ind - 4 * w - 3];
				c_col += data[c_ind - 4 * w - 2];
				c_col += data[c_ind - 4 * w - 1];
				c_col += data[c_ind - 4 * w];
				c_col += data[c_ind - 4 * w + 1];
				c_col += data[c_ind - 4 * w + 2];
				c_col += data[c_ind - 4 * w + 3];
				c_col += data[c_ind - 4 * w + 4];

				c_col += data[c_ind - 3 * w - 4];
				c_col += data[c_ind - 3 * w - 3];
				c_col += data[c_ind - 3 * w - 2];
				c_col += data[c_ind - 3 * w - 1];
				c_col += data[c_ind - 3 * w];
				c_col += data[c_ind - 3 * w + 1];
				c_col += data[c_ind - 3 * w + 2];
				c_col += data[c_ind - 3 * w + 3];
				c_col += data[c_ind - 3 * w + 4];

				c_col += data[c_ind - 2 * w - 4];
				c_col += data[c_ind - 2 * w - 3];
				c_col += data[c_ind - 2 * w - 2];
				c_col += data[c_ind - 2 * w - 1];
				c_col += data[c_ind - 2 * w];
				c_col += data[c_ind - 2 * w + 1];
				c_col += data[c_ind - 2 * w + 2];
				c_col += data[c_ind - 2 * w + 3];
				c_col += data[c_ind - 2 * w + 4];

				c_col += data[c_ind - w - 4];
				c_col += data[c_ind - w - 3];
				c_col += data[c_ind - w - 2];
				c_col += data[c_ind - w - 1];
				c_col += data[c_ind - w];
				c_col += data[c_ind - w + 1];
				c_col += data[c_ind - w + 2];
				c_col += data[c_ind - w + 3];
				c_col += data[c_ind - w + 4];

				c_col += data[c_ind - 4];
				c_col += data[c_ind - 3];
				c_col += data[c_ind - 2];
				c_col += data[c_ind - 1];
				c_col += data[c_ind + 1];
				c_col += data[c_ind + 2];
				c_col += data[c_ind + 3];
				c_col += data[c_ind + 4];

				c_col += data[c_ind + 4 * w - 4];
				c_col += data[c_ind + 4 * w - 3];
				c_col += data[c_ind + 4 * w - 2];
				c_col += data[c_ind + 4 * w - 1];
				c_col += data[c_ind + 4 * w];
				c_col += data[c_ind + 4 * w + 1];
				c_col += data[c_ind + 4 * w + 2];
				c_col += data[c_ind + 4 * w + 3];
				c_col += data[c_ind + 4 * w + 4];

				c_col += data[c_ind + 3 * w - 4];
				c_col += data[c_ind + 3 * w - 3];
				c_col += data[c_ind + 3 * w - 2];
				c_col += data[c_ind + 3 * w - 1];
				c_col += data[c_ind + 3 * w];
				c_col += data[c_ind + 3 * w + 1];
				c_col += data[c_ind + 3 * w + 2];
				c_col += data[c_ind + 3 * w + 3];
				c_col += data[c_ind + 3 * w + 4];

				c_col += data[c_ind + 2 * w - 4];
				c_col += data[c_ind + 2 * w - 3];
				c_col += data[c_ind + 2 * w - 2];
				c_col += data[c_ind + 2 * w - 1];
				c_col += data[c_ind + 2 * w];
				c_col += data[c_ind + 2 * w + 1];
				c_col += data[c_ind + 2 * w + 2];
				c_col += data[c_ind + 2 * w + 3];
				c_col += data[c_ind + 2 * w + 4];

				c_col += data[c_ind + w - 4];
				c_col += data[c_ind + w - 3];
				c_col += data[c_ind + w - 2];
				c_col += data[c_ind + w - 1];
				c_col += data[c_ind + w];
				c_col += data[c_ind + w + 1];
				c_col += data[c_ind + w + 2];
				c_col += data[c_ind + w + 3];
				c_col += data[c_ind + w + 4];

				data[c_ind] = (int)(c_col / 81.0);
			}
		}
	}
	
	*/
	return haits;
}
void hMapGen::writePNG(const char* filename)
{
	size_t size = w * h * channels;
	stbi_write_png(filename, w, h, channels, data, w*channels);
	delete[] data;
	delete[] pnoise1;
	delete[] pnoise2;
	delete[] pnoise3;
}