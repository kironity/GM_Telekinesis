// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TelekinesisCharacter.h"
#include "Animation/AnimInstance.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/SceneComponent.h"
#include "GameFramework/InputSettings.h"
#include "HeadMountedDisplayFunctionLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Math/UnrealMathVectorCommon.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Engine/World.h"
#include "Camera/PlayerCameraManager.h"
#include "GameFramework/PlayerController.h"
#include "Components/AudioComponent.h"

DEFINE_LOG_CATEGORY_STATIC(LogFPChar, Warning, All);

//////////////////////////////////////////////////////////////////////////
// ATelekenesisCharacter

ATelekinesisCharacter::ATelekinesisCharacter()
{
	// Set size for collision capsule
	GetCapsuleComponent()->InitCapsuleSize(55.f, 96.0f);

	// Create a CameraComponent	
	FirstPersonCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
	FirstPersonCameraComponent->SetupAttachment(GetCapsuleComponent());
	FirstPersonCameraComponent->SetRelativeLocation(FVector(-39.56f, 1.75f, 64.f)); // Position the camera
	FirstPersonCameraComponent->bUsePawnControlRotation = true;
	
	// Create a mesh component that will be used when being viewed from a '1st person' view (when controlling this pawn)
	Mesh1P = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("CharacterMesh1P"));
	Mesh1P->SetOnlyOwnerSee(true);
	Mesh1P->SetupAttachment(FirstPersonCameraComponent);
	Mesh1P->bCastDynamicShadow = false;
	Mesh1P->CastShadow = false;
	Mesh1P->SetRelativeRotation(FRotator(1.9f, -19.19f, 5.2f));
	Mesh1P->SetRelativeLocation(FVector(-0.5f, -4.4f, -155.7f));

	// Create a gun mesh component
	FP_Gun = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("FP_Gun"));
	FP_Gun->SetOnlyOwnerSee(true);			// only the owning player will see this mesh
	FP_Gun->bCastDynamicShadow = false;
	FP_Gun->CastShadow = false;
	// FP_Gun->SetupAttachment(Mesh1P, TEXT("GripPoint"));
	FP_Gun->SetupAttachment(RootComponent);

	FP_MuzzleLocation = CreateDefaultSubobject<USceneComponent>(TEXT("MuzzleLocation"));
	FP_MuzzleLocation->SetupAttachment(FP_Gun);
	FP_MuzzleLocation->SetRelativeLocation(FVector(0.2f, 48.4f, -10.6f));

	// Default offset from the character location for projectiles to spawn
	GunOffset = FVector(100.0f, 0.0f, 10.0f);

	// Create a Physics Handle
	PhysicHandle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("Physics"));
	PhysicHandle->InterpolationSpeed = 5.f;

	CurrentTelekinesisPower = CreateDefaultSubobject<USceneComponent>(TEXT("CurrentPosition"));
	CurrentTelekinesisPower->SetupAttachment(FirstPersonCameraComponent);

	MinimumTelekinesisPower = CreateDefaultSubobject<USceneComponent>(TEXT("MinimumPosition"));
	MinimumTelekinesisPower->SetupAttachment(FirstPersonCameraComponent);

	MaximumTelekinesisPower = CreateDefaultSubobject<USceneComponent>(TEXT("MaximumPosition"));
	MaximumTelekinesisPower->SetupAttachment(FirstPersonCameraComponent);

	// Set Default value properties 
	MaxLengthTelekinesis = 2000.f;
	MinimumFailedDistance = 1000.f;
	ImpulseStrength = 4000.f;

	// Set Default Volume value 
	HoldTelekinesisVolume = 1.f;
	ThrowSoundVolume = 1.f;

	// Set a default value that affects component interpolation
	StepDistanceValue = 150.f;
}

void ATelekinesisCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Set Max Distance to Physics Grabbed ability 
	MaximumTelekinesisPower->SetRelativeLocation(FVector(MaxLengthTelekinesis, 0.f, 0.f));

	if (bPlayHoldSoundOnGrabbedComponent)
	{
		TelekinesisUpSoundComponent = CreateAttachedSound(GetCapsuleComponent(), HoldTelekinesisSound, bPauseSoundOnSpawn);
	}
	else
	{
		TelekinesisUpSoundComponent = nullptr;
	}
}

