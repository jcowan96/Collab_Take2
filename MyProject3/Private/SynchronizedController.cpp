// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SynchronizedController.h"
#include "../Public/SynchronizationManager.h"
#include "../Public/SynchronizedUser.h"

//===================================================================================================================
//Constructors
//===================================================================================================================
// Sets default values for this component's properties
USynchronizedController::USynchronizedController()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

USynchronizedController::~USynchronizedController()
{
	//Do Nothing
}

//===================================================================================================================
//Public Function Implementation
//===================================================================================================================

// Called when the game starts
void USynchronizedController::BeginPlay()
{
	Super::BeginPlay();
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizedController] Beginning Play"));

	//Find If VRController is Left or Right side Controller
	TArray<UMotionControllerComponent*> motionControllers;
	GetOwner()->GetComponents<UMotionControllerComponent>(motionControllers);
	FString left("MC_Left"); //UE4 Convention
	FString right("MC_Right"); //UE4 Convention
	for (int i = 0; i < motionControllers.Num(); i++)
	{
		FString controllerPath = motionControllers[i]->GetPathName();
		if (controllerSide == ControllerSide::Left && controllerPath.Find(left) != -1)
		{
			VRController = motionControllers[i];
			controllerSide = ControllerSide::Left;
			UE_LOG(LogTemp, Warning, TEXT("[SynchronizedController] Initialized Left Controller"));

		}
		else if (controllerSide == ControllerSide::Right && controllerPath.Find(right) != -1)
		{
			VRController = motionControllers[i];
			controllerSide = ControllerSide::Right;
			UE_LOG(LogTemp, Warning, TEXT("[SynchronizedController] Initialized Right Controller"));
		}
	}

	//Find it from list of Actor's components
	if (!synchronizedUser.IsValid())
	{
		UActorComponent* comp = GetOwner()->FindComponentByClass<USynchronizedUser>();
		USynchronizedUser* user = Cast<USynchronizedUser>(comp);
		synchronizedUser = user; //If this works, it's suspicious
	}

	parentPosition = VRController->GetOwner()->GetTransform().GetTranslation(); //Keep Track of this to base relative controller location off of

	lastRecordedPosition = VRController->GetComponentTransform().GetTranslation() + parentPosition;
	lastRecordedRotation = VRController->GetComponentTransform().GetRotation();
	lastRecordedScale = VRController->GetComponentTransform().GetScale3D();

	UE_LOG(LogTemp, Warning, TEXT("[SynchronizedController] Initialized"));
}



// Called every frame
void USynchronizedController::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
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
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedController] Found Valid SynchronizationManager in scene"));
				synchronizationManager = *It;
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizedController] synchronizationManager was not valid/initalized"));
			}
		}
	}

	if (synchronizedUser->isControlled)
	{
		std::lock_guard<std::mutex> lock(entityLock); //Locked from this point
		if (throttleCounter >= throttleFactor && synchronizationManager.IsValid())
		{
			//rigid body stuff here

			//Position (relative to parent character position)
			if (VRController->GetComponentTransform().GetTranslation() + VRController->GetOwner()->GetTransform().GetTranslation() != lastRecordedPosition)
			{
				if (((VRController->GetComponentTransform().GetTranslation() + VRController->GetOwner()->GetTransform().GetTranslation()) - lastRecordedPosition).Size() > 0.1f)
				{
					//Log something here
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizedController] position changed"));

					parentPosition = VRController->GetOwner()->GetTransform().GetTranslation();
					lastRecordedPosition = VRController->GetComponentTransform().GetTranslation() + parentPosition;
					synchronizationManager->sendPositionChange(this);

					throttleCounter = 0;
				}
			}

			//Rotation
			if (VRController->GetComponentTransform().GetRotation() != lastRecordedRotation)
			{
				if ((VRController->GetComponentTransform().GetRotation().Euler() - lastRecordedRotation.Euler()).Size() > 1.0f)
				{
					//Log something here
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizedController] rotation changed"));

					lastRecordedRotation = VRController->GetComponentTransform().GetRotation();
					synchronizationManager->sendRotationChange(this);

					throttleCounter = 0;
				}
			}

			//Scale
			if (VRController->GetComponentTransform().GetScale3D() != lastRecordedScale)
			{
				if ((VRController->GetComponentTransform().GetScale3D() - lastRecordedScale).Size() > 0.1f)
				{
					//Log something here
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizedController] scale changed"));

					lastRecordedScale = VRController->GetComponentTransform().GetScale3D();
					synchronizationManager->sendScaleChange(this);

					throttleCounter = 0;
				}
			}
			//transform.hasChanged = false;
			//throttleCounter = 0;
		}
		throttleCounter++;
	}
}

