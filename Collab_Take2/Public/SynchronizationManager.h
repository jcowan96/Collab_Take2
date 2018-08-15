// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "../Public/SessionAdvertiser.h"
#include "../Public/SessionMasterNode.h"
#include "../Public/SynchronizedUser.h"
#include "../Public/SynchronizedObject.h"
#include "../Public/SynchronizedController.h"
#include <string>
#include <iostream>
#include <string>
#include <locale>
#include <vector>
#include <ctime>
#include <thread>
#include <mutex>
#include "Runtime/Core/Public/Misc/Paths.h"
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Runtime/Engine/Public/EngineUtils.h"
#include "SynchronizationManager.generated.h"

//Forward Declarations
class USessionAdvertiser;
class USessionMasterNode;
class USynchronizedUser;
class USynchronizedObject;
class USynchronizedController;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COLLAB_TAKE2_API USynchronizationManager : public UActorComponent
{
	GENERATED_BODY()

public:
	//Constructors/Destructors
	// Sets default values for this component's properties
	USynchronizationManager();
	~USynchronizationManager();

	//Public Fields
	enum ConnectionTypes { bolt, mb, amq383, amq384, ws71, ws75, ws80 };
	enum EntityStateAttributeTypes
	{
		create, destroy, reinitialize, localPosition, localRotation, localScale, absolutePosition,
		absoluteRotation, absoluteScale, relativePosition, relativeRotation, relativeScale, edit
	};
	ConnectionTypes connectionType = bolt;
	std::string server = "localhost:9100";
	std::string missionName = "UNSET";
	std::string satName = "UNSET";
	std::string groupID = "UNSET";
	std::string projectName = "UNSET";
	std::string userName = "UNSET";
	int throttleFactor = 5;

	//Object Pointers
	TWeakObjectPtr<USessionAdvertiser> sessionAdvertiser;
	TWeakObjectPtr<USessionMasterNode> sessionMasterNode;
	std::unique_ptr<gmsec::api::mist::ConnectionManager> connMgr;

	//Functions
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void initialize();
	void sendPositionChange(UActorComponent* synchronizedObject);
	void sendPositionChange(UActorComponent* synchronizedObject, time_t occurenceTime);
	void sendRotationChange(UActorComponent* synchronizedObject);
	void sendRotationChange(UActorComponent* synchronizedObject, time_t occurenceTime);
	void sendScaleChange(UActorComponent* synchronizedObject);
	void sendScaleChange(UActorComponent* synchronizedObject, time_t occurenceTime);
	void publishStateAttributeDataMessage(std::string subjectToPublish, EntityStateAttributeTypes stateAttributeType,
		std::string entityName, std::time_t time, FTransform transformToApply);
	void publishStateAttributeDataMessage(std::string subjectToPublish, EntityStateAttributeTypes stateAttributeType,
		std::string entityName, std::time_t time, FTransform transformToApply, FVector offset3, FQuat offset4);
	void receiveMessage();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	std::string connectionTypeToString(ConnectionTypes rawConnType);
	std::string getFullPath();
	std::string toUpper(std::string str);
	std::vector<std::string> split(const std::string &text, char sep);
	std::string getTimestamp(std::time_t time);
	std::string getFullPath(UActorComponent* object);

	bool subscribed = false;
	bool updateSent = false;
	std::string levelPath = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel."; //This will probably change in different Level or outside of editor
};
