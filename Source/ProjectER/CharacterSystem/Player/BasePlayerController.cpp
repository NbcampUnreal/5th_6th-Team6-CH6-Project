#include "CharacterSystem/Player/BasePlayerController.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "CharacterSystem/Data/InputConfig.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"

ABasePlayerController::ABasePlayerController()
{
	bShowMouseCursor = true; // 마우스 커서 표시 설정
	DefaultMouseCursor = EMouseCursor::Default;
}

void ABasePlayerController::BeginPlay()
{
	Super::BeginPlay();
	
	// Enhanced Input Subsystem에 매핑 컨텍스트 등록
	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ABasePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();
	
	// CastChecked를 써서 EnhancedInputComponent가 아니면 크래시(경고)를 냄
	if (UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent))
	{
		if (InputConfig)
		{
			// 이동(우클릭): 누르고 있는 동안, 그리고 눌렀을 때
			EnhancedInputComponent->BindAction(InputConfig->InputMove, ETriggerEvent::Started, this, &ABasePlayerController::OnMoveTriggered);
			EnhancedInputComponent->BindAction(InputConfig->InputMove, ETriggerEvent::Completed, this, &ABasePlayerController::OnMoveReleased);
			// 필요하다면 Canceled, Triggered 등도 바인딩
		}
	}
}

void ABasePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);
	
	// 마우스를 꾹 누르고 있으면 계속 이동 위치 갱신 
	if (bIsMousePressed)
	{
		MoveToMouseCursor();
	}
}

void ABasePlayerController::OnMoveTriggered()
{
	bIsMousePressed = true;
	MoveToMouseCursor(); // 클릭 즉시 1회 이동
}

void ABasePlayerController::OnMoveReleased()
{
	bIsMousePressed = false;
}

void ABasePlayerController::MoveToMouseCursor()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	FHitResult Hit;
	// 마우스 커서 위치의 땅을 찾음 (ECC_Visibility 채널 사용)
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		if (Hit.bBlockingHit)
		{
			// 네비게이션 시스템을 이용해 해당 위치로 이동 명령
			// [중요] CharacterMovementComponent가 NavMesh를 따라가게 함
			UAIBlueprintHelperLibrary::SimpleMoveToLocation(this, Hit.Location);
            
			// (옵션) 클릭 지점에 FX 재생
			// UNiagaraFunctionLibrary::SpawnSystemAtLocation(...);
		}
	}
}
