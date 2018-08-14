// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "../Public/SynchronizationManager.h"
#include <string>
#include <mutex>
#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "SynchronizedObject.generated.h"

//Forward Declarations
class USynchronizationManager;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COLLAB_TAKE2_API USynchronizedObject : public UActorComponent
{
	GENERATED_BODY()

public:
	//Constructors/Destructors
	// Sets default values for this component's properties
	USynchronizedObject();
	~USynchronizedObject();

	//Public Fields
	int throttleFactor = 10;
	std::mutex entityLock;

	//Object Pointers
	TWeakObjectPtr<USynchronizationManager> synchronizationManager;

	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	FVector lastRecordedPosition, lastRecordedScale;
	FQuat lastRecordedRotation;
	int throttleCounter = 0;
	AActor* owner;
};
