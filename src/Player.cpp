#include <SDL_rect.h>
#include "Main.h"
#include "IDSheet.h"
#include "Inputor.h"
#include "TextureLoader.h"
#include "SoundLoader.h"
#include "World.h"
#include "Player.h"
#include "Dice.h"
#include "Camera.h"
#include "XmlParser.h"

#define PLAYERACCERLATION 0.7f
#define AIRACCELERATION 0.3f
#define MIDAIRACCERLATION 0.05f
#define DEFAULTMAXSPEED 5.0f
#define JUMPSPEED -10.f
#define LADDERSPEED 2.0f
#define LADDERJUMPSPEED -12.f
#define SEARCH_LADDER 1
#define MIDAIR 1
#define MIDAIR_LADDER 2
#define MIDAIR_LADDER_JUMP 3

const int Player::LvUpBonus[TOTALATTRIBUTES][MAXLEVEL]
{
	{ 10,2,2,2,3,6,3,3,3,6 },
	{ 1,2,1,2,1,4,1,3,1,4 },
	{ 100,20,20,20,20,30,20,20,20,30 },
	{ 30,5,5,5,5,10,5,5,5,10 },
	{ 1,1,2,1,1,3,1,1,2,2 }
};

Player::Player(int id, int x, int y)
{
	name = "Who am I?";
	position.set(x, y);
	uniqueID = id;
	//display_pos = position;
	onLadder = false;
	movingUp = false;
	movingDown = false;
	characterPanel = new CharacterPanel();
	skillPanel = new SkillPanel();
	inventory = new Inventory();
	characterPanel->active = false;
	skillPanel->active = false;
	inventory->active = false;
	dialog = NULL;
	selectingItem = NULL;
	rightHand_equ = NULL;
	leftHand_equ = NULL;
	helmet_equ = NULL;
	frameTimer = MyTimer(true);
	keyCooldown = MyTimer(true);
	mouseCooldown = MyTimer(true);
	Load();
}

Player::~Player()
{
	delete characterPanel;
	delete skillPanel;
	delete inventory;
}

void Player::Load()
{
	width = 46;
	height = 70;
	numFrames = 5;
	acceleration.y = GRAVITY;
	footstepTick = 0;
	attackingFrameTick = 0;
	//rpg properties
	attackTick = 0;
	lifeRegenTick = 0;
	manaRegenTick = 0;
	invulnerableTick = 0;

	level = XmlParser::Inst()->level;
	exp = XmlParser::Inst()->xp;
	expToNextLevel = ExpSheet(level);
	life = XmlParser::Inst()->life;
	mana = XmlParser::Inst()->mana;

	baseMaxSpeed = DEFAULTMAXSPEED;
	baseMaxLife = LvUpBonus[HP][0];
	baseMaxMana = LvUpBonus[MP][0];
	baseATT = LvUpBonus[ATK][0];
	baseDEF = LvUpBonus[DEF][0];
	critChance = 10;

	attackInterval = 45;
	baseLifeRegenInterval = 600;
	baseLifeRegenAmount = 1;
	baseManaRegenInterval = 300;
	baseManaRegenAmount = 1;
	baseInvulnerableInterval = 60;
}

void Player::update()
{
	IsDead();
	IsLevelingup();
	UpdateAttributes();
	HandleInput();
	HandleMovement();
	HandleDisplay();
	///update character panel
	if (characterPanel->active)
		characterPanel->update();
	///update skill panel
	if (skillPanel->active)
		skillPanel->update();
	///update inventory and selecting item
	if (inventory->active)
		inventory->update();
	if (selectingItem != NULL)
	{
		if (!inventory->active)
		{
			selectingItem->beingPicked = false;
			if (selectingItem->slotType == ItemslotTypeSplited)
				inventory->addItem(selectingItem->getUniqueID(), selectingItem->stack);
			selectingItem = NULL;
		}
		else
		{
			selectingItem->setPosition(Inputor::Inst()->getMouseRelativePosition());
			selectingItem->update();
		}
	}
}

