// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/FSeekerWorker.h"

FSeekerWorker* FSeekerWorker::Runnable = NULL;

FSeekerWorker::FSeekerWorker(std::vector<SessionInformation>& TheVector, gmsec::api::mist::ConnectionManager& TheConnMgr, gmsec::api::Message& msg)
{
	availableSessions = &TheVector;
	connectionManager = &TheConnMgr;
	message = &msg;

	Thread = FRunnableThread::Create(this, TEXT("FSeekerWorker"), 0, TPri_BelowNormal);
}

FSeekerWorker::~FSeekerWorker()
{
	delete Thread;
	Thread = NULL;
}

//Init
bool FSeekerWorker::Init()
{
	return true;
}

//Run
uint32 FSeekerWorker::Run()
{
	//FGenericPlatformProcess::Sleep(0.03);
	requestSessionInfo();
	return 0;
}

//Stop
void FSeekerWorker::Stop()
{
	StopTaskCounter.Increment();
}

FSeekerWorker* FSeekerWorker::JoyInit(std::vector<SessionInformation>& TheVector, gmsec::api::mist::ConnectionManager& TheConnMgr, gmsec::api::Message& msg)
{
	//Create new instance of thread if it does not exist, and the platform supports multithreading
	if (!Runnable)
	{
		Runnable = new FSeekerWorker(TheVector, TheConnMgr, msg);
	}
	return Runnable;
}

void FSeekerWorker::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}

void FSeekerWorker::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
	}
}

bool FSeekerWorker::IsThreadFinished()
{
	//if (Runnable) return Runnable->IsFinished();
	return true;
}

void FSeekerWorker::requestSessionInfo()
{
	connectionManager->request(*message, 10, -1);

	for (int i = 0; i < 200; i++)
	{
		gmsec::api::Message* rpl = connectionManager->receive(10);

		if (rpl != NULL)
		{
			//Spilt this field somehow
			std::string msgData = rpl->getStringField("DATA").getValue(); // The ease of this conversion is suspicious
			std::vector<std::string> messageData = split(msgData, ';');
			if (messageData.size() == 3)
			{
				SessionInformation sessionInfo(messageData[0], messageData[1], messageData[2]);
				availableSessions->push_back(sessionInfo);
				UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Added sessionInfo to availableSessions"));
			}
			UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Logged and processed the reply from [SessionAdvertiser]"));
			//Keep listening for more potential Advertiser messages
		}
		else if (i == 199 && availableSessions->size() == 0)
		{
			UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Did not receive a reply to it's request for session info"));
		}
	}

	//Maybe remove this?
	connectionManager->cleanup();
	UE_LOG(LogTemp, Warning, TEXT("[SessionSeeker] Cleaned Up"));
}


//Private
std::vector<std::string> FSeekerWorker::split(const std::string &text, char sep)
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
