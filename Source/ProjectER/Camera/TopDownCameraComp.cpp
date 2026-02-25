// Fill out your copyright notice in the Description page of Project Settings.

#include "TopDownCameraComp.h"

#include "CurvedWorldSubsystem.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "GameFramework/SpringArmComponent.h"
#include "LineOfSight/CameraVisionManager.h"

#include "LogHelper/DebugLogHelper.h"

//Log
DEFINE_LOG_CATEGORY(MainCameraComp);

// Sets default values for this component's properties
UTopDownCameraComp::UTopDownCameraComp()
{
	PrimaryComponentTick.bCanEverTick = true; // this is only for when the tick is never be used. not for switchable case
	PrimaryComponentTick.bStartWithTickEnabled = false;

	// Create but DO NOT attach here
	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->TargetArmLength = ArmLength;
	SpringArm->bDoCollisionTest = false;
	SpringArm->bEnableCameraLag = false;
	SpringArm->bInheritPitch = false;
	SpringArm->bInheritYaw  = false;
	SpringArm->bInheritRoll = false;
	SpringArm->SetRelativeRotation(FRotator(ArmPitch, 0.f, 0.f));

	CameraComp = CreateDefaultSubobject<UCameraComponent>(TEXT("CameraComp"));
	CameraComp->bUsePawnControlRotation = false;

	CameraVisionManager = CreateDefaultSubobject<UCameraVisionManager>(TEXT("CameraVisionManager"));

	UE_LOG(MainCameraComp, Log,
		TEXT("%s UTopDownCameraComp::Constructor >> Component created"),
		*DebugLogHelper::GetClientDebugName(this));
}

void UTopDownCameraComp::TickComponent(float DeltaTime, ELevelTick TickType,
                                       FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	UE_LOG(MainCameraComp, Verbose,
		TEXT("%s UTopDownCameraComp::TickComponent >> Ticking | FreeCamMode:%d | CurveSubSystem:%s | CurrentLoc:%s"),
		*DebugLogHelper::GetClientDebugName(this),
		bIsFreeCamMode,
		CurveSubSystem ? TEXT("Valid") : TEXT("NULL"),
		*GetComponentLocation().ToString());

	if (bIsFreeCamMode)
	{
		TickFreeCamMode(DeltaTime);
	}
	else
	{
		TickFollowMode(DeltaTime);
	}

	PendingKeyInput = FVector2D::ZeroVector;

	UpdateCameraTransform();

	if (CameraVisionManager)
	{
		CameraVisionManager->UpdateCameraLOS();
	}
}

void UTopDownCameraComp::OnRegister()
{
	Super::OnRegister();

	//attach in here
	if (SpringArm && !SpringArm->GetAttachParent())
	{
		SpringArm->SetupAttachment(this);
		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::OnRegister >> SpringArm attached to root"),
			*DebugLogHelper::GetClientDebugName(this));
	}

	if (CameraComp && !CameraComp->GetAttachParent())
	{
		CameraComp->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::OnRegister >> CameraComp attached to SpringArm"),
			*DebugLogHelper::GetClientDebugName(this));
	}
}

void UTopDownCameraComp::AddKeyPanInput(FVector2D Input)
{
	if (Input.IsNearlyZero()) return;

	PendingKeyInput += Input;

	if (!bIsFreeCamMode)
	{
		bIsCameraLocked = false; // 수동으로 이동하려 하므로 카메라 고정 강제 해제
		bIsFreeCamMode = true;
		FreeCamPivotLocation = GetComponentLocation();

		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::AddKeyPanInput >> Entered FREE CAM mode | Pivot: %s"),
			*DebugLogHelper::GetClientDebugName(this),
			*FreeCamPivotLocation.ToString());
	}
}

void UTopDownCameraComp::ToggleCameraMode()
{
	if (bIsFreeCamMode)
	{
		bIsCameraLocked = true; // Y키로 고정함
		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::ToggleCameraMode >> FREE CAM -> FOLLOW (Locked)"),
			*DebugLogHelper::GetClientDebugName(this));

		RecenterOnPawn();
	}
	else
	{
		bIsCameraLocked = false; // Y키로 고정 해제함
		bIsFreeCamMode = true;
		FreeCamPivotLocation = GetComponentLocation();

		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::ToggleCameraMode >> FOLLOW -> FREE CAM (Unlocked) | Pivot: %s"),
			*DebugLogHelper::GetClientDebugName(this),
			*FreeCamPivotLocation.ToString());
	}
}