void ATelekinesisCharacter::Tick(float DeltaSeconds)
{
	if (bObjectGrabbed && PhysicHandle != nullptr)
	{
		if (CheckHoldComponents(PhysicHandle->GetGrabbedComponent(), CurrentTelekinesisPower, MinimumFailedDistance))
		{
			// Update Telekinesis mesh position
			PhysicHandle->SetTargetLocationAndRotation(CurrentTelekinesisPower->GetComponentLocation(), 
				                                       CurrentTelekinesisPower->GetComponentRotation());

		}
		else
		{
			// Return Outline Color grabbed mesh to default if custom render condition true
			if (bCanAffectCustomRender)
			{
				PhysicHandle->GetGrabbedComponent()->SetRenderCustomDepth(false);
			}
			// Stop Grabbed Telekinesis Component 
			PhysicHandle->ReleaseComponent();
			bObjectGrabbed = false;
			UE_LOG(LogTemp, Warning, TEXT("True"));
			OnOffAttachedSound(TelekinesisUpSoundComponent, false);
		}
	}
}

//////////////////////////////////////////////////////////////////////////
// Input
void ATelekinesisCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind telekinesis event Left Mouse Button 
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ATelekinesisCharacter::TelekinesisUp);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ATelekinesisCharacter::TelekinesisRelease);

	// Bind Telekinesis event Right Mouse Button 
	PlayerInputComponent->BindAction("Throw", IE_Pressed, this, &ATelekinesisCharacter::ThrowObject);
	
	// Bind WheelMouse events 
	PlayerInputComponent->BindAction("SlideForward", IE_Pressed, this, &ATelekinesisCharacter::WheelUp);
	PlayerInputComponent->BindAction("SlideBackward", IE_Pressed, this, &ATelekinesisCharacter::WheelDown);
	
	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ATelekinesisCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATelekinesisCharacter::MoveRight);

	// Bind rotate arround events 
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
}

void ATelekinesisCharacter::TelekinesisUp()
{
	FHitResult HitResult;

	// Take value from Hited Primitive scene components 
	if (LineTrace(HitResult))
	{
		if (HitResult.GetComponent() != nullptr)
		{
			if (EComponentMobility::Movable == HitResult.GetComponent()->Mobility.GetValue())
			{
				// Add Hit Component value, to our PhysicHandle 
				PhysicHandle->GrabComponentAtLocationWithRotation
				              (HitResult.GetComponent(), FName("None"), HitResult.Location,
					           UKismetMathLibrary::MakeRotFromX(HitResult.Location));

				//Update Location 
				CurrentTelekinesisPower->SetWorldLocation(HitResult.Location);
				
				FVector TargetLocation = CurrentTelekinesisPower->GetComponentLocation();
				FRotator TargetRotation = CurrentTelekinesisPower->GetComponentRotation();

				// Set Grabbed Component desired location and rotation 
				PhysicHandle->SetTargetLocationAndRotation(TargetLocation, TargetRotation);

				bObjectGrabbed = true;

				// Change Outline Color grabbed mesh
				if (bCanAffectCustomRender)
				{
					PhysicHandle->GetGrabbedComponent()->SetRenderCustomDepth(true);
				}

				// Start Play telekinesis sound 
				OnOffAttachedSound(TelekinesisUpSoundComponent, bObjectGrabbed);
			}
		}
	}
	
	// try and play the sound if specified
	if (FireSound != nullptr)
	{
		UGameplayStatics::PlaySoundAtLocation(this, FireSound, GetActorLocation());
	}

	// try and play a firing animation if specified
	if (FireAnimation != nullptr)
	{
		// Get the animation object for the arms mesh
		UAnimInstance* AnimInstance = Mesh1P->GetAnimInstance();
		if (AnimInstance != NULL)
		{
			AnimInstance->Montage_Play(FireAnimation, 1.f);
		}
	}
}

void ATelekinesisCharacter::TelekinesisRelease()
{
	bObjectGrabbed = false;

	if (PhysicHandle != nullptr && PhysicHandle->GetGrabbedComponent() != nullptr)
	{
		// Return Outline Color grabbed mesh to default if custom render condition true
		if (bCanAffectCustomRender)
		{
			PhysicHandle->GetGrabbedComponent()->SetRenderCustomDepth(false);
		}

		// Stop Grabbed our mesh 
		PhysicHandle->ReleaseComponent();

		// Stop Playing Telekinesis Sound 
		OnOffAttachedSound(TelekinesisUpSoundComponent, bObjectGrabbed);
	}
}

