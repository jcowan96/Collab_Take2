// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SessionMasterNode.h"
#include "../Public/SynchronizationManager.h"
#include "../Public/SessionInformation.h"
#include "../Public/SynchronizedUser.h"

#include "Runtime/Engine/Classes/Engine/Engine.h"

//===================================================================================================================
//Constructors
//===================================================================================================================
// Sets default values for this component's properties
USessionMasterNode::USessionMasterNode()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	//Find cylinder and Sphere meshes to use as example for avatar and controllers
	static ConstructorHelpers::FObjectFinder<UStaticMesh> UserAsset(TEXT("StaticMesh'/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ControllerAsset(TEXT("StaticMesh'/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube'"));

	Avatar = UserAsset.Object;
	Avatar_Controller = ControllerAsset.Object;

}

USessionMasterNode::~USessionMasterNode()
{
	//Do Nothing
}

//===================================================================================================================
//Public Function Implementations
//===================================================================================================================

// Called when the game starts
void USessionMasterNode::BeginPlay()
{
	Super::BeginPlay();

	//Figure out what type of world this is, to only attach to components in that world
	EWorldType::Type wld = GetWorld()->WorldType;
	worldType = worldTypeToString(wld);
	
	//Find sessionAdvertiser in scene to get SessionInformation from (should be changed to user-supplied parameters passed in from a GUI)
	for (TObjectIterator<USessionAdvertiser> It; It; ++It)
	{
		TWeakObjectPtr<USessionAdvertiser> temp = *It;
		if (temp.IsValid())
		{
			FString path(temp->GetPathName());
			FString correctLevel("UEDPIE"); //May have to change this if not running in editor
			if (path.Find(correctLevel) != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] Found a valid SessionAdvertiser in the scene"));
				sessionAdvertiser = temp;
				break;
			}
		}
	}
	//Important to initialize this here to stop crashes
	currentSessionInfo.reset(new SessionInformation());
}


// Called every frame
void USessionMasterNode::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//Get sessionInformation from sessionAdvertiser
	if (sessionAdvertiser.IsValid() && sessionAdvertiser->initialized && !gotSessionInfo)
	{
		SessionInformation sessionInfo = *sessionAdvertiser->sessionInformation;
		currentSessionInfo->groupName = sessionAdvertiser->sessionInformation->groupName;
		currentSessionInfo->projectName = sessionAdvertiser->sessionInformation->projectName;
		currentSessionInfo->sessionName = sessionAdvertiser->sessionInformation->sessionName;
		gotSessionInfo = true;
		UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] Start Running"));

		time_t currentSecond = time(0); //current time, should be able to synchronize with slave node this way
		while (currentSecond >= 10)
		{
			currentSecond %= 10;
			UE_LOG(LogTemp, Warning, TEXT("current second: %d"), currentSecond);
		}
		std::string alias = "alias" + std::to_string(currentSecond);
		StartRunning(*sessionAdvertiser->sessionInformation, alias);
	}


	//Receive messages while subscribed to GMSEC bus
	if (subscribed)
	{
		receiveMessage();
	}
}