void UTopDownCameraComp::SetFreeCamMode(bool bIsKeyPressed)
{
	// 스페이스바를 누르고 있으면(true) -> 자유 시점 해제(false)
	// 스페이스바를 떼면(false) -> 자유 시점 허용(true)
	bool bTargetFreeCamMode = !bIsKeyPressed;

	// 현재 상태와 같으면 무시
	if (bIsFreeCamMode == bTargetFreeCamMode) return;
	
	// 자유 모드로 전환(풀어줌) 시도 시, 만약 Y키로 하드락이 걸려있다면 무시
	if (bTargetFreeCamMode && bIsCameraLocked)
	{
		return;
	}
	
	bIsFreeCamMode = bTargetFreeCamMode;

	if (bIsFreeCamMode)
	{
		FreeCamPivotLocation = GetComponentLocation();
		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::SetFreeCamMode >> Changed to FREE CAM | Pivot: %s"),
			*DebugLogHelper::GetClientDebugName(this),
			*FreeCamPivotLocation.ToString());
	}
	else
	{
		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::SetFreeCamMode >> Changed to FOLLOW"),
			*DebugLogHelper::GetClientDebugName(this));
		RecenterOnPawn();
	}
}

void UTopDownCameraComp::InitializeCompRequirements()
{
	const FString DebugName = DebugLogHelper::GetClientDebugName(this);

	UE_LOG(MainCameraComp, Log,
		TEXT("%s UTopDownCameraComp::InitializeCompRequirements >> Called"),
		*DebugName);

	if (GetNetMode() == NM_DedicatedServer)
	{
		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::InitializeCompRequirements >> Dedicated server, skipping"),
			*DebugName);
		return;
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (!OwnerPawn)
	{
		UE_LOG(MainCameraComp, Error,
			TEXT("%s UTopDownCameraComp::InitializeCompRequirements >> Owner is not a Pawn, aborting"),
			*DebugName);
		return;
	}

	if (!OwnerPawn->IsLocallyControlled())
	{
		UE_LOG(MainCameraComp, Warning,
			TEXT("%s UTopDownCameraComp::InitializeCompRequirements >> Pawn is not locally controlled, skipping"),
			*DebugName);
		return;
	}

	UE_LOG(MainCameraComp, Log,
		TEXT("%s UTopDownCameraComp::InitializeCompRequirements >> Locally controlled Pawn confirmed, activating tick"),
		*DebugName);

	SetComponentTickEnabled(true);
	SmoothedFollowLocation = OwnerPawn->GetActorLocation();
	FreeCamPivotLocation   = SmoothedFollowLocation;

	UE_LOG(MainCameraComp, Log,
		TEXT("%s UTopDownCameraComp::InitializeCompRequirements >> Initial locations set | SmoothedFollow:%s | FreePivot:%s"),
		*DebugName,
		*SmoothedFollowLocation.ToString(),
		*FreeCamPivotLocation.ToString());

	PrepareRequirements();

	UpdateCameraCurveValues(CurveBendWeight);
	UpdateCameraTransform();

	UE_LOG(MainCameraComp, Log,
		TEXT("%s UTopDownCameraComp::InitializeCompRequirements >> Initialization complete"),
		*DebugName);
}

void UTopDownCameraComp::RecenterOnPawn()
{
	const FString DebugName = DebugLogHelper::GetClientDebugName(this);

	if (APawn* Owner = Cast<APawn>(GetOwner()))
	{
		const FVector PawnLoc = Owner->GetActorLocation();
		FreeCamPivotLocation   = PawnLoc;
		SmoothedFollowLocation = PawnLoc;

		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::RecenterOnPawn >> Snapped to pawn location: %s"),
			*DebugName,
			*PawnLoc.ToString());
	}
	else
	{
		UE_LOG(MainCameraComp, Warning,
			TEXT("%s UTopDownCameraComp::RecenterOnPawn >> Owner is not a Pawn!"),
			*DebugName);
	}

	bIsFreeCamMode = false;
}

void UTopDownCameraComp::TickFollowMode(float DeltaTime)
{
	APawn* Owner = Cast<APawn>(GetOwner());
	if (!Owner)
	{
		UE_LOG(MainCameraComp, Warning,
			TEXT("%s UTopDownCameraComp::TickFollowMode >> Owner is not a Pawn, skipping"),
			*DebugLogHelper::GetClientDebugName(this));
		return;
	}

	const FVector PawnLoc     = Owner->GetActorLocation();
	const FVector PrevSmoothed = SmoothedFollowLocation;

	SmoothedFollowLocation = FMath::VInterpTo(
		SmoothedFollowLocation,
		PawnLoc,
		DeltaTime,
		FollowLagSpeed);

	SetWorldLocation(SmoothedFollowLocation);
	FreeCamPivotLocation = SmoothedFollowLocation;

	UE_LOG(MainCameraComp, Verbose,
		TEXT("%s UTopDownCameraComp::TickFollowMode >> PawnLoc:%s | PrevSmoothed:%s | NewSmoothed:%s | CompLoc:%s"),
		*DebugLogHelper::GetClientDebugName(this),
		*PawnLoc.ToString(),
		*PrevSmoothed.ToString(),
		*SmoothedFollowLocation.ToString(),
		*GetComponentLocation().ToString());
}

