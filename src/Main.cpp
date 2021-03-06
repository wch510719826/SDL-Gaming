#include <SDL_image.h>
#include <SDL_ttf.h>
#include "Main.h"
#include "Inputor.h"
#include "TextureLoader.h"
#include "SoundLoader.h"
#include "IDSheet.h"
#include "World.h"
#include "Player.h"
#include "XmlParser.h"
#include "Camera.h"
#include "Dice.h"

using namespace tinyxml2;

Main* Main::INSTANCE = 0;

bool Main::initialize(const char* title, int xpos, int ypos, int width, int height)
{
	///init sdl_video and sdl_audio
	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0)
	{
		printf("SDL_ERROR initializing video and audio: %s\n", SDL_GetError());
		return false;
	}
	///init sdl_mixer
	if (Mix_OpenAudio(22050, MIX_DEFAULT_FORMAT, 2, 2048) < 0)
	{
		printf("SDL_MIXER_ERROR opening audio: %s\n", Mix_GetError());
		return false;
	}
	///create window
	if(XmlParser::Inst()->fullscreen)
		window = SDL_CreateWindow(title, xpos, ypos, width, height, SDL_WINDOW_FULLSCREEN);
	else
		window = SDL_CreateWindow(title, xpos, ypos, width, height, SDL_WINDOW_SHOWN);
	if (window == NULL)
	{
		printf("SDL_ERROR creating window: %s\n", SDL_GetError());
		return false;
	}
	windowWidth = width;
	windowHeight = height; 
	///create renderer for the window
	renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
	if (renderer == NULL)
	{
		printf("SDL_ERROR creating renderer: %s\n", SDL_GetError());
		return false;
	}
	///init sdl_image
	int imgFlags = IMG_INIT_PNG;
	if (!(IMG_Init(imgFlags) & imgFlags))
	{
		printf("SDL_IMAGE_ERROR initializing IMAGE: %s\n", IMG_GetError());
		return false;
	}
	///init sdl_ttf
	if (TTF_Init() < 0)
	{
		printf("SDL_TTF_ERROR initializing ttf: %s\n", TTF_GetError());
		return false;
	}
	///create fonts
	//segoeui
	theFont[segoeui18] = TTF_OpenFont(segoeuiFile, 18);
	if (theFont[segoeui18] == NULL)
	{
		printf("SDL_TTF_ERROR loading .ttf: %s\n", TTF_GetError());
		return false;
	}
	theFont[segoeui22] = TTF_OpenFont(segoeuiFile, 22);
	if (theFont[segoeui22] == NULL)
	{
		printf("SDL_TTF_ERROR loading .ttf: %s\n", TTF_GetError());
		return false;
	}
	//TTF_SetFontOutline(theFont[segoeui22], 1);
	theFont[segoeui28] = TTF_OpenFont(segoeuiFile, 28);
	if (theFont[segoeui28] == NULL)
	{
		printf("SDL_TTF_ERROR loading .ttf: %s\n", TTF_GetError());
		return false;
	}
	//arial
	theFont[arial28_bold] = TTF_OpenFont(arialFile, 28);
	if (theFont[arial28_bold] == NULL)
	{
		printf("SDL_TTF_ERROR loading .ttf: %s\n", TTF_GetError());
		return false;
	}
	TTF_SetFontStyle(theFont[arial28_bold], TTF_STYLE_BOLD);
	theFont[arial48_bold] = TTF_OpenFont(arialFile, 48);
	if (theFont[arial48_bold] == NULL)
	{
		printf("SDL_TTF_ERROR loading .ttf: %s\n", TTF_GetError());
		return false;
	}
	TTF_SetFontStyle(theFont[arial48_bold], TTF_STYLE_BOLD);
	theFont[arial72_bold] = TTF_OpenFont(arialFile, 48);
	if (theFont[arial72_bold] == NULL)
	{
		printf("SDL_TTF_ERROR loading .ttf: %s\n", TTF_GetError());
		return false;
	}
	TTF_SetFontStyle(theFont[arial72_bold], TTF_STYLE_BOLD);
	///load menu res
	TextureLoader::Inst()->load(MainMenuPicFile, MainMenuPic);
	TextureLoader::Inst()->load(MainMenuBackgroundViewFile, MainMenuBackgroundView);
	TextureLoader::Inst()->load(OptionsMenuViewFile, OptionsMenuView);
	TextureLoader::Inst()->load(FullscreenCheckboxFile, FullscreenCheckbox);
	TextureLoader::Inst()->load(VolumnLButtonFile, VolumnLButton);
	TextureLoader::Inst()->load(VolumnRButtonFile, VolumnRButton);
	///load menu music and sounds
	//SoundLoader::Inst()->load(Music01File, Music01, SOUND_MUSIC);
	//SoundLoader::Inst()->playMusic(Music01, 2);
	//SoundLoader::Inst()->load(Music02File, Music02, SOUND_MUSIC);
	//SoundLoader::Inst()->playMusic(Music02, 2);
	SoundLoader::Inst()->load(MenuMouseClickFile, MenuMouseClick, SOUND_SFX);
	SoundLoader::Inst()->load(ControlKeyChangeSound1File, ControlKeyChangeSound1, SOUND_SFX);
	SoundLoader::Inst()->load(ControlKeyChangeSound2File, ControlKeyChangeSound2, SOUND_SFX);
	SoundLoader::Inst()->load(ControlKeyChangeSound3File, ControlKeyChangeSound3, SOUND_SFX);
	SoundLoader::Inst()->load(ControlKeyChangeSound4File, ControlKeyChangeSound4, SOUND_SFX);
	SoundLoader::Inst()->load(ControlKeyChangeSound5File, ControlKeyChangeSound5, SOUND_SFX);
	///actual game starts
	frameTick = 0;
	keyCooldown = new MyTimer(true);
	changeMenu(MenuMain);
	_running = true;
	inMainMenu = true;
	inGameMenu = false;
	return true;
}

