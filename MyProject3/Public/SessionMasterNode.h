// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "../Public/SynchronizationManager.h"
#include "../Public/SessionInformation.h"
#include "../Public/SynchronizedUser.h"
#include "../Public/SessionAdvertiser.h"
#include <string>
#include <vector>
#include <ctime>
#include <stdlib.h>
#include "IXRTrackingSystem.h"
#include "HeadMountedDisplay.h"
#include "MotionControllerComponent.h"
#include "EngineUtils.h"
#include "Runtime/Engine/Classes/Engine/EngineTypes.h"
#include "Runtime/Engine/Classes/Camera/CameraActor.h"
#include "Runtime/Engine/Classes/Camera/PlayerCameraManager.h"
#include "Runtime/Engine/Classes/Camera/CameraComponent.h"
#include "Runtime/Engine/Classes/Engine/StaticMeshActor.h"
#include "Runtime/CoreUObject/Public/UObject/ConstructorHelpers.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SessionMasterNode.generated.h"

//Forward Declarations
class USynchronizationManager;
class SessionInformation;
class USynchronizedUser;
class USessionAdvertiser;
class ACameraActor;
class UMotionControllerComponent;
class AStaticMeshActor;


UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT3_API USessionMasterNode : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USessionMasterNode();
	~USessionMasterNode();

	//Public Fields
	enum ConnectionTypes { bolt, mb, amq383, amq384, ws71, ws75, ws80 };
	ConnectionTypes connectionType = bolt;
	std::string server = "198.120.194.135:9100";
	std::string missionName = "UNSET";
	std::string satName = "UNSET";

	//Object Pointers
	TWeakObjectPtr<USynchronizationManager> synchronizationManager;
	TWeakObjectPtr<USynchronizedUser> synchronizedUser;
	TWeakObjectPtr<USessionAdvertiser> sessionAdvertiser;
	TWeakObjectPtr<UCameraComponent> camera;
	TWeakObjectPtr<UMotionControllerComponent> leftCon;
	TWeakObjectPtr<UMotionControllerComponent> rightCon;
	std::unique_ptr<gmsec::api::mist::ConnectionManager> connMgr;
	//GameObjects should be replaced with Actors (probably) or possibly Components
	//GameObject headsetFollower;
	//GameObject defaultAvatarPrefab, userContainer;

	//Functions
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void StartRunning(SessionInformation sessionInfo, std::string alias);
	void sendEntityStateResponse(gmsec::api::Message msg);
	void receiveMessage();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	bool subscribed = false;
	bool gotSessionInfo = false;
	std::unique_ptr<SessionInformation> currentSessionInfo;
	std::vector<TWeakObjectPtr<USynchronizedUser>> synchronizedUsers;

	std::string connectionTypeToString(ConnectionTypes rawConnType);
	std::string toUpper(std::string str);
	FString worldTypeToString(EWorldType::Type world);

	UStaticMesh* Avatar;
	UStaticMesh* Avatar_Controller;

	FString vrPawn = FString(TEXT("FirstPersonCharacter_HISEAS_2"));
	FString worldType = FString(TEXT("UEDPIE"));
};
