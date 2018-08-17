// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "../Public/SynchronizationManager.h"
#include "../Public/SessionInformation.h"
#include "../Public/SynchronizedUser.h"
#include "../Public/FSlaveworker.h"
#include "../Public/SessionSeeker.h"
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
#include "SessionSlaveNode.generated.h"

//Forward Declarations
class USynchronizationManager;
class SessionInformation;
class USynchronizedUser;
class USessionSeeker;
class ACameraActor;
class UMotionControllerComponent;
class AStaticMeshActor;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT3_API USessionSlaveNode : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	USessionSlaveNode();
	~USessionSlaveNode();

	//Public Fields
	enum ConnectionTypes { bolt, mb, amq383, amq384, ws71, ws75, ws80 };
	ConnectionTypes connectionType = bolt;
	std::string server = "198.120.194.135:9100";
	std::string missionName = "UNSET";
	std::string satName = "UNSET";

	//Object Pointers
	TWeakObjectPtr<USynchronizationManager> synchronizationManager;
	TWeakObjectPtr<USessionSeeker> sessionSeeker;
	TWeakObjectPtr<UCameraComponent> camera;
	TWeakObjectPtr<UMotionControllerComponent> leftCon;
	TWeakObjectPtr<UMotionControllerComponent> rightCon;
	std::unique_ptr<gmsec::api::mist::ConnectionManager> connMgr;
	std::unique_ptr<gmsec::api::Message> reply; //Message pointer to get reply from MasterNode via SlaveWorker

	//Functions
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void connect(SessionInformation sessionInfo, std::string alias);

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	//Process incoming responses from MasterNode
	void processEntityResponse(gmsec::api::Message response);

	//void sendGMSECEntityRequestMessage(SessionInformation sessionInfo, std::string alias);
	std::string connectionTypeToString(ConnectionTypes rawConnType);

	//Set this true after a GMSECEntityRequestMessage has been sent
	bool requestSent = false;
	//Set this true after a new entityResponseMessage has been received
	bool newResponse = false;

	std::vector<std::string> split(const std::string &text, char sep);
	std::string toUpper(std::string str);
	FString worldTypeToString(EWorldType::Type world);

	UStaticMesh* Avatar;
	UStaticMesh* Avatar_Controller;

	FString vrPawn = FString(TEXT("FirstPersonCharacter_HISEAS_2"));
	FString worldType = FString(TEXT("UEDPIE"));
};
