// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SessionAdvertiser.h"
#include "../Public/SessionInformation.h"

//===================================================================================================================
//Constructors/Destructors
//===================================================================================================================
// Sets default values for this component's properties
USessionAdvertiser::USessionAdvertiser()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

USessionAdvertiser::~USessionAdvertiser()
{
	//Do Nothing
}

//===================================================================================================================
//Public Method Implementations
//===================================================================================================================

// Called when the game starts
void USessionAdvertiser::BeginPlay()
{
	Super::BeginPlay();
	//Create Config
	gmsec::api::Config cfg;
	cfg.addValue("connectionType", connectionTypeToString(connectionType).c_str());
	cfg.addValue("server", server.c_str());
	cfg.addValue("GMSEC-REQ-RESP", "OPEN-RESP"); //Enable GMSEC open response

	//Initialize pointers to real objects
	connMgr.reset(new gmsec::api::mist::ConnectionManager(cfg));
	sessionInformation.reset(new SessionInformation());

	//Initialize ConnectionManager
	std::string sub = "GMSEC." + missionName + "." + satName + ".REQ.DIR.SEEKER.LIST"; //Listening for Request messages
	const char* subscriptionSubject = sub.c_str();
	try
	{
		connMgr->initialize();
		connMgr->subscribe(subscriptionSubject);
	}
	catch (gmsec::api::Exception e)
	{
		UE_LOG(LogTemp, Warning, TEXT("s"), *FString(e.what()));
	}
	
	GMSEC_DEBUG << "[SessionAdvertiser] Subscribed to: " << sub.c_str();
	subscribed = true;
	initialized = true;
	GMSEC_DEBUG << "[SessionAdvertiser] Advertiser Initialized";
}

void USessionAdvertiser::Initialize()
{
	BeginPlay();
}

void USessionAdvertiser::respondToListRequest(gmsec::api::Message msg)
{
	//Reply subject
	std::string rpl = "GMSEC." + missionName + "." + satName + ".RESP.DIR.ADVERTISER.1";
	const char* rplSubject = rpl.c_str();

	//Create reply message with header
	gmsec::api::Message reply(rplSubject, gmsec::api::Message::MessageKind::REPLY);
	reply.addField("HEADER_VERSION", (GMSEC_F32)2010);
	reply.addField("MESSAGE-TYPE", "RESP");
	reply.addField("MESSAGE-SUBTYPE", "DIR");
	reply.addField("CONTENT-VERSION", (GMSEC_F32)2016);

	//Add fields to reply message
	reply.addField("RESPONSE-STATUS", (GMSEC_I16)1);
	std::string data = sessionInformation->sessionName + ";"
		+ sessionInformation->projectName + ";" + sessionInformation->groupName;
	const char* dataVal = data.c_str();
	reply.addField("DATA", dataVal);

	//Send reply
	connMgr->reply(msg, reply);
}

void USessionAdvertiser::handleListRequests() {
	//msg should be received
	gmsec::api::Message *msg = connMgr->receive(0);
	if (msg != NULL) {
		UE_LOG(LogTemp, Warning, TEXT("[SessionAdvertiser] List Request Received"));

		respondToListRequest(*msg);
	}
}


// Called every frame
void USessionAdvertiser::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	if (subscribed) {
		handleListRequests();
	}
}

//===================================================================================================================
//Private Method Implementations
//===================================================================================================================
std::string USessionAdvertiser::connectionTypeToString(USessionAdvertiser::ConnectionTypes rawConnType) {
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
