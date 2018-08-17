// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

#include "MyProject3GameMode.h"
#include "MyProject3HUD.h"
#include "MyProject3Character.h"
#include "UObject/ConstructorHelpers.h"

AMyProject3GameMode::AMyProject3GameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = AMyProject3HUD::StaticClass();
}
