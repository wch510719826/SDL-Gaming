#pragma once
#include <iostream>
#include "tinyxml2.h"
#include "IDSheet.h"

using namespace std;
using namespace tinyxml2;

class XmlParser
{
private:
	XmlParser();
	~XmlParser() {}
	XmlParser(const XmlParser&);
	static XmlParser* INSTANCE;
public:
	int window_x = 100;
	int window_y = 100;
	int window_w = 1024;
	int window_h = 768;
	bool fullscreen = false;
	int volumn_master = 10;
	int volumn_music = 10;
	int volumn_sfx = 10;

	int mapID = MapTest01;
	int level = 1;
	int xp = 0;
	int life = 100;
	int mana = 30;

	static XmlParser* Inst()
	{
		if (INSTANCE == 0)
		{
			INSTANCE = new XmlParser();
			return INSTANCE;
		}

		return INSTANCE;
	}

	void load();
	void save();
	void loadCharacter();
	void saveCharacter();
};