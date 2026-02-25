#include "CharacterSystem/Player/BasePlayerController.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Data/InputConfig.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "CharacterSystem/Interface/TargetableInterface.h"
#include "CharacterSystem/Player/BasePlayerState.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

#include "Kismet/GameplayStatics.h"
#include "Components/DecalComponent.h"
#include "GameFramework/Character.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Engine/World.h"
#include "Blueprint/AIBlueprintHelperLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"

// [김현수 추가분] 상호작용 인터페이스 포함
#include "ItemSystem/Interface/I_ItemInteractable.h"
#include "ItemSystem/Actor/BaseItemActor.h"
#include "ItemSystem/Actor/BaseBoxActor.h"
#include "ItemSystem/UI/W_LootingPopup.h"
#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "ItemSystem/Component/LootableComponent.h"

#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/GameMode/ER_OutGameMode.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "Blueprint/UserWidget.h"

//Camera comp added
#include "Camera/TopDownCameraComp.h"

// UI System
#include "UI/UI_MainHUD.h"


//Log
DEFINE_LOG_CATEGORY(Controller_Camera);

ABasePlayerController::ABasePlayerController()
{
	bShowMouseCursor = true;
	DefaultMouseCursor = EMouseCursor::Default;

	bIsMousePressed = false;
	bIsAttackInputMode = false;
	LastRPCUpdateTime = 0.f;
	CachedDestination = FVector::ZeroVector;

	AttackRangeDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("AttackRangeDecal"));
	AttackRangeDecal->SetupAttachment(RootComponent);
	AttackRangeDecal->SetVisibility(false); // 평소엔 꺼둠
	AttackRangeDecal->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));

	// [김현수 추가분] 변수 초기화
	InteractionTarget = nullptr;
	
	//Camera comp as null in the constructor. the caching will be done in the runtime --> on possess
	TopDownCameraComp = nullptr;
	
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

	// Get curved world subsystem reference
	if (UWorld* World = GetWorld())
	{
		CurvedWorldSubsystem = World->GetSubsystem<UCurvedWorldSubsystem>();
		if (!CurvedWorldSubsystem)
		{
			UE_LOG(LogTemp, Warning, TEXT("CurvedWorldSubsystem not found!"));
		}
	}
}

void ABasePlayerController::OnPossess(APawn* InPawn)
{
	Super::OnPossess(InPawn);

	ControlledBaseChar = Cast<ABaseCharacter>(InPawn);

	if (ControlledBaseChar)
	{
		// 포제스 할 때 캐릭터의 TopDownCameraComp를 가져오는 게 없었음 그래서 추가
		TopDownCameraComp = ControlledBaseChar->GetComponentByClass<UTopDownCameraComp>();
	}
	else
	{
		// UE_LOG(LogTemp, Warning, TEXT("OnPossess: ControlledBaseChar is Null!"));
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

		EnhancedInputComponent->BindAction(InputConfig->InputAttack, ETriggerEvent::Started, this, &ABasePlayerController::OnAttackModePressed);
		EnhancedInputComponent->BindAction(InputConfig->StopMove, ETriggerEvent::Triggered, this, &ABasePlayerController::OnStopTriggered);

		EnhancedInputComponent->BindAction(InputConfig->InputConfirm, ETriggerEvent::Started, this, &ABasePlayerController::OnConfirm);
		EnhancedInputComponent->BindAction(InputConfig->InputCancel, ETriggerEvent::Started, this, &ABasePlayerController::OnCanceled);

		for (const FInputData& Action : InputConfig->AbilityInputAction)
		{
			if (Action.InputAction && Action.InputTag.IsValid())
			{
				// Pressed 바인딩
				EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Started, this, &ABasePlayerController::AbilityInputTagPressed, Action.InputTag);

				// Released 바인딩 (차징 스킬 등을 위해 필요)
				EnhancedInputComponent->BindAction(Action.InputAction, ETriggerEvent::Completed, this, &ABasePlayerController::AbilityInputTagReleased, Action.InputTag);
			}
		}

		//Camera Control binding
		
		//   InputCameraPanX  (Axis1D) — A/D, Left/Right Arrow
		//   InputCameraPanY  (Axis1D) — W/S, Up/Down Arrow
		//   InputCameraToggle (Digital) — Y key
		if (InputConfig->InputCameraPanX)
		{
			EnhancedInputComponent->BindAction(
				InputConfig->InputCameraPanX,
				ETriggerEvent::Triggered,
				this,
				&ABasePlayerController::OnCameraPanX);
		}
		if (InputConfig->InputCameraPanY)
		{
			EnhancedInputComponent->BindAction(
				InputConfig->InputCameraPanY,
				ETriggerEvent::Triggered,
				this, &ABasePlayerController::OnCameraPanY);
		}
		if (InputConfig->InputCameraToggle)
		{
			EnhancedInputComponent->BindAction(
				InputConfig->InputCameraToggle,
				ETriggerEvent::Started,
				this, &ABasePlayerController::OnCameraToggle);
		}
		if (InputConfig->InputCameraHold)
		{
			EnhancedInputComponent->BindAction(
				InputConfig->InputCameraHold,
				ETriggerEvent::Started,
				this, &ABasePlayerController::OnCameraHold_Started);
			EnhancedInputComponent->BindAction(
				InputConfig->InputCameraHold,
				ETriggerEvent::Completed,
				this, &ABasePlayerController::OnCameraHold_Completed);
		}
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
			MoveToMouseCursor();
			LastRPCUpdateTime = GetWorld()->GetTimeSeconds();
		}
	}

	if (bIsAttackInputMode && ControlledBaseChar && AttackRangeDecal->IsVisible())
	{
		AttackRangeDecal->SetWorldLocation(ControlledBaseChar->GetActorLocation());
	}
}

