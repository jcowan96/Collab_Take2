// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SynchronizationManager.h"
#include "../Public/SessionAdvertiser.h"
#include "../Public/SessionMasterNode.h"
#include "../Public/SynchronizedUser.h"
#include "../Public/SynchronizedObject.h"
#include "../Public/SynchronizedController.h"

//===================================================================================================================
//Constructors
//===================================================================================================================
// Sets default values for this component's properties
USynchronizationManager::USynchronizationManager()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

}

USynchronizationManager::~USynchronizationManager()
{
	//Do Nothing
}

//==================================================================================================================
//Public Function Definitions
//==================================================================================================================
// Called when the game starts
void USynchronizationManager::BeginPlay()
{
	Super::BeginPlay();

	initialize();
}

void USynchronizationManager::initialize()
{
	//Set up Config
	gmsec::api::Config cfg;
	cfg.addValue("connectionType", connectionTypeToString(connectionType).c_str());
	cfg.addValue("server", server.c_str());
	cfg.addValue("GMSEC-REQ-RESP", "OPEN-RESP"); //GMSEC Open Response Feature
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Config Initialized"));

	//Initialize pointers to real objects
	connMgr.reset(new gmsec::api::mist::ConnectionManager(cfg));

	//Find sessionAdvertiser and masterNode if they are attached to the same object CHANGE THIS TO LOOK IN OVERALL SCENE
	for (TObjectIterator<USessionAdvertiser> It; It; ++It)
	{
		TWeakObjectPtr<USessionAdvertiser> temp = *It;

		if (temp.IsValid())
		{
			sessionAdvertiser = temp;
			break; //Found the right component, no need to waste more time iterating through components
		}

	}
	for (TObjectIterator<USessionMasterNode> It; It; ++It)
	{
		TWeakObjectPtr<USessionMasterNode> temp = *It;

		if (temp.IsValid())
		{
			sessionMasterNode = temp;
			break; //Found the right component, no need to waste more time iterating through components
		}

	}

	//Initialize ConnectionManager
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Initializing Connection Manager"));
	connMgr->initialize();
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Connection Manager Initialized"));

	//Subscribe to Subscription Subject
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Subscribing"));
	std::string subject = "GMSEC." + toUpper(missionName) + "." + toUpper(satName)
		+ ".MSG.ESTATE." + toUpper(groupID) + "." + toUpper(projectName) + ".*.*";
	connMgr->subscribe(subject.c_str());
	subscribed = true;
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Subscribed"));

	//Test Message
	gmsec::api::Message msg("GMSEC.TEST.PUBLISH", gmsec::api::Message::MessageKind::PUBLISH);
	msg.addField("SYNC-MANAGER", "Sync manager successfully initialized!!");
	connMgr->publish(msg);
	//End Test Message

	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(server.c_str()));
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(missionName.c_str()));
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(satName.c_str()));
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(groupID.c_str()));
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(projectName.c_str()));
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(userName.c_str()));
}

// Called every frame
void USynchronizationManager::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (subscribed)
	{
		receiveMessage();
	}
}

void USynchronizationManager::sendPositionChange(UActorComponent* synchronizedObject)
{
	FString getName = synchronizedObject->GetName();
	std::string nameToUse = toUpper(std::string(TCHAR_TO_UTF8(*getName)));
	std::string pathToUse = std::string(TCHAR_TO_UTF8(*synchronizedObject->GetPathName()));

	AActor* owner = synchronizedObject->GetOwner();
	FTransform transformToUse = owner->GetTransform();

	if (synchronizedObject->IsA(USynchronizedUser::StaticClass()))
	{
		USynchronizedUser* user = Cast<USynchronizedUser>(synchronizedObject);
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias) + "." + (user->userAlias);

		transformToUse = user->VRCamera->GetComponentTransform(); //If user, send transform of CameraComponent, not the owner itself
	}
	else if (synchronizedObject->IsA(USynchronizedController::StaticClass()))
	{
		USynchronizedController* controller = Cast<USynchronizedController>(synchronizedObject);
		TWeakObjectPtr<USynchronizedUser> user = controller->synchronizedUser;
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias);
		pathToUse += (controller->controllerSide == USynchronizedController::ControllerSide::Left ? ".MC_Left.MotionControllerComponent_0" : ".MC_Right.MotionControllerComponent_0"); //Name of MCC is hardcoded

		transformToUse = controller->VRController->GetComponentTransform();
	}

	//Write replace method to replace " " with "-" in nameToUse
	time_t currentTime = time(0);
	std::string subject = "GMSEC." + toUpper(missionName) + "." + toUpper(satName) + ".MSG.ESTATE."
		+ toUpper(groupID) + "." + toUpper(projectName) + "." + nameToUse + "." + toUpper(userName); //Might have to replace '.mtproj' in projectName as well, TBD

	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Subject: %s"), *FString(subject.c_str()));

	publishStateAttributeDataMessage(subject, EntityStateAttributeTypes::absolutePosition, pathToUse, currentTime, transformToUse);
}