void Main::prossessing()
{
	frameTick++; // 1 prossession = 1 frame
	renderWidth = windowWidth;
	renderHeight = windowHeight - UIHEIGHT;
	
	Inputor::Inst()->updating();

	SDL_RenderClear(Main::Inst()->getRenderer());

	if (!inMainMenu && !inGameMenu)
		World::Inst()->updating();
	if (!inMainMenu)
		World::Inst()->rendering();
	if (inMainMenu|| inGameMenu)
	{
		if (HandleMenuEvents())
			return;
		UpdateMenu();
		RenderMenu();
	}

	SDL_RenderPresent(renderer);
	SoundLoader::Inst()->applyVolumn();
}

bool Main::HandleMenuEvents()
{
	Player* player = Camera::Inst()->getTarget_nonConst();
	if (Inputor::Inst()->isKeyDown(SDL_SCANCODE_ESCAPE) && keyCooldown->getTicks() > PRESSCOOLDOWN)
	{
		switch (currentMenu)
		{
		case MenuOptions:
			changeMenu(MenuMain);
			return true;
			break;
		case MenuGameMain:
			inGameMenu = false;
			player->keyCooldown.start();
			return true;
			break;
		}
	}

	int i, len;
	len = menuButtons.size();
	for (i = 0; i < len; i++)
	{
		if (menuButtons[i]->outsideUpdate())
		{
			///main menu
			if (menuButtons[i]->getUniqueID() == DemoButton)
			{
				SoundLoader::Inst()->playSound(MenuMouseClick);
				inMainMenu = false;
				menuButtons.clear();
				World::Inst()->startNewGame(true);
				World::Inst()->initialize();
				return true;
			}
			if (menuButtons[i]->getUniqueID() == NewGameButton)
			{
				SoundLoader::Inst()->playSound(MenuMouseClick);
				inMainMenu = false;
				menuButtons.clear();
				World::Inst()->startNewGame(false);
				World::Inst()->initialize();
				return true;
			}
			if (menuButtons[i]->getUniqueID() == ContinueButton)
			{
				SoundLoader::Inst()->playSound(MenuMouseClick);
				inMainMenu = false;
				menuButtons.clear();
				World::Inst()->startOldGame();
				World::Inst()->initialize();
				return true;
			}
			if (menuButtons[i]->getUniqueID() == OptionButton)
			{
				SoundLoader::Inst()->playSound(MenuMouseClick);
				changeMenu(MenuOptions);
				return true;
			}
			if (menuButtons[i]->getUniqueID() == ControlSettingsButton)
			{
				SoundLoader::Inst()->playSound(MenuMouseClick);
				changeMenu(MenuControlSettings);
				return true;
			}
			if (menuButtons[i]->getUniqueID() == ExitButton)
			{
				SoundLoader::Inst()->playSound(MenuMouseClick);
				SDL_Delay(350);
				quit();
				return true;
			}
			///options menu
			if (menuButtons[i]->getUniqueID() == ResolutionListbox)
			{
				SoundLoader::Inst()->playSound(MenuMouseClick);
				menuButtons[i]->flag++;
				return false;
			}
			if (menuButtons[i]->getUniqueID() == FullscreenCheckbox)
			{
				SoundLoader::Inst()->playSound(MenuMouseClick);
				XmlParser::Inst()->fullscreen = !XmlParser::Inst()->fullscreen;
				return false;
			}
			if (menuButtons[i]->getUniqueID() == VolumnLButton)
			{
				switch (menuButtons[i]->flag)
				{
				case 0:
					XmlParser::Inst()->volumn_master--;
					if (XmlParser::Inst()->volumn_master == -1)
						XmlParser::Inst()->volumn_master = 10;
					break;
				case 1:
					XmlParser::Inst()->volumn_music--;
					if (XmlParser::Inst()->volumn_music == -1)
						XmlParser::Inst()->volumn_music = 10;
					break;
				case 2:
					XmlParser::Inst()->volumn_sfx--;
					if (XmlParser::Inst()->volumn_sfx == -1)
						XmlParser::Inst()->volumn_sfx = 10;
					break;
				}
			}
			if (menuButtons[i]->getUniqueID() == VolumnRButton)
			{
				switch (menuButtons[i]->flag)
				{
				case 0:
					XmlParser::Inst()->volumn_master++;
					if (XmlParser::Inst()->volumn_master == 11)
						XmlParser::Inst()->volumn_master = 0;
					break;
				case 1:
					XmlParser::Inst()->volumn_music++;
					if (XmlParser::Inst()->volumn_music == 11)
						XmlParser::Inst()->volumn_music = 0;
					break;
				case 2:
					XmlParser::Inst()->volumn_sfx++;
					if (XmlParser::Inst()->volumn_sfx == 11)
						XmlParser::Inst()->volumn_sfx = 0;
					break;
				}
			}
			if (menuButtons[i]->getUniqueID() == BackButton)
			{
				SoundLoader::Inst()->playSound(MenuMouseClick);
				changeMenu(MenuMain);
				return true;
			}
			///control menu
			if (menuButtons[i]->getUniqueID() == ControlMovingUpButton)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingUp = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_movingUp));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlMovingDownButton)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingDown = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_movingDown));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlMovingLeftButton)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingLeft = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_movingLeft));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlMovingRightButton)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingRight = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_movingRight));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlCharacterPanelButton)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingRight = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_openCharacter));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlInventoryButton)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingDown = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_openCharacter));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlSkillPanelButton)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingLeft = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_openSkill));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlSkillHotkey1Button)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingRight = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_skillHotkey1));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlSkillHotkey2Button)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingRight = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_skillHotkey2));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlSkillHotkey3Button)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingRight = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_skillHotkey3));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlSkillHotkey4Button)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingRight = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_skillHotkey4));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlSkillHotkey5Button)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingRight = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_skillHotkey5));
				return false;
			}
			if (menuButtons[i]->getUniqueID() == ControlSkillHotkey6Button)
			{
				SDL_Scancode key;
				menuButtons[i]->buttonText->changeText("_");
				RefreshMenu();
				key = ChangeControlKey();
				if (key != SDL_SCANCODE_ESCAPE)
					XmlParser::Inst()->key_movingRight = key;
				menuButtons[i]->buttonText->changeText(ScancodeToString(XmlParser::Inst()->key_skillHotkey6));
				return false;
			}
			///game menu
			if (menuButtons[i]->getUniqueID() == ResumeButton)
			{
				inGameMenu = false;
				player->mouseCooldown.start();
				return true;
			}
			if (menuButtons[i]->getUniqueID() == ExittoMainMenuButton)
			{
				XmlParser::Inst()->saveCharacter();
				World::Inst()->clearWorld();
				World::Inst()->getLayer_player().clear();
				inGameMenu = false;
				changeMenu(MenuMain);
				return true;
			}
			if (menuButtons[i]->getUniqueID() == ExittoDesktopButton)
			{
				XmlParser::Inst()->saveCharacter();
				quit();
				return true;
			}
		}
	}
	return false;
}