void ABasePlayerController::OnRep_Pawn()
{
	Super::OnRep_Pawn();

	ControlledBaseChar = Cast<ABaseCharacter>(GetPawn());

	if (ControlledBaseChar)
	{
		TopDownCameraComp = ControlledBaseChar->GetComponentByClass<UTopDownCameraComp>();
	}

}

void ABasePlayerController::OnMoveStarted()
{
	if (bIsAttackInputMode)
	{
		CancelAttackMode();
	}

	Client_CloseLootUI();

	bIsMousePressed = true;
	MoveToMouseCursor();
}

void ABasePlayerController::OnMoveTriggered()
{

}

void ABasePlayerController::OnMoveReleased()
{
	FHitResult Hit;

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
		return;
	}

	FHitResult Hit;
	//if (GetHitResultUnderCursor(ECC_Visibility, false, Hit)) //2026/02/10
	if (GetCurvedHitResultUnderCursor(ECC_Visibility, false, Hit))//<- Replaced with a curve world accurate-hit result
	{
		if (Hit.bBlockingHit)
		{
			AActor* HitActor = Hit.GetActor();

			// [디버깅] 클릭 대상 확인
#if WITH_EDITOR
			if (HitActor)
			{
				UE_LOG(LogTemp, Log, TEXT("Clicked Actor: %s"), *HitActor->GetName());
			}
#endif

			if (ITargetableInterface* TargetObj = Cast<ITargetableInterface>(HitActor))
			{
				if (TargetObj->IsTargetable())
				{
					ETeamType MyTeam = ControlledBaseChar->GetTeamType();
					ETeamType TargetTeam = TargetObj->GetTeamType();

					bool bIsEnemy = (MyTeam != TargetTeam) &&
						(MyTeam != ETeamType::None) &&
						(TargetTeam != ETeamType::None);

					if (bIsEnemy)
					{
						/* === 공격 로직 === */
						ControlledBaseChar->SetTarget(HitActor); // 타겟 지정
#if WITH_EDITOR
						UE_LOG(LogTemp, Warning, TEXT("[%s] Set Target Actor -> %s"),
							*ControlledBaseChar->GetName(),
							HitActor ? *HitActor->GetName() : TEXT("None"));
#endif
						return;
					}
					else if (ABaseCharacter* HitChar = Cast<ABaseCharacter>(HitActor))
					{
						// [1-2] 타겟팅 불가능한데 만약 내 아군이고 빈사(Down) 상태라면? -> 부활 로직
						bool bIsAlly = (HitChar->GetTeamType() == ControlledBaseChar->GetTeamType());
						bool bIsDown = false;
					
						if (UAbilitySystemComponent* TargetASC = HitChar->GetAbilitySystemComponent())
						{
							bIsDown = TargetASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("State.Life.Down")));
						}

						if (bIsAlly && HitChar != ControlledBaseChar && bIsDown)
						{
							InteractionTargetDistance = FVector::Dist(ControlledBaseChar->GetActorLocation(), HitActor->GetActorLocation());
							InteractionTarget = HitActor; 
						
							ControlledBaseChar->SetTarget(nullptr); // 공격 타겟 초기화
							ControlledBaseChar->MoveToLocation(Hit.Location); // 아군을 향해 이동
							return; 
						}
					}	
				}
			}
			
			if (HitActor->GetComponentByClass<ULootableComponent>())
			{
				InteractionTargetDistance = FVector::Dist(ControlledBaseChar->GetActorLocation(), HitActor->GetActorLocation());
				InteractionTarget = HitActor; 
			}
			else if (HitActor->GetClass()->ImplementsInterface(UI_ItemInteractable::StaticClass()))
			{
				InteractionTargetDistance = FVector::Dist(ControlledBaseChar->GetActorLocation(), HitActor->GetActorLocation());
				InteractionTarget = HitActor; 
			}
			else
			{
				// 아무것도 아니면 (땅바닥 클릭) 타겟 초기화
				InteractionTarget = nullptr;
			}
			
			// 바닥(또는 아군) 클릭 시 이동
			UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledBaseChar);
			if (IsValid(ASC))
			{
				FGameplayTagContainer CancelTags;
				CancelTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action.AutoAttack")));
				ASC->CancelAbilities(&CancelTags);
			}

			ControlledBaseChar->SetTarget(nullptr);
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

			// LootableComponent가 있고 상호작용 가능한지 체크
			if (ULootableComponent* LootComp = HitActor->GetComponentByClass<ULootableComponent>())
			{
				// 몬스터인 경우 IsTargetable 체크 (살아있으면 루팅 불가)
				if (ITargetableInterface* TargetableActor = Cast<ITargetableInterface>(HitActor))
				{
					if (TargetableActor->IsTargetable())
					{
						// 몬스터가 살아있음 - 루팅 불가
						InteractionTarget = nullptr;
					}
					else
					{
						// 몬스터가 사망함 - 루팅 가능
						InteractionTargetDistance = FVector::Dist(ControlledBaseChar->GetActorLocation(), HitActor->GetActorLocation());
						InteractionTarget = HitActor;
					}
				}
				else
				{
					// 몬스터가 아닌 경우 (박스, 플레이어 시체 등) - 루팅 가능
					InteractionTargetDistance = FVector::Dist(ControlledBaseChar->GetActorLocation(), HitActor->GetActorLocation());
					InteractionTarget = HitActor;
				}
			}
			else if (HitActor && HitActor->GetClass()->ImplementsInterface(UI_ItemInteractable::StaticClass()))
			{
				// 멀리서 클릭해도 상호작용되도록 목표 거리 계산
				InteractionTargetDistance = FVector::Dist(ControlledBaseChar->GetActorLocation(), HitActor->GetActorLocation());
				InteractionTarget = HitActor; // 바닥에 떨어진 아이템 줍는용도
			}
			else
			{
				InteractionTarget = nullptr;
			}


			/*if (HitActor && HitActor->GetClass()->ImplementsInterface(UI_ItemInteractable::StaticClass()))
			{
				InteractionTarget = HitActor;
			}
			else
			{
				InteractionTarget = nullptr;
			}*/

			ControlledBaseChar->MoveToLocation(Hit.Location);
		}
	}
}