void Player::draw()
{
	///draw player
	TextureLoader::Inst()->drawFrame(uniqueID, position.x - Camera::Inst()->getPosition().x + Main::Inst()->getRenderWidth() / 2, position.y - Camera::Inst()->getPosition().y + Main::Inst()->getRenderHeight() / 2, width, height, currentRow, currentFrame, angle, alpha);

	///draw buffs
	int len = buffs.size();
	for (int i = 0; i < len; i++)
		if (buffs[i]->active)
			buffs[i]->draw();
	///draw dialog
	if (dialog != NULL)
		dialog->draw();
	///draw character panel
	if (characterPanel->active)
		characterPanel->draw();
	///draw skill panel and skill hotkeys
	if (skillPanel->active)
		skillPanel->draw();
	skillPanel->outsideDrawHotkeys();
	///draw inventory
	if (inventory->active)
		inventory->draw();
	///draw selecting item
	if (selectingItem != NULL)
		selectingItem->draw();
}

void Player::IsDead()
{
	if (life <= 0)
		Main::Inst()->changeMenu(MenuGameOver);
}

void Player::IsLevelingup()
{
	while (exp >= expToNextLevel)
	{
		SoundLoader::Inst()->playSound(LevelupSound);
		LevelUpBonus();
		level++;
		exp -= expToNextLevel;
		expToNextLevel = ExpSheet(level);
	}
}

void Player::LevelUpBonus()
{
	baseATT += LvUpBonus[ATK][level];
	baseDEF += LvUpBonus[DEF][level];
	baseMaxLife += LvUpBonus[HP][level];
	baseMaxMana += LvUpBonus[MP][level];
	skillPanel->skillPoints += LvUpBonus[SP][level];

	maxlife += LvUpBonus[HP][level];
	maxmana += LvUpBonus[MP][level];
	life = maxlife;
	mana = maxmana;
}

void Player::UpdateAttributes()
{
	///refresh attributes
	if (rightHand_equ == NULL || !rightHand_equ->active)
	{
		minATT = baseATT;
		maxATT = baseATT + 5;
	}
	else
	{
		minATT = baseATT + rightHand_equ->minATT;
		maxATT = baseATT + 5 + rightHand_equ->maxATT;
	}
	maxSpeed = baseMaxSpeed;
	defense = baseDEF;
	maxlife = baseMaxLife;
	maxmana = baseMaxMana;
	lifeRegenInterval = baseLifeRegenInterval;
	lifeRegenAmount = baseLifeRegenAmount;
	manaRegenInterval = baseManaRegenInterval;
	manaRegenAmount = baseManaRegenAmount;
	invulnerableInterval = baseInvulnerableInterval;
	///apply buff effects
	int len = buffs.size();
	for (int i = 0; i < len; i++)
		if(buffs[i]->active)
			buffs[i]->update();

	///skill cooldown
	skillPanel->outsideUpdateSkills();
	///attack speed
	if (attackTick)
	{
		attackTick++;
		if (attackTick == attackInterval)
			attackTick = 0;
	}
	///life regen speed
	if (lifeRegenTick >= 0)
	{
		lifeRegenTick++;
		if (lifeRegenTick >= lifeRegenInterval)
		{
			lifeRegenTick = 0;
			if (life < maxlife)
				life += lifeRegenAmount;
		}
	}
	///mana regen speed
	if (manaRegenTick >= 0)
	{
		manaRegenTick++;
		if (manaRegenTick >= manaRegenInterval)
		{
			manaRegenTick = 0;
			if (mana < maxmana)
				mana += manaRegenAmount;
		}
	}
	///invulnerable tick
	if (invulnerableTick)
	{
		invulnerableTick++;
		if (invulnerableTick == invulnerableInterval)
			invulnerableTick = 0;
		if (invulnerableTick % 5 == 0)
		{
			if (alpha == 255)
				alpha = 50;
			else
				alpha = 255;
		}
	}
	else
		alpha = 255;
}

