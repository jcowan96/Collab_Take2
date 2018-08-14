// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SessionSlaveNode.h"
#include "../Public/SynchronizationManager.h"
#include "../Public/SessionInformation.h"


//===================================================================================================================
//Constructors
//===================================================================================================================
// Sets default values for this component's properties
USessionSlaveNode::USessionSlaveNode()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	//Find cylinder and Sphere meshes to use as example for avatar and controllers
	static ConstructorHelpers::FObjectFinder<UStaticMesh> UserAsset(TEXT("StaticMesh'/Game/StarterContent/Shapes/Shape_Cylinder.Shape_Cylinder'"));
	static ConstructorHelpers::FObjectFinder<UStaticMesh> ControllerAsset(TEXT("StaticMesh'/Game/StarterContent/Shapes/Shape_Cube.Shape_Cube'"));
	static ConstructorHelpers::FObjectFinder<UBlueprint> VR_Pawn(TEXT("Blueprint'/Game/VR_Pawn.VR_Pawn'"));

	Avatar = UserAsset.Object;
	Avatar_Controller = ControllerAsset.Object;
}

USessionSlaveNode::~USessionSlaveNode()
{
	//Shutdown messaging thread
	FSlaveWorker::Shutdown();

	//Unitialize Pointers
	synchronizationManager.Reset();
	synchronizationManager = NULL;
	connMgr.release();

}


//===================================================================================================================
//Public Function Implementation
//===================================================================================================================

// Called when the game starts
void USessionSlaveNode::BeginPlay()
{
	Super::BeginPlay();

	//Find sessionSeeker in scene to get SessionInformation from (should be changed to user-supplied parameters passed in from a GUI)
	for (TObjectIterator<USessionSeeker> It; It; ++It)
	{
		TWeakObjectPtr<USessionSeeker> temp = *It;
		if (temp.IsValid())
		{
			FString path(temp->GetPathName());
			FString correctLevel("UEDPIE"); //May have to change this if not running in editor
			if (path.Find(correctLevel) != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SessionSlave] Found a valid SessionSeeker in the scene"));
				sessionSeeker = temp;
				break;
			}
		}
	}
}


// Called every frame
void USessionSlaveNode::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	//If the request to master node has not yet been sent, send it
	if (sessionSeeker.IsValid() && !requestSent && sessionSeeker->availableSessions.size() > 0)
	{
		time_t currentSecond = time(0); //current time, should be able to synchronize with master node this way
		while (currentSecond >= 10)
		{
			currentSecond %= 10;
			UE_LOG(LogTemp, Warning, TEXT("current second: %d"), currentSecond);
		}
		std::string alias = "alias" + std::to_string(currentSecond);
		connect(sessionSeeker->availableSessions[0], alias);
		requestSent = true;
	}

	//reply exists, it's not the dummy instantiation, and it's actually a new message
	if (reply && std::strcmp("TEST", reply->getSubject()) != 0 && newResponse)
	{
		newResponse = false;
		UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode]: %s Reply is not null!"), *FString(reply->getSubject()));
		processEntityResponse(*reply);
	}
}

