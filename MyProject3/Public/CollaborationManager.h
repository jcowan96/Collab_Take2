// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "../Public/SynchronizationManager.h"
#include "../Public/SessionMasterNode.h"
#include "../Public/SessionSlaveNode.h"
#include "../Public/SessionInformation.h"
#include "../Public/SessionAdvertiser.h"
#include <string>
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "CollaborationManager.generated.h"

//Forward Declarations
class USynchronizationManager;
class USessionMasterNode;
class USessionSlaveNode;
class SessionInformation;
class USessionAdvertiser;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT3_API UCollaborationManager : public UActorComponent
{
	GENERATED_BODY()

public:
	// Sets default values for this component's properties
	UCollaborationManager();
	~UCollaborationManager();

	//Public Fields
	enum ConnectionTypes { bolt, mb, amq383, amq384, ws71, ws75, ws80 };
	ConnectionTypes connectionType = bolt;
	std::string server = "localhost:9100";
	std::string missionName = "UNSET";
	std::string satName = "UNSET";

	//Pointers
	TWeakObjectPtr<USynchronizationManager> synchManager;
	TWeakObjectPtr<USessionMasterNode> masterNode;
	TWeakObjectPtr<USessionSlaveNode> slaveNode;

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	void enterMasterMode(SessionInformation sessionInfo, std::string alias);
	void enterSlaveMode(SessionInformation sessionInfo, std::string alias);

private:
	TWeakObjectPtr<USessionAdvertiser> sessionAdvertiser;


};