void Player::HandleInput()
{
	if (dialog != NULL)
	{
		movingRight = false;
		movingLeft = false;
		if (Inputor::Inst()->getMouseButtonState(MOUSE_LEFT) && mouseCooldown.getTicks() > CLICKCOOLDOWN)
		{
			Dialog* temp = dialog->next;
			delete dialog;
			dialog = temp;

			mouseCooldown.start();
		}
		else
			return;
	}

	if (Inputor::Inst()->isKeyDown(SDL_SCANCODE_ESCAPE) && keyCooldown.getTicks() > PRESSCOOLDOWN)
	{
		if (characterPanel->active || inventory->active || skillPanel->active)
		{
			characterPanel->active = false;
			skillPanel->active = false;
			inventory->active = false;
		}
		else
		{
			Main::Inst()->changeMenu(MenuGameMain);
		}
		keyCooldown.start();
	}
	if (Inputor::Inst()->isKeyDown(XmlParser::Inst()->key_movingUp))
	{
		if (!onLadder)
			CheckInteractive();
		else
		{
			movingUp = true;
			movingDown = false;
		}
	}
	else if (Inputor::Inst()->isKeyDown(XmlParser::Inst()->key_movingDown))
	{
		if (onLadder)
		{
			movingDown = true;
			movingUp = false;
		}
		else {
			CheckInteractive(SEARCH_LADDER);
		}
	}
	else
	{
		movingUp = false;
		movingDown = false;
	}
	if (Inputor::Inst()->isKeyDown(SDL_SCANCODE_LCTRL))
	{
		CheckPickup();
	}
	if (Inputor::Inst()->isKeyDown(SDL_SCANCODE_C) && keyCooldown.getTicks() > PRESSCOOLDOWN)
	{
		characterPanel->active = !characterPanel->active;
		keyCooldown.start();
	}
	if (Inputor::Inst()->isKeyDown(SDL_SCANCODE_K) && keyCooldown.getTicks() > PRESSCOOLDOWN)
	{
		skillPanel->active = !skillPanel->active;
		keyCooldown.start();
	}
	if (Inputor::Inst()->isKeyDown(SDL_SCANCODE_I) && keyCooldown.getTicks() > PRESSCOOLDOWN)
	{
		inventory->active = !inventory->active;
		keyCooldown.start();
	}

	/// try to move the character
	if (attackingFrameTick)
	{
		movingLeft = false;
		movingRight = false;
	}
	else if (Inputor::Inst()->isKeyDown(XmlParser::Inst()->key_movingLeft) && !Inputor::Inst()->isKeyDown(XmlParser::Inst()->key_movingRight))
	{
		facingLeft = true;
		facingRight = false;
		movingLeft = true;
		movingRight = false;

		acceleration.x = -PLAYERACCERLATION;
	}
	else if (Inputor::Inst()->isKeyDown(XmlParser::Inst()->key_movingRight) && !Inputor::Inst()->isKeyDown(XmlParser::Inst()->key_movingLeft))
	{
		facingRight = true;
		facingLeft = false;
		movingRight = true;
		movingLeft = false;

		acceleration.x = -MIDAIRACCERLATION;
	}
	else
	{
		movingLeft = false;
		movingRight = false;
	}

	if (Inputor::Inst()->isKeyDown(SDL_SCANCODE_SPACE) && keyCooldown.getTicks() > PRESSCOOLDOWN)
	{
		jumped = true;
		onLadder = false;
	}

	if (skillPanel->active && skillPanel->outsideCheckMouseOver())
	{
		if (Inputor::Inst()->getMouseButtonState(MOUSE_LEFT) && skillPanel->outsideCheckMouseTitle())
		{
			Vector2D motion = Inputor::Inst()->getMouseMotionVector();
			if (motion.length() > 1.5)
				skillPanel->addPosition(Inputor::Inst()->getMouseMotionVector()); // **********motion is too sensitive
		}
	}
	else if (characterPanel->active && characterPanel->outsideCheckMouseOver())
	{
		if (Inputor::Inst()->getMouseButtonState(MOUSE_LEFT) && characterPanel->outsideCheckMouseTitle())
		{
			characterPanel->addPosition(Inputor::Inst()->getMouseMotionVector()); // **********motion is too sensitive
		}
	}
	else if (inventory->active && inventory->outsideCheckMouseOver())
	{
		if (Inputor::Inst()->getMouseButtonState(MOUSE_LEFT) && inventory->outsideCheckMouseTitle())
		{
			inventory->addPosition(Inputor::Inst()->getMouseMotionVector()); // **********motion is too sensitive
		}
	}
	else if (selectingItem != NULL && Inputor::Inst()->getMouseButtonState(MOUSE_LEFT))
	{
		Vector2D mousepos = Inputor::Inst()->getMouseDefinitePosition();
		Vector2D direction = mousepos - entityCenter;
		direction.normalize();
		direction.x *= 4;
		direction.y *= 10;
		World::Inst()->newItem(selectingItem->getUniqueID(), selectingItem->stack, entityCenter.x, entityCenter.y, direction.x, direction.y);
		selectingItem->active = false;
		selectingItem->beingPicked = false;
		if (selectingItem->slotType == ItemslotTypeSplited)
			delete selectingItem;

		selectingItem = NULL;

		mouseCooldown.start(100); // **********cooldown broken
	}
	else
	{
		if (!attackTick && Inputor::Inst()->getMouseButtonState(MOUSE_LEFT) && mouseCooldown.getTicks() > CLICKCOOLDOWN)
		{
			attacking();
		}
		if (!attackTick && mana > 0 && Inputor::Inst()->getMouseButtonState(MOUSE_RIGHT))
		{
			//right mouse function remains empty for now----------------------------------------------------
		}
	}

	if (Inputor::Inst()->isKeyDown(XmlParser::Inst()->key_skillHotkey1) && keyCooldown.getTicks() > PRESSCOOLDOWN)
		if (skillPanel->hotkeySkillIndexes[0] != -1)
			skillPanel->skills[skillPanel->hotkeySkillIndexes[0]]->castSkill();
	if (Inputor::Inst()->isKeyDown(XmlParser::Inst()->key_skillHotkey2) && keyCooldown.getTicks() > PRESSCOOLDOWN)
		if (skillPanel->hotkeySkillIndexes[1] != -1)
			skillPanel->skills[skillPanel->hotkeySkillIndexes[1]]->castSkill();
	if (Inputor::Inst()->isKeyDown(XmlParser::Inst()->key_skillHotkey3) && keyCooldown.getTicks() > PRESSCOOLDOWN)
		if (skillPanel->hotkeySkillIndexes[2] != -1)
			skillPanel->skills[skillPanel->hotkeySkillIndexes[2]]->castSkill();
}