void UTopDownCameraComp::TickFreeCamMode(float DeltaTime)
{
	const FVector2D EdgeInput    = GatherEdgeScrollInput();
	const FVector2D CombinedInput = PendingKeyInput + EdgeInput;

	UE_LOG(MainCameraComp, Verbose,
		TEXT("%s UTopDownCameraComp::TickFreeCamMode >> KeyInput:%s | EdgeInput:%s | CombinedInput:%s"),
		*DebugLogHelper::GetClientDebugName(this),
		*PendingKeyInput.ToString(),
		*EdgeInput.ToString(),
		*CombinedInput.ToString());

	ApplyPanInput(CombinedInput, DeltaTime);

	SetWorldLocation(FreeCamPivotLocation);

	UE_LOG(MainCameraComp, Verbose,
		TEXT("%s UTopDownCameraComp::TickFreeCamMode >> NewFreePivot:%s | CompLoc:%s"),
		*DebugLogHelper::GetClientDebugName(this),
		*FreeCamPivotLocation.ToString(),
		*GetComponentLocation().ToString());
}

void UTopDownCameraComp::PrepareRequirements()
{
	UE_LOG(MainCameraComp, Log,
		TEXT("%s UTopDownCameraComp::PrepareRequirements >> Caching subsystems"),
		*DebugLogHelper::GetClientDebugName(this));

	CacheCurveWorldSubsystem();

	if (CameraVisionManager)
	{
		CameraVisionManager->Initialize();

		UE_LOG(MainCameraComp, Log,
			TEXT("%s UTopDownCameraComp::PrepareRequirements >> CameraVisionManager initialized"),
			*DebugLogHelper::GetClientDebugName(this));
	}
	else
	{
		UE_LOG(MainCameraComp, Error,
			TEXT("%s UTopDownCameraComp::PrepareRequirements >> CameraVisionManager is NULL"),
			*DebugLogHelper::GetClientDebugName(this));
	}
}

void UTopDownCameraComp::CacheCurveWorldSubsystem()
{
	const FString DebugName = DebugLogHelper::GetClientDebugName(this);

	UCurvedWorldSubsystem* CurveSub = GetWorld()->GetSubsystem<UCurvedWorldSubsystem>();
	if (!CurveSub)
	{
		UE_LOG(MainCameraComp, Error,
			TEXT("%s UTopDownCameraComp::CacheCurveWorldSubsystem >> Failed to get UCurvedWorldSubsystem — UpdateCameraTransform will be skipped every tick!"),
			*DebugName);
		return;
	}

	CurveSubSystem = CurveSub;

	UE_LOG(MainCameraComp, Log,
		TEXT("%s UTopDownCameraComp::CacheCurveWorldSubsystem >> UCurvedWorldSubsystem cached successfully"),
		*DebugName);
}

FVector2D UTopDownCameraComp::GatherEdgeScrollInput() const
{
	APlayerController* PC = GetOwnerPlayerController();
	if (!PC)
	{
		UE_LOG(MainCameraComp, Verbose,
			TEXT("%s UTopDownCameraComp::GatherEdgeScrollInput >> No PlayerController, skipping edge scroll"),
			*DebugLogHelper::GetClientDebugName(this));
		return FVector2D::ZeroVector;
	}

	UGameViewportClient* Viewport = GetWorld()->GetGameViewport();
	if (!Viewport) return FVector2D::ZeroVector;

	FVector2D ViewportSize;
	Viewport->GetViewportSize(ViewportSize);
	if (ViewportSize.IsNearlyZero()) return FVector2D::ZeroVector;

	float MouseX = 0.f, MouseY = 0.f;
	if (!PC->GetMousePosition(MouseX, MouseY)) return FVector2D::ZeroVector;

	FVector2D EdgeInput = FVector2D::ZeroVector;

	if (MouseX < EdgeScrollMargin)
		EdgeInput.X = -1.f;
	else if (MouseX > ViewportSize.X - EdgeScrollMargin)
		EdgeInput.X =  1.f;

	if (MouseY < EdgeScrollMargin)
		EdgeInput.Y =  1.f;
	else if (MouseY > ViewportSize.Y - EdgeScrollMargin)
		EdgeInput.Y = -1.f;

	if (!EdgeInput.IsNearlyZero())
	{
		UE_LOG(MainCameraComp, Verbose,
			TEXT("%s UTopDownCameraComp::GatherEdgeScrollInput >> Edge scroll triggered | Mouse:(%f,%f) | Viewport:(%f,%f) | EdgeInput:%s"),
			*DebugLogHelper::GetClientDebugName(this),
			MouseX, MouseY,
			ViewportSize.X, ViewportSize.Y,
			*EdgeInput.ToString());
	}

	return EdgeInput * EdgeScrollSpeedMultiplier;
}

