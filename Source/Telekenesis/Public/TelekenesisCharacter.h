// Copyright 1998-2019 Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "TelekenesisCharacter.generated.h"

class UInputComponent;

UCLASS(config=Game)
class ATelekinesisCharacter : public ACharacter
{
	GENERATED_BODY()

public:

	ATelekinesisCharacter();

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
	UPROPERTY(EditInstanceOnly, Category = "Telekenesis")
	class USceneComponent* CurrentTelekinesisPower;

	/** ћинимальна€ дистанци€ расположени€ меша подверженного телекинезу */
	UPROPERTY(EditInstanceOnly, Category = "Telekenesis|Position")
	class USceneComponent* MinimumTelekinesisPower;

	/** ћаксимальна€ дистанци€ расположени€ меша подверженного телекинезу */
	UPROPERTY(EditInstanceOnly, Category = "Telekenesis")
	class USceneComponent* MaximumTelekinesisPower;

protected:

	virtual void BeginPlay();

	// Called every frame 
	virtual void Tick(float DeltaSeconds);

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

	/** On/Off Sound when we holdet grabbed component */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Properties")
	bool bPlayHoldSoundOnGrabbedComponent;

	/** Pause When Sound Spawned */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Properties", meta = (DisplayName = "SetSoundPauseWhenSpawn"))
	bool bPauseSoundOnSpawn;

	/** Base HoldedTelekinesisSound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Sound", meta = (EditCondition = "bPlayHoldSoundOnGrabbedComponent"))
	class USoundBase* HoldTelekinesisSound;

	/** Controll HoldTelekinesisSound Volume if sound specified */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Sound", meta = (EditCondition = "HoldTelekinesisSound != nullptr"))
	float HoldTelekinesisVolume;

	/** Base ThowTelekinesisSound */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Sound")
	class USoundBase* ThrowTelekinesisSound;

	/** Controll ThrowTelekinesisSound Volume if sound specified*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Telekinesis|Sound", meta = (EditCondition = "ThrowTelekinesisSound != nullptr"))
    float ThrowSoundVolume;

    /** Set maximum value for telekenesis strength */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Properties", meta = (ClampMin = 0.f))
	float MaxLengthTelekinesis;
	
	/** Mimimum value for interrupt Telekinesis  */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Properties", meta = (ClampMin = 200.f))
	float MinimumFailedDistance;

	/** How Strength we push  grabbed component */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Properties", meta = (ClampMin = 0.f))
	float ImpulseStrength;

	/** Value to change CurrentTelekinesisPower position between Minimum and Maximum Telekinesis Power */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Properties", meta = (ClampMin = 0.f))
	float StepDistanceValue;

	UPROPERTY(EditAnywhere, Category = "Telekinesis|Properties")
	bool bCanAffectCustomRender;

	/** Actor be able to spawn particle effects */
	UPROPERTY(EditAnywhere, Category = "Telekinesis|Effects", meta = (DisplayName = "ActorToReleaseEffect"))
	TSubclassOf<AActor> ThrowEffect;

protected:
	
	/** Input action  */
	void TelekinesisUp();
	/** Input action */
	void TelekinesisRelease();
	/** Input action */
	void ThrowObject();
	/** Input action */
	void WheelUp();
	/** Input action */
	void WheelDown();

	/** @param - Filled incoming HitResult 
	    @return - Is Valid Blocking Hit return true  */
	bool LineTrace(FHitResult& OutHit);

	/** Spawn Sound at Selected PrimitiveComponent and Attach him 
	    @param RequiredSpawnComponent - A primitive that will hold the sound.
		@param SpawnedSound - the sound that will be played. 
		@param PauseOnSpawn - Default Value for Spawned sound. true = Play On Spawn, false = Don't play.
		@return - Component that was Spawned. */ 
	UAudioComponent* CreateAttachedSound(UPrimitiveComponent* RequiredSpawnComponent, USoundBase* SpawnedSound, bool PauseOnSpawn);

	/** Pause sound 
	    @param ComponentToChange - AudioComponent to be change  
		@param Condition - true = unpause , false = set pause */
	void OnOffAttachedSound(UAudioComponent* ComponentToChange, bool Condition);

	/**  Interrupt Telekinesis when distance more then MaxOffset
	    @pararm ComponentToCheck - Our Grabbed Component with PhysicsHandle
		@param ComparedComponent - CurrentComponent which responsible for Telekinesis
		@param MaxOffset - Distance between Grabbed Component and Desired Position
		@warning - For correct work , MaxOffset must be more than 500.f */
	bool CheckHoldComponents(UPrimitiveComponent* GrabbedComponent, USceneComponent* ComparedComponent, float MaxOffset);

	/** GetLocation  and interpolate her to desired location */
	void InterpTo(FVector CurrentLocation, FVector DesiredLocation, USceneComponent* ComponentToChange, float StepDistance);

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

