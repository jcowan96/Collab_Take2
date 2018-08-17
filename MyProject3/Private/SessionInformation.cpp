// Fill out your copyright notice in the Description page of Project Settings.

#include "../Public/SessionInformation.h"

SessionInformation::SessionInformation()
{
	sessionName = "NEW-SESSION";
	projectName = "NEW-PROJECT";
	groupName = "NEW-GROUP";
}

SessionInformation::SessionInformation(std::string sessName, std::string projName, std::string grpName)
{
	sessionName = sessName;
	projectName = projName;
	groupName = grpName;
}

SessionInformation::~SessionInformation()
{
	//Do Nothing
}