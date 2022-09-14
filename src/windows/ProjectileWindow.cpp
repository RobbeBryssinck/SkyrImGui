#include "ProjectileWindow.h"

#include <inttypes.h>
#include <imgui.h>

using TProjectile = RE::BSPointerHandleSmartPointer<RE::BSPointerHandleManagerInterface<RE::Projectile>>;

const char* GetProjectileTypeName(RE::BGSProjectile* apProjectileBase)
{
	if (!apProjectileBase)
		return "<Unknown Type>";

	using Type = RE::BGSProjectileData::Type;

	auto& data = apProjectileBase->data;

	if (data.types.all(Type::kMissile)) {
		return "Missile";
	} else if (data.types.all(Type::kGrenade)) {
		return "Grenade";
	} else if (data.types.all(Type::kBeam)) {
		return "Beam";
	} else if (data.types.all(Type::kFlamethrower)) {
		return "Flamethrower";
	} else if (data.types.all(Type::kCone)) {
		return "Cone";
	} else if (data.types.all(Type::kBarrier)) {
		return "Barrier";
	} else if (data.types.all(Type::kArrow)) {
		return "Arrow";
	} else {
		return "<Unknown Type>";
	}
}

void RenderProjectileData(RE::ProjectileHandle aHandle)
{
	auto pProjectile = aHandle.get();
	if (!pProjectile) {
		logger::error("Failed to find ref for projectile");
		return;
	}

	auto* obj = pProjectile->GetObjectReference();
	auto* projectileBase = obj ? obj->As<RE::BGSProjectile>() : nullptr;
	const char* projectileName = GetProjectileTypeName(projectileBase);
	float look = pProjectile->data.angle.x;
	float head = pProjectile->data.angle.z;
	float speed = pProjectile->GetSpeed();
	float age = pProjectile->livingTime;
	float range = pProjectile->range;
	float distanceMoved = pProjectile->distanceMoved;
	char impacted = (pProjectile->flags & 0x400000) != 0 ? 'Y' : 'N';

	ImGui::Text("%s: look %.2f ,head %.2f, speed %.2f, age %.1f, range: %.1f, dist: %.1f, impacted? %c", projectileName, look, head, speed, age, range, distanceMoved, impacted);

	ImGui::Separator();
}

void ProjectileWindow::Update()
{
	ImGui::Begin("Projectiles");

	auto* pManager = RE::Projectile::Manager::GetSingleton();

	pManager->projectileLock.Lock();

	if (ImGui::CollapsingHeader("Unlimited", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (auto projectileHandle : pManager->unlimited) {
			RenderProjectileData(projectileHandle);
		}
	}

	if (ImGui::CollapsingHeader("Limited", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (auto projectileHandle : pManager->limited) {
			RenderProjectileData(projectileHandle);
		}
	}

	if (ImGui::CollapsingHeader("Pending", ImGuiTreeNodeFlags_DefaultOpen)) {
		for (auto projectileHandle : pManager->pending) {
			RenderProjectileData(projectileHandle);
		}
	}

	pManager->projectileLock.Unlock();

	ImGui::End();
}