void Player::attacking()
{
	ChangeFacingDirection();
	attackingFrameTick = 20;

	if (rightHand_equ == NULL || !rightHand_equ->active)
	{
		attackTick = 1;
		Vector2D mousepos = Inputor::Inst()->getMouseDefinitePosition();
		Vector2D direction = mousepos - entityCenter;
		direction.normalize();
		direction.x *= 10;
		direction.y *= 5;
		Vector2D randpos(entityCenter.x + Dice::Inst()->randInverse(15), entityCenter.y + Dice::Inst()->randInverse(15));
		World::Inst()->newProjectile(SubiDartProjectile, randpos, direction.x, direction.y, minATT, maxATT, critChance, true);
		SoundLoader::Inst()->playSound(AttackSound);
	}
	else if (rightHand_equ->getUniqueID() == IronDartItem)
	{
		attackTick = 1;
		Vector2D mousepos = Inputor::Inst()->getMouseDefinitePosition();
		Vector2D direction = mousepos - entityCenter;
		direction.normalize();
		direction.x *= 10;
		direction.y *= 5;
		Vector2D randpos(entityCenter.x + Dice::Inst()->randInverse(15), entityCenter.y + Dice::Inst()->randInverse(15));
		World::Inst()->newProjectile(IronDartProjectile, randpos, direction.x, direction.y, minATT, maxATT, critChance, true);
		SoundLoader::Inst()->playSound(AttackSound);
	}
	else if (rightHand_equ->getUniqueID() == CrystalDartItem)
	{
		attackTick = 1;
		Vector2D mousepos = Inputor::Inst()->getMouseDefinitePosition();
		Vector2D direction = mousepos - entityCenter;
		direction.normalize();
		direction.x *= 10;
		direction.y *= 5;
		Vector2D randpos(entityCenter.x + Dice::Inst()->randInverse(15), entityCenter.y + Dice::Inst()->randInverse(15));
		World::Inst()->newProjectile(CrystalDartProjectile, randpos, direction.x, direction.y, minATT, maxATT, critChance, true);
		SoundLoader::Inst()->playSound(AttackSound);
	}
	else if (rightHand_equ->getUniqueID() == MokbiDartItem)
	{
		attackTick = 1;
		Vector2D mousepos = Inputor::Inst()->getMouseDefinitePosition();
		Vector2D direction = mousepos - entityCenter;
		direction.normalize();
		direction.x *= 10;
		direction.y *= 5;
		Vector2D randpos(entityCenter.x + Dice::Inst()->randInverse(15), entityCenter.y + Dice::Inst()->randInverse(15));
		World::Inst()->newProjectile(MokbiDartProjectile, randpos, direction.x, direction.y, minATT, maxATT, critChance, true);
		SoundLoader::Inst()->playSound(AttackSound);
	}
	else if (rightHand_equ->getUniqueID() == SteelyThrowingKnivesItem)
	{
		attackTick = 1;
		Vector2D mousepos = Inputor::Inst()->getMouseDefinitePosition();
		Vector2D direction = mousepos - entityCenter;
		direction.normalize();
		direction.x *= 10;
		direction.y *= 5;
		Vector2D randpos(entityCenter.x + Dice::Inst()->randInverse(15), entityCenter.y + Dice::Inst()->randInverse(15));
		World::Inst()->newProjectile(SteelyThrowingKnivesProjectile, randpos, direction.x, direction.y, minATT, maxATT, critChance, true);
		SoundLoader::Inst()->playSound(AttackSound);
	}
}