// [김현수 추가분] 거리 체크 및 실제 습득 함수
void ABasePlayerController::CheckInteractionDistance()
{
	if (InteractionTarget && ControlledBaseChar)
	{
		float CurrentDistance = FVector::Dist(ControlledBaseChar->GetActorLocation(), InteractionTarget->GetActorLocation());
		

		// InteractionTargetDistance는 처음 클릭했을 때의 거리
		if (CurrentDistance <= 150.f) // 거리에 따라서, 박스에 닿기 직전에 멈추는 현상이 남아 있음 수정 필요
		{
			if (ControlledBaseChar)
			{
				ControlledBaseChar->StopMove();
			}
			// 1. 타겟 상호작용 종류 판별 및 우선 실행 (UI 띄우기 / 스킬 쓰기)
			if (ABaseCharacter* TargetChar = Cast<ABaseCharacter>(InteractionTarget))
			{
				// 타겟이 나와 같은 팀인지 확인
				if (TargetChar->GetTeamType() == ControlledBaseChar->GetTeamType())
				{
					// 내 ASC를 가져와서 부활 스킬(GA_Revive)을 태그로 강제 실행시킵니다.
					if (UAbilitySystemComponent* ASC = ControlledBaseChar->GetAbilitySystemComponent())
					{
						FGameplayTag ReviveTag = FGameplayTag::RequestGameplayTag(FName("Ability.Action.Revive"));
						ASC->TryActivateAbilitiesByTag(FGameplayTagContainer(ReviveTag));
						UE_LOG(LogTemp, Warning, TEXT("아군 구조 스킬 발동 시도!"));
					}
					
					InteractionTarget = nullptr; 
				}
			}
			else if (II_ItemInteractable* Interactable = Cast<II_ItemInteractable>(InteractionTarget))
			{
				// 땅바닥 아이템 (BaseItemActor)
				if (ABaseItemActor* AAA = Cast<ABaseItemActor>(Interactable))
				{
					AAA->PickupItem(ControlledBaseChar);
					Server_RequestPickup(AAA);
					InteractionTarget = nullptr;
				}
			}
			else // LootableComponent가 있는 액터 (박스, 플레이어 시체, 몬스터 시체 등)
			{
				if (InteractionTarget)
				{
					Server_BeginLoot(InteractionTarget);
					InteractionTarget = nullptr;
					
				}
			}
			
			// 2. 상호작용 명령을 모두 내렸으니 이제서야 캐릭터를 정지시킴 (부드러운 정지 및 창 띄우기 동시 체감)
			if (ControlledBaseChar)
			{
				//ControlledBaseChar->StopMove();
			}
		}
	}
}