void USynchronizationManager::sendPositionChange(UActorComponent* synchronizedObject, time_t occurenceTime)
{
	FString getName = synchronizedObject->GetName();
	std::string nameToUse = toUpper(std::string(TCHAR_TO_UTF8(*getName)));
	std::string pathToUse = std::string(TCHAR_TO_UTF8(*synchronizedObject->GetPathName()));

	AActor* owner = synchronizedObject->GetOwner();
	FTransform transformToUse = owner->GetTransform();

	if (synchronizedObject->IsA(USynchronizedUser::StaticClass()))
	{
		USynchronizedUser* user = Cast<USynchronizedUser>(synchronizedObject);
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias) + "." + (user->userAlias);

		transformToUse = user->VRCamera->GetComponentTransform(); //If user, send transform of CameraComponent, not the owner itself
	}
	else if (synchronizedObject->IsA(USynchronizedController::StaticClass()))
	{
		USynchronizedController* controller = Cast<USynchronizedController>(synchronizedObject);
		TWeakObjectPtr<USynchronizedUser> user = controller->synchronizedUser;
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias);
		pathToUse += (controller->controllerSide == USynchronizedController::ControllerSide::Left ? ".MC_Left.MotionControllerComponent_0" : ".MC_Right.MotionControllerComponent_0"); //Name of MCC is hardcoded

		transformToUse = controller->VRController->GetComponentTransform();
	}

	std::string subject = "GMSEC." + toUpper(missionName) + "." + toUpper(satName) + ".MSG.ESTATE."
		+ toUpper(groupID) + "." + toUpper(projectName) + "." + nameToUse + "." + toUpper(userName); //Might have to replace '.mtproj' in projectName as well, TBD

	publishStateAttributeDataMessage(subject, EntityStateAttributeTypes::absolutePosition, pathToUse, occurenceTime, transformToUse);
}

void USynchronizationManager::sendRotationChange(UActorComponent* synchronizedObject)
{
	FString getName = synchronizedObject->GetName();
	std::string nameToUse = toUpper(std::string(TCHAR_TO_UTF8(*getName)));
	std::string pathToUse = std::string(TCHAR_TO_UTF8(*synchronizedObject->GetPathName()));

	AActor* owner = synchronizedObject->GetOwner();
	FTransform transformToUse = owner->GetTransform();

	if (synchronizedObject->IsA(USynchronizedUser::StaticClass()))
	{
		USynchronizedUser* user = Cast<USynchronizedUser>(synchronizedObject);
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias) + "." + (user->userAlias);

		transformToUse = user->VRCamera->GetComponentTransform(); //If user, send transform of CameraComponent, not the owner itself
	}
	else if (synchronizedObject->IsA(USynchronizedController::StaticClass()))
	{
		USynchronizedController* controller = Cast<USynchronizedController>(synchronizedObject);
		TWeakObjectPtr<USynchronizedUser> user = controller->synchronizedUser;
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias);
		pathToUse += (controller->controllerSide == USynchronizedController::ControllerSide::Left ? ".MC_Left.MotionControllerComponent_0" : ".MC_Right.MotionControllerComponent_0"); //Name of MCC is hardcoded

		transformToUse = controller->VRController->GetComponentTransform();
	}

	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Path to Use: %s"), *FString(pathToUse.c_str()));

	time_t currentTime = time(0);
	std::string subject = "GMSEC." + toUpper(missionName) + "." + toUpper(satName) + ".MSG.ESTATE."
		+ toUpper(groupID) + "." + toUpper(projectName) + "." + nameToUse + "." + toUpper(userName); //Might have to replace '.mtproj' in projectName as well, TBD

	publishStateAttributeDataMessage(subject, EntityStateAttributeTypes::absoluteRotation, pathToUse, currentTime, transformToUse);
}