void USessionMasterNode::StartRunning(SessionInformation sessionInfo, std::string alias)
{
	//Set up Synchronized User as VR_Pawn controlled by ths user
	//Find HMD/Camera
	for (TObjectIterator<UCameraComponent> It; It; ++It)
	{
		camera = *It;
		if (camera.IsValid())
		{
			FString path(camera->GetPathName());
			FString cameraName = vrPawn + FString(TEXT(".VRCamera")); //VR Camera named this by UE4 convention

			if (path.Find(worldType) != -1 && path.Find(cameraName) != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: %s"), *FString(camera->GetPathName()));
				UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: %s"), *FString(camera->GetName()));
				UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: %s"), *FString(camera->GetOwner()->GetName()));
				break;
			}
		}
	}

	//Find UMotionControllers
	for (TObjectIterator<UMotionControllerComponent> It; It; ++It)
	{
		TWeakObjectPtr<UMotionControllerComponent> controller = *It;
		if (controller.IsValid())
		{
			FString path(controller->GetPathName());
			FString leftControllerName = vrPawn + FString(TEXT(".MC_Left"));  //UE4 Convention
			FString rightControllerName = vrPawn + FString(TEXT(".MC_Right"));  //UE4 Convention

			if (path.Find(worldType) != -1 && path.Find(leftControllerName) != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: %s"), *FString(controller->GetPathName()));
				UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: %s"), *FString(controller->GetName()));
				UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: %s"), *FString(controller->GetOwner()->GetName()));
				leftCon = controller;
			}
			else if (path.Find(worldType) != -1 && path.Find(rightControllerName) != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: %s"), *FString(controller->GetPathName()));
				UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: %s"), *FString(controller->GetName()));
				UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: %s"), *FString(controller->GetOwner()->GetName()));
				rightCon = controller;
			}
		}
	}

	//Create and register user, setting camera as its parent, if it has not already been added to the Pawn
	if (camera->GetOwner()->FindComponentByClass<USynchronizedUser>() == nullptr) //Does not already exist in Pawn
	{
		TWeakObjectPtr<USynchronizedUser> thisUser = NewObject<USynchronizedUser>(camera->GetOwner(), USynchronizedUser::StaticClass());
		thisUser->userObject = camera->GetOwner();
		UE_LOG(LogTemp, Warning, TEXT("[MasterNode] Registering User Here"));
		thisUser->RegisterComponent();

		//Set some properties of thisUser
		thisUser->userAlias = alias;
		thisUser->isControlled = true;

		//Test to make sure thisUser really is part of the camera
		TWeakObjectPtr<USynchronizedUser> usr = camera->GetOwner()->FindComponentByClass<USynchronizedUser>();
		if (usr.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[MasterNode] User Successfully initialized and attached to camera"));
			UE_LOG(LogTemp, Warning, TEXT("[MasterNode] User Name: %s"), *FString(usr->GetName()));
			UE_LOG(LogTemp, Warning, TEXT("[MasterNode] Camera Name: %s"), *FString(camera->GetName()));
			UE_LOG(LogTemp, Warning, TEXT("[MasterNode] Camera Path Name: %s"), *FString(camera->GetPathName()));

			FVector cameraLoc = usr->GetOwner()->GetTransform().GetLocation();

			UE_LOG(LogTemp, Warning, TEXT("[MasterNode] Camera X: %f"), cameraLoc.X);
			UE_LOG(LogTemp, Warning, TEXT("[MasterNode] Camera Y: %f"), cameraLoc.Y);
			UE_LOG(LogTemp, Warning, TEXT("[MasterNode] Camera Z: %f"), cameraLoc.Z);
		}
		//Test complete

		//Add SynchronizedControllers to the controllerObjects
		TWeakObjectPtr<USynchronizedController> leftController = NewObject<USynchronizedController>(leftCon->GetOwner(), USynchronizedController::StaticClass());
		TWeakObjectPtr<USynchronizedController> rightController = NewObject<USynchronizedController>(rightCon->GetOwner(), USynchronizedController::StaticClass());
		leftController->controllerSide = USynchronizedController::ControllerSide::Left;
		rightController->controllerSide = USynchronizedController::ControllerSide::Right;
		//leftController->Rename(*FString("MC_Left"));
		//rightController->Rename(*FString("MC_Right"));
		leftController->synchronizedUser = thisUser;
		rightController->synchronizedUser = thisUser;
		leftController->RegisterComponent();
		rightController->RegisterComponent();

		//Add thisUser to synchronizedUsers list
		synchronizedUsers.push_back(thisUser);
	}

	//Initialize GMSEC connection
	UE_LOG(LogTemp, Warning, TEXT("[SessionMasterNode] Setting up Config"));
	gmsec::api::Config cfg;
	cfg.addValue("connectionType", connectionTypeToString(connectionType).c_str());
	cfg.addValue("server", server.c_str());
	cfg.addValue("GMSEC-REQ-RESP", "OPEN-RESP"); //GMSEC Open Response Feature
	UE_LOG(LogTemp, Warning, TEXT("[SessionMasterNode] Config Initialized"));

	//Initialize Pointers to real objects
	connMgr.reset(new gmsec::api::mist::ConnectionManager(cfg));
	synchronizationManager = NewObject<USynchronizationManager>(GetOwner(), USynchronizationManager::StaticClass());

	//Initialize ConnectionManager
	UE_LOG(LogTemp, Warning, TEXT("[SessionMasterNode] Initializing Connection Manager"));
	connMgr->initialize();
	UE_LOG(LogTemp, Warning, TEXT("[SessionMasterNode] Connection Manager Initialized"));

	//Set up synchronization manager
	synchronizationManager->server = server;
	synchronizationManager->missionName = missionName;
	synchronizationManager->satName = satName;
	synchronizationManager->groupID = sessionInfo.groupName;
	synchronizationManager->projectName = sessionInfo.projectName;
	synchronizationManager->userName = alias;
	synchronizationManager->RegisterComponent();

	//Subscribe using SessionInfo
	currentSessionInfo->groupName = sessionInfo.groupName;
	currentSessionInfo->projectName = sessionInfo.projectName;
	currentSessionInfo->sessionName = sessionInfo.sessionName;
	std::string subscription = "GMSEC." + toUpper(missionName) + "." + toUpper(satName) + ".REQ.ESTATE."
		+ toUpper(currentSessionInfo->groupName) + "." + toUpper(currentSessionInfo->projectName) + ".*"; //Might have to replace '.mtproj' in projectName
	UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] Subscribed to: %s"), *FString(subscription.c_str()));
	connMgr->subscribe(subscription.c_str());
	subscribed = true;
}