void UTopDownCameraComp::ApplyPanInput(FVector2D Direction, float DeltaTime)
{
	if (Direction.IsNearlyZero()) return;

	const FVector PrevPivot = FreeCamPivotLocation;

	FreeCamPivotLocation += FVector::RightVector   * Direction.X * PanSpeed * DeltaTime;
	FreeCamPivotLocation += FVector::ForwardVector * Direction.Y * PanSpeed * DeltaTime;

	UE_LOG(MainCameraComp, Verbose,
		TEXT("%s UTopDownCameraComp::ApplyPanInput >> Direction:%s | DeltaTime:%f | PanSpeed:%f | PrevPivot:%s | NewPivot:%s"),
		*DebugLogHelper::GetClientDebugName(this),
		*Direction.ToString(),
		DeltaTime,
		PanSpeed,
		*PrevPivot.ToString(),
		*FreeCamPivotLocation.ToString());
}

APlayerController* UTopDownCameraComp::GetOwnerPlayerController() const
{
	if (APawn* Owner = Cast<APawn>(GetOwner()))
		return Cast<APlayerController>(Owner->GetController());

	return nullptr;
}

void UTopDownCameraComp::UpdateCameraTransform()
{
	if (!CurveSubSystem)
	{
		UE_LOG(MainCameraComp, Verbose,
			TEXT("%s UTopDownCameraComp::UpdateCameraTransform >> CurveSubSystem is NULL, skipping"),
			*DebugLogHelper::GetClientDebugName(this));
		return;
	}

	const FVector WorldLocation = GetComponentLocation();
	const FRotator CamRotation = CameraComp->GetComponentRotation();
	const FVector  RawForward = CamRotation.Vector();
	const FVector  UpDirection = FVector::UpVector;

	const FVector ForwardDirection = FVector::VectorPlaneProject(RawForward, UpDirection).GetSafeNormal();
	const FVector RightDirection = FVector::CrossProduct(UpDirection, ForwardDirection).GetSafeNormal();

	UE_LOG(MainCameraComp, Verbose,
		TEXT("%s UTopDownCameraComp::UpdateCameraTransform >> Loc:%s | Forward:%s | Right:%s | Up:%s"),
		*DebugLogHelper::GetClientDebugName(this),
		*WorldLocation.ToString(),
		*ForwardDirection.ToString(),
		*RightDirection.ToString(),
		*UpDirection.ToString());

	CurveSubSystem->UpdateCameraParameters(WorldLocation, ForwardDirection, RightDirection, UpDirection);
}

void UTopDownCameraComp::UpdateCameraCurveValues(float RadialCurveStrength)
{
	if (!CurveSubSystem)
	{
		UE_LOG(MainCameraComp, Warning,
			TEXT("%s UTopDownCameraComp::UpdateCameraCurveValues >> CurveSubSystem is NULL, cannot update curve values"),
			*DebugLogHelper::GetClientDebugName(this));
		return;
	}

	UE_LOG(MainCameraComp, Log,
		TEXT("%s UTopDownCameraComp::UpdateCameraCurveValues >> Applying RadialCurveStrength:%f"),
		*DebugLogHelper::GetClientDebugName(this),
		RadialCurveStrength);

	CurveSubSystem->UpdateCurveParameters(-0.001f, -0.001f, RadialCurveStrength);
}

bool UTopDownCameraComp::ShouldUseClientLogic()
{
	const bool bIsClient = GetNetMode() != NM_DedicatedServer;

	UE_LOG(MainCameraComp, Verbose,
		TEXT("%s UTopDownCameraComp::ShouldUseClientLogic >> Result:%d"),
		*DebugLogHelper::GetClientDebugName(this),
		bIsClient);

	return bIsClient;
}