void ABasePlayerController::OnConfirm()
{
	if (!bIsAttackInputMode)
	{
		APawn* ControlledPawn = GetPawn();
		if (!ControlledPawn) return;

		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
		if (IsValid(ASC)) {
			ASC->LocalInputConfirm();
		}

		return;
	}

	FHitResult Hit;
	if (GetCurvedHitResultUnderCursor(ECC_Visibility, false, Hit))
	{
		if (Hit.bBlockingHit)
		{
			RequestAttackMove(Hit);
		}
	}

	CancelAttackMode();
}

void ABasePlayerController::OnCanceled() {
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
	if (IsValid(ASC)) {
		//UE_LOG(LogTemp, Log, TEXT("OnCanceled"));
		ASC->LocalInputCancel();
	}
}

void ABasePlayerController::OnAttackModePressed()
{
	if (!ControlledBaseChar) return;

	bIsAttackInputMode = true; // 공격 모드 활성화

	// 사거리 가져오기 (Stat 컴포넌트나 AttributeSet에서)
	float AttackRange = ControlledBaseChar->GetAttackRange();

	// 데칼 크기 조절 (X, Y는 반지름, Z는 두께)
	AttackRangeDecal->DecalSize = FVector(100.0f, AttackRange, AttackRange);

	// 캐릭터 위치로 데칼 이동 (바닥에 붙이기)
	AttackRangeDecal->SetWorldLocation(ControlledBaseChar->GetActorLocation());
	AttackRangeDecal->SetVisibility(true);

	// (선택) 마우스 커서를 공격 전용 커서로 변경
	// CurrentMouseCursor = EMouseCursor::Crosshairs;
}

void ABasePlayerController::CancelAttackMode()
{
	if (bIsAttackInputMode)
	{
		bIsAttackInputMode = false;
		if (AttackRangeDecal)
		{
			AttackRangeDecal->SetVisibility(false);
		}
		// CurrentMouseCursor = DefaultMouseCursor;
	}
}

void ABasePlayerController::RequestAttackMove(const FHitResult& Hit)
{
	if (!ControlledBaseChar) return;

	AActor* HitActor = Hit.GetActor();

	// Case A: 적을 직접 클릭함 -> 타겟팅 공격
	if (ITargetableInterface* TargetObj = Cast<ITargetableInterface>(HitActor))
	{
		if (TargetObj->IsTargetable()) // 적군인지 확인하는 로직 추가 필요 (기존 코드 참고)
		{
			ETeamType MyTeam = ControlledBaseChar->GetTeamType();
			ETeamType TargetTeam = TargetObj->GetTeamType();

			if (MyTeam != TargetTeam && TargetTeam != ETeamType::None)
			{
				ControlledBaseChar->SetTarget(HitActor); // 타겟 설정
				return;
			}
		}
	}

	// Case B: 땅을 클릭함 -> 어택 땅 (이동하다가 적 만나면 공격)
	// 기존 MoveToLocation은 무시하고 이동만 하므로, 새로운 함수 필요
	ControlledBaseChar->Server_AttackMoveToLocation(Hit.Location);
}

