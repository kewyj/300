#include "Physics/PhysX.h"
#include "Physics/ContactCallback.h"
#include "Physics/FilterShader.h"

PhysX::PhysX()
{
	mFoundation = PxCreateFoundation(PX_PHYSICS_VERSION, mDefaultAllocator, mDefaultErrorCallBack);

	if (!mFoundation) throw ("PxCreateFoundation Failed!");

	mPvd = PxCreatePvd(*mFoundation);
	physx::PxPvdTransport* transport = physx::PxDefaultPvdSocketTransportCreate("127.0.0.1", 5425, 10);
	mPvd->connect(*transport, physx::PxPvdInstrumentationFlag::eALL);

	mPhysics = PxCreatePhysics(PX_PHYSICS_VERSION, *mFoundation, mToleranceScale, true, mPvd);

	physx::PxSceneDesc sceneDesc(mPhysics->getTolerancesScale());
	mDispatcher = physx::PxDefaultCpuDispatcherCreate(2);
	sceneDesc.gravity = physx::PxVec3(0.f, -98.1f, 0.f);
	sceneDesc.cpuDispatcher = mDispatcher;
	sceneDesc.filterShader = FilterShader;
	sceneDesc.simulationEventCallback = &gContactCallback;
	mScene = mPhysics->createScene(sceneDesc);
	physx::PxPvdSceneClient* pvdClient = mScene->getScenePvdClient();
	
	if (pvdClient)
	{
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONSTRAINTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_CONTACTS, true);
		pvdClient->setScenePvdFlag(physx::PxPvdSceneFlag::eTRANSMIT_SCENEQUERIES, true);

	}
}

PhysX::~PhysX()
{
	PX_RELEASE(mScene);
	PX_RELEASE(mDispatcher);
	mPhysics->release();
	if (mPvd)
	{
		physx::PxPvdTransport* transport = mPvd->getTransport();
		mPvd->release();	
		mPvd = NULL;
		PX_RELEASE(transport);
	}
	mFoundation->release();
}
