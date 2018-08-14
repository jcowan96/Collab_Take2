// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "../Public/SynchronizationManager.h"
#include "../Public/SynchronizedUser.h"
#include <string>
#include <mutex>
#include "HeadMountedDisplay.h"
#include "MotionControllerComponent.h"
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SynchronizedController.generated.h"

//Forward Declarations
class USynchronizationManager;
class USynchronizedUser;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COLLAB_TAKE2_API USynchronizedController : public UActorComponent
{
	GENERATED_BODY()

public:
	//Constructors/Destructors
	//Sets default values for this component's properties
	USynchronizedController();
	~USynchronizedController();

	//Public Fields
	enum ControllerSide { Left, Right };
	std::mutex entityLock;
	ControllerSide controllerSide = Left;
	int throttleFactor = 10;

	//Object Pointers
	TWeakObjectPtr<USynchronizationManager> synchronizationManager;
	TWeakObjectPtr<USynchronizedUser> synchronizedUser;
	TWeakObjectPtr<UMotionControllerComponent> VRController;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	FVector lastRecordedPosition, lastRecordedScale;
	FQuat lastRecordedRotation;
	int throttleCounter = 0;
};
