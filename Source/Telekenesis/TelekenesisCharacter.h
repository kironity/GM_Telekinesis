// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TelekenesisCharacter.generated.h"

class UInputComponent;

UCLASS(config=Game)
class ATelekenesisCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ATelekenesisCharacter();

	/** Pawn mesh: 1st person view (arms; seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category=Mesh)
	class USkeletalMeshComponent* Mesh1P;

	/** Gun mesh: 1st person view (seen only by self) */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USkeletalMeshComponent* FP_Gun;

	/** Location on gun mesh where projectiles should spawn. */
	UPROPERTY(VisibleDefaultsOnly, Category = Mesh)
	class USceneComponent* FP_MuzzleLocation;

	/** First person camera */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = Camera, meta = (AllowPrivateAccess = "true"))
	class UCameraComponent* FirstPersonCameraComponent;

	/** PhysicsHandle дл€ геймплейной фичи телекинез */
	UPROPERTY(EditDefaultsOnly, Category = "Telekenesis")
	class UPhysicsHandleComponent* PhysicHandle;

	/**  онтроль позиции меша подверженного телекинезу */
	UPROPERTY(EditDefaultsOnly, Category = "Telekenesis")
	class USceneComponent* TelekenesisPosition;

	/** ћинимальна€ дистанци€ расположени€ меша подверженного телекинезу */
	UPROPERTY(EditDefaultsOnly, Category = "Telekenesis|Position")
	class USceneComponent* MinimumTelekinesisPosition;

	/** ћаксимальна€ дистанци€ расположени€ меша подверженного телекинезу */
	UPROPERTY(EditDefaultsOnly, Category = "Telekenesis")
	class USceneComponent* MaximumTelekenesisPosition;

protected:

	virtual void BeginPlay();

public:

	/** Gun muzzle's offset from the characters location */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	FVector GunOffset;

	/** Sound to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category=Gameplay)
	class USoundBase* FireSound;

	/** AnimMontage to play each time we fire */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	class UAnimMontage* FireAnimation;

	/** Whether to use motion controller location for aiming. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = Gameplay)
	uint32 bUsingMotionControllers : 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Sound")
	class USoundBase* HoldTelekinesisSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Sound")
	class USoundBase* ThrowTelekinesisSound;

    // Set maximum value for telekenesis strength
	UPROPERTY(EditAnywhere, Category = "Telekinesis", meta = (ClampMin = 0.f))
	float MaxLengthTelekinesis;

protected:
	
	/** Telekinesis start */
	void TelekinesisUp();

	void TelekinesisRelease();

	/** @param - Filled incoming HitResult 
	    @return - Is Valid Blocking Hit return true  */
	bool LineTrace(FHitResult& OutHit);

	/** Spawn Sound at Selected PrimitiveComponent and Attach him 
	    @param RequiredSpawnComponent - A primitive that will hold the sound.
		@param SpawnedSound - the sound that will be played. 
		@param PauseOnSpawn - Default Value for Spawned sound. true = Play On Spawn, false = Don't play.
		@return - Component that was Spawned. */ 
	UAudioComponent* CreateAttachedSound(UPrimitiveComponent* RequiredSpawnComponent, USoundBase* SpawnedSound, bool PauseOnSpawn);

	void OnOffAttachedSound(UAudioComponent* ComponenToChange, bool Condition);
	/** Handles moving forward/backward */
	void MoveForward(float Val);

	/** Handles stafing movement, left and right */
	void MoveRight(float Val);

	// APawn interface
	virtual void SetupPlayerInputComponent(UInputComponent* InputComponent) override;
	// End of APawn interface

protected:

	class UAudioComponent* TelekinesisUpSoundComponent;
	bool bObjectGrabbed;

public:

	/** Returns Mesh1P subobject **/
	FORCEINLINE class USkeletalMeshComponent* GetMesh1P() const { return Mesh1P; }

	/** Returns FirstPersonCameraComponent subobject **/
	FORCEINLINE class UCameraComponent* GetFirstPersonCameraComponent() const { return FirstPersonCameraComponent; }

};