void Main::changeMenu(int menuID)
{
	keyCooldown->start();

	menuButtons.clear();
	currentMenu = menuID;
	switch (menuID)
	{
	case MenuMain:
		inMainMenu = true;
		menuButtons.push_back(new Button(MainMenuBackgroundView));
		menuButtons.push_back(new Button(DemoButton));
		menuButtons.push_back(new Button(NewGameButton));
		menuButtons.push_back(new Button(ContinueButton));
		menuButtons.push_back(new Button(ExitButton));
		menuButtons.push_back(new Button(OptionButton));
		menuButtons.push_back(new Button(ControlSettingsButton));
		break;
	case MenuOptions:
		menuButtons.push_back(new Button(OptionsMenuView));
		menuButtons.push_back(new Button(ResolutionText));
		menuButtons.push_back(new Button(ResolutionListbox));
		menuButtons.push_back(new Button(FullscreenCheckbox));
		menuButtons.push_back(new Button(FullscreenText));
		menuButtons.push_back(new Button(VolumnMasterText));
		menuButtons.push_back(new Button(VolumnLButton, 0));
		menuButtons.push_back(new Button(VolumnRButton, 0));
		menuButtons.push_back(new Button(VolumnMasterNumber));
		menuButtons.push_back(new Button(VolumnMusicText));
		menuButtons.push_back(new Button(VolumnLButton, 1));
		menuButtons.push_back(new Button(VolumnRButton, 1));
		menuButtons.push_back(new Button(VolumnMusicNumber));
		menuButtons.push_back(new Button(VolumnSfxText));
		menuButtons.push_back(new Button(VolumnLButton, 2));
		menuButtons.push_back(new Button(VolumnRButton, 2));
		menuButtons.push_back(new Button(VolumnSfxNumber));
		menuButtons.push_back(new Button(BackButton));
		break;
	case MenuGameMain:
		inGameMenu = true;
		menuButtons.push_back(new Button(GameMenuBackground));
		menuButtons.push_back(new Button(ResumeButton));
		menuButtons.push_back(new Button(ExittoMainMenuButton));
		menuButtons.push_back(new Button(ExittoDesktopButton));
		break;
	case MenuControlSettings:
		menuButtons.push_back(new Button(ControlMovingUpText));
		menuButtons.push_back(new Button(ControlMovingUpButton));
		menuButtons.push_back(new Button(ControlMovingDownText));
		menuButtons.push_back(new Button(ControlMovingDownButton));
		menuButtons.push_back(new Button(ControlMovingLeftText));
		menuButtons.push_back(new Button(ControlMovingLeftButton));
		menuButtons.push_back(new Button(ControlMovingRightText));
		menuButtons.push_back(new Button(ControlMovingRightButton));
		menuButtons.push_back(new Button(ControlCharacterPanelText));
		menuButtons.push_back(new Button(ControlCharacterPanelButton));
		menuButtons.push_back(new Button(ControlSkillPanelText));
		menuButtons.push_back(new Button(ControlSkillPanelButton));
		menuButtons.push_back(new Button(ControlInventoryText));
		menuButtons.push_back(new Button(ControlInventoryButton));
		menuButtons.push_back(new Button(ControlSkillHotkey1Text));
		menuButtons.push_back(new Button(ControlSkillHotkey1Button));
		menuButtons.push_back(new Button(ControlSkillHotkey2Text));
		menuButtons.push_back(new Button(ControlSkillHotkey2Button));
		menuButtons.push_back(new Button(ControlSkillHotkey3Text));
		menuButtons.push_back(new Button(ControlSkillHotkey3Button));
		menuButtons.push_back(new Button(ControlSkillHotkey4Text));
		menuButtons.push_back(new Button(ControlSkillHotkey4Button));
		menuButtons.push_back(new Button(ControlSkillHotkey5Text));
		menuButtons.push_back(new Button(ControlSkillHotkey5Button));
		menuButtons.push_back(new Button(ControlSkillHotkey6Text));
		menuButtons.push_back(new Button(ControlSkillHotkey6Button));

		menuButtons.push_back(new Button(BackButton));
		break;
	case MenuGameOver:
		inGameMenu = true;
		menuButtons.push_back(new Button(GameMenuBackground));
		menuButtons.push_back(new Button(ExittoMainMenuButton));
		menuButtons.push_back(new Button(ExittoDesktopButton));
		menuButtons.push_back(new Button(GameOverText));
		break;
	}
}

