#pragma once

#include <SDL.h>
#include <SDL_ttf.h>
#include <iostream>
#include <vector>
#include <unordered_map>
#include "Object.h"
#include "Button.h"

using namespace std;

class Main
{
private:
	Main() {}
	~Main() {}
	Main(const Main&);
	static Main* INSTANCE;
public:
	static Main* Inst()
	{
		if (INSTANCE == 0)
		{
			INSTANCE = new Main();
			return INSTANCE;
		}

		return INSTANCE;
	}
private:
	MyTimer* keyCooldown;
	SDL_Window* window;
	SDL_Renderer* renderer;
	unordered_map<int, TTF_Font*> theFont;

	bool _running;
	
	int windowWidth;
	int windowHeight;
	int currentMenu;
	int frameTick;
	bool HandleMenuEvents();
	void UpdateMenu();
	void RenderMenu();
	void RefreshMenu();
	SDL_Scancode ChangeControlKey();
public:
	int renderWidth;
	int renderHeight;
	bool inMainMenu;
	bool inGameMenu;
	vector<Button*> menuButtons;

	SDL_Window* getWindow() const { return window; }
	SDL_Renderer* getRenderer() const { return renderer; }
	TTF_Font* getFont(int fontID) { return theFont[fontID]; }
	int getWindowWidth() const { return windowWidth; }
	int getWindowHeight() const { return windowHeight; }
	int getRenderWidth() const { return renderWidth; }
	int getRenderHeight() const { return renderHeight; }
	int getFrameTick() const { return frameTick; }
	bool isRunning() { return _running; }
	void quit() { _running = false; }

	bool initialize(const char* title, int xpos, int ypos, int width, int height);
	void prossessing();
	void changeMenu(int menuID);
	void close();
};