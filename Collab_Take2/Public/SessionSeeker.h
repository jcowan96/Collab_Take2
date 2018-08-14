// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "../Public/SessionInformation.h"
#include "../Public/FSeekerWorker.h"
#include <string>
#include <list>
#include <thread>
#include <vector>
#include <iostream>
#include "CoreMinimal.h"
#include "Runtime/Core/Public/HAL/Runnable.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "Components/ActorComponent.h"
#include "SessionSeeker.generated.h"

//Forward Declarations
class SessionInformation;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class COLLAB_TAKE2_API USessionSeeker : public UActorComponent
{
	GENERATED_BODY()

public:
	//Constructors/Destructors
	// Sets default values for this component's properties
	USessionSeeker();
	~USessionSeeker();

	enum ConnectionTypes { bolt, mb, amq383, amq384, ws71, ws75, ws80 };
	ConnectionTypes connectionType = bolt;
	std::string subscriptionSubject = "GMSEC.*.*.RESP.DIR.ADVERTISER.>";
	std::string server = "localhost:9100";
	std::string missionName = "UNSET";
	std::string satName = "UNSET";
	std::string alias = "New_User";
	std::vector<SessionInformation> availableSessions;

	//Object Pointers
	std::unique_ptr<gmsec::api::mist::ConnectionManager> connMgr;

	//Functions
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void searchForSessions();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	bool searched = false;
	//void requestSessionInfo();
	std::string connectionTypeToString(ConnectionTypes rawConnType);
	std::vector<std::string> stringSplit(std::string str, std::string delimiter);
};