void USynchronizationManager::sendRotationChange(UActorComponent* synchronizedObject, time_t occurenceTime)
{
	FString getName = synchronizedObject->GetName();
	std::string nameToUse = toUpper(std::string(TCHAR_TO_UTF8(*getName)));
	std::string pathToUse = std::string(TCHAR_TO_UTF8(*synchronizedObject->GetPathName()));

	AActor* owner = synchronizedObject->GetOwner();
	FTransform transformToUse = owner->GetTransform();

	if (synchronizedObject->IsA(USynchronizedUser::StaticClass()))
	{
		USynchronizedUser* user = Cast<USynchronizedUser>(synchronizedObject);
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias) + "." + (user->userAlias);

		transformToUse = user->VRCamera->GetComponentTransform(); //If user, send transform of CameraComponent, not the owner itself
	}
	else if (synchronizedObject->IsA(USynchronizedController::StaticClass()))
	{
		USynchronizedController* controller = Cast<USynchronizedController>(synchronizedObject);
		TWeakObjectPtr<USynchronizedUser> user = controller->synchronizedUser;
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias);
		pathToUse += (controller->controllerSide == USynchronizedController::ControllerSide::Left ? ".MC_Left.MotionControllerComponent_0" : ".MC_Right.MotionControllerComponent_0"); //Name of MCC is hardcoded

		transformToUse = controller->VRController->GetComponentTransform();
	}

	std::string subject = "GMSEC." + toUpper(missionName) + "." + toUpper(satName) + ".MSG.ESTATE."
		+ toUpper(groupID) + "." + toUpper(projectName) + "." + nameToUse + "." + toUpper(userName); //Might have to replace '.mtproj' in projectName as well, TBD

	publishStateAttributeDataMessage(subject, EntityStateAttributeTypes::absoluteRotation, pathToUse, occurenceTime, transformToUse);
} 

void USynchronizationManager::sendScaleChange(UActorComponent* synchronizedObject)
{
	FString getName = synchronizedObject->GetName();
	std::string nameToUse = toUpper(std::string(TCHAR_TO_UTF8(*getName)));
	std::string pathToUse = std::string(TCHAR_TO_UTF8(*synchronizedObject->GetPathName()));

	AActor* owner = synchronizedObject->GetOwner();
	FTransform transformToUse = owner->GetTransform();

	if (synchronizedObject->IsA(USynchronizedUser::StaticClass()))
	{
		USynchronizedUser* user = Cast<USynchronizedUser>(synchronizedObject);
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias) + "." + (user->userAlias);

		transformToUse = user->VRCamera->GetComponentTransform(); //If user, send transform of CameraComponent, not the owner itself
	}
	else if (synchronizedObject->IsA(USynchronizedController::StaticClass()))
	{
		USynchronizedController* controller = Cast<USynchronizedController>(synchronizedObject);
		TWeakObjectPtr<USynchronizedUser> user = controller->synchronizedUser;
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias);
		pathToUse += (controller->controllerSide == USynchronizedController::ControllerSide::Left ? ".MC_Left.MotionControllerComponent_0" : ".MC_Right.MotionControllerComponent_0"); //Name of MCC is hardcoded

		transformToUse = controller->VRController->GetComponentTransform();
	}

	time_t currentTime = time(0);
	std::string subject = "GMSEC." + toUpper(missionName) + "." + toUpper(satName) + ".MSG.ESTATE."
		+ toUpper(groupID) + "." + toUpper(projectName) + "." + nameToUse + "." + toUpper(userName); //Might have to replace '.mtproj' in projectName as well, TBD
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(subject.c_str()));
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(pathToUse.c_str()));

	publishStateAttributeDataMessage(subject, EntityStateAttributeTypes::absoluteScale, pathToUse, currentTime, transformToUse);
}