void Player::HandlePlayerPhysics()
{
	/*
		water judge
	*/
	//	int status = CheckCollision_tileX( );

}

void Player::HandleMovement()
{
	if (onLadder)
	{
		velocity.x = 0;
		acceleration.x = 0;
		currentRow = 2;
		if (movingUp)
		{
			velocity.y = -LADDERSPEED;
		}
		else if (movingDown)
		{
			velocity.y = LADDERSPEED;
		}
		else
		{
			velocity.y = 0;
		}
		float newposition_y = position.y + velocity.y;
		if (CheckCollision_tileY(newposition_y)) {
			position.y = newposition_y;
			onLadder = false;
			midair = MIDAIR;

		}
		else {
			position.y += velocity.y;
			entityCenter.set(position.x + width / 2, position.y + height / 2);
			return;
		}
	}
	////calculate acc and direction
	if (!midair)
	{
		if (movingLeft)
		{
			acceleration.x = -PLAYERACCERLATION;
			if (velocity.x > 0) //turn around from right
				velocity.x = 0;
		}
		else if (movingRight)
		{
			acceleration.x = PLAYERACCERLATION;
			if (velocity.x < 0) //turn around from left
				velocity.x = 0;
		}
		else
		{
			velocity.x = 0;
			acceleration.x = 0;
		}
	}
	else
	{
		if (movingLeft)
		{
			acceleration.x = -AIRACCELERATION;
		}
		else if (movingRight)
		{
			acceleration.x = AIRACCELERATION;
		}
	}

	if (jumped && !midair)
	{
		midair = MIDAIR;
		velocity.y = JUMPSPEED;
	}
	if (jumped && (midair == MIDAIR_LADDER))
	{
		onLadder = false;
		midair = MIDAIR_LADDER_JUMP;
		velocity.y = LADDERJUMPSPEED;
	}
	if (midair)
	{
		acceleration.y = GRAVITY;
		//acceleration.x = -0.1f;
	}

	//calculate speed
	Entity::update();

	//calculate position
	Vector2D newposition = position + velocity;

	if (midair == MIDAIR_LADDER_JUMP || !CheckCollision_tileX(newposition.x)) {
		position.x = newposition.x;
	}
	if (!CheckCollision_tileY(newposition.y)) {
		position.y = newposition.y;
	}

	CheckCollision_hostile(newposition);
}

