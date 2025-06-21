#include "CPP_Skatista.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/InputComponent.h"
#include "Components/CapsuleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

ACPP_Skatista::ACPP_Skatista()
{
	PrimaryActorTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength = 400.0f;
	SpringArm->bUsePawnControlRotation = true;

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	bUseControllerRotationYaw = false;
	bUseControllerRotationPitch = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->bUseControllerDesiredRotation = false;

	GetCharacterMovement()->BrakingFrictionFactor = 0.0f;
	GetCharacterMovement()->GroundFriction = 0.0f;
	GetCharacterMovement()->MaxWalkSpeed = 0.0f;

	CurrentVelocity = FVector::ZeroVector;
	CurrentForwardSpeed = 0.0f;
	bIsAcceleratingForward = false;
	bIsBraking = false;
	bIsInAir = false;
	bDidAttemptTrick = false;
	bTrickSuccessful = false;
	CurrentAnimation = nullptr;

	bIsPlayingPushAnimation = false;
	PushAnimationTime = 0.0f;

	SkateMesh = nullptr;
	SkateComponent = nullptr;
}

void ACPP_Skatista::BeginPlay()
{
	Super::BeginPlay();

	if (SkateMesh)
	{
		SkateComponent = NewObject<UStaticMeshComponent>(this);
		SkateComponent->RegisterComponent();
		SkateComponent->SetStaticMesh(SkateMesh);
		SkateComponent->AttachToComponent(GetMesh(), FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("foot_l"));
	}

	MeshRef = GetMesh();
	MeshRef->SetAnimationMode(EAnimationMode::AnimationSingleNode);
	LastGroundedPosition = GetActorLocation();
	GetCharacterMovement()->JumpZVelocity = 725.f;

	GetCapsuleComponent()->OnComponentBeginOverlap.AddDynamic(this, &ACPP_Skatista::OnSkateOverlapBegin);
}

void ACPP_Skatista::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (ForwardInputValue > 0.1f && !bIsPlayingPushAnimation)
	{
		bIsPlayingPushAnimation = true;
		PushAnimationTime = 0.0f;

		if (PushAnimation && PushAnimation != CurrentAnimation)
		{
			GetMesh()->PlayAnimation(PushAnimation, false);
			CurrentAnimation = PushAnimation;
		}
	}
	else if (bIsPlayingPushAnimation)
	{
		PushAnimationTime += DeltaTime;

		if (PushAnimation && PushAnimationTime >= PushAnimation->GetPlayLength())
		{
			bIsPlayingPushAnimation = false;

			if (IdleAnimation && IdleAnimation != CurrentAnimation)
			{
				GetMesh()->PlayAnimation(IdleAnimation, true);
				CurrentAnimation = IdleAnimation;
			}
		}
	}
	else if (!bIsPlayingPushAnimation && IdleAnimation && CurrentAnimation != IdleAnimation)
	{
		GetMesh()->PlayAnimation(IdleAnimation, true);
		CurrentAnimation = IdleAnimation;
	}

	if (GetCharacterMovement()->IsFalling())
	{
		bIsInAir = true;
	}
	else
	{
		if (bIsInAir)
		{
			bIsInAir = false;
			if (bDidAttemptTrick && !bTrickSuccessful)
			{
				//EnterRagdoll();
			}
			else
			{
				bDidAttemptTrick = false;
				bTrickSuccessful = false;
				LastGroundedPosition = GetActorLocation();
			}
		}
	}
	if (bIsAcceleratingForward)
	{
		float AccelPerSecond = MaxSkateSpeed / TimeToMaxSpeed;
		CurrentForwardSpeed = FMath::Clamp(CurrentForwardSpeed + AccelPerSecond * DeltaTime, 0.0f, MaxSkateSpeed);
	}
	else
	{
		CurrentForwardSpeed = FMath::FInterpTo(CurrentForwardSpeed, 0.0f, DeltaTime, Friction / 100.0f);
	}

	FVector Forward = GetActorForwardVector();
	CurrentVelocity = Forward * CurrentForwardSpeed;

	if (FMath::Abs(TurnInputValue) > 0.1f)
	{
		FRotator CurrentRotation = GetActorRotation();
		CurrentRotation.Yaw += TurnInputValue * 2.0f;
		SetActorRotation(CurrentRotation);

		CurrentVelocity += GetActorForwardVector() * TurnAcceleration * DeltaTime;
	}

	ApplySkatePhysics(DeltaTime);

	if (!CurrentVelocity.IsNearlyZero())
	{
		AddMovementInput(CurrentVelocity.GetSafeNormal(), 1.0f);
	}
}

