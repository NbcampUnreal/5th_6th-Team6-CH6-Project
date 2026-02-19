// Fill out your copyright notice in the Description page of Project Settings.


#include "TopDownCameraComp.h"

#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"

//Log
DEFINE_LOG_CATEGORY(MainCameraComp);

// Sets default values for this component's properties
UTopDownCameraComp::UTopDownCameraComp()
{
	PrimaryComponentTick.bCanEverTick = true;

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(this);
	SpringArm->TargetArmLength = ArmLength;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = false; //handle lag manually because of the curve world update
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw  = false;
	SpringArm->bInheritRoll = false;

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->SetupAttachment(SpringArm, USpringArmComponent::SocketName); // attach here
	CameraComp->bUsePawnControlRotation = false;

	UE_LOG(MainCameraComp, Log,
		TEXT("[TopDownCameraComp] Constructor called"));
}


// Called when the camera comp is created(when possessed)
void UTopDownCameraComp::BeginPlay()
{
	Super::BeginPlay();

	if (!bInitialized)
	{
		InitializeCamera();
	}
	
}


// Called every frame
void UTopDownCameraComp::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!bInitialized) return;

	if (bIsFreeCamMode)
	{
		TickFreeCamMode(DeltaTime);
	}
	else
	{
		TickFollowMode(DeltaTime);
	}

	// Always clear key input at end of tick
	PendingKeyInput = FVector2D::ZeroVector;
}

void UTopDownCameraComp::InitializeCamera()
{
	// Arm length
	SpringArm->TargetArmLength = ArmLength;

	// Use relative rotation with inherit flags disabled
	// This works regardless of world transform readiness
	// unlike SetUsingAbsoluteRotation + SetWorldRotation which
	// depends on a valid world transform being present
	SpringArm->SetUsingAbsoluteRotation(false);
	SpringArm->SetRelativeRotation(FRotator(ArmPitch, 0.f, 0.f));
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw   = false;
	SpringArm->bInheritRoll  = false;

	// Activate camera
	CameraComp->SetActive(true);

	UE_LOG(MainCameraComp, Log,
		TEXT("[TopDownCameraComp] InitializeCamera — ArmLength: %.1f | ArmPitch: %.1f | SpringArm RelRot: %s"),
		ArmLength, ArmPitch,
		*SpringArm->GetRelativeRotation().ToString());

	bInitialized = true;
}

void UTopDownCameraComp::AddKeyPanInput(FVector2D Input)
{
	if (Input.IsNearlyZero()) return;// no meaningful input

	PendingKeyInput += Input;

	// Auto-enter free cam on first key press
	if (!bIsFreeCamMode)
	{
		bIsFreeCamMode = true;
		FreeCamPivotLocation = GetComponentLocation();

		UE_LOG(MainCameraComp, Log,
			TEXT("[TopDownCameraComp] AddKeyPanInput — Entered FREE CAM mode | Pivot: %s"),
			*FreeCamPivotLocation.ToString());
	}
}

void UTopDownCameraComp::ToggleCameraMode()
{
	if (bIsFreeCamMode)
	{
		UE_LOG(MainCameraComp, Log,
			TEXT("[TopDownCameraComp] ToggleCameraMode — FREE CAM -> FOLLOW"));
		RecenterOnPawn();
	}
	else
	{
		bIsFreeCamMode = true;
		FreeCamPivotLocation = GetComponentLocation();

		UE_LOG(MainCameraComp, Log,
			TEXT("[TopDownCameraComp] ToggleCameraMode — FOLLOW -> FREE CAM | Pivot: %s"),
			*FreeCamPivotLocation.ToString());
	}
}

void UTopDownCameraComp::RecenterOnPawn()
{
	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		const FVector PawnLoc = Owner->GetActorLocation();
		FreeCamPivotLocation = PawnLoc;
		SmoothedFollowLocation = PawnLoc;

		UE_LOG(MainCameraComp, Log,
			TEXT("[TopDownCameraComp] RecenterOnPawn — Snapped to: %s"),
			*PawnLoc.ToString());
	}
	else
	{
		UE_LOG(MainCameraComp, Warning,
			TEXT("[TopDownCameraComp] RecenterOnPawn — Owner is not a Pawn!"));
	}

	bIsFreeCamMode = false;
}

void UTopDownCameraComp::TickFollowMode(float DeltaTime)
{
	APawn* Owner = Cast<APawn>(GetOwner());
	if (!Owner) return;

	// Smooth lerp toward pawn position
	SmoothedFollowLocation = FMath::VInterpTo(
		SmoothedFollowLocation,
		Owner->GetActorLocation(),
		DeltaTime,
		FollowLagSpeed);

	SetWorldLocation(SmoothedFollowLocation);

	// Keep free cam pivot in sync so switching modes has no jump
	FreeCamPivotLocation = SmoothedFollowLocation;
}

void UTopDownCameraComp::TickFreeCamMode(float DeltaTime)
{
	// Gather edge scroll input (mouse near screen edges)
	const FVector2D EdgeInput = GatherEdgeScrollInput();

	// Combine key input and edge scroll input, then move pivot
	// Both are just directional values — same movement logic handles them
	const FVector2D CombinedInput = PendingKeyInput + EdgeInput;
	ApplyPanInput(CombinedInput, DeltaTime);

	// Drive this component to the new pivot location
	SetWorldLocation(FreeCamPivotLocation);
}

FVector2D UTopDownCameraComp::GatherEdgeScrollInput() const
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC) return FVector2D::ZeroVector;

	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!Viewport) return FVector2D::ZeroVector;

	FVector2D ViewportSize;
	Viewport->GetViewportSize(ViewportSize);
	if (ViewportSize.IsNearlyZero()) return FVector2D::ZeroVector;

	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY)) return FVector2D::ZeroVector;

	FVector2D EdgeInput = FVector2D::ZeroVector;

	// Horizontal
	if (MouseX < EdgeScrollMargin)
	{
		EdgeInput.X = -1.f;
	}
	else if (MouseX > ViewportSize.X - EdgeScrollMargin)
	{
		EdgeInput.X =  1.f;
	}


	// Vertical (screen Y up = world forward)
	if(MouseY < EdgeScrollMargin)
	{
		EdgeInput.Y =  1.f;
	}
	else if (MouseY > ViewportSize.Y - EdgeScrollMargin)
	{
		EdgeInput.Y = -1.f;
	}

	return EdgeInput * EdgeScrollSpeedMultiplier;
}

void UTopDownCameraComp::ApplyPanInput(FVector2D Direction, float DeltaTime)
{
	if (Direction.IsNearlyZero()) return;

	// Fixed top-down pitch-only camera, no yaw — world axes are constant:
	// Screen right = World X+ (RightVector)
	// Screen up = World Y+ (ForwardVector, derived from Z-up / neg gravity direction)
	// No rotation needed, axes never change
	
	FreeCamPivotLocation += FVector::RightVector   * Direction.X * PanSpeed * DeltaTime;
	FreeCamPivotLocation += FVector::ForwardVector * Direction.Y * PanSpeed * DeltaTime;
}

APlayerController* UTopDownCameraComp::GetOwnerPlayerController() const
{
	if (APawn* Owner = Cast<APawn>(GetOwner()))
		return Cast<APlayerController>(Owner->GetController());

	return nullptr;
}