void USynchronizationManager::sendScaleChange(UActorComponent* synchronizedObject, time_t occurenceTime)
{
	FString getName = synchronizedObject->GetName();
	std::string nameToUse = toUpper(std::string(TCHAR_TO_UTF8(*getName)));
	std::string pathToUse = std::string(TCHAR_TO_UTF8(*synchronizedObject->GetPathName()));

	AActor* owner = synchronizedObject->GetOwner();
	FTransform transformToUse = owner->GetTransform();

	if (synchronizedObject->IsA(USynchronizedUser::StaticClass()))
	{
		USynchronizedUser* user = Cast<USynchronizedUser>(synchronizedObject);
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias) + "." + (user->userAlias);

		transformToUse = user->VRCamera->GetComponentTransform(); //If user, send transform of CameraComponent, not the owner itself
	}
	else if (synchronizedObject->IsA(USynchronizedController::StaticClass()))
	{
		USynchronizedController* controller = Cast<USynchronizedController>(synchronizedObject);
		TWeakObjectPtr<USynchronizedUser> user = controller->synchronizedUser;
		nameToUse = toUpper(user->userAlias);

		//Figure out better way to get the name of the user // This is hardcoded
		pathToUse = "/Game/StarterContent/Maps/UEDPIE_0_Minimal_Default.Minimal_Default:PersistentLevel." + (user->userAlias) + "." + (user->userAlias);
		pathToUse += (controller->controllerSide == USynchronizedController::ControllerSide::Left ? ".Left" : ".Right");
	}

	std::string subject = "GMSEC." + toUpper(missionName) + "." + toUpper(satName) + ".MSG.ESTATE."
		+ toUpper(groupID) + "." + toUpper(projectName) + "." + nameToUse + "." + toUpper(userName); //Might have to replace '.mtproj' in projectName as well, TBD
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(subject.c_str()));
	UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: %s"), *FString(pathToUse.c_str()));

	publishStateAttributeDataMessage(subject, EntityStateAttributeTypes::absoluteScale, pathToUse, occurenceTime, transformToUse);
}

void USynchronizationManager::publishStateAttributeDataMessage(std::string subjectToPublish, EntityStateAttributeTypes stateAttributeType,
	std::string entityName, std::time_t time, FTransform transformToApply)
{
	publishStateAttributeDataMessage(subjectToPublish, stateAttributeType, entityName,
		time, transformToApply, FVector::ZeroVector, FQuat::Identity);
}

