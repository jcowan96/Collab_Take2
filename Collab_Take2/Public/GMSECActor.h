// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include <string>
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GMSECActor.generated.h"

UCLASS()
class COLLAB_TAKE2_API AGMSECActor : public AActor
{
	GENERATED_BODY()

protected:
	// Called when the game starts or when spawned
	virtual void BeginPlay() override;

public:
	//Public Fields
	enum ConnectionTypes { bolt, mb, amq383, amq384, ws71, ws75, ws80 };
	ConnectionTypes connectionType = bolt;
	std::string server = "localhost:9100";
	std::string missionName = "UNSET";
	std::string satName = "UNSET";
	std::string groupID = "UNSET";
	std::string projectName = "UNSET";
	std::string userName = "UNSET";
	int throttleFactor = 5;

	// Sets default values for this actor's properties
	AGMSECActor();
	// Called every frame
	virtual void Tick(float DeltaTime) override;


	float RunningTime;
	float lastMsgSent;

	//Object Pointers
	std::unique_ptr<gmsec::api::mist::ConnectionManager> connMgr;

private:
	std::string connectionTypeToString(ConnectionTypes rawConnType);
};