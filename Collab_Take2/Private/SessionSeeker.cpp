// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SessionSeeker.h"
#include "../Public/SessionInformation.h"

//===================================================================================================================
//Constructors
//===================================================================================================================
// Sets default values for this component's properties
USessionSeeker::USessionSeeker()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;
}

USessionSeeker::~USessionSeeker()
{
	UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Available Sessions:"));
	for (int i = 0; i < availableSessions.size(); i++)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Session Name: %s Project Name: %s Group Name: %s"), *FString(availableSessions[i].sessionName.c_str()), *FString(availableSessions[i].projectName.c_str()), *FString(availableSessions[i].groupName.c_str()));
	}
	FSeekerWorker::Shutdown();
}

//===================================================================================================================
//Public Function Implementations
//===================================================================================================================

// Called when the game starts
void USessionSeeker::BeginPlay()
{
	Super::BeginPlay();

	gmsec::api::Config cfg;
	cfg.addValue("connectionType", connectionTypeToString(connectionType).c_str());
	cfg.addValue("server", server.c_str());
	cfg.addValue("GMSEC-REQ-RESP", "OPEN-RESP"); //GMSEC Open Response Feature

	connMgr.reset(new gmsec::api::mist::ConnectionManager(cfg));

	UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Initializing Seeker"));
	connMgr->initialize();
	connMgr->subscribe(subscriptionSubject.c_str());
	UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Seeker Initialized"));

	//Test to make sure connMgr is functional
	gmsec::api::Message msg("GMSEC.TEST.PUBLISH", gmsec::api::Message::MessageKind::PUBLISH); //Test message
	msg.addField("INIT", "Session Seeker SUCCESS!");
	connMgr->publish(msg);

	searchForSessions();
}


// Called every frame
void USessionSeeker::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

void USessionSeeker::searchForSessions() {
	GMSEC_DEBUG << "[SessionSeeker] Connecting to GMSEC bus and searching for sessions";

	std::string hostName = "unknown";
	std::string requestSubject = "GMSEC." + missionName + "." + satName + ".REQ.DIR.SEEKER.LIST";
	std::string user = alias + "." + hostName;

	//Create Request Message with Header
	gmsec::api::Message msg(requestSubject.c_str(), gmsec::api::Message::MessageKind::REQUEST);
	msg.addField("HEADER-VERSION", (GMSEC_F32)2010);
	msg.addField("MESSAGE-TYPE", "REQ");
	msg.addField("MESSAGE-SUBTYPE", "DIR");
	msg.addField("CONTENT-VERSION", (GMSEC_F32)2016);
	msg.addField("USER", user.c_str());
	msg.addField("DIRECTIVE-STRING", "LISTSESSIONS");
	msg.addField("RESPONSE", true);

	//Start thread to seek sessions
	FSeekerWorker::JoyInit(availableSessions, *connMgr, msg);
}


//===================================================================================================================
//Private Function Implementations
//===================================================================================================================

/*void USessionSeeker::requestSessionInfo() {
	std::string hostName = "unknown";
	std::string requestSubject = "GMSEC." + missionName + "." + satName + ".REQ.DIR.SEEKER.LIST";
	std::string user = alias + "." + hostName;

	//Create Request Message with Header
	gmsec::api::Message msg(requestSubject.c_str(), gmsec::api::Message::MessageKind::REQUEST);
	msg.addField("HEADER-VERSION", (GMSEC_F32)2010);
	msg.addField("MESSAGE-TYPE", "REQ");
	msg.addField("MESSAGE-SUBTYPE", "DIR");
	msg.addField("CONTENT-VERSION", (GMSEC_F32)2016);
	msg.addField("USER", user.c_str());
	msg.addField("DIRECTIVE-STRING", "LISTSESSIONS");
	msg.addField("RESPONSE", true);

	try //Because it's not a simple PUBLISH, wrap with try/catch to handle exceptions
	{
		gmsec::api::Message* rpl = connMgr->request(msg, 2000, -1);
		if (rpl != NULL)
		{
			//Spilt this field somehow
			std::string msgData = rpl->getStringField("DATA").getValue(); // The ease of this conversion is suspicious
			std::vector<std::string> messageData = stringSplit(msgData, ";");
			if (messageData.size() == 3)
			{
				SessionInformation sessionInfo(messageData[0], messageData[1], messageData[2]);
				availableSessions.push_back(sessionInfo);
			}
			UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Logged and processed the reply from [SessionAdvertiser]"));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Did not receive a reply to it's request for session info"));
		}
	}

	catch (gmsec::api::Exception e)
	{
		FString str(e.what());
		UE_LOG(LogTemp, Warning, TEXT("%s"), *str);
	}

	//Maybe remove this?
	gmsec::api::Message cleanupMsg("GMSEC.TEST.PUBLISH", gmsec::api::Message::MessageKind::PUBLISH); //Test message
	cleanupMsg.addField("CLEANUP", "Session Seeker Cleaned Up!");
	connMgr->publish(cleanupMsg);
	connMgr->cleanup();

	for (SessionInformation sess : availableSessions) {
		std::string info = sess.sessionName + " " + sess.projectName + " " + sess.groupName;
		GMSEC_DEBUG << info.c_str();
	}

} */

//==================================================================================================================
//Private Function Definitions
//==================================================================================================================
std::string USessionSeeker::connectionTypeToString(USessionSeeker::ConnectionTypes rawConnType) {
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

std::vector<std::string> USessionSeeker::stringSplit(std::string str, std::string delimiter) {
	size_t pos = 0;
	std::vector<std::string> split;
	std::string tok;

	while ((pos = str.find(delimiter)) != std::string::npos) {
		tok = str.substr(0, pos);
		std::cout << tok << std::endl;
		split.push_back(tok);
		str.erase(0, pos + delimiter.length());
	}
	return split;
}
