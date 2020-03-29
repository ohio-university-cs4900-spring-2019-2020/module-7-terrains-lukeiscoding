#include <iostream>
#include "PhysXModule.h"
#include "AftrGlobals.h"
//terrain
#include "cooking/PxCooking.h"
#include "PxPhysics.h"
#include "PxScene.h"

using namespace Aftr;
using namespace physx;

PxDefaultAllocator PhysXModule::gAllocator;
PxDefaultErrorCallback PhysXModule::gErrorCallback;
PxFoundation* PhysXModule::gFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, gAllocator, gErrorCallback);
PxPhysics* PhysXModule::gPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *gFoundation, PxTolerancesScale(), true);
PxSceneDesc PhysXModule::gSceneDesc(gPhysics->getTolerancesScale());
PxDefaultCpuDispatcher* PhysXModule::gCpuDispatcher = PxDefaultCpuDispatcherCreate(2);
PxScene* PhysXModule::scene;
PxCooking* PhysXModule::c;



void PhysXModule::init() {
	gSceneDesc.cpuDispatcher = gCpuDispatcher;
	gSceneDesc.filterShader = PxDefaultSimulationFilterShader;
	std::cout << "PhysX engine creating!!!!" << std::endl;
	scene = gPhysics->createScene(gSceneDesc);
	scene->setFlag(PxSceneFlag::eENABLE_ACTIVE_ACTORS, true);
	if (!scene) {
		std::cout << "Unable to create PhysX engine" << std::endl;
	}
	else {
		std::cout << "PhysX engine initialized!" << std::endl;
		scene->setGravity(PxVec3(0, 0, -1 * GRAVITY));
	}
	c = PxCreateCooking(PX_PHYSICS_VERSION, *gFoundation, physx::PxCookingParams(physx::PxTolerancesScale()));
	if (!c)
	{
		std::cout << "Cooking error" << std::endl;
		std::cin.get();
	}
}

void PhysXModule::addActor(void* pointer, PxActor* actor) {
	std::cout << "PhysX engine try to add Actor!" << std::endl;
	actor->userData = pointer;
	scene->addActor(*actor);
}

void PhysXModule::setGravity(Vector gravity) {
	gravity = gravity * -1 * GRAVITY;
	scene->setGravity(PxVec3(gravity.x, gravity.y, gravity.z));
}