#include "CharacterSystem/Player/BasePlayerController.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Data/InputConfig.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"

#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "AbilitySystemBlueprintLibrary.h"

// [김현수 추가분] 상호작용 인터페이스 포함
#include "ItemSystem/I_ItemInteractable.h"

ABasePlayerController::ABasePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	bIsMousePressed = false;
	LastRPCUpdateTime = 0.f;
	CachedDestination = FVector::ZeroVector;

	// [김현수 추가분] 변수 초기화
	InteractionTarget = nullptr;
}

void ABasePlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
	{
		if (DefaultMappingContext)
		{
			Subsystem->AddMappingContext(DefaultMappingContext, 0);
		}
	}
}

void ABasePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledBaseChar = Cast<ABaseCharacter>(InPawn);

	if (!ControlledBaseChar)
	{
		UE_LOG(LogTemp, Warning, TEXT("OnPossess: ControlledBaseChar is Null!"));
	}
}

void ABasePlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	if (IsLocalController())
	{
		UEnhancedInputComponent* EnhancedInputComponent = CastChecked<UEnhancedInputComponent>(InputComponent);
		if (!EnhancedInputComponent || !InputConfig) return;

		EnhancedInputComponent->BindAction(InputConfig->InputMove, ETriggerEvent::Started, this, &ABasePlayerController::OnMoveStarted);
		EnhancedInputComponent->BindAction(InputConfig->InputMove, ETriggerEvent::Triggered, this, &ABasePlayerController::OnMoveTriggered);
		EnhancedInputComponent->BindAction(InputConfig->InputMove, ETriggerEvent::Completed, this, &ABasePlayerController::OnMoveReleased);
		EnhancedInputComponent->BindAction(InputConfig->InputMove, ETriggerEvent::Canceled, this, &ABasePlayerController::OnMoveReleased);

		EnhancedInputComponent->BindAction(InputConfig->StopMove, ETriggerEvent::Triggered, this, &ABasePlayerController::OnStopTriggered);

		/*for (const FInputData& Action : InputConfig->AbilityInputAction)
		{
			if (Action.InputAction && Action.InputTag.IsValid())
			{
				// Pressed 바인딩
				EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Started, this, &ABasePlayerController::AbilityInputTagPressed, Action.InputTag);

				// Released 바인딩 (차징 스킬 등을 위해 필요)
				EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &ABasePlayerController::AbilityInputTagReleased, Action.InputTag);
			}
		}*/
	}
}

void ABasePlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	// [김현수 추가분] 거리 체크 로직 호출
	CheckInteractionDistance();

	// 마우스를 꾹 누르고 있으면 계속 이동 위치 갱신 
	if (bIsMousePressed)
	{
		// 0.1초 쿨타임 체크
		if (GetWorld()->GetTimeSeconds() - LastRPCUpdateTime > RPCUpdateInterval)
		{
			//MoveToMouseCursor(); 태웅님 기존 코드
			// [김현수 추가분] 아이템 판별 기능이 포함된 함수로 변경 호출
			ProcessMouseInteraction();
			LastRPCUpdateTime = GetWorld()->GetTimeSeconds();
		}
	}
}

void ABasePlayerController::OnMoveStarted()
{
	bIsMousePressed = true;
	//MoveToMouseCursor(); 태웅님 기존 코드
	// [김현수 추가분] 아이템 판별 기능이 포함된 함수로 변경 호출
	ProcessMouseInteraction();
}

void ABasePlayerController::OnMoveTriggered()
{

}

void ABasePlayerController::OnMoveReleased()
{
	bIsMousePressed = false;
}

void ABasePlayerController::MoveToMouseCursor()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	if (!ControlledBaseChar)
	{
		ControlledBaseChar = Cast<ABaseCharacter>(ControlledPawn);
	}

	if (!IsValid(ControlledBaseChar))
	{
		UE_LOG(LogTemp, Warning, TEXT("MoveToMouseCursor: ControlledBaseChar is Null!"));
		return;
	}

	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		if (Hit.bBlockingHit)
		{
			ControlledBaseChar->MoveToLocation(Hit.Location);

			// SpawnDestinationEffect(Hit.Location);
		}
	}
}

// [김현수 추가분] 아이템 상호작용 프로세스 (기존 MoveToMouseCursor 로직 기반)
void ABasePlayerController::ProcessMouseInteraction()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	if (!ControlledBaseChar) ControlledBaseChar = Cast<ABaseCharacter>(ControlledPawn);
	if (!IsValid(ControlledBaseChar)) return;

	FHitResult Hit;
	if (GetHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		if (Hit.bBlockingHit)
		{
			// 마우스 아래 액터가 인터페이스를 구현했는지 확인
			AActor* HitActor = Hit.GetActor();
			if (HitActor && HitActor->GetClass()->ImplementsInterface(UI_ItemInteractable::StaticClass()))
			{
				InteractionTarget = HitActor;
			}
			else
			{
				InteractionTarget = nullptr;
			}

			ControlledBaseChar->MoveToLocation(Hit.Location);
		}
	}
}

// [김현수 추가분] 거리 체크 및 실제 습득 함수
void ABasePlayerController::CheckInteractionDistance()
{
	if (InteractionTarget && ControlledBaseChar)
	{
		float Distance = FVector::Dist(ControlledBaseChar->GetActorLocation(), InteractionTarget->GetActorLocation());
		if (Distance <= 150.f)
		{
			if (II_ItemInteractable* Interactable = Cast<II_ItemInteractable>(InteractionTarget))
			{
				Interactable->PickupItem(ControlledBaseChar);
			}
			InteractionTarget = nullptr;
		}
	}
}

void ABasePlayerController::OnStopTriggered()
{
	bIsMousePressed = false;

	// [김현수 추가분] 정지 시 상호작용 타겟 초기화
	InteractionTarget = nullptr;

	if (ControlledBaseChar)
	{
		ControlledBaseChar->StopMove();
	}
}

void ABasePlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;
	
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
	if (!ASC) return;

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Input Tag Pressed: %s"), *InputTag.ToString()));
}

void ABasePlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
}