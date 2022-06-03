#pragma once

//namespace Direction
//{
//	enum Direction
//	{
//		Invalid = 0,
//		Left, Right, Up, Down, Count
//	};
//}
//

enum class Direction
{
	Invalid,
	Left,
	Right,
	Up,
	Down,
	Count,
};

constexpr int kDirectionCount = static_cast<int>(Direction::Count);




struct GameState
{
	Camera camera;
	SpriteSheet sprites{};
	GameMap map;

	enum_array<bool, Direction> moveDown{};
	enum_array<float, Direction> moveDownTimestamp{};

	Vec2 playerPos{0, 0};
	Direction playerFacing = Direction::Right;
	Vec2 moveInput{};
	bool requestDash{};
	Vec2 dashVelocity{};
	float dashDecay{ 0.05f };
};

namespace game
{
	using vec2::Length;
	using vec2::Normalize;
	using vec2::Dot;

	void Init(GameState& game)
	{
		game.playerPos = { 8, 5 };
	}

	void DirectionInput(enum_array<bool, Direction>& down, enum_array<float, Direction>& timestamp, SDL_Scancode key, Direction direction, float time)
	{
		down[direction] = input::GetKey(key);
		if (input::GetKeyDown(key)) timestamp[direction] = time;
	}

	inline Vec2 DirectionVelocity(Direction direction)
	{
		Vec2 moveVectors[kDirectionCount] = {
			{0, 0}, {-1, 0}, {1, 0}, {0, -1}, {0, 1}
		};
		return moveVectors[static_cast<int>(direction)];
	}

	void Update(GameState& game, const GameTime& time)
	{
		DirectionInput(game.moveDown, game.moveDownTimestamp, SDL_SCANCODE_LEFT, Direction::Left, time.t());
		DirectionInput(game.moveDown, game.moveDownTimestamp, SDL_SCANCODE_RIGHT, Direction::Right, time.t());
		DirectionInput(game.moveDown, game.moveDownTimestamp, SDL_SCANCODE_UP, Direction::Up, time.t());
		DirectionInput(game.moveDown, game.moveDownTimestamp, SDL_SCANCODE_DOWN, Direction::Down, time.t());

		Direction direction = Direction::Invalid;
		float latestTime = 0.0f;
		for (Direction dir = Direction::Left; dir < Direction::Count; ++dir)
		{
			if (game.moveDown[dir] && game.moveDownTimestamp[dir] > latestTime)
			{
				direction = dir;
				latestTime = game.moveDownTimestamp[dir];
			}
		}

		game.moveInput = DirectionVelocity(direction);

		if (input::GetKeyDown(SDL_SCANCODE_Z))
		{
			game.requestDash = true;
		}

		if (direction != Direction::Invalid)
			game.playerFacing = direction;

		if (game.requestDash)
		{
			game.requestDash = false;
			if (Length(game.dashVelocity) < 0.1f)
			{
				game.dashVelocity = DirectionVelocity(game.playerFacing) * 25.0f;
			}
		}

		float dashMag = Length(game.dashVelocity);
		float newDashMag = math::Damp(dashMag, 0.0f, 0.1f, time.dt());
		game.dashVelocity = Normalize(game.dashVelocity) * newDashMag;

		Vec2 velocity = (game.moveInput * 10.0f + game.dashVelocity) * time.dt();

		game.playerPos = game.playerPos + velocity;

		if (game.playerPos.x > game.camera.position.x + 10) game.camera.position.x = game.playerPos.x - 10;
		if (game.playerPos.x < game.camera.position.x + 5) game.camera.position.x = game.playerPos.x - 5;
		if (game.playerPos.y > game.camera.position.y + 6) game.camera.position.y = game.playerPos.y - 6;
		if (game.playerPos.y < game.camera.position.y + 3) game.camera.position.y = game.playerPos.y - 3;
	}

	void FixedUpdate(GameState& game, const GameTime& time)
	{
	}

	void Render(const DrawContext& ctx, const GameState& game, const GameTime& time)
	{
		map::DrawLayers(ctx, game.map, game.camera, game.sprites, std::array{ StrId("Background") });

		int spriteId;
		switch (game.playerFacing)
		{
		default:
		case Direction::Left:
		case Direction::Right:
			spriteId = 1043;
			break;
		case Direction::Up:
			spriteId = 1042;
			break;
		case Direction::Down:
			spriteId = 1041;
			break;
		}

		SpriteFlipFlags flip = SpriteFlipFlags::None;
		if (game.playerFacing == Direction::Left)
			flags::Set(flip, SpriteFlipFlags::FlipX, true);

		Vec2 screenPos = camera::WorldToScreen(game.camera, game.playerPos);
		draw::Sprite(ctx, game.sprites, spriteId, screenPos, 0.0f, flip, Vec2{0.5f, 0.5f});
		SDL_SetRenderDrawColor(ctx.renderer, 255, 0, 0, 255);
		SDL_RenderDrawPointF(ctx.renderer, screenPos.x, screenPos.y);

		map::DrawLayers(ctx, game.map, game.camera, game.sprites, std::array{ StrId("Foreground") });
	}
}