void ACPP_Skatista::ApplySkatePhysics(float DeltaTime)
{
	if (bIsBraking)
	{
		CurrentVelocity = FMath::VInterpTo(CurrentVelocity, FVector::ZeroVector, DeltaTime, Friction * 2.5f / 100.0f);
	}
	else if (!bIsAcceleratingForward)
	{
		CurrentVelocity = FMath::VInterpTo(CurrentVelocity, FVector::ZeroVector, DeltaTime, Friction / 100.0f);
	}

	if (!CurrentVelocity.IsNearlyZero())
	{
		FVector FlatVel = FVector(CurrentVelocity.X, CurrentVelocity.Y, 0.0f);
		FRotator TargetRot = FlatVel.ToOrientationRotator();
		FRotator NewRot = FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, RotationInterpSpeed);
		SetActorRotation(NewRot);
	}
}

void ACPP_Skatista::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	PlayerInputComponent->BindAxis("MoveForward", this, &ACPP_Skatista::MoveForward);
	PlayerInputComponent->BindAxis("MoveRight", this, &ACPP_Skatista::MoveRight);
	PlayerInputComponent->BindAxis("Turn", this, &APawn::AddControllerYawInput);
	PlayerInputComponent->BindAxis("LookUp", this, &APawn::AddControllerPitchInput);
	PlayerInputComponent->BindAction("Jump", IE_Pressed, this, &ACPP_Skatista::StartJump);
	PlayerInputComponent->BindAction("Jump", IE_Released, this, &ACPP_Skatista::StopJump);
	PlayerInputComponent->BindAction("Brake", IE_Pressed, this, &ACPP_Skatista::StartBrake);
	PlayerInputComponent->BindAction("Brake", IE_Released, this, &ACPP_Skatista::StopBrake);

}

void ACPP_Skatista::MoveForward(float Value)
{
	ForwardInputValue = Value;
	bIsAcceleratingForward = (Value > 0.1f);
}

void ACPP_Skatista::MoveRight(float Value)
{
	TurnInputValue = Value;
}

void ACPP_Skatista::StartJump()
{
	bPressedJump = true;
}

void ACPP_Skatista::StopJump()
{
	bPressedJump = false;
}

void ACPP_Skatista::StartBrake()
{
	bIsBraking = true;
}

void ACPP_Skatista::StopBrake()
{
	bIsBraking = false;
}

void ACPP_Skatista::OnSkateOverlapBegin(UPrimitiveComponent* OverlappedComp, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this) return;

	if (OtherActor->ActorHasTag("Water"))
	{
		UE_LOG(LogTemp, Warning, TEXT("Caiu na água! Respawnando..."));
		CurrentVelocity = FVector::ZeroVector;
		CurrentForwardSpeed = 0.0f;
		SetActorLocation(LastGroundedPosition);
	}
}

/* I would like to do more, but the time limit was very short for my abilities
I always seek perfomance and quality over time, but sometimes we need to go thru time limits
I hope this is enough to show my capacity and that I can do a project in Unreal
most debug texts are in portuguese since its my native language, all the other part of the code I tried to put in english (it could be that I forgot something)*/