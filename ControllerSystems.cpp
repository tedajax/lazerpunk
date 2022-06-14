#include "ControllerSystems.h"

void EnemyFollowTargetSystem::Update(const GameTime& time) const
{
	if (!targetEntity)
		return;

	const auto& targetTransform = GetWorld().GetComponent<Transform>(targetEntity);

	for (Entity entity : entities)
	{
		auto [transform, velocity, _] = GetArchetype(entity);

		Vec2 delta = targetTransform.position - transform.position;
		Vec2 dir = vec2::Normalize(delta);
		Vec2 vel = velocity.velocity;
		vel = vec2::Damp(vel, dir * 10, 0.4f, time.dt());
		velocity.velocity = vel;
	}
}


void GameCameraControlSystem::SnapFocusToFollow(Entity cameraEntity) const
{
	auto [transform, view, camera] = GetArchetype(cameraEntity);

	if (!camera.followTarget)
		return;

	const auto& [position, _, __] = GetWorld().GetComponent<Transform>(camera.followTarget);

	transform.position = position - view.extents / 2 / view.scale;
	view.center = position;
}

void GameCameraControlSystem::Update(const GameTime& time) const
{
	for (Entity entity : entities)
	{
		auto [transform, view, camera] = GetArchetype(entity);

		if (camera.followTarget)
		{
			const auto& targetTransform = GetWorld().GetComponent<Transform>(camera.followTarget);

			auto [dx, dy] = targetTransform.position - view.center;

			if (dx < camera.followBounds.Left())
				transform.position.x += dx - camera.followBounds.Left();
			if (dx > camera.followBounds.Right())
				transform.position.x += dx - camera.followBounds.Right();
			if (dy < camera.followBounds.Top())
				transform.position.y += dy - camera.followBounds.Top();
			if (dy > camera.followBounds.Bottom())
				transform.position.y += dy - camera.followBounds.Bottom();
		}

		if (camera.clampViewMap)
		{
			const auto& gameMap = map::Get(camera.clampViewMap);
			Bounds2D viewBounds = gameMap.worldBounds;
			viewBounds.max = viewBounds.max - camera_view::WorldExtents(view);
			viewBounds = Bounds2D::Grow(viewBounds, { -0.5f, -0.5f });
			transform.position = viewBounds.ClampPoint(transform.position);
		}
	}
}


void PlayerControlSystem::Update(const GameTime& time) const
{
	using namespace math;
	using namespace vec2;

	for (Entity entity : entities)
	{
		auto [input, transform, facing, velocity, control] = GetArchetype(entity);

		control.velocity = input.moveInput * 3.0f;

		if (input.requestDash)
		{
			input.requestDash = false;
			if (Length(control.dashVelocity) < 0.1f)
			{
				control.dashVelocity = DirectionVector(facing.facing) * 25.0f;
			}
		}

		float dashMag = Length(control.dashVelocity);
		if (float newDashMag = math::Damp(dashMag, 0.0f, 5.0f, time.dt()); newDashMag < control.minDashThreshold)
		{
			control.dashVelocity = vec2::Zero;
		}
		else
		{
			control.dashVelocity = Normalize(control.dashVelocity) * newDashMag;
		}

		Vec2 newVelocity = control.velocity + control.dashVelocity;

		if (input.direction != Direction::Invalid)
		{
			facing.facing = input.direction;

			float posRefComp = transform.position.y;
			float velRefComp = input.moveInput.x;
			float* velTargetComp = &newVelocity.y;

			if (IsDirectionVert(input.direction))
			{
				posRefComp = transform.position.x;
				velRefComp = input.moveInput.y;
				velTargetComp = &newVelocity.x;
			}

			float current = posRefComp;
			float moveMag = abs(velRefComp) / 16;
			float target = round((current - 0.5f) * 2) / 2 + 0.5f;
			float deltaDiff = (target - current);
			*velTargetComp += min(moveMag, abs(deltaDiff)) * Sign(deltaDiff) / time.dt();
		}

		velocity.velocity = newVelocity;
	}
}

void PlayerShootControlSystem::Update(const GameTime& time) const
{
	for (Entity entity : entities)
	{
		auto [input, transform, facing, velocity, shootControl] = GetArchetype(entity);

		if (shootControl.cooldownRemaining > 0)
			shootControl.cooldownRemaining -= time.dt();

		if (input.requestShoot)
		{
			if (shootControl.cooldownRemaining <= 0.0f)
			{
				shootControl.cooldownRemaining += shootControl.cooldownSec;

				Vec2 bulletVel = DirectionVector(facing.facing) * 25;
				SpriteFlipFlags flags{};
				switch (facing.facing)
				{
				case Direction::Left:
					flags = SpriteFlipFlags::FlipDiag;
					break;
				case Direction::Right:
					flags = SpriteFlipFlags::FlipDiag | SpriteFlipFlags::FlipX;
					break;
				case Direction::Down:
					flags = SpriteFlipFlags::FlipY;
					break;
				}
				Entity bulletEntity = GetWorld().CreateEntity();
				GetWorld().AddComponents(bulletEntity,
					Transform{ {transform.position} },
					Velocity{ bulletVel },
					PhysicsBody{},
					Facing{ facing.facing },
					SpriteRender{ 14	, flags, vec2::Half },
					Expiration{ 1.0f });
			}
		}
	}
}
#include "input.h"

namespace spawner
{
	Entity Spawn(World& world, const Spawner& spawner, Vec2 position, float rotation)
	{
		if (Entity spawned = world.CloneEntity(spawner.prefab))
		{
			if (world.HasComponent<Transform>(spawned))
			{
				auto& transform = world.GetComponent<Transform>(spawned);
				transform.position = position;
				transform.rotation = rotation;
			}
			return spawned;
		}
		return 0;
	}
}

void SpawnerSystem::Update(const GameTime& time) const
{
	auto sourceQuery = GetWorld().CreateQuery<SpawnSource>({}, [this](Entity e)
	{
		auto [source] = GetWorld().GetComponent<SpawnSource>(e);
		for (Entity entity : entities)
		{
			if (entity == source)
			{
				GetWorld().GetComponent<Spawner>(entity).spawnedEnemies--;
			}
		}
	});

	for (Entity entity : entities)
	{
		auto [transform, spawner] = GetArchetype(entity);

		if (spawner.spawnTimer > 0)
			spawner.spawnTimer -= time.dt();

		if (spawner.spawnTimer <= 0)
		{
			if (spawner.maxAlive)
			{
				if (spawner.spawnedEnemies < spawner.maxAlive)
				{
					spawner.spawnTimer += spawner.interval;
					if (Entity spawned = spawner::Spawn(GetWorld(), spawner, transform.position, transform.rotation))
					{
						GetWorld().AddComponent(spawned, SpawnSource{ entity });
						spawner.spawnedEnemies++;
					}
				}
			}
			else
			{
				spawner.spawnTimer += spawner.interval;
				spawner::Spawn(GetWorld(), spawner, transform.position, transform.rotation);
			}
		}

		
		/*if (input::GetKeyDown(SDL_SCANCODE_K))
			if (!spawner.spawnedEnemies.empty())
				GetWorld().AddComponent(spawner.spawnedEnemies[0], Expiration{ 0 });*/
	}
}

void SpawnerSystem::OnEntityDestroyed(Entity destroyedEntity)
{
	//for (Entity entity : entities)
	//{
	//	if (auto [transform, spawner] = GetArchetype(entity); spawner.maxAlive > 0)
	//	{
	//		types::erase_if(spawner.spawnedEnemies, [destroyedEntity](Entity e) { return e == destroyedEntity; });
	//	}
	//}
}