void USessionMasterNode::sendEntityStateResponse(gmsec::api::Message msg)
{
	//Set Reply Subject
	std::string rplSubject = "GMSEC." + toUpper(missionName) + "." + toUpper(satName) + ".RESP.ESTATE."
		+ toUpper(currentSessionInfo->groupName) + "." + toUpper(currentSessionInfo->projectName); //Might have to replace '.mtproj' in projectName
	UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] Reply Subject: %s"), *FString(rplSubject.c_str()));

	//Create Reply Message
	gmsec::api::Message reply(rplSubject.c_str(), gmsec::api::Message::MessageKind::REPLY);
	reply.addField("HEADER-VERSION", (GMSEC_F32)2010);
	reply.addField("MESSAGE-TYPE", "RESP");
	reply.addField("MESSAGE-SUBTYPE", "ESTATE");
	reply.addField("CONTENT-VERSION", (GMSEC_F32)2018);
	reply.addField("RESPONSE-STATUS", (GMSEC_I16)1);
	reply.addField("NUM-USERS", (GMSEC_I16)synchronizedUsers.size());

	//Add parameters from synchronizedUsers to message
	for (int i = 0; i < synchronizedUsers.size(); i++)
	{
		std::string uName = "USER." + std::to_string(i) + ".NAME";
		std::string uAvatar = "USER." + std::to_string(i) + ".AVATAR";
		reply.addField(uName.c_str(), synchronizedUsers[i]->userAlias.c_str());
		reply.addField(uAvatar.c_str(), synchronizedUsers[i]->avatarName.c_str());
		UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] %s: %s"), *FString(uName.c_str()), *FString(synchronizedUsers[i]->userAlias.c_str()));
		UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] %s: %s"), *FString(uAvatar.c_str()), *FString(synchronizedUsers[i]->avatarName.c_str()));

		FVector userPosition = synchronizedUsers[i]->GetOwner()->GetTransform().GetTranslation();
		std::string uPos = "USER." + std::to_string(i) + ".POSITION";
		std::string uPosField = std::to_string(userPosition.X) + "/" + std::to_string(userPosition.Y) + "/" + std::to_string(userPosition.Z);
		reply.addField(uPos.c_str(), uPosField.c_str());
		UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] %s: %s"), *FString(uPos.c_str()), *FString(uPosField.c_str()));

		FQuat userRotation = synchronizedUsers[i]->GetOwner()->GetTransform().GetRotation();
		std::string uRot = "USER." + std::to_string(i) + ".ROTATION";
		std::string uRotField = std::to_string(userRotation.X) + "/" + std::to_string(userRotation.Y) + "/" + std::to_string(userRotation.Z) + "/" + std::to_string(userRotation.W);
		reply.addField(uRot.c_str(), uRotField.c_str());
		UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] %s: %s"), *FString(uRot.c_str()), *FString(uRotField.c_str()));

		FVector userScale = synchronizedUsers[i]->GetOwner()->GetTransform().GetScale3D();
		std::string uScl = "USER." + std::to_string(i) + ".SCALE";
		std::string uSclField = std::to_string(userScale.X) + "/" + std::to_string(userScale.Y) + "/" + std::to_string(userScale.Z);
		reply.addField(uScl.c_str(), uSclField.c_str());
		UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] %s: %s"), *FString(uScl.c_str()), *FString(uSclField.c_str()));
	}

	//Send Reply
	connMgr->reply(msg, reply);
	UE_LOG(LogTemp, Warning, TEXT("[SessionMasterNode] replied to entityStateRequest with EntityStateResponse"));

	//==================================================================================
	//Spawn User
	//==================================================================================
	UE_LOG(LogTemp, Warning, TEXT("[SessionMasterNode] Starting to spawn Avatar here"));
	//Set up some variables
	UWorld* World = GetWorld();
	std::string newUserName = msg.getStringField("TOKEN-INFO").getValue(); //Read from msg
	std::string avatarName = newUserName == "" ? "New_User" : newUserName;

	FActorSpawnParameters spawnParams;
	spawnParams.Name = FName(avatarName.c_str());

	//Spawn Static Mesh Actor for User
	TWeakObjectPtr<AStaticMeshActor> newUserAvatar = World->SpawnActor<AStaticMeshActor>(spawnParams);
	newUserAvatar->SetMobility(EComponentMobility::Type::Movable);
	newUserAvatar->GetStaticMeshComponent()->SetStaticMesh(Avatar);

	//Create dummy UCameraComponent to satisfy USynchronizedUser
	TWeakObjectPtr<UCameraComponent> newCamera = NewObject<UCameraComponent>(newUserAvatar.Get(), UCameraComponent::StaticClass());
	newCamera->RegisterComponent();
	UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: newCamera Path Name: %s"), *newCamera->GetPathName());

	//Create a new SynchronizedUser with newUserAvatar as the parent, and set some properties
	TWeakObjectPtr<USynchronizedUser> newUser = NewObject<USynchronizedUser>(newUserAvatar.Get(), USynchronizedUser::StaticClass());
	newUser->avatarName = avatarName;
	newUser->userObject = newUserAvatar;
	newUser->Rename(*FString(avatarName.c_str()));
	newUser->RegisterComponent();
	UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: newUser Path Name: %s"), *newUser->GetPathName());
	UE_LOG(LogTemp, Warning, TEXT("[SessionMasterNode] Finished Spawning Avatar Here"));

	//==================================================================================
	//Spawn Controllers
	//==================================================================================
	UE_LOG(LogTemp, Warning, TEXT("[SessionMasterNode] Starting to spawn Controllers here"));
	//Set up Controller-Specific Variables
	FActorSpawnParameters leftParams;
	std::string leftName = avatarName + ".MC_Left"; //Kinda hacky but it works
	leftParams.Name = FName(leftName.c_str()); //UE4 Convention
	leftParams.Owner = newUserAvatar.Get();

	FActorSpawnParameters rightParams;
	std::string rightName = avatarName + ".MC_Right"; //Kinda hacky but it works
	rightParams.Name = FName(rightName.c_str()); //UE4 Convention
	rightParams.Owner = newUserAvatar.Get();

	//Spawn Static Meshes for Controllers
	TWeakObjectPtr<AStaticMeshActor> newLeftController = World->SpawnActor<AStaticMeshActor>(leftParams);
	newLeftController->SetMobility(EComponentMobility::Type::Movable);
	newLeftController->GetStaticMeshComponent()->SetStaticMesh(Avatar_Controller);
	newLeftController->SetActorLocation(FVector(0.0, 100.0, 0.0));
	newLeftController->SetActorScale3D(FVector(0.25, 0.25, 0.25));
	UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: newLeftController Owner: %s"), *newLeftController->GetOwner()->GetPathName());

	TWeakObjectPtr<AStaticMeshActor> newRightController = World->SpawnActor<AStaticMeshActor>(rightParams);
	newRightController->SetMobility(EComponentMobility::Type::Movable);
	newRightController->GetStaticMeshComponent()->SetStaticMesh(Avatar_Controller);
	newRightController->SetActorLocation(FVector(0.0, -100.0, 0.0));
	newRightController->SetActorScale3D(FVector(0.25, 0.25, 0.25));
	UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: newRightController Owner: %s"), *newRightController->GetOwner()->GetPathName());

	//Set up dummy UMotionControllerComponents to satisfy USynchronizedControllers
	TWeakObjectPtr<UMotionControllerComponent> newLeftComponent = NewObject<UMotionControllerComponent>(newLeftController.Get(), UMotionControllerComponent::StaticClass());
	newLeftComponent->RegisterComponent();
	UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: newLeftComponent Path Name: %s"), *newLeftComponent->GetPathName());

	TWeakObjectPtr<UMotionControllerComponent> newRightComponent = NewObject<UMotionControllerComponent>(newRightController.Get(), UMotionControllerComponent::StaticClass());
	newRightComponent->RegisterComponent();
	UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: newRightComponent Path Name: %s"), *newRightComponent->GetPathName());

	//Create new SynchronizedControllers with Meshes as the parents, and set some properties
	TWeakObjectPtr<USynchronizedController> leftControllerComponent = NewObject<USynchronizedController>(newLeftController.Get(), USynchronizedController::StaticClass());
	leftControllerComponent->controllerSide = USynchronizedController::ControllerSide::Left;
	leftControllerComponent->Rename(*FString("MC_Left")); //UE4 Convention
	leftControllerComponent->synchronizedUser = newUser;
	leftControllerComponent->RegisterComponent();
	UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: leftControllerComponent Path Name: %s"), *leftControllerComponent->GetPathName());

	TWeakObjectPtr<USynchronizedController> rightControllerComponent = NewObject<USynchronizedController>(newRightController.Get(), USynchronizedController::StaticClass());
	rightControllerComponent->controllerSide = USynchronizedController::ControllerSide::Right;
	rightControllerComponent->Rename(*FString("MC_Right")); //UE4 Convention
	rightControllerComponent->synchronizedUser = newUser;
	rightControllerComponent->RegisterComponent();
	UE_LOG(LogTemp, Warning, TEXT("[MASTERNODE]: rightControllerComponent Path Name: %s"), *rightControllerComponent->GetPathName());

	UE_LOG(LogTemp, Warning, TEXT("[SessionMasterNode] Finished Spawning Controllers Here"));
	//==================================================================================
	//Spawn Finished
	//==================================================================================

	//Add the newUser to vector of synchronizedUsers
	synchronizedUsers.push_back(newUser);

	//Output log of all synchronizedUsers in vector
	for (int i = 0; i < synchronizedUsers.size(); i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("[MasterNode] synchronizedUser name: %s"), *FString(synchronizedUsers[i]->GetName()));
	}
}

void USessionMasterNode::receiveMessage()
{
	gmsec::api::Message *msg = connMgr->receive(0);
	if (msg != NULL)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] Received Non-Null entity request message"));
		if (msg->getKind() == gmsec::api::Message::MessageKind::REQUEST)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SessionMaster] Sending Entity State Response"));
			sendEntityStateResponse(*msg);
		}
	}
}

//===================================================================================================================
//Private Function Implementations
//===================================================================================================================
std::string USessionMasterNode::connectionTypeToString(USessionMasterNode::ConnectionTypes rawConnType) {
	std::string connType = "gmsec_bolt";

	switch (rawConnType) {
	case amq383:
		connType = "gmsec_amq383";
		break;
	case amq384:
		connType = "gmsec_amq384";
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

std::string USessionMasterNode::toUpper(std::string str) {
	std::string ret = "";
	std::locale loc;
	for (char& c : str) {
		c = std::toupper(c, loc);
		ret += c;
	}

	return ret;
}

//Could update this for other worldTypes if relevant
FString USessionMasterNode::worldTypeToString(EWorldType::Type world)
{
	if (world == EWorldType::Type::PIE)
	{
		return FString(TEXT("UEDPIE"));
	}
	else 
	{
		return FString(TEXT(""));
	}
}