void Player::HandleDisplay()
{
	///calculate frame status
	if (!onLadder)
	{
		if (attackingFrameTick)
		{
			currentFrame = 0;
			attackingFrameTick--;
			if (facingLeft)
				currentRow = 5;
			else
				currentRow = 6;
		}
		else if (!jumped)
		{
			if (facingLeft)
				currentRow = 0;
			else
				currentRow = 1;
			if (movingLeft || movingRight || movingUp || movingDown)
			{
				if(frameTimer.getTicks() % 20 == 0)
					switch (footstepTick++ % 3)
					{
					case 0:
						SoundLoader::Inst()->playSound(WalkOnSnow1);
						break;
					case 1:
						SoundLoader::Inst()->playSound(WalkOnSnow2);
						break;
					case 2:
						SoundLoader::Inst()->playSound(WalkOnSnow3);
						break;
					case 3:
						SoundLoader::Inst()->playSound(WalkOnSnow4);
						break;
					}
				currentFrame = int(((frameTimer.getTicks() / animatedSpeed) % (numFrames - 1))) + 1;
			}
			else
				currentFrame = 0;
		}
		else
		{
			if (facingLeft)
				currentRow = 3;
			else
				currentRow = 4;
			currentFrame = 0;
		}
	}
	else
	{
		if(movingUp || movingDown)
			currentFrame = int(((frameTimer.getTicks() / animatedSpeed) % (numFrames - 1))) + 1;
		else
			currentFrame = 0;
	}
}

bool Player::CheckCollision_tileX(float& x)
{
	//touch world border
	if (x < 0)
	{
		position.x = 0;
		return true;
	}
	if (x + width > World::Inst()->getWorldWidth())
	{
		position.x = World::Inst()->getWorldWidth() - width;
		return true;
	}

	//touch tile or enemy
	vector<Tile*>& objects = World::Inst()->getLayer_tile();
	int i;
	int len = objects.size();
	for (i = 0; i < len; i++)
	{
		if (objects[i]->type() != TypeTile)
			continue;

		float& objectLeft = objects[i]->position.x;
		float objectRight = objectLeft + objects[i]->width;
		float& objectTop = objects[i]->position.y;
		float objectBottom = objectTop + objects[i]->height;


		if (!(position.y >= objectBottom || position.y + height <= objectTop))
		{
			if (x + width <= objectLeft || x >= objectRight)
				continue;
			else if (x + width >= objectLeft && position.x + width <= objectLeft)
			{
				position.x = objectLeft - width;
				velocity.x = 0;
				acceleration.x = 0;
				return true;
			}
			else if (x <= objectRight && position.x >= objectRight)
			{
				position.x = objectRight;
				velocity.x = 0;
				acceleration.x = 0;
				return true;
			}
		}
	}
	//cout << position.x << endl;
	return false;
}