void Main::UpdateMenu()
{
	int i, len;
	len = menuButtons.size();
	for (i = 0; i < len; i++)
		menuButtons[i]->update();
}

void Main::RenderMenu()
{
	if(inMainMenu)
		TextureLoader::Inst()->drawEx2(MainMenuPic, 0, 0, 1920, 1080, windowWidth, windowHeight);

	int i, len;
	len = menuButtons.size();
	for (i = 0; i < len; i++)
		menuButtons[i]->draw();
}

void Main::RefreshMenu()
{
	SDL_RenderClear(Main::Inst()->getRenderer());
	RenderMenu();
	SDL_RenderPresent(renderer);
}

SDL_Scancode Main::ChangeControlKey()
{
	SDL_Event e;
	while (true)
	{
		while (SDL_PollEvent(&e))
		{
			if(e.type == SDL_KEYDOWN)
				switch (e.key.keysym.sym)
				{
				case SDLK_ESCAPE:
					switch (Dice::Inst()->rand(5))
					{
					case 0:	SoundLoader::Inst()->playSound(ControlKeyChangeSound1); break;
					case 1:	SoundLoader::Inst()->playSound(ControlKeyChangeSound2); break;
					case 2:	SoundLoader::Inst()->playSound(ControlKeyChangeSound3); break;
					case 3:	SoundLoader::Inst()->playSound(ControlKeyChangeSound4); break;
					case 4:	SoundLoader::Inst()->playSound(ControlKeyChangeSound5); break;
					}
					Inputor::Inst()->resetMouseState();
					return SDL_SCANCODE_ESCAPE;
					break;
				default:
					switch (Dice::Inst()->rand(5))
					{
					case 0:	SoundLoader::Inst()->playSound(ControlKeyChangeSound1); break;
					case 1:	SoundLoader::Inst()->playSound(ControlKeyChangeSound2); break;
					case 2:	SoundLoader::Inst()->playSound(ControlKeyChangeSound3); break;
					case 3:	SoundLoader::Inst()->playSound(ControlKeyChangeSound4); break;
					case 4:	SoundLoader::Inst()->playSound(ControlKeyChangeSound5); break;
					}
					Inputor::Inst()->resetMouseState();
					return e.key.keysym.scancode;
					break;
				}
		}
	}
	return SDL_SCANCODE_ESCAPE;
}

void Main::close()
{
	cout << "Saving settings to xml..." << endl;
	XmlParser::Inst()->save();
	cout << "Closing game..." << endl;
	SDL_DestroyWindow(window);
	SDL_DestroyRenderer(renderer);

	SDL_Quit();
} 