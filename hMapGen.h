#pragma once
#include <vector>
class hMapGen
{
private:
	int w;
	int h;
	int channels;
	unsigned char* data;
	unsigned char* pnoise1;
	unsigned char* pnoise2;
	unsigned char* pnoise3;

public:
	hMapGen();
	~hMapGen();
	std::vector<int> createheightMap8bit(int w, int h, int channels);
	void writePNG(const char* filename);
};

