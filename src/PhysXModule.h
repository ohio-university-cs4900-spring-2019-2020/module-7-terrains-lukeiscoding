#pragma once

//PhysX
#include "PxPhysicsAPI.h"
#include "WO.h"
//terrain
#include "cooking/PxCooking.h"
#include "PxPhysics.h"
#include "PxScene.h"

using namespace physx;

namespace Aftr
{
	class PhysXModule {
	public:
		static void init();
		static void addActor(void* pointer, physx::PxActor* actor);

		static void setGravity(Vector gravity);

		static PxPhysics* gPhysics;
		static PxScene* scene;
		static PxCooking* c;

	protected:
		static PxDefaultAllocator gAllocator;
		static PxDefaultErrorCallback gErrorCallback;
		static PxFoundation* gFoundation;
		static PxSceneDesc gSceneDesc;
		static PxDefaultCpuDispatcher* gCpuDispatcher;
	};
}