void USessionSlaveNode::connect(SessionInformation sessionInfo, std::string alias)
{
	//Find HMD/Camera
	for (TObjectIterator<UCameraComponent> It; It; ++It)
	{
		camera = *It;
		if (camera.IsValid())
		{
			FString path(camera->GetPathName());
			FString correctLevel("UEDPIE"); //May have to change this if not running in editor
			FString cameraName("VR_Pawn_2.VRCamera"); //VR Camera named this by UE4 convention
			if (path.Find(correctLevel) != -1 && path.Find(cameraName) != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *FString(camera->GetPathName()));
				UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *FString(camera->GetName()));
				UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *FString(camera->GetOwner()->GetName()));
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
			FString correctLevel("UEDPIE"); //May have to change this if not running in editor
			FString leftControllerName("VR_Pawn_2.MC_Left"); //UE4 Convention
			FString rightControllerName("VR_Pawn_2.MC_Right"); //UE4 Convention

			if (path.Find(correctLevel) != -1 && path.Find(leftControllerName) != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *FString(controller->GetPathName()));
				UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *FString(controller->GetName()));
				UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *FString(controller->GetOwner()->GetName()));
				leftCon = controller;
			}
			else if (path.Find(correctLevel) != -1 && path.Find(rightControllerName) != -1)
			{
				UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *FString(controller->GetPathName()));
				UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *FString(controller->GetName()));
				UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *FString(controller->GetOwner()->GetName()));
				rightCon = controller;
			}
		}
	}

	//Create and register user, setting camera as its parent, if it has not already been added to the Pawn
	if (camera->GetOwner()->FindComponentByClass<USynchronizedUser>() == nullptr) //Does not already exist in Pawn
	{
		TWeakObjectPtr<USynchronizedUser> thisUser = NewObject<USynchronizedUser>(camera->GetOwner(), USynchronizedUser::StaticClass());
		thisUser->userObject = camera->GetOwner();
		UE_LOG(LogTemp, Warning, TEXT("[SlaveNode] Registering User Here"));
		thisUser->RegisterComponent();

		//Set some properties of thisUser
		thisUser->userAlias = alias;
		thisUser->isControlled = true;

		//Test to make sure thisUser really is part of the camera
		TWeakObjectPtr<USynchronizedUser> usr = camera->GetOwner()->FindComponentByClass<USynchronizedUser>();
		if (usr.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[SlaveNode] User Successfully initialized and attached to camera"));
			UE_LOG(LogTemp, Warning, TEXT("[SlaveNode] User Name: %s"), *FString(usr->GetName()));
			UE_LOG(LogTemp, Warning, TEXT("[SlaveNode] Camera Name: %s"), *FString(camera->GetName()));
			UE_LOG(LogTemp, Warning, TEXT("[SlaveNode] Camera Path Name: %s"), *FString(camera->GetPathName()));

			FVector cameraLoc = usr->GetOwner()->GetTransform().GetLocation();

			UE_LOG(LogTemp, Warning, TEXT("[SlaveNode] Camera X: %f"), cameraLoc.X);
			UE_LOG(LogTemp, Warning, TEXT("[SlaveNode] Camera Y: %f"), cameraLoc.Y);
			UE_LOG(LogTemp, Warning, TEXT("[SlaveNode] Camera Z: %f"), cameraLoc.Z);
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
	}

	//Create Config
	gmsec::api::Config cfg;
	cfg.addValue("connectionType", connectionTypeToString(connectionType).c_str());
	cfg.addValue("server", server.c_str());
	cfg.addValue("GMSEC-REQ-RESP", "OPEN-RESP"); //GMSEC Open Response Feature

	//Initialize Pointers to real objects
	connMgr.reset(new gmsec::api::mist::ConnectionManager(cfg));
	reply.reset(new gmsec::api::Message("TEST", gmsec::api::Message::MessageKind::REPLY));
	synchronizationManager = NewObject<USynchronizationManager>(GetOwner(), USynchronizationManager::StaticClass());

	//Initialize ConnectionManager
	UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode] Initializing Connection Manager"));
	connMgr->initialize();
	UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode] Connection Manager Initialized"));

	//ConnectionManager Subscribe
	std::string subscriptionSubject = "GMSEC." + missionName + "." + satName + ".RESP.ESTATE."
		+ sessionInfo.groupName + "." + sessionInfo.projectName; //Fix this * to alias
	connMgr->subscribe(subscriptionSubject.c_str());

	//Set up synchronization manager
	synchronizationManager->server = server;
	synchronizationManager->missionName = missionName;
	synchronizationManager->satName = satName;
	synchronizationManager->groupID = sessionInfo.groupName;
	synchronizationManager->projectName = sessionInfo.projectName;
	synchronizationManager->userName = alias;
	synchronizationManager->RegisterComponent();

	//Create entity request message
	std::string subString = "GMSEC." + missionName + "." + satName + ".REQ.ESTATE." + sessionInfo.groupName + "." + sessionInfo.projectName + "." + toUpper(alias);
	gmsec::api::Message msg(subString.c_str(), gmsec::api::Message::MessageKind::REQUEST);
	msg.addField("HEADER-VERSION", (GMSEC_F32)2010);
	msg.addField("MESSAGE-TYPE", "REQ");
	msg.addField("MESSAGE-SUBTYPE", "ESTATE");
	msg.addField("CONTENT-VERSION", (GMSEC_F32)2018);
	msg.addField("REQUEST-TYPE", (GMSEC_I16)1);
	msg.addField("SESSION-NAME", sessionInfo.sessionName.c_str());
	msg.addField("PROJECT-NAME", sessionInfo.projectName.c_str());
	msg.addField("GROUP-NAME", sessionInfo.groupName.c_str());
	msg.addField("TOKEN-INFO", alias.c_str());

	//Start thread to send entity request to master
	FSlaveWorker::JoyInit(*connMgr, msg, *reply, newResponse); 
	UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode] Node Initialized"));
}

