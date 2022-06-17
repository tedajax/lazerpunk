﻿#include "CoreSystems.h"


void EntityExpirationSystem::Update(const GameTime& time)
{
	std::queue<Entity> removeQueue{};

	for (Entity entity : GetEntities())
	{
		auto& [secRemaining] = GetWorld().GetComponent<Expiration>(entity);

		secRemaining -= time.dt();

		if (secRemaining <= 0)
			removeQueue.push(entity);
	}

	while (!removeQueue.empty())
	{
		GetWorld().DestroyEntity(removeQueue.front());
		removeQueue.pop();
	}
}

void ViewSystem::Update(const GameTime& time)
{
	activeCameraEntity = 0;

	for (Entity entity : GetEntities())
	{
		if (!activeCameraEntity)
			activeCameraEntity = entity;

		auto [transform, view] = GetWorld().GetComponents<Transform, CameraView>(entity);
		view.center = transform.position + view.extents / 2 / view.scale;
	}

	if (activeCameraEntity)
	{
		const auto& transform = GetWorld().GetComponent<Transform>(activeCameraEntity);
		const auto& cameraView = GetWorld().GetComponent<CameraView>(activeCameraEntity);

		activeCamera.position = transform.position * cameraView.scale;
		activeCamera.extents = cameraView.extents;
		activeCamera.scale = cameraView.scale;
	}
}

Vec2 ViewSystem::WorldScaleToScreen(Vec2 worldScale) const
{
	return camera::WorldScaleToScreen(activeCamera, worldScale);
}

Vec2 ViewSystem::WorldToScreen(Vec2 worldPosition) const
{
	return camera::WorldToScreen(activeCamera, worldPosition);
}