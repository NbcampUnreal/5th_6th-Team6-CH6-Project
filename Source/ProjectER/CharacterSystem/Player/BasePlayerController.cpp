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

ABasePlayerController::ABasePlayerController()
{
	bShowMouseCursor = true; 
	DefaultMouseCursor = EMouseCursor::Default;
	
	bIsMousePressed = false;
	LastRPCUpdateTime = 0.f;
	CachedDestination = FVector::ZeroVector;
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
	
	// Possess 할 시 캐릭터 캐싱 
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
	
	// 마우스를 꾹 누르고 있으면 계속 이동 위치 갱신 
	if (bIsMousePressed)
	{
		MoveToMouseCursor();
	}
}

void ABasePlayerController::OnMoveStarted()
{
	bIsMousePressed = true;
	MoveToMouseCursor(); 
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
