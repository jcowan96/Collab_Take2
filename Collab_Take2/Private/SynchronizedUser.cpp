// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SynchronizedUser.h"
#include "../Public/SynchronizationManager.h"

//===================================================================================================================
//Constructors
//===================================================================================================================
// Sets default values for this component's properties
USynchronizedUser::USynchronizedUser()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	//synchronizationManager.reset(new SynchronizationManager());
}

USynchronizedUser::~USynchronizedUser()
{
	//Do Nothing
}

//===================================================================================================================
//Public Function Implementations
//===================================================================================================================
// Called when the game starts
void USynchronizedUser::BeginPlay()
{
	Super::BeginPlay();

	VRCamera = GetOwner()->FindComponentByClass<UCameraComponent>();

	lastRecordedPosition = VRCamera->GetComponentTransform().GetTranslation();
	lastRecordedRotation = VRCamera->GetComponentTransform().GetRotation();
	lastRecordedScale = VRCamera->GetComponentTransform().GetScale3D();

	UE_LOG(LogTemp, Warning, TEXT("[SynchronizedUser] Initialized"));
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizedUser] Owner is: %s"), *FString(VRCamera->GetName()));
}


// Called every frame
void USynchronizedUser::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedUser] Found Valid SynchronizationManager in scene"));
				synchronizationManager = *It;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedUser] synchronizationManager was not valid/initalized"));
			}
		}
	}

	//Check for Transform updates and send them to the SynchronizationManager
	if (isControlled)
	{
		//Lock here
		std::lock_guard<std::mutex> lock(entityLock);
		if (throttleCounter >= throttleFactor && synchronizationManager.IsValid())
		{
			//rigid body stuff here

			//Position
			if (VRCamera->GetComponentTransform().GetTranslation() != lastRecordedPosition)
			{
				if ((VRCamera->GetComponentTransform().GetTranslation() - lastRecordedPosition).Size() > 0.1f)
				{
					//Log something here
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizedUser] position changed"));

					lastRecordedPosition = VRCamera->GetComponentTransform().GetTranslation();
					synchronizationManager->sendPositionChange(this);

					throttleCounter = 0;
				}
			}

			//Rotation
			if (VRCamera->GetComponentTransform().GetRotation() != lastRecordedRotation)
			{
				if ((VRCamera->GetComponentTransform().GetRotation().Euler() - lastRecordedRotation.Euler()).Size() > 1.0f)
				{
					//Log something here
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizedUser] rotation changed"));

					lastRecordedRotation = VRCamera->GetComponentTransform().GetRotation();
					synchronizationManager->sendRotationChange(this);

					throttleCounter = 0;
				}
			}

			//Scale
			if (VRCamera->GetComponentTransform().GetScale3D() != lastRecordedScale)
			{
				if ((VRCamera->GetComponentTransform().GetScale3D() - lastRecordedScale).Size() > 0.1f)
				{
					//Log something here
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizedUser] scale changed"));

					lastRecordedScale = VRCamera->GetComponentTransform().GetScale3D();
					synchronizationManager->sendScaleChange(this);

					throttleCounter = 0;
				}
			}
		}
		throttleCounter++;
	}
}