void USynchronizationManager::publishStateAttributeDataMessage(std::string subjectToPublish, EntityStateAttributeTypes stateAttributeType,
	std::string entityName, std::time_t time, FTransform transformToApply, FVector offset3, FQuat offset4)
{


	//Replace unneccesary characters in the subject and entity

	//Create Message with Header
	gmsec::api::Message msg(subjectToPublish.c_str(), gmsec::api::Message::MessageKind::PUBLISH);
	msg.addField("HEADER-VERSION", (GMSEC_F32)2010);
	msg.addField("MESSAGE-TYPE", "MSG");
	msg.addField("MESSAGE-SUBTYPE", "ESTATE");
	msg.addField("CONTENT-VERSION", (GMSEC_F32)2018);

	//Time Stamp
	std::string timestamp = getTimestamp(time);
	msg.addField("ATTRIBUTE-TIMESTAMP", timestamp.c_str());

	//Entity Name
	msg.addField("ENTITY", entityName.c_str());

	//Switch on stateAttributeType
	switch (stateAttributeType)
	{
	case create:
		msg.addField("ATTRIBUTE", (GMSEC_I16)1);
		break;

	case destroy:
		msg.addField("ATTRIBUTE", (GMSEC_I16)2);
		break;

	case reinitialize:
		msg.addField("ATTRIBUTE", (GMSEC_I16)3);
		break;

	case localPosition:
		msg.addField("ATTRIBUTE", (GMSEC_I16)4);
		msg.addField("X-VALUE", (GMSEC_F32)transformToApply.GetTranslation().X);
		msg.addField("Y-VALUE", (GMSEC_F32)transformToApply.GetTranslation().Y);
		msg.addField("Z-VALUE", (GMSEC_F32)transformToApply.GetTranslation().Z);
		msg.addField("UNITS", "UNREAL");
		break;

	case localRotation:
		msg.addField("ATTRIBUTE", (GMSEC_I16)5);
		msg.addField("X-VALUE", (GMSEC_F32)transformToApply.GetRotation().X);
		msg.addField("Y-VALUE", (GMSEC_F32)transformToApply.GetRotation().Y);
		msg.addField("Z-VALUE", (GMSEC_F32)transformToApply.GetRotation().Z);
		msg.addField("W-VALUE", (GMSEC_F32)transformToApply.GetRotation().W);
		msg.addField("UNITS", "UNREAL");
		break;

	case localScale:
		msg.addField("ATTRIBUTE", (GMSEC_I16)6);
		msg.addField("X-VALUE", (GMSEC_F32)transformToApply.GetScale3D().X);
		msg.addField("Y-VALUE", (GMSEC_F32)transformToApply.GetScale3D().Y);
		msg.addField("Z-VALUE", (GMSEC_F32)transformToApply.GetScale3D().Z);
		msg.addField("UNITS", "UNREAL");
		break;

	case absolutePosition:
		msg.addField("ATTRIBUTE", (GMSEC_I16)7);
		msg.addField("X-VALUE", (GMSEC_F32)transformToApply.GetTranslation().X);
		msg.addField("Y-VALUE", (GMSEC_F32)transformToApply.GetTranslation().Y);
		msg.addField("Z-VALUE", (GMSEC_F32)transformToApply.GetTranslation().Z);
		msg.addField("UNITS", "UNREAL");
		break;

	case absoluteRotation:
		msg.addField("ATTRIBUTE", (GMSEC_I16)8);
		msg.addField("X-VALUE", (GMSEC_F32)transformToApply.GetRotation().X);
		msg.addField("Y-VALUE", (GMSEC_F32)transformToApply.GetRotation().Y);
		msg.addField("Z-VALUE", (GMSEC_F32)transformToApply.GetRotation().Z);
		msg.addField("W-VALUE", (GMSEC_F32)transformToApply.GetRotation().W);
		msg.addField("UNITS", "UNREAL");
		break;

	case absoluteScale:
		msg.addField("ATTRIBUTE", (GMSEC_I16)9);
		msg.addField("X-VALUE", (GMSEC_F32)transformToApply.GetScale3D().X);
		msg.addField("Y-VALUE", (GMSEC_F32)transformToApply.GetScale3D().Y);
		msg.addField("Z-VALUE", (GMSEC_F32)transformToApply.GetScale3D().Z);
		msg.addField("UNITS", "UNREAL");
		break;

	case relativePosition:
		msg.addField("ATTRIBUTE", (GMSEC_I16)10);
		msg.addField("X-VALUE", (GMSEC_F32)offset3.X);
		msg.addField("Y-VALUE", (GMSEC_F32)offset3.Y);
		msg.addField("Z-VALUE", (GMSEC_F32)offset3.Z);
		msg.addField("UNITS", "UNREAL");
		break;

	case relativeRotation:
		msg.addField("ATTRIBUTE", (GMSEC_I16)11);
		msg.addField("X-VALUE", (GMSEC_F32)offset4.X);
		msg.addField("Y-VALUE", (GMSEC_F32)offset4.Y);
		msg.addField("Z-VALUE", (GMSEC_F32)offset4.Z);
		msg.addField("W-VALUE", (GMSEC_F32)offset4.W);
		msg.addField("UNITS", "UNREAL");
		break;

	case relativeScale:
		msg.addField("ATTRIBUTE", (GMSEC_I16)12);
		msg.addField("X-VALUE", (GMSEC_F32)offset3.X);
		msg.addField("Y-VALUE", (GMSEC_F32)offset3.Y);
		msg.addField("Z-VALUE", (GMSEC_F32)offset3.Z);
		msg.addField("UNITS", "UNREAL");
		break;

	case edit:
		msg.addField("ATTRIBUTE", (GMSEC_I16)13);
		break;

	default:
		//Do nothing, this is an invalid EntityStateAttribute
		return;
	}

	try //This could somehow throw an error I guess
	{
		connMgr->publish(msg);
	}
	catch (gmsec::api::Exception e)
	{
		std::string str = std::string(e.what());
		FString what = FString(str.c_str());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *what);
	}
}