bool Player::CheckCollision_tileY(float& y)
{
	if (y < 0)
	{
		position.y = 0;
		return true;
	}
	if (y + height > World::Inst()->getWorldHeight())
	{
		position.y = World::Inst()->getWorldHeight() - height;
		HitGround();
		return true;
	}

	//touch tile or enemy
	vector<Tile*>& objects = World::Inst()->getLayer_tile();
	int i;
	int len = objects.size();
	for (i = 0; i < len; i++)
	{
		if (objects[i]->type() != TypeTile)
			continue;

		float& objectLeft = objects[i]->position.x;
		float objectRight = objectLeft + objects[i]->width;
		float& objectTop = objects[i]->position.y;
		float objectBottom = objectTop + objects[i]->height;

		if (!(position.x + width <= objectLeft || position.x >= objectRight))
		{
			if (onLadder)
			{
				//cout << "onladder";
				if (y + height <= objectTop && position.y + height >= objectTop)
				{
					//	cout << "true";
					return true;
				}
			}
			else if (y >= objectBottom || y + height <= objectTop)
			{
				continue;
			}
			else {
				if (y + height >= objectTop && position.y + height <= objectTop)
				{
					position.y = objectTop - height;
					HitGround();
					return true;
				}
				else if (midair != MIDAIR_LADDER_JUMP && y <= objectBottom && position.y >= objectBottom) {
					position.y = objectBottom + 1;
					velocity.y = -velocity.y * 0.6;
					return true;
				}
			}
		}

	}
	if (onLadder)
		midair = MIDAIR_LADDER;
	else if (midair != MIDAIR_LADDER_JUMP)
		midair = MIDAIR;
	return false;
}

void Player::CheckCollision_hostile(Vector2D newpos)
{
	if (invulnerableTick > 0)
		return;
	int pRight = newpos.x + width;
	int pBottom = newpos.y + height;

	vector<Entity*>& entities = World::Inst()->getLayer_entity();
	int i;
	int len = entities.size();
	for (i = 0; i < len; i++)
	{
		if (entities[i]->type() != TypeHostile)
			continue;
		if (!entities[i]->active || entities[i]->friendly)
			continue;

		if (pBottom <= entities[i]->position.y)
			continue;
		if (newpos.y >= entities[i]->position.y + entities[i]->height)
			continue;
		if (pRight <= entities[i]->position.x)
			continue;
		if (newpos.x >= entities[i]->position.x + entities[i]->width)
			continue;

		//onhit
		SoundLoader::Inst()->playSound(PlayerDamageSound);
		int damage = Dice::Inst()->rand(entities[i]->minATT, entities[i]->maxATT) - defense;
		life -= damage;
		Vector2D textShift(entityCenter.x + Dice::Inst()->randInverse(20), position.y + Dice::Inst()->randInverse(20));
		World::Inst()->createText(textShift, 0, -0.1f, to_string(damage), segoeui22, COLOR_RED, 60);
		velocity.y = -10.f;
		//acceleration.y = -GRAVITY;
		if (entities[i]->position.x > position.x) {
			velocity.x = -5.f;
			acceleration.x = 0;
		}
		else {
			velocity.x = 5.f;
			acceleration.x = 0;
		}
		invulnerableTick = 1;
		return;
	}
}

void Player::HitGround()
{
	onLadder = false;
	midair = false;
	jumped = false;
	velocity.y = 0;
	acceleration.y = 0;

	if (facingLeft)
		currentRow = 0;
	else
		currentRow = 1;
}

