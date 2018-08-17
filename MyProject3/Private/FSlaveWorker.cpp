// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/FSlaveWorker.h"

FSlaveWorker* FSlaveWorker::Runnable = NULL;

FSlaveWorker::FSlaveWorker(gmsec::api::mist::ConnectionManager& TheConnMgr, gmsec::api::Message& TheMessage, gmsec::api::Message& Reply, bool& response)
{
	connectionManager = &TheConnMgr;
	message = &TheMessage;
	reply = &Reply;
	newResponse = &response;

	Thread = FRunnableThread::Create(this, TEXT("FSlaveWorker"), 0, TPri_BelowNormal);
}

FSlaveWorker::~FSlaveWorker()
{
	delete Thread;
	Thread = NULL;
}

//Init
bool FSlaveWorker::Init()
{
	return true;
}

//Run
uint32 FSlaveWorker::Run()
{
	//FGenericPlatformProcess::Sleep(0.03);
	sendGMSECEntityRequestMessage();
	return 0;
}

//Stop
void FSlaveWorker::Stop()
{
	StopTaskCounter.Increment();
}

FSlaveWorker* FSlaveWorker::JoyInit(gmsec::api::mist::ConnectionManager& TheConnMgr, gmsec::api::Message& TheMessage, gmsec::api::Message& Reply, bool& response)
{
	//Create new instance of thread if it does not exist, and the platform supports multithreading
	if (!Runnable)
	{
		Runnable = new FSlaveWorker(TheConnMgr, TheMessage, Reply, response);
	}
	return Runnable;
}

void FSlaveWorker::EnsureCompletion()
{
	Stop();
	Thread->WaitForCompletion();
}

void FSlaveWorker::Shutdown()
{
	if (Runnable)
	{
		Runnable->EnsureCompletion();
		delete Runnable;
		Runnable = NULL;
	}
}

bool FSlaveWorker::IsThreadFinished()
{
	//if (Runnable) return Runnable->IsFinished();
	return true;
}

void FSlaveWorker::sendGMSECEntityRequestMessage()
{
	UE_LOG(LogTemp, Warning, TEXT("[SessionSlaveNode] Sending GMSECEntityRequestMessage to [SessionMasterNode] via Thread"));
	*reply = *connectionManager->request(*message, 10000, -1); //Dereference pointer to pass value back to SessionSlaveNode
	if (reply) //Got a new Message, set flag true
	{
		*newResponse = true;
	}
}