//Read message from gmsec bus, and process it appropriately by moving, rotating, or scaling the actor
void USynchronizationManager::receiveMessage() {
	gmsec::api::Message *msg = connMgr->receive(0);

	if (msg != NULL) {
		//These are all pointers that will be set to an object to be found when position/rotation/scale are being set
		TWeakObjectPtr<AActor> entityBeingControlled;
		TWeakObjectPtr<USynchronizedObject> synchedObj;
		TWeakObjectPtr<USynchronizedUser> synchedUser;
		TWeakObjectPtr<USynchronizedController> synchedController;

		std::vector<std::string> subjectFields = split(msg->getSubject(), '.');
		if (subjectFields.size() > 3 && subjectFields[4] == "ESTATE")
		{
			if (subjectFields[3] == "MSG")
			{

				std::string messageUser = subjectFields[8]; //Compare alias of this synchronizationManager to alias in message subject
				
				if (toUpper(messageUser) == toUpper(userName))
				{
					//This was the message we sent, ignore it
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]This was the message we sent, ignore it"));
					return;
				}
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] This was a message from a different syncmanager"));

				std::string timeStamp = msg->getStringField("ATTRIBUTE-TIMESTAMP").getStringValue();
				std::string strEntity = msg->getStringField("ENTITY").getStringValue();
				FString entity(strEntity.c_str());
				UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Entity Being Controlled: %s"), *entity);
				int attributeType = msg->getI16Field("ATTRIBUTE").getValue();

				//Switch on the attribute type to decide what to do with the message
				switch (attributeType)
				{
					//Create
				case 1:
					break;

					//Destroy
				case 2:
					break;

					//Reinitialize
				case 3:
					break;

					//Set local position
				case 4:
					break;

					//Set local rotation
				case 5:
					break;

					//Set local scale
				case 6:
					break;

					//Set global position
				case 7:
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Global Position Change"));
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Original Entity Path: %s"), *entity);
					for (TObjectIterator<UActorComponent> It; It; ++It)
					{
						TWeakObjectPtr<UActorComponent> temp = *It;
						FString componentPath = temp->GetPathName();

						if (componentPath == entity)
						{
							UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Component Path: %s"), *componentPath);
							UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Found Component with same path as Entity"));
							entityBeingControlled = temp->GetOwner();
							break; //Found the right component, no need to waste more time iterating through components
						}

					}

					if (entityBeingControlled.IsValid())
					{
						UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] entityBeingControlled is valid"));

						synchedObj = entityBeingControlled->FindComponentByClass<USynchronizedObject>();
						if (!synchedObj.IsValid())
						{
							synchedUser = entityBeingControlled->FindComponentByClass<USynchronizedUser>();
							if (!synchedUser.IsValid())
							{
								synchedController = entityBeingControlled->FindComponentByClass<USynchronizedController>();
								if (!synchedController.IsValid())
								{
									//Object, User, and Controller are all invalid, nothing to do 
									UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Object, User, and Controller are all invalid"));
									return;
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("[SynchedController] is valid"));
									//Lock here
									std::lock_guard<std::mutex> lock(synchedController->entityLock);
									if (entityBeingControlled.IsValid())
									{
										float xValue = (float)msg->getF32Field("X-VALUE").getValue();
										float yValue = (float)msg->getF32Field("Y-VALUE").getValue();
										float zValue = (float)msg->getF32Field("Z-VALUE").getValue();

										FVector newPos(xValue, yValue, zValue);
										if ((newPos - entityBeingControlled->GetTransform().GetTranslation()).Size() > 0.1f)
										{
											//Reset the entity position here
											entityBeingControlled->SetActorLocation(newPos);
										}
									}
								}
							}
							else
							{
								UE_LOG(LogTemp, Warning, TEXT("[SynchedUser] is valid"));
								//Lock here
								std::lock_guard<std::mutex> lock(synchedUser->entityLock);
								if (entityBeingControlled.IsValid())
								{
									float xValue = (float)msg->getF32Field("X-VALUE").getValue();
									float yValue = (float)msg->getF32Field("Y-VALUE").getValue();
									float zValue = (float)msg->getF32Field("Z-VALUE").getValue();
									UE_LOG(LogTemp, Warning, TEXT("[SynchedUser] NewPos: %f / %f / %f"), xValue, yValue, zValue);

									FVector newPos(xValue, yValue, zValue);
									if ((newPos - entityBeingControlled->GetTransform().GetTranslation()).Size() > 0.1f)
									{
										//Reset the entity position here
										UE_LOG(LogTemp, Warning, TEXT("[SynchedUser] Location should be changed here"));
										entityBeingControlled->SetActorLocation(newPos);
									}
								}
							}
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("[SynchedObject] is valid"));
							//Lock here
							std::lock_guard<std::mutex> lock(synchedObj->entityLock);
							if (entityBeingControlled.IsValid())
							{
								float xValue = (float)msg->getF32Field("X-VALUE").getValue();
								float yValue = (float)msg->getF32Field("Y-VALUE").getValue();
								float zValue = (float)msg->getF32Field("Z-VALUE").getValue();

								FVector newPos(xValue, yValue, zValue);
								if ((newPos - entityBeingControlled->GetTransform().GetTranslation()).Size() > 0.1f)
								{
									//Reset the entity position here
									entityBeingControlled->SetActorLocation(newPos);
								}
							}
						}
					}
					//If the entity is not valid, do nothing
					break;

					//Set global rotation
				case 8:
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Global Rotation Change"));
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Original Entity Path: %s"), *entity);
					for (TObjectIterator<UActorComponent> It; It; ++It)
					{
						TWeakObjectPtr<UActorComponent> temp = *It;
						FString componentPath = temp->GetPathName();
					
						if (componentPath == entity)
						{
							UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Component Path: %s"), *componentPath);
							UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Found Component with same path as Entity"));
							entityBeingControlled = temp->GetOwner();
							break; //Found the right component, no need to waste more time iterating through components
						}

					}

					if (entityBeingControlled.IsValid())
					{
						UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] entityBeingControlled is valid"));

						synchedObj = entityBeingControlled->FindComponentByClass<USynchronizedObject>();
						if (!synchedObj.IsValid())
						{
							synchedUser = entityBeingControlled->FindComponentByClass<USynchronizedUser>();
							if (!synchedUser.IsValid())
							{
								synchedController = entityBeingControlled->FindComponentByClass<USynchronizedController>();
								if (!synchedController.IsValid())
								{
									//Object, User, and Controller are all invalid, nothing to do 
									UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Object, User, and Controller are all invalid"));
									return;
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("[SynchedController] is valid"));
									//Lock here
									std::lock_guard<std::mutex> lock(synchedController->entityLock);
									if (entityBeingControlled.IsValid())
									{
										float xValue = (float)msg->getF32Field("X-VALUE").getValue();
										float yValue = (float)msg->getF32Field("Y-VALUE").getValue();
										float zValue = (float)msg->getF32Field("Z-VALUE").getValue();
										float wValue = (float)msg->getF32Field("W-VALUE").getValue();

										FQuat newRot(xValue, yValue, zValue, wValue);
										if ((newRot.Euler() - entityBeingControlled->GetTransform().GetRotation().Euler()).Size() > 0.1f)
										{
											//Reset the entity rotation here
											entityBeingControlled->SetActorRotation(newRot);
										}
									}
								}
							}
							else
							{
								UE_LOG(LogTemp, Warning, TEXT("[SynchedUser] is valid"));
								//Lock here
								std::lock_guard<std::mutex> lock(synchedUser->entityLock);
								if (entityBeingControlled.IsValid())
								{
									float xValue = (float)msg->getF32Field("X-VALUE").getValue();
									float yValue = (float)msg->getF32Field("Y-VALUE").getValue();
									float zValue = (float)msg->getF32Field("Z-VALUE").getValue();
									float wValue = (float)msg->getF32Field("W-VALUE").getValue();

									FQuat newRot(xValue, yValue, zValue, wValue);
									if ((newRot.Euler() - entityBeingControlled->GetTransform().GetRotation().Euler()).Size() > 0.1f)
									{
										//Reset the entity rotation here
										entityBeingControlled->SetActorRotation(newRot);
									}
								}
							}
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("[SynchedObject] is valid"));
							//Lock here
							std::lock_guard<std::mutex> lock(synchedObj->entityLock);
							if (entityBeingControlled.IsValid())
							{
								float xValue = (float)msg->getF32Field("X-VALUE").getValue();
								float yValue = (float)msg->getF32Field("Y-VALUE").getValue();
								float zValue = (float)msg->getF32Field("Z-VALUE").getValue();
								float wValue = (float)msg->getF32Field("W-VALUE").getValue();

								FQuat newRot(xValue, yValue, zValue, wValue);
								if ((newRot.Euler() - entityBeingControlled->GetTransform().GetRotation().Euler()).Size() > 0.1f)
								{
									//Reset the entity rotation here
									entityBeingControlled->SetActorRotation(newRot);
								}
							}
						}
					}

					//If the entity is not valid, do nothing
					break;

					//Set global scale
				case 9:
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Global Scale Change"));
					UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Original Entity Path: %s"), *entity);
					for (TObjectIterator<UActorComponent> It; It; ++It)
					{
						TWeakObjectPtr<UActorComponent> temp = *It;
						FString componentPath = temp->GetPathName();

						if (componentPath == entity)
						{
							UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Component Path: %s"), *componentPath);
							UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager]: Found Component with same path as Entity"));
							entityBeingControlled = temp->GetOwner();
							break; //Found the right component, no need to waste more time iterating through components
						}

					}

					if (entityBeingControlled.IsValid())
					{
						UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] entityBeingControlled is valid"));

						synchedObj = entityBeingControlled->FindComponentByClass<USynchronizedObject>();
						if (!synchedObj.IsValid())
						{
							synchedUser = entityBeingControlled->FindComponentByClass<USynchronizedUser>();
							if (!synchedUser.IsValid())
							{
								synchedController = entityBeingControlled->FindComponentByClass<USynchronizedController>();
								if (!synchedController.IsValid())
								{
									//Object, User, and Controller are all invalid, nothing to do 
									UE_LOG(LogTemp, Warning, TEXT("[SynchronizationManager] Object, User, and Controller are all invalid"));
									return;
								}
								else
								{
									UE_LOG(LogTemp, Warning, TEXT("[SynchedController] is valid"));
									//Lock here
									std::lock_guard<std::mutex> lock(synchedController->entityLock);
									if (entityBeingControlled.IsValid())
									{
										float xValue = (float)msg->getF32Field("X-VALUE").getValue();
										float yValue = (float)msg->getF32Field("Y-VALUE").getValue();
										float zValue = (float)msg->getF32Field("Z-VALUE").getValue();

										FVector newScl(xValue, yValue, zValue);
										if ((newScl - entityBeingControlled->GetTransform().GetTranslation()).Size() > 0.1f)
										{
											//Reset the entity scale here
											entityBeingControlled->SetActorScale3D(newScl);
										}
									}
								}
							}
							else
							{
								UE_LOG(LogTemp, Warning, TEXT("[SynchedUser] is valid"));
								//Lock here
								std::lock_guard<std::mutex> lock(synchedUser->entityLock);
								if (entityBeingControlled.IsValid())
								{
									float xValue = (float)msg->getF32Field("X-VALUE").getValue();
									float yValue = (float)msg->getF32Field("Y-VALUE").getValue();
									float zValue = (float)msg->getF32Field("Z-VALUE").getValue();

									FVector newScl(xValue, yValue, zValue);
									if ((newScl - entityBeingControlled->GetTransform().GetTranslation()).Size() > 0.1f)
									{
										//Reset the entity scale here
										entityBeingControlled->SetActorScale3D(newScl);
									}
								}
							}
						}
						else
						{
							UE_LOG(LogTemp, Warning, TEXT("[SynchedObject] is valid"));
							//Lock here
							std::lock_guard<std::mutex> lock(synchedObj->entityLock);
							if (entityBeingControlled.IsValid())
							{
								float xValue = (float)msg->getF32Field("X-VALUE").getValue();
								float yValue = (float)msg->getF32Field("Y-VALUE").getValue();
								float zValue = (float)msg->getF32Field("Z-VALUE").getValue();

								FVector newScl(xValue, yValue, zValue);
								if ((newScl - entityBeingControlled->GetTransform().GetTranslation()).Size() > 0.1f)
								{
									//Reset the entity scale here
									entityBeingControlled->SetActorScale3D(newScl);
								}
							}
						}
					}

					//If the entity is not valid, do nothing
					break;

					//Modify position
				case 10:
					break;

					//Modify rotation
				case 11:
					break;

					//Modify scale
				case 12:
					break;

					//Edit
				case 13:
					break;

					//Unknown
				default:
					break;
				}
			}
			else if (subjectFields[3] == "REQ")
			{
				if (sessionMasterNode.IsValid())
				{
					sessionMasterNode->sendEntityStateResponse(*msg); //Dereference msg pointer
				}
			}
		}
		else if (subjectFields.size() > 4 && subjectFields[5] == "SEEKER")
		{
			if (sessionAdvertiser.IsValid())
			{
				sessionAdvertiser->respondToListRequest(*msg); //Dereference msg pointer
			}
		}
	}
}


