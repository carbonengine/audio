// Copyright © 2020 CCP ehf.

#include "stdafx.h"

#include "DebugUtilities.h"


namespace DebugUtilities
{
	Color GenerateDebugColor( float minRange, float maxRange )
	{
		float rand1 = std::max(GenerateRandomNumber( minRange, maxRange ), 0.3f);
		float rand2 = std::max(GenerateRandomNumber( minRange, maxRange ), 0.3f);
		float rand3 = std::max(GenerateRandomNumber( minRange, maxRange ), 0.3f);
		return Color( rand1, rand2, rand3, 1.0f );
	}

	float GenerateRandomNumber( float minRange, float maxRange )
	{
		float random = static_cast<float>(minRange + (rand() / double( RAND_MAX )) * (maxRange - minRange));
		return random;
	}
}