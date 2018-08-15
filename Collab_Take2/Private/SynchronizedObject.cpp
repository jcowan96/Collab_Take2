// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SynchronizedObject.h"

//===================================================================================================================
//Constructors
//===================================================================================================================
// Sets default values for this component's properties
USynchronizedObject::USynchronizedObject()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

USynchronizedObject::~USynchronizedObject()
{
	//Do Nothing
}

//===================================================================================================================
//Public Function Implementations
//===================================================================================================================

// Called when the game starts
void USynchronizedObject::BeginPlay()
{
	Super::BeginPlay();
	owner = GetOwner(); //Send this to synchronizationManager to create stateAttributeDataMessages

	lastRecordedPosition = owner->GetTransform().GetTranslation();
	lastRecordedRotation = owner->GetTransform().GetRotation();
	lastRecordedScale = owner->GetTransform().GetScale3D();

	UE_LOG(LogTemp, Warning, TEXT("[SynchronizedObject] Initialized"));
}


// Called every frame
void USynchronizedObject::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Look for SynchronizationManager in scene while it has not been initialized
	if (!synchronizationManager.IsValid())
	{
		for (TObjectIterator<USynchronizationManager> It; It; ++It)
		{
			TWeakObjectPtr<USynchronizationManager> temp = *It; //Assigning a pointer like this lets us check if the SynchronizationManager is valid

			if (temp.IsValid())
			{
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedObject] Found Valid SynchronizationManager in scene"));
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedObject] Parent name is: %s"), *(It->GetOwner()->GetName()));
				synchronizationManager = *It;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedObject] synchronizationManager was not valid/initalized"));
			}
		}
	}

	//Check for Transform updates and send them to the synchronizationManager
	std::lock_guard<std::mutex> lock(entityLock); //Locked from this point
	if (throttleCounter >= throttleFactor && synchronizationManager.IsValid())
	{
		//rigid body stuff here

		//Position
		if (owner->GetTransform().GetTranslation() != lastRecordedPosition)
		{
			if ((owner->GetTransform().GetTranslation() - lastRecordedPosition).Size() > 0.1f)
			{
				//Log something here
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedObject] position changed"));

				lastRecordedPosition = owner->GetTransform().GetTranslation();
				synchronizationManager->sendPositionChange(this);
			}
		}

		//Rotation
		if (owner->GetTransform().GetRotation() != lastRecordedRotation)
		{
			if ((owner->GetTransform().GetRotation().Euler() - lastRecordedRotation.Euler()).Size() > 1.0f)
			{
				//Log something here
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedObject] rotation changed"));

				lastRecordedRotation = owner->GetTransform().GetRotation();
				synchronizationManager->sendRotationChange(this);
			}
		}

		//Scale
		if (owner->GetTransform().GetScale3D() != lastRecordedScale)
		{
			if ((owner->GetTransform().GetScale3D() - lastRecordedScale).Size() > 0.1f)
			{
				//Log something here
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedObject] scale changed"));

				lastRecordedScale = owner->GetTransform().GetScale3D();
				synchronizationManager->sendScaleChange(this);
			}
		}
		throttleCounter = 0;
	}
	throttleCounter++;
}