void ABasePlayerController::OnCameraPanX(const FInputActionValue& Value)
{
	if (IsValid(TopDownCameraComp))
	{
		FVector2D PanXValue=FVector2D(Value.Get<float>(), 0.f);
		TopDownCameraComp->AddKeyPanInput(PanXValue);
		
		UE_LOG(Controller_Camera, Warning,
			TEXT("ABasePlayerController::OnCameraPanX >> CameraPanX[%s]"),
			*PanXValue.ToString());
	}
}

void ABasePlayerController::OnCameraPanY(const FInputActionValue& Value)
{
	if (IsValid(TopDownCameraComp))
	{
		FVector2D PanXValue=FVector2D(0.f, Value.Get<float>());
		TopDownCameraComp->AddKeyPanInput(PanXValue);

		UE_LOG(Controller_Camera, Warning,
			TEXT("ABasePlayerController::OnCameraPanY >> CameraPanX[%s]"),
			*PanXValue.ToString());
	}
}

void ABasePlayerController::OnCameraToggle()
{
	if (IsValid(TopDownCameraComp))
	{
		TopDownCameraComp->ToggleCameraMode();

		FString ModeString=TopDownCameraComp->IsCameraPanFreeMode()? TEXT("Free") : TEXT("AttachedMode");
		
		UE_LOG(Controller_Camera, Warning,
			TEXT("ABasePlayerController::OnCameraToggle >> Toggled to %s"),
			*ModeString);
	}
}

void ABasePlayerController::OnCameraHold_Started()
{
	if (IsValid(TopDownCameraComp))
	{
		TopDownCameraComp->SetFreeCamMode(true);
	}
}

void ABasePlayerController::OnCameraHold_Completed()
{
	if (IsValid(TopDownCameraComp))
	{
		TopDownCameraComp->SetFreeCamMode(false);
	}
}




/*void ABasePlayerController::Test_ChangeTeamToA()
{
	if (ControlledBaseChar)
	{
		ControlledBaseChar->Server_SetTeamID(ETeamType::Team_A);
		UE_LOG(LogTemp, Log, TEXT("Request Change Team to A"));
	}
}*/

/*void ABasePlayerController::Test_ChangeTeamToB()
{
	if (ControlledBaseChar)
	{
		ControlledBaseChar->Server_SetTeamID(ETeamType::Team_B);
		UE_LOG(LogTemp, Log, TEXT("Request Change Team to B"));
	}
}*/

/*void ABasePlayerController::Test_ReviveInput()
{
	if (ControlledBaseChar)
	{
		FVector CurrentLocation = ControlledBaseChar->GetActorLocation();

		// 서버에게 부활 요청
		ControlledBaseChar->Server_Revive(CurrentLocation);
	}
}*/

/*void ABasePlayerController::Test_GainXP()
{
	UE_LOG(LogTemp, Warning, TEXT("[Client] Zero Key Pressed!"));

	Server_TestGainXP();
}*/

/*void ABasePlayerController::Server_TestGainXP_Implementation()
{
	if (ControlledBaseChar)
	{
		if (UAbilitySystemComponent* ASC = ControlledBaseChar->GetAbilitySystemComponent())
		{
			if (ABasePlayerState* PS = ControlledBaseChar->GetPlayerState<ABasePlayerState>())
			{
				UBaseAttributeSet* AS = PS->GetAttributeSet();
				if (AS)
				{
					ASC->ApplyModToAttribute(AS->GetIncomingXPAttribute(), EGameplayModOp::Additive, 100.0f);

					UE_LOG(LogTemp, Log, TEXT("[Server] Gained 100 XP"));
				}
			}
		}
	}
}*/

void ABasePlayerController::OnStopTriggered()
{
	bIsMousePressed = false;

	// [김현수 추가분] 정지 시 상호작용 타겟 초기화
	InteractionTarget = nullptr;

	if (ControlledBaseChar)
	{
		ControlledBaseChar->SetTarget(nullptr);
		ControlledBaseChar->StopMove();
	}
}

