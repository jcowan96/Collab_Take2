// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "../Public/SynchronizationManager.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"
#include <string>
#include <mutex>
#include "HeadMountedDisplay.h"
#include "MotionControllerComponent.h"
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SynchronizedUser.generated.h"

//Forward Declarations
class USynchronizationManager;
//Test comment here

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COLLAB_TAKE2_API USynchronizedUser : public UActorComponent
{
	GENERATED_BODY()

public:
	//Constructors/Destructors
	// Sets default values for this component's properties
	USynchronizedUser();
	~USynchronizedUser();

	//Public Fields
	std::string userAlias = "Test-User";
	std::string avatarName = "default";
	bool isControlled = false;
	std::mutex entityLock;
	int throttleFactor = 5;

	//Object Pointers
	TWeakObjectPtr<USynchronizationManager> synchronizationManager;
	TWeakObjectPtr<AActor> userObject;
	TWeakObjectPtr<UCameraComponent> VRCamera;

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