//===================================================================================================================
//Private Function Implementations
//===================================================================================================================

void USessionSlaveNode::processEntityResponse(gmsec::api::Message response)
{
	int numUsers = response.getI16Field("NUM-USERS").getValue();
	UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode] %d Users"), numUsers);
	for (int i = 0; i < numUsers; i++)
	{
		//Create strings to access message fields
		std::string nameField = "USER." + std::to_string(i) + ".NAME";
		std::string avatarField = "USER." + std::to_string(i) + ".AVATAR";
		std::string posField = "USER." + std::to_string(i) + ".POSITION";
		std::string rotField = "USER." + std::to_string(i) + ".ROTATION";
		std::string sclField = "USER." + std::to_string(i) + ".SCALE";

		std::string newUserAvatar = response.getStringField(avatarField.c_str()).getValue();

		//==================================================================================
		//Spawn User
		//==================================================================================
		UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode] Starting to spawn Avatar here"));
		//Set up some variables
		UWorld* World = GetWorld();
		std::string newUserName = response.getStringField(nameField.c_str()).getValue();
		std::string avatarName = newUserName == "" ? "New_User" : newUserName;

		FActorSpawnParameters spawnParams;
		spawnParams.Name = FName(newUserAvatar.c_str());

		//Spawn Static Mesh Avatar for User	
		TWeakObjectPtr<AStaticMeshActor> newUserObject = World->SpawnActor<AStaticMeshActor>(spawnParams);
		newUserObject->SetMobility(EComponentMobility::Type::Movable);
		newUserObject->GetStaticMeshComponent()->SetStaticMesh(Avatar);

		//Create dummy UCameraComponent to satisfy UsynchronizedUser
		TWeakObjectPtr<UCameraComponent> newCamera = NewObject<UCameraComponent>(newUserObject.Get(), UCameraComponent::StaticClass());
		newCamera->RegisterComponent();
		UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: newCamera Path Name: %s"), *newCamera->GetPathName());

		//Create a new SynchronizedUser with newUserAvatar as the parent, and set some properties
		TWeakObjectPtr<USynchronizedUser> newUser = NewObject<USynchronizedUser>(newUserObject.Get(), USynchronizedUser::StaticClass());
		newUser->userObject = newUserObject;
		newUser->userAlias = newUserName;
		newUser->userObject->Rename(*FString(newUserName.c_str()));
		//newUser->avatarName = avatarName;
		newUser->Rename(*FString(avatarName.c_str()));
		newUser->RegisterComponent();
		UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: %s"), *newUser->GetPathName());

		UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode] Finished Spawning Avatar here"));

		//==================================================================================
		//Spawn Controllers
		//==================================================================================
		UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode] Starting to spawn Controllers here"));
		//Set up Controller-Specific Variables
		FActorSpawnParameters leftParams;
		std::string leftName = avatarName + ".MC_Left"; //Kinda hacky but it works
		leftParams.Name = FName(leftName.c_str()); //UE4 Convention
		leftParams.Owner = newUserObject.Get();

		FActorSpawnParameters rightParams;
		std::string rightName = avatarName + ".MC_Right"; //Kinda hacky but it works
		rightParams.Name = FName(rightName.c_str()); //UE4 Convention
		rightParams.Owner = newUserObject.Get();

		//Spawn Static Meshes for Controllers
		TWeakObjectPtr<AStaticMeshActor> newLeftController = World->SpawnActor<AStaticMeshActor>(leftParams);
		newLeftController->SetMobility(EComponentMobility::Type::Movable);
		newLeftController->GetStaticMeshComponent()->SetStaticMesh(Avatar_Controller);
		newLeftController->SetActorLocation(FVector(0.0, 100.0, 0.0));
		newLeftController->SetActorScale3D(FVector(0.25, 0.25, 0.25));
		UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: newLeftController Owner: %s"), *newLeftController->GetOwner()->GetPathName());

		TWeakObjectPtr<AStaticMeshActor> newRightController = World->SpawnActor<AStaticMeshActor>(rightParams);
		newRightController->SetMobility(EComponentMobility::Type::Movable);
		newRightController->GetStaticMeshComponent()->SetStaticMesh(Avatar_Controller);
		newRightController->SetActorLocation(FVector(0.0, -100.0, 0.0));
		newRightController->SetActorScale3D(FVector(0.25, 0.25, 0.25));
		UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: newRightController Owner: %s"), *newRightController->GetOwner()->GetPathName());

		//Set up dummy UMotionControllerComponents to satisfy USynchronizedControllers
		TWeakObjectPtr<UMotionControllerComponent> newLeftComponent = NewObject<UMotionControllerComponent>(newLeftController.Get(), UMotionControllerComponent::StaticClass());
		newLeftComponent->RegisterComponent();
		UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: newLeftComponent Path Name: %s"), *newLeftComponent->GetPathName());

		TWeakObjectPtr<UMotionControllerComponent> newRightComponent = NewObject<UMotionControllerComponent>(newRightController.Get(), UMotionControllerComponent::StaticClass());
		newRightComponent->RegisterComponent();
		UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: newRightComponent Path Name: %s"), *newRightComponent->GetPathName());

		//Create new SynchronizedControllers with Meshes as the parents, and set some properties
		TWeakObjectPtr<USynchronizedController> leftControllerComponent = NewObject<USynchronizedController>(newLeftController.Get(), USynchronizedController::StaticClass());
		leftControllerComponent->controllerSide = USynchronizedController::ControllerSide::Left;
		leftControllerComponent->Rename(*FString("MC_Left")); //UE4 Convention
		leftControllerComponent->synchronizedUser = newUser;
		leftControllerComponent->RegisterComponent();
		UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: leftControllerComponent Path Name: %s"), *leftControllerComponent->GetPathName());

		TWeakObjectPtr<USynchronizedController> rightControllerComponent = NewObject<USynchronizedController>(newRightController.Get(), USynchronizedController::StaticClass());
		rightControllerComponent->controllerSide = USynchronizedController::ControllerSide::Right;
		rightControllerComponent->Rename(*FString("MC_Right")); //UE4 Convention
		rightControllerComponent->synchronizedUser = newUser;
		rightControllerComponent->RegisterComponent();
		UE_LOG(LogTemp, Warning, TEXT("[SLAVENODE]: rightControllerComponent Path Name: %s"), *rightControllerComponent->GetPathName());

		UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode] Finished Spawning Controllers Here"));
		//==================================================================================
		//Spawn Finished
		//==================================================================================


		//Set position, location, and scale of newUser
		std::vector<std::string> posComponents = split(response.getStringField(posField.c_str()).getValue(), '/');
		float posX = std::stof(posComponents[0]);
		float posY = std::stof(posComponents[1]);
		float posZ = std::stof(posComponents[2]);
		FVector pos(posX, posY, posZ);
		newUser->userObject->SetActorLocation(pos);

		std::vector<std::string> rotComponents = split(response.getStringField(rotField.c_str()).getValue(), '/');
		float rotX = std::stof(rotComponents[0]);
		float rotY = std::stof(rotComponents[1]);
		float rotZ = std::stof(rotComponents[2]);
		float rotW = std::stof(rotComponents[3]);
		FQuat rot(rotX, rotY, rotZ, rotW);
		newUser->userObject->SetActorRotation(rot);

		std::vector<std::string> sclComponents = split(response.getStringField(sclField.c_str()).getValue(), '/');
		float sclX = std::stof(sclComponents[0]);
		float sclY = std::stof(sclComponents[1]);
		float sclZ = std::stof(sclComponents[2]);
		FVector scl(sclX, sclY, sclZ);
		newUser->userObject->SetActorScale3D(scl);
	}
}

//===================================================================================================================
//Private Function Implementations
//===================================================================================================================
std::string USessionSlaveNode::connectionTypeToString(USessionSlaveNode::ConnectionTypes rawConnType) {
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

std::vector<std::string> USessionSlaveNode::split(const std::string &text, char sep)
{
	std::vector<std::string> tokens;
	std::size_t start = 0, end = 0;
	while ((end = text.find(sep, start)) != std::string::npos)
	{
		tokens.push_back(text.substr(start, end - start));
		start = end + 1;
	}
	tokens.push_back(text.substr(start));
	return tokens;
}

std::string USessionSlaveNode::toUpper(std::string str) {
	std::string ret = "";
	std::locale loc;
	for (char& c : str) {
		c = std::toupper(c, loc);
		ret += c;
	}

	return ret;
}