void ABasePlayerController::AbilityInputTagPressed(FGameplayTag InputTag)
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(ControlledPawn);
	if (!ASC) return;

	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			if (Spec.IsActive())
			{
				// [방법 2 핵심] 태그를 담은 이벤트를 어빌리티에 직접 쏩니다.
				FGameplayEventData Payload;
				Payload.EventTag = InputTag; // 전달할 태그
				Payload.Instigator = this;   // 보낸 사람

				// 활성화된 어빌리티에게 이벤트를 전달합니다.
				ASC->HandleGameplayEvent(InputTag, &Payload);
				UE_LOG(LogTemp, Log, TEXT("Gameplay Event Sent: %s"), *InputTag.ToString());
			}
			else
			{
				ASC->TryActivateAbility(Spec.Handle);
			}
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, FString::Printf(TEXT("Input Tag Pressed: %s"), *InputTag.ToString()));
}

void ABasePlayerController::AbilityInputTagReleased(FGameplayTag InputTag)
{
}

// ------------------------------------------------------------
// [전민성 추가분]
void ABasePlayerController::ConnectToDedicatedServer(const FString& Ip, int32 Port, const FString& PlayerName)
{
	if (!IsLocalController())
		return;

	const FString Address = FString::Printf(TEXT("%s:%d?PlayerName=%s"), *Ip, Port, *PlayerName);

	UE_LOG(LogTemp, Log, TEXT("[PC] Connecting to server: %s"), *Address);

	ClientTravel(Address, TRAVEL_Absolute);
}

void ABasePlayerController::Client_SetLose_Implementation()
{
	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	PS->bIsLose = true;
	ShowLoseUI();
}

void ABasePlayerController::Client_SetWin_Implementation()
{
	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	PS->bIsWin = true;
	ShowWinUI();
}

void ABasePlayerController::Client_SetDead_Implementation()
{
	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	PS->bIsDead = true;
}

void ABasePlayerController::Client_StartRespawnTimer_Implementation()
{
	ShowRespawnTimerUI();
}

void ABasePlayerController::Client_StopRespawnTimer_Implementation()
{
	HideRespawnTimerUI();
}

void ABasePlayerController::Client_OutGameInputMode_Implementation()
{
	FInputModeUIOnly InputMode;
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ABasePlayerController::Client_InGameInputMode_Implementation()
{
	FInputModeGameAndUI InputMode;
	InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
	InputMode.SetHideCursorDuringCapture(false);
	SetInputMode(InputMode);
	bShowMouseCursor = true;
}

void ABasePlayerController::Client_ReturnToMainMenu_Implementation(const FString& Reason)
{
	UE_LOG(LogTemp, Warning, TEXT("ReturnToMainMenu: %s"), *Reason);
	UGameplayStatics::OpenLevel(this, FName(TEXT("/Game/Level/Level_MainMenu")));
}

void ABasePlayerController::Server_StartGame_Implementation()
{
	auto OutGameMode = Cast<AER_OutGameMode>(GetWorld()->GetAuthGameMode());
	OutGameMode->StartGame();
}

void ABasePlayerController::Server_DisConnectServer_Implementation()
{
	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());

	InGameMode->DisConnectClient(this);
}

void ABasePlayerController::Server_TEMP_SpawnNeutrals_Implementation()
{
	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
	InGameMode->TEMP_SpawnNeutrals();
}

void ABasePlayerController::Server_TEMP_DespawnNeutrals_Implementation()
{
	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
	InGameMode->TEMP_DespawnNeutrals();
}

void ABasePlayerController::Server_MoveTeam_Implementation(int32 TeamIdx)
{
	auto OutGameMode = Cast<AER_OutGameMode>(GetWorld()->GetAuthGameMode());
	OutGameMode->MoveTeam(this, TeamIdx);
}

void ABasePlayerController::Server_RequestPickup_Implementation(ABaseItemActor* Item)
{ // 바닥에 있는 아이템 줍기
	if (!Item) return;

	APawn* PlayerPawn = GetPawn();
	if (!PlayerPawn) return;

	constexpr float MaxDist = 200.f;
	if (FVector::DistSquared(PlayerPawn->GetActorLocation(), Item->GetActorLocation()) > FMath::Square(MaxDist))
		return;

	Item->PickupItem(PlayerPawn);
}

