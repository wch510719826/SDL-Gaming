#include "Textbox.h"
#include "Camera.h"

Textbox::Textbox() : textTexture(NULL)
{
	maxlife = -1;
}

Textbox::Textbox(Vector2D pos, string _text, string _fontID, SDL_Color _color, int lastingTime) : textTexture(NULL)
{
	position = pos;
	text = _text;
	fontID = _fontID;
	color = _color;
	maxlife = lastingTime;
	Load();
}

Textbox::Textbox(Vector2D pos, float velocity_x, float velocity_y, string _text, string _fontID, SDL_Color _color, int lastingTime) : textTexture(NULL)
{
	position = pos;
	velocity.x = velocity_x;
	velocity.y = velocity_y;
	text = _text;
	fontID = _fontID;
	color = _color;
	maxlife = lastingTime;
	Load();
}

void Textbox::init(Vector2D* pos, string _text, string _fontID, SDL_Color _color) 
{
	position.x = pos->x;
	position.y = pos->y;
	text = _text;
	fontID = _fontID;
	color = _color;
	Load();
}

void Textbox::changeText(string& _text)
{
	text = _text;
	Load();
}

void Textbox::changeFont(string _fontID)
{
	fontID = _fontID;
	Load();
}

void Textbox::changeColor(SDL_Color _color)
{
	color = _color;
	Load();
}

void Textbox::Load()
{
	SDL_Surface* textSurface = TTF_RenderText_Blended(Main::Inst()->getFont(fontID), text.c_str(), color);
	if (textSurface == NULL)
	{
		printf("SDL_TTF_ERROR rendering text surface: %s\n", TTF_GetError());
	}
	else
	{
		//if(textTexture)
		//	SDL_DestroyTexture(textTexture);
		textTexture = SDL_CreateTextureFromSurface(Main::Inst()->getRenderer(), textSurface);

		if (textTexture == NULL)
			printf("SDL_ERROR creating texture from rendered text: %s\n", SDL_GetError());
		else
		{
			width = textSurface->w;
			height = textSurface->h;
		}

		SDL_FreeSurface(textSurface);
	}
}

void Textbox::update()
{
	if (maxlife == -1)
		return;

	VisiableCheck();

	if (maxlife != -1 && life > maxlife)
	{
		active = false;
		return;
	}
	life++;

	Entity::update();
	if(life < 60)
		position += velocity;
}

void Textbox::draw()
{
	if (active)
		drawText();
}

void Textbox::drawText(SDL_RendererFlip flip)
{
	SDL_Rect destRect;

	if (maxlife != -1)
	{
		destRect.x = position.x - Camera::Inst()->getPosition().x + Main::Inst()->getRenderWidth() / 2;
		destRect.y = position.y - Camera::Inst()->getPosition().y + Main::Inst()->getRenderHeight() / 2;
	}
	else
	{
		destRect.x = position.x;
		destRect.y = position.y;
	}
	destRect.w = width;
	destRect.h = height;

	SDL_SetTextureAlphaMod(textTexture, alpha);
	SDL_RenderCopyEx(Main::Inst()->getRenderer(), textTexture, NULL, &destRect, angle, 0, flip);
}