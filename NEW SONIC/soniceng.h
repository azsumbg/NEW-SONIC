#pragma once

#ifdef SONICENG_EXPORTS
#define SONIC_API _declspec(dllexport)
#else 
#define SONIC_API _declspec(dllimport)
#endif

constexpr float scr_width = 800.0f;
constexpr float scr_height = 750.0f;

constexpr int DLL_OK = 30001;
constexpr int DLL_FAIL = 30002;
constexpr int DLL_NaN = 30003;

enum class SONIC_API dirs { up = 0, down = 1, left = 2, right = 3, stop = 4 };
enum class SONIC_API field_type
{
	field = 0, brick = 1, bush = 2, gold = 3, gold_brick = 4,
	platform = 5, portal = 6, rip = 7, sky = 8, tree = 9, no_type = 10
};
enum class creature_type { sonic = 0, mushroom = 1};

namespace engine
{
	
	class SONIC_API ATOM
	{
		protected:
			float width = 0;
			float height = 0;

		public:
			float x = 0;
			float y = 0;
			float ex = 0;
			float ey = 0;

			ATOM(float _x, float _y, float _width = 1.0f, float _height = 1.0f)
			{
				x = _x;
				y = _y;
				width = _width;
				height = _height;
				ex = x + width;
				ey = y + height;
			}
			virtual ~ATOM() {};

			float GetWidth() const
			{
				return width;
			}
			float GetHeight() const
			{
				return height;
			}
			void SetWidth(float _new_width)
			{
				width = _new_width;
				ex = x + width;
			}
			void SetHeight(float _new_height)
			{
				height = _new_height;
				ey = y + height;
			}

			void SetEdges()
			{
				ex = x + width;
				ey = y + height;
			}
			void NewDims(float _new_width, float _new_height)
			{
				width = _new_width;
				height = _new_height;
				ex = x + width;
				ey = y + height;
			}
	};

	class SONIC_API BASICFIELD :public ATOM
	{
		protected:
			float speed = 1.0f;
			BASICFIELD(float _x, float _y, field_type _type) :ATOM(_x, _y)
			{
				type = _type;

				switch (type)
				{
				case field_type::field:
					NewDims(800.0f, 100.0f);
					break;

				case field_type::sky:
					NewDims(800.0f, 650.0f);
					break;

				case field_type::brick:
					NewDims(80.0f, 80.0f);
					break;

				case field_type::bush:
					NewDims(60.0f, 60.0f);
					break;

				case field_type::gold:
					NewDims(10.0f, 10.0f);
					break;

				case field_type::gold_brick:
					NewDims(80.0f, 80.0f);
					break;


				case field_type::platform:
					NewDims(120.0f, 62.0f);
					break;

				case field_type::portal:
					NewDims(100.0f, 100.0f);
					break;

				case field_type::rip:
					NewDims(80.0f, 94.0f);
					break;

				case field_type::tree:
					NewDims(55.0f, 60.0f);
					break;
				}
			}

		public:
			dirs dir = dirs::stop;
			field_type type = field_type::no_type;

			
			void Release()
			{
				delete this;
			}
			bool Move(float gear)
			{
				float myspeed = speed * gear;
				switch (dir)
				{
				case dirs::left:
					x -= myspeed;
					SetEdges();
					if (ex <= 0)return false;
					break;

				case dirs::right:
					x += myspeed;
					SetEdges();
					if (x >= scr_width)return false;
					break;
				}
				return true;
			}
			int Transform()
			{
				if (type == field_type::gold_brick)
				{
					type = field_type::brick;
					return DLL_OK;
				}
				return DLL_NaN;
			}

			friend BASICFIELD* CreateFieldFactory(field_type what, float sx, float sy);
	};

	class SONIC_API BASICCREATURES :public ATOM
	{
		protected:
			float speed = 0;

			int frame = 0;
			int frame_delay = 5;

			float start_x = 0;
			float start_y = 0;
			float dest_x = 0;
			float dest_y = 0;

			float slope = 0;
			float intercept = 0;

			bool need_to_set_line = true;
			bool jump_up = true;
			bool now_jumping = false;

			bool is_dizzy = false;

			int dizzy_counter = 0;
			int max_dizzy = 100;

			BASICCREATURES(float _x, float _y, creature_type what_type) :ATOM(_x, _y)
			{
				type = what_type;
				switch (type)
				{
				case creature_type::sonic:
					NewDims(50.0f, 55.0f);
					speed = 1.0f;
					break;

				case creature_type::mushroom:
					NewDims(40.0f, 40.0f);
					speed = 0.8f;
					break;
				}
			}

			void SetNewLine(float _dest_x, float _dest_y)
			{
				start_x = x;
				start_y = y;
				dest_x = _dest_x;
				dest_y = _dest_y;

				if (dest_x - x != 0)
				{
					slope = (dest_y - y) / (dest_x - x);
					intercept = y - slope * x;
				}
			}

		public: 
			creature_type type = creature_type::sonic;
			dirs dir = dirs::stop;
			bool evil_fall = false;
			
			virtual ~BASICCREATURES() {};

			bool NowJumping() const
			{
				return now_jumping;
			}

			virtual void Release() = 0;
			virtual bool Move(float gear) = 0;
			virtual void Jump(bool interrupt_jump) = 0;
			
			
			void Fall()
			{
				if (y + GetHeight() < scr_width - 100.0f)
				{
					y += speed;
					SetEdges();
				}
				else
				{
					y = scr_width - 100.0f - GetHeight();
					SetEdges();
					evil_fall = false;
				}
			}

			virtual int GetFrame() = 0;
			
			bool IsDizzy() const
			{
				return is_dizzy;
			}
			
			bool Dizzy()
			{
				is_dizzy = true;
				dizzy_counter++;
				if (dizzy_counter >= max_dizzy)
				{
					dizzy_counter = 0;
					is_dizzy = false;
				}
				return is_dizzy;
			}
	};

	//****************************************

	typedef BASICFIELD* FieldItem;
	typedef BASICCREATURES* Creature;

	extern SONIC_API Creature CreatureFactory(float _where_x, creature_type _what);
	BASICFIELD* CreateFieldFactory(field_type what, float sx, float sy)
	{
		return new engine::BASICFIELD(sx, sy, what);
	}
}