// 박스 아이템 루팅 RPC 시작
void ABasePlayerController::Server_BeginLoot_Implementation(AActor* Actor)
{
	if (!Actor) return;

	ABaseCharacter* Char = Cast<ABaseCharacter>(GetPawn());
	if (!Char) return;

	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	if (!PS) return;

	// 서버 권위 거리 검증 (치트 방지)
	const float Dist = FVector::Dist(Char->GetActorLocation(), Actor->GetActorLocation());
	if (Dist > 150.f) return;

	// 루팅 시작 시 캐릭터 정지
	Char->StopMove();

	// Target에 Actor를 담는다
	FGameplayEventData Payload;
	Payload.Instigator = Char;
	Payload.Target = Actor;

	const FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Interact.OpenBox"));

	// GA 트리거: Char(Avatar)에게 이벤트 보냄
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PS, EventTag, Payload);
}

void ABasePlayerController::Server_EndLoot_Implementation()
{

	// GA_OpenBox 종료
	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	if (PS)
	{
		UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
		if (ASC)
		{
			FGameplayTagContainer CancelTags;
			CancelTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Event.Interact.OpenBox")));
			ASC->CancelAbilities(&CancelTags);

			UE_LOG(LogTemp, Log, TEXT("Server_EndLoot: Cancelled OpenBox ability"));
		}
	}
}

void ABasePlayerController::Server_TakeItem_Implementation(ABaseBoxActor* Box, int32 SlotIndex)
{
	////box->TryTakeItem 예정
	//UBaseInventoryComponent* Inv = GetPawn()->FindComponentByClass<UBaseInventoryComponent>();
	//UBaseItemData* TargetItem = Box->GetItemData(SlotIndex);
	////Inv->AddItem(TargetItem);
	//// 여기서 아이템 인벤에 넣기
	//Box->ReduceItem(SlotIndex);

	// 유효성 검증
	if (!Box || !GetPawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_TakeItem: Invalid Box or Pawn"));
		return;
	}

	// 슬롯 범위 체크
	if (SlotIndex < 0 || SlotIndex >= Box->CurrentItemList.Num())
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_TakeItem: Invalid SlotIndex %d"), SlotIndex);
		return;
	}

	// 빈 슬롯 체크
	if (Box->CurrentItemList[SlotIndex].ItemId == -1)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_TakeItem: Empty slot at index %d"), SlotIndex);
		return;
	}

	// 인벤토리 컴포넌트 찾기
	UBaseInventoryComponent* Inv = GetPawn()->FindComponentByClass<UBaseInventoryComponent>();
	if (!Inv)
	{
		UE_LOG(LogTemp, Error, TEXT("Server_TakeItem: No InventoryComponent found"));
		return;
	}

	// 아이템 데이터 가져오기 (범위 체크 포함)
	UBaseItemData* TargetItem = Box->GetItemData(SlotIndex);
	if (!TargetItem)
	{
		UE_LOG(LogTemp, Error, TEXT("Server_TakeItem: Failed to get item data"));
		return;
	}

	// 인벤토리에 추가 (주석 해제!)
	if (Inv->AddItem(TargetItem))
	{
		// 성공 시에만 박스에서 제거
		Box->ReduceItem(SlotIndex);
	}
	else
	{
		// 인벤토리가 꽉 찬 경우
		UE_LOG(LogTemp, Warning, TEXT("Server_TakeItem: Inventory full, cannot add item"));
	}
}

void ABasePlayerController::Client_OpenLootUI_Implementation(const AActor* Box)
{
	UE_LOG(LogTemp, Log, TEXT("Client_OpenLootUI START"));

	if (!Box || !LootWidgetClass) return;

	// 중복 방지
	if (IsValid(LootWidgetInstance))
	{
		LootWidgetInstance->RemoveFromParent();
		LootWidgetInstance = nullptr;
	}

	LootWidgetInstance = CreateWidget<UW_LootingPopup>(this, LootWidgetClass);

	if (IsValid(LootWidgetInstance))
	{
		LootWidgetInstance->InitPopup(Box);
		LootWidgetInstance->AddToViewport(10);
		LootWidgetInstance->UpdateLootingSlots(Box);
		LootWidgetInstance->Refresh();
	}
}

void ABasePlayerController::Client_CloseLootUI_Implementation()
{
	if (!IsValid(LootWidgetInstance))
	{
		return;
	}
	/// 25.02.18. mpyi _ 루팅창 꺼질 때 툴팁도 꺼지도록 추가
	LootWidgetInstance->HideTooltip();
	LootWidgetInstance->RemoveFromParent();
	LootWidgetInstance = nullptr;
}

