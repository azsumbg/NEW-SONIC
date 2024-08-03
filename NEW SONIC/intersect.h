#pragma once

#ifdef INTERSECT_EXPORTS 
#define INTERSECT_API _declspec(dllexport)
#else 
#define INTERSECT_API _declspec(dllimport)
#endif

namespace intersect
{
	enum class INTERSECT_API collision 
	{
		no = 0, up = 1, down = 2, left = 3, right = 4, up_left = 5, up_right = 6,
		down_left = 7, down_right = 8
	};
	
	struct INTERSECT_API Object
	{
		float left = 0;
		float right = 0;
		float up = 0;
		float down = 0;
	};

	//***********************************

	extern INTERSECT_API collision IsThereCollision(Object& first, Object& second);

	extern INTERSECT_API Object IntersectArea(Object& first, Object& second);
}