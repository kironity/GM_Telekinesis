// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TelekinesisGameMode.h"
#include "TelekinesisHUD.h"
#include "TelekinesisCharacter.h"
#include "UObject/ConstructorHelpers.h"

ATelekinesisGameMode::ATelekinesisGameMode()
	: Super()
{
	// set default pawn class to our Blueprinted character
	/*static ConstructorHelpers::FClassFinder<APawn> PlayerPawnClassFinder(TEXT("/Game/FirstPersonCPP/Blueprints/FirstPersonCharacter"));
	DefaultPawnClass = PlayerPawnClassFinder.Class;

	// use our custom HUD class
	HUDClass = ATelekenesisHUD::StaticClass();*/
}