void ABasePlayerController::Server_BeginLootFromActor_Implementation(AActor* TargetActor)
{
	if (!TargetActor)
		return;

	ABaseCharacter* Char = Cast<ABaseCharacter>(GetPawn());
	if (!Char)
		return;

	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	if (!PS)
		return;

	// LootableComponent 찾기
	ULootableComponent* LootComp = TargetActor->FindComponentByClass<ULootableComponent>();
	if (!LootComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_BeginLootFromActor: No LootableComponent found"));
		return;
	}

	// 거리 체크
	const float Dist = FVector::Dist(Char->GetActorLocation(), TargetActor->GetActorLocation());
	if (Dist > 500.f)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_BeginLootFromActor: Too far (%.1f)"), Dist);
		return;
	}

	// 루팅 가능한지 확인
	if (!LootComp->HasLootRemaining())
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_BeginLootFromActor: No loot remaining"));
		return;
	}

	// 루팅 시작 시 캐릭터 정지
	Char->StopMove();

	// GA 트리거
	FGameplayEventData Payload;
	Payload.Instigator = Char;
	Payload.Target = TargetActor;

	const FGameplayTag EventTag = FGameplayTag::RequestGameplayTag(TEXT("Event.Interact.OpenBox"));
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(PS, EventTag, Payload);

	UE_LOG(LogTemp, Log, TEXT("Server_BeginLootFromActor: Opening loot for %s"), *TargetActor->GetName());
}

void ABasePlayerController::Server_TakeItemFromActor_Implementation(const AActor* TargetActor, int32 SlotIndex)
{
	if (!TargetActor || !GetPawn())
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_TakeItemFromActor: Invalid actor or pawn"));
		return;
	}

	// LootableComponent 찾기
	ULootableComponent* LootComp = TargetActor->FindComponentByClass<ULootableComponent>();
	if (!LootComp)
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_TakeItemFromActor: No LootableComponent found"));
		return;
	}

	// LootableComponent의 TakeItem 호출
	if (LootComp->TakeItem(SlotIndex, GetPawn()))
	{
		UE_LOG(LogTemp, Log, TEXT("Server_TakeItemFromActor: Item taken successfully"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Server_TakeItemFromActor: Failed to take item"));
	}
}




void ABasePlayerController::UI_RespawnStart(float RespawnTime)
{
	if (IsValid(MainHUD))
	{
		MainHUD->StartRespawn(RespawnTime);
	}
}

void ABasePlayerController::ShowWinUI()
{
	if (!WinUIClass)
		return;

	if (IsValid(WinUIInstance))
		return;

	UE_LOG(LogTemp, Log, TEXT("[PC] : ShowWinUI"));
	WinUIInstance = CreateWidget<UUserWidget>(this, WinUIClass);
	WinUIInstance->AddToViewport();
}

void ABasePlayerController::ShowLoseUI()
{
	if (!LoseUIClass)
		return;

	if (IsValid(LoseUIInstance))
		return;

	UE_LOG(LogTemp, Log, TEXT("[PC] : ShowLoseUI"));
	LoseUIInstance = CreateWidget<UUserWidget>(this, LoseUIClass);
	LoseUIInstance->AddToViewport();
}

void ABasePlayerController::ShowRespawnTimerUI()
{
	if (!RespawnUIClass)
		return;

	if (IsValid(RespawnUIInstance))
		return;

	UE_LOG(LogTemp, Log, TEXT("[PC] : ShowRespawnUI"));
	RespawnUIInstance = CreateWidget<UUserWidget>(this, RespawnUIClass);
	RespawnUIInstance->AddToViewport();
}

void ABasePlayerController::HideRespawnTimerUI()
{
	if (IsValid(RespawnUIInstance))
	{
		RespawnUIInstance->RemoveFromParent();
		RespawnUIInstance = nullptr;
	}
}

bool ABasePlayerController::GetCurvedHitResultUnderCursor(ECollisionChannel TraceChannel, bool bTraceComplex,
	FHitResult& OutHitResult)
{
	if (!CurvedWorldSubsystem)
	{
		// Fallback to normal trace if subsystem not available
		return GetHitResultUnderCursor(TraceChannel, bTraceComplex, OutHitResult);
	}

	// Use curved world corrected trace -> for now, just do the z height only modification
	return FCurvedWorldUtil::GetHitResultUnderCursorCorrected(
		this,
		CurvedWorldSubsystem,
		OutHitResult,
		TraceChannel,
		ECurveMathType::ZHeightOnly);
}



