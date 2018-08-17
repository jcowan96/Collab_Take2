// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "../Public/SessionInformation.h"
#include <string>
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "SessionAdvertiser.generated.h"

//Forward Declarations
class SessionInformation;

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class MYPROJECT3_API USessionAdvertiser : public UActorComponent
{
	GENERATED_BODY()

public:
	//Constructors/Destructors
	// Sets default values for this component's properties
	USessionAdvertiser();
	~USessionAdvertiser();

	//Public Fields
	enum ConnectionTypes { bolt, mb, amq383, amq384, ws71, ws75, ws80 };
	ConnectionTypes connectionType = bolt;
	std::string server = "198.120.194.135:9100";
	std::string missionName = "UNSET";
	std::string satName = "UNSET";
	bool initialized = false;

	//Object Pointers
	std::unique_ptr<SessionInformation> sessionInformation;
	std::unique_ptr<gmsec::api::mist::ConnectionManager> connMgr;

	//Functions
	void respondToListRequest(gmsec::api::Message msg);
	void handleListRequests();
	// Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;
	void Initialize();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

private:
	bool subscribed = false;
	std::string connectionTypeToString(ConnectionTypes rawConnType);



};
