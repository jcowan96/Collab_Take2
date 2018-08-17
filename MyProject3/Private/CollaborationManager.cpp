// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/CollaborationManager.h"
//#include "SessionAdvertiser.h"

//===================================================================================================================
//Constructors/Destructors
//===================================================================================================================
// Sets default values for this component's properties
UCollaborationManager::UCollaborationManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}

UCollaborationManager::~UCollaborationManager()
{
	//Do Nothing
}

//===================================================================================================================
//Public Method Implementations
//===================================================================================================================
// Called when the game starts
void UCollaborationManager::BeginPlay()
{
	Super::BeginPlay();

	//Initialize pointers
	synchManager = NewObject<USynchronizationManager>(this->GetOwner(), USynchronizationManager::StaticClass());
	masterNode = NewObject<USessionMasterNode>(this->GetOwner(), USessionMasterNode::StaticClass());
	slaveNode = NewObject<USessionSlaveNode>(this->GetOwner(), USessionSlaveNode::StaticClass());

}


// Called every frame
void UCollaborationManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// ...
}

void UCollaborationManager::enterMasterMode(SessionInformation sessionInfo, std::string alias)
{
	masterNode->StartRunning(sessionInfo, alias);

	sessionAdvertiser = NewObject<USessionAdvertiser>();

	synchManager->sessionAdvertiser = sessionAdvertiser;
	synchManager->sessionMasterNode = masterNode;
	sessionAdvertiser->sessionInformation.reset(&sessionInfo); //Reset sessionInformation to pointer pointing to sessionInfo
	sessionAdvertiser->connectionType = (USessionAdvertiser::ConnectionTypes)connectionType; //Cast between basically equal values
	sessionAdvertiser->server = server;
	sessionAdvertiser->missionName = missionName;
	sessionAdvertiser->satName = satName;
	sessionAdvertiser->RegisterComponent();
	//sessionAdvertiser->Initialize();
}

void UCollaborationManager::enterSlaveMode(SessionInformation sessionInfo, std::string alias)
{
	sessionAdvertiser->DestroyComponent();

	slaveNode->connect(sessionInfo, alias);
}
