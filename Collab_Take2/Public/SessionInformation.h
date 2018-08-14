// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#define WIN32_LEAN_AND_MEAN //Include for UE 4.20.1

#include "CoreMinimal.h"
#include "gmsec4_cpp.h"
#include <string>

/**
*
*/
class COLLAB_TAKE2_API SessionInformation
{
public:
	SessionInformation();
	SessionInformation(std::string sessName, std::string projName, std::string grpName);
	~SessionInformation();

	std::string sessionName;
	std::string projectName;
	std::string groupName;
};
