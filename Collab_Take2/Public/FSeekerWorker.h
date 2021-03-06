// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "gmsec4_cpp.h"
#include "Runtime/Core/Public/HAL/Runnable.h"
#include "Runtime/Core/Public/HAL/RunnableThread.h"
#include "Runtime/Core/Public/GenericPlatform/GenericPlatformProcess.h"
#include "../Public/SessionInformation.h"
#include <vector>
#include <string>
#include <iostream>
#include "CoreMinimal.h"

/**
*
*/
class COLLAB_TAKE2_API FSeekerWorker : public FRunnable
{
	//Static reference to thread
	static FSeekerWorker* Runnable;

	//Thread to run worker FRunnable on
	FRunnableThread* Thread;

	//Data pointer
	std::vector<SessionInformation>* availableSessions;

	//ConnectionManager
	gmsec::api::mist::ConnectionManager* connectionManager;

	//Message to send
	gmsec::api::Message* message;

	//Stop thread? Use Thread Safe Counter
	FThreadSafeCounter StopTaskCounter;

	//Actual Seeking of Sessions
	void requestSessionInfo();
public:
	FSeekerWorker(std::vector<SessionInformation>& TheVector, gmsec::api::mist::ConnectionManager& TheConnMgr, gmsec::api::Message& msg);
	virtual ~FSeekerWorker();

	//Begin FRunnable interface
	virtual bool Init();
	virtual uint32 Run();
	virtual void Stop();
	//End FRunnable interface

	//Make sure therad has stopped properly
	void EnsureCompletion();

	//Starting/Stopping Thread
	static FSeekerWorker* JoyInit(std::vector<SessionInformation>& TheVector, gmsec::api::mist::ConnectionManager& TheConnMgr, gmsec::api::Message& msg);

	static void Shutdown();

	static bool IsThreadFinished();

private:
	std::vector<std::string> split(const std::string &text, char sep);
};