void Player::CheckInteractive(short flag)
{
	vector<Entity*>& entities = World::Inst()->getLayer_entity();
	int i;
	int len = entities.size();
	for (i = 0; i < len; i++)
	{
		float pRight = position.x + width;
		float& oLeft = entities[i]->position.x;
		if (pRight < oLeft)
			continue;
		float& pLeft = position.x;
		float oRight = entities[i]->position.x + entities[i]->width;
		if (pLeft > oRight)
			continue;
		float pBottom = position.y + height;
		float& oTop = entities[i]->position.y;
		if (pBottom < oTop)
			continue;
		float& pTop = position.y;
		float oBottom = entities[i]->position.y + entities[i]->height;
		if (pTop > oBottom)
			continue;



		if (entities[i]->getUniqueID() == TestPortal)
		{


			if (entityCenter.x <= oRight && entityCenter.x >= oLeft && entityCenter.y >= oTop && entityCenter.y <= oBottom)
			{
				DoInteractive(entities[i]);
				return;
			}

		}
	}

	vector<Sprite*>& objects = World::Inst()->getLayer_foreground();
	len = objects.size();
	for (i = 0; i < len; i++)
	{
		if (flag == SEARCH_LADDER && objects[i]->getUniqueID() == LadderSprite) {
			Player* player = Camera::Inst()->getTarget_nonConst();
			if (player->entityCenter.x <= objects[i]->position.x + objects[i]->width &&
				player->entityCenter.x >= objects[i]->position.x &&
				player->entityCenter.y <= objects[i]->position.y &&
				player->entityCenter.y >= objects[i]->position.y - player->height / 2
				) {
				DoInteractive(objects[i]);
				position.y -= LADDERSPEED;
			}
			return;
		}
		else if (objects[i]->outsideCheckPlayerWithin()) // player in range
		{
			DoInteractive(objects[i]);
			return;
		}
	}
}
void Player::DoInteractive(Object * obj)
{
	int id = obj->getUniqueID();

	if (id == TestPortal)
	{
		SoundLoader::Inst()->playSound(PortalNoise);
		position.x = 2500;
		position.y = World::Inst()->getWorldHeight() - height;
	}
	if (id == LadderSprite)
	{
		position.x = obj->position.x + obj->width / 14;
		onLadder = true;
		jumped = false;
		midair = MIDAIR_LADDER;
		//currentRow = 3;	
		return;
	}
	if (id == MapGate)
	{
		SoundLoader::Inst()->playSound(WrapGateNoise);
		World::Inst()->changeMap(MapTest02, MAPCHANGE_RIGHT);
		return;
	}
	if (id == MapGate2)
	{
		SoundLoader::Inst()->playSound(WrapGateNoise);
		World::Inst()->changeMap(MapTest01, MAPCHANGE_LEFT);
		return;
	}
}

void Player::CheckPickup()
{
	vector<Entity*>& objects = World::Inst()->getLayer_entity();
	int i;
	int len = objects.size();
	for (i = 0; i < len; i++)
	{
		if (objects[i]->type() != TypeItem || objects[i]->dead)
			continue;

		float pBottom = position.y + height;
		if (pBottom <= objects[i]->position.y)
			continue;
		if (position.y >= objects[i]->position.y + objects[i]->height)
			continue;

		float pRight = position.x + width;
		if (pRight <= objects[i]->position.x)
			continue;
		if (position.x >= objects[i]->position.x + objects[i]->width)
			continue;

		if (inventory->addItem((Item*)objects[i]))
		{
			SoundLoader::Inst()->playSound(PickupSound);
			objects[i]->active = false;
			objects[i]->dead = true;
		}

	}
}

void Player::ChangeFacingDirection()
{
	if (Inputor::Inst()->getMouseDefinitePosition().x > position.x)
	{
		facingRight = true;
		facingLeft = false;
	}
	else
	{
		facingRight = false;
		facingLeft = true;
	}
}

void Player::addBuff(int buffID, int ATT, int duration)
{
	int i, len = buffs.size();
	for (i = 0; i < len; i++)
		if (buffs[i]->getUniqueID() == buffID)
		{
			buffs[i]->extend();
			return;
		}
	for (i = 0; i < len; i++)
		if (!buffs[i]->active)
		{
			delete buffs[i];
			buffs[i] = new Buff(buffID, ATT, duration);
			return;
		}

	buffs.push_back(new Buff(buffID, ATT, duration));
}