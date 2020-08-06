// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#include "TelekenesisCharacter.h"
#include "TelekenesisProjectile.h"
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

ATelekenesisCharacter::ATelekenesisCharacter()
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

	TelekenesisPosition = CreateDefaultSubobject<USceneComponent>(TEXT("CurrentPosition"));
	TelekenesisPosition->SetupAttachment(FirstPersonCameraComponent);

	MinimumTelekinesisPosition = CreateDefaultSubobject<USceneComponent>(TEXT("MinimumPosition"));
	MinimumTelekinesisPosition->SetupAttachment(FirstPersonCameraComponent);

	MaximumTelekenesisPosition = CreateDefaultSubobject<USceneComponent>(TEXT("MaximumPosition"));
	MaximumTelekenesisPosition->SetupAttachment(FirstPersonCameraComponent);

}

void ATelekenesisCharacter::BeginPlay()
{
	// Call the base class  
	Super::BeginPlay();

	//Attach gun mesh component to Skeleton, doing it here because the skeleton is not yet created in the constructor
	FP_Gun->AttachToComponent(Mesh1P, FAttachmentTransformRules(EAttachmentRule::SnapToTarget, true), TEXT("GripPoint"));

	// Set Max Distance to Physics Grabbed ability 
	MaximumTelekenesisPosition->SetRelativeLocation(FVector(MaxLengthTelekinesis, 0.f, 0.f));

	TelekinesisUpSoundComponent = CreateAttachedSound(Mesh1P, HoldTelekinesisSound, true);
	
}

//////////////////////////////////////////////////////////////////////////
// Input
void ATelekenesisCharacter::SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent)
{
	// set up gameplay key bindings
	check(PlayerInputComponent);

	// Bind jump events
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACharacter::Jump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACharacter::StopJumping);

	// Bind telekinesis event
	PlayerInputComponent->BindAction("Fire", IE_Pressed, this, &ATelekenesisCharacter::TelekinesisUp);
	PlayerInputComponent->BindAction("Fire", IE_Released, this, &ATelekenesisCharacter::TelekinesisRelease);

	// Bind movement events
	PlayerInputComponent->BindAxis("MoveForward", this, &ATelekenesisCharacter::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ATelekenesisCharacter::MoveRight);

	// Bind rotate arround events 
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
}

void ATelekenesisCharacter::TelekinesisUp()
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
				TelekenesisPosition->SetWorldLocation(HitResult.Location);
				
				FVector TargetLocation = TelekenesisPosition->GetComponentLocation();
				FRotator TargetRotation = TelekenesisPosition->GetComponentRotation();

				// Set Grabbed Component desired location and rotation 
				PhysicHandle->SetTargetLocationAndRotation(TargetLocation, TargetRotation);

				bObjectGrabbed = true;

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

void ATelekenesisCharacter::TelekinesisRelease()
{
	bObjectGrabbed = false;

	if (PhysicHandle != nullptr && PhysicHandle->GetGrabbedComponent() != nullptr)
	{
		// Stop Grabbed our mesh 
		PhysicHandle->ReleaseComponent();

		// Stop Playing Telekinesis Sound 
		OnOffAttachedSound(TelekinesisUpSoundComponent, bObjectGrabbed);
	}
}

bool ATelekenesisCharacter::LineTrace(FHitResult& OutHit)
{

	// Get Our Camera Manager for trace 
	APlayerCameraManager* CameraManager = UGameplayStatics::GetPlayerCameraManager(this, 0);
	if (CameraManager != nullptr)
	{
		// Get location First trace
		FVector TraceStart = CameraManager->GetCameraLocation();

		// Culc Desired Trace length 
		float TraceLength = FVector(MaximumTelekenesisPosition->GetComponentLocation() - TraceStart).Size();

		// Get location End trace
		FVector TraceEnd = CameraManager->GetActorForwardVector() * TraceLength + TraceStart;

		// Line Trace 
		GetWorld()->LineTraceSingleByChannel(OutHit, TraceStart, TraceEnd, ECollisionChannel::ECC_Visibility);

		return OutHit.IsValidBlockingHit();
	}
	return false;
}

UAudioComponent* ATelekenesisCharacter::CreateAttachedSound(UPrimitiveComponent* RequiredSpawnComponent, USoundBase* SpawnedSound, bool PauseOnSpawn)
{
	// Try Spawn the Telekinesis sound if specified
	if (SpawnedSound != nullptr)
	{
		UAudioComponent* SpawnSound = UGameplayStatics::SpawnSoundAttached(SpawnedSound, RequiredSpawnComponent);

		// Set Default Sound value on the basis "DefaultCondition"
		SpawnSound->SetPaused(PauseOnSpawn);
		return SpawnSound;
	}
	else
	{
		FString CurrentActorName = ATelekenesisCharacter::GetName();
		UE_LOG(LogTemp, Warning, TEXT("SpawnedSound on &s .cpp 196 line == nullptr"), &CurrentActorName);
		return nullptr;
	}
}



void ATelekenesisCharacter::OnOffAttachedSound(UAudioComponent* ComponenToChange, bool Condition)
{
	ComponenToChange->SetPaused(!Condition);
}


void ATelekenesisCharacter::MoveForward(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorForwardVector(), Value);
	}
}

void ATelekenesisCharacter::MoveRight(float Value)
{
	if (Value != 0.0f)
	{
		// add movement in that direction
		AddMovementInput(GetActorRightVector(), Value);
	}
}
