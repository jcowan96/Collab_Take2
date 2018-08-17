// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/GMSECActor.h"
#include <iostream>

// Sets default values
AGMSECActor::AGMSECActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;


	lastMsgSent = 0.0;
}

// Called when the game starts or when spawned
void AGMSECActor::BeginPlay()
{
	Super::BeginPlay();

	gmsec::api::Config cfg;
	cfg.addValue("connectionType", connectionTypeToString(connectionType).c_str());
	cfg.addValue("server", server.c_str());
	cfg.addValue("GMSEC-REQ-RESP", "OPEN-RESP"); //GMSEC Open Response Feature
	connMgr.reset(new gmsec::api::mist::ConnectionManager(cfg));

	//Try to publish a message on startup
	try {
		connMgr->initialize();
	}
	catch (gmsec::api::Exception e) {
		std::cout << e.what();
	}

	FString path = GetPathName();
	UE_LOG(LogTemp, Warning, TEXT("%s"), *path);
}

// Called every frame
void AGMSECActor::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	FVector NewLoc = GetActorScale3D();
	float DeltaHeight = (FMath::Sin(RunningTime + DeltaTime) - FMath::Sin(RunningTime));
	NewLoc.Z += DeltaHeight * 20.0f;
	RunningTime += DeltaTime;
	SetActorScale3D(NewLoc);
}

std::string AGMSECActor::connectionTypeToString(AGMSECActor::ConnectionTypes rawConnType) {
	std::string connType = "gmsec_bolt";

	switch (rawConnType) {
	case amq383:
		connType = "gmsec_activemq383";
		break;
	case amq384:
		connType = "gmsec_activemq384";
		break;
	case bolt:
		connType = "gmsec_bolt";
		break;
	case mb:
		connType = "gmsec_mb";
		break;
	case ws71:
		connType = "gmsec_websphere71";
		break;
	case ws75:
		connType = "gmsec_websphere75";
		break;
	case ws80:
		connType = "gmsec_websphere80";
		break;
	default:
		connType = "gmsec_bolt";
		break;
	}

	return connType;
}
