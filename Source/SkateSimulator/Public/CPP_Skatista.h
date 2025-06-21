#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "Components/InputComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/PrimitiveComponent.h"
#include "Engine/EngineTypes.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "CPP_Skatista.generated.h"

UCLASS()
class SKATESIMULATOR_API ACPP_Skatista : public ACharacter
{
	GENERATED_BODY()

public:
	ACPP_Skatista();

protected:
	virtual void BeginPlay() override;

	UPROPERTY(EditAnywhere, Category = "Skate")
	float Acceleration = 500.0f;

	UPROPERTY(EditAnywhere, Category = "Skate")
	float MaxSkateSpeed = 1200.0f;

	UPROPERTY(EditAnywhere, Category = "Skate")
	float Friction = 200.0f;

	UPROPERTY(EditAnywhere, Category = "Skate")
	float TimeToMaxSpeed = 5.0f;

	UPROPERTY(EditAnywhere, Category = "Skate")
	float TurnAcceleration = 300.0f;

	UPROPERTY(EditAnywhere, Category = "Skate")
	float RotationInterpSpeed = 10.0f;

	UPROPERTY(EditAnywhere, Category = "Animations")
	UAnimSequence* IdleAnimation;

	UPROPERTY(EditAnywhere, Category = "Animations")
	UAnimSequence* PushAnimation;

	UAnimSequence* CurrentAnimation;

	FVector CurrentVelocity;
	float CurrentForwardSpeed;
	float ForwardInputValue;
	float TurnInputValue;
	bool bIsAcceleratingForward;
	bool bIsBraking;

	bool bIsInAir;
	bool bDidAttemptTrick;
	bool bTrickSuccessful;

	FVector LastGroundedPosition;
	USkeletalMeshComponent* MeshRef;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	USpringArmComponent* SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera")
	UCameraComponent* FollowCamera;

	UPROPERTY(EditAnywhere, Category = "Skate")
	UStaticMesh* SkateMesh;

	UStaticMeshComponent* SkateComponent;

	bool bIsPlayingPushAnimation;
	float PushAnimationTime;

	void ApplySkatePhysics(float DeltaTime);

	// Input
	void MoveForward(float Value);
	void MoveRight(float Value);
	void StartJump();
	void StopJump();
	void StartBrake();
	void StopBrake();

	UFUNCTION()
	void OnSkateOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
		UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult);

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;
};