//==================================================================================================================
//Private Function Implementations
//==================================================================================================================

std::string USynchronizationManager::connectionTypeToString(USynchronizationManager::ConnectionTypes rawConnType) {
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

std::string USynchronizationManager::getFullPath() {
	return "abc";
}

std::string USynchronizationManager::toUpper(std::string str) {
	std::string ret = "";
	std::locale loc;
	for (char& c : str) {
		c = std::toupper(c, loc);
		ret += c;
	}

	return ret;
}

std::vector<std::string> USynchronizationManager::split(const std::string &text, char sep)
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

std::string USynchronizationManager::getTimestamp(std::time_t time) {
	char buff[80];
	std::tm* now = std::localtime(&time);

	strftime(buff, sizeof(buff), "%Y-%m-%d.%X", now);
	return buff;
}

std::string USynchronizationManager::getFullPath(UActorComponent* object)
{
	AActor* highestObj = object->GetOwner(); //Components should be direct children of actor
	std::string fullPath = std::string(TCHAR_TO_UTF8(*highestObj->GetName())) + "/" + std::string(TCHAR_TO_UTF8(*object->GetName()));

	while (highestObj->GetOwner() != NULL)
	{
		highestObj = highestObj->GetOwner();
		fullPath = std::string(TCHAR_TO_UTF8(*highestObj->GetName())) + "/" + fullPath;
	}

	return fullPath;
}