void ATelekinesisCharacter::ThrowObject()
{
	bObjectGrabbed = false; 

	//If Grabbed Component valid, Add Impulse 
	if (PhysicHandle->GetGrabbedComponent() != nullptr)
	{
		FVector Impulse = FVector(FirstPersonCameraComponent->GetForwardVector() * ImpulseStrength);
		UPrimitiveComponent* GrabbedComponent = PhysicHandle->GetGrabbedComponent();
		GrabbedComponent->AddImpulse(Impulse, FName("None"), true);
		
		OnOffAttachedSound(TelekinesisUpSoundComponent, false);

		// try and play the sound if specified
		UGameplayStatics::PlaySound2D(this, ThrowTelekinesisSound, ThrowSoundVolume);

		// try and spawn Emitters  actor
		FVector SpawnedLocation = GrabbedComponent->GetComponentLocation();
		FRotator SpawnedRotation = GrabbedComponent->GetComponentRotation();
		FActorSpawnParameters SpawnParameters;
		GetWorld()->SpawnActor<AActor>(ThrowEffect, SpawnedLocation, SpawnedRotation, SpawnParameters);

		// Return Outline Color grabbed mesh to default if custom render condition true
		if (bCanAffectCustomRender)
		{
			PhysicHandle->GetGrabbedComponent()->SetRenderCustomDepth(false);
		}

		PhysicHandle->ReleaseComponent();
	}
}

void ATelekinesisCharacter::WheelUp()
{
	FVector CurrentPosition = CurrentTelekinesisPower->GetComponentLocation();
	FVector DesiredPosition = MaximumTelekinesisPower->GetComponentLocation();
	InterpTo(CurrentPosition, DesiredPosition, CurrentTelekinesisPower, StepDistanceValue);
}

void ATelekinesisCharacter::WheelDown()
{
	FVector CurrentPosition = CurrentTelekinesisPower->GetComponentLocation();
	FVector DesiredPosition = MinimumTelekinesisPower->GetComponentLocation();
	InterpTo(CurrentPosition, DesiredPosition, CurrentTelekinesisPower, StepDistanceValue);
}

bool ATelekinesisCharacter::LineTrace(FHitResult& OutHit)
{

	// Get Our Camera Manager for trace 
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (CameraManager != nullptr)
	{
		// Get location First trace
		FVector TraceStart = CameraManager->GetCameraLocation();

		// Culc Desired Trace length 
		float TraceLength = FVector(MaximumTelekinesisPower->GetComponentLocation() - TraceStart).Size();

		// Get location End trace
		FVector TraceEnd = CameraManager->GetActorForwardVector() * TraceLength + TraceStart;

		// Line Trace 
		GetWorld()->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);

		return OutHit.IsValidBlockingHit();
	}
	return false;
}

UAudioComponent* ATelekinesisCharacter::CreateAttachedSound(UPrimitiveComponent* RequiredSpawnComponent, USoundBase* SpawnedSound, bool PauseOnSpawn)
{
	// Try Spawn the Telekinesis sound if specified
	if (SpawnedSound != nullptr)
	{
		EAttachLocation::Type AttachType = EAttachLocation::KeepRelativeOffset;
		UAudioComponent* SpawnSound = UGameplayStatics::SpawnSoundAttached(SpawnedSound, RequiredSpawnComponent, FName("None"), FVector(0.f),
			                                                               AttachType, true, HoldTelekinesisVolume, 1.f, 0.f);

		// Set Default Sound value on the basis "DefaultCondition"
		SpawnSound->SetPaused(!PauseOnSpawn);
		return SpawnSound;
	}
	else
	{
		FString CurrentActorName = ATelekinesisCharacter::GetName();
		UE_LOG(LogTemp, Warning, TEXT("SpawnedSound on &s .cpp 196 line == nullptr"), &CurrentActorName);
		return nullptr;
	}
}



void ATelekinesisCharacter::OnOffAttachedSound(UAudioComponent* ComponenToChange, bool Condition)
{
	if (ComponenToChange != nullptr)
	{
		ComponenToChange->SetPaused(!Condition);
	}
	return;
}


bool ATelekinesisCharacter::CheckHoldComponents(UPrimitiveComponent* GrabbedComponent, USceneComponent* ComparedComponent, float MaxOffset)
{
	// If Component Valid Compare Distance 
	if (GrabbedComponent != nullptr && ComparedComponent != nullptr)
	{
		float LengthBetweenComponents = FVector(GrabbedComponent->GetComponentLocation() - ComparedComponent->GetComponentLocation()).Size();

		if (LengthBetweenComponents < MaxOffset)
		{
			return true;
		}
	}
	return false;
}

void ATelekinesisCharacter::InterpTo(FVector CurrentLocation, FVector DesiredLocation, USceneComponent * ComponentToChange, float StepDistance)
{
	float Length = FVector(CurrentLocation - DesiredLocation).Size();
	float Alpha = StepDistance / Length;

	if (Alpha < 1.f)
	{
		ComponentToChange->SetWorldLocation(FMath::Lerp(CurrentLocation, DesiredLocation, Alpha));
	}
}

void ATelekinesisCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ATelekinesisCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}
