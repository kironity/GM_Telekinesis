// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TelekenesisGameMode.h"
#include "TelekenesisHUD.h"
#include "TelekenesisCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATelekenesisGameMode::ATelekenesisGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	/*static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ATelekenesisHUD::StaticClass();*/
}
