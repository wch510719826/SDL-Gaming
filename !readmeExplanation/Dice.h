#pragma once
#include <random>

using namespace std;

class Dice
{
private:
	static Dice* INSTANCE;
	Dice() {};
	Dice(const Dice&);
	Dice& operator=(const Dice&) {};

	random_device random_generator;
public:
	static Dice* Inst()
	{
		if (INSTANCE == 0)
		{
			INSTANCE = new Dice();
			return INSTANCE;
		}
		return INSTANCE;
	}

	int rand(int max);
};