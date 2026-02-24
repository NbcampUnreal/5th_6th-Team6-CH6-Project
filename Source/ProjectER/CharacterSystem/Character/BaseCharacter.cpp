#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Player/BasePlayerState.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "CharacterSystem/Data/CharacterData.h"
#include "CharacterSystem/Player/BasePlayerController.h"

#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/SpringArmComponent.h"
#include "Net/UnrealNetwork.h"
#include "Kismet/KismetMathLibrary.h"
#include "Engine/World.h"
#include "Engine/EngineTypes.h"
#include "Engine/OverlapResult.h"
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"
#include "DrawDebugHelpers.h"
#include "Camera/TopDownCameraComp.h"

#include "UI/UI_HUDFactory.h" // UI시스템 관리자

#include "SkillSystem/SkillDataAsset.h"

#include "Components/SceneCaptureComponent2D.h" // 미니맵용

#include "GameModeBase/State/ER_PlayerState.h"

ABaseCharacter::ABaseCharacter()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;
	bReplicates = true;
	SetReplicateMovement(true);

	/* === 기본 컴포넌트 === */
	GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = false;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 640.f, 0.f);
	GetCharacterMovement()->bConstrainToPlane = true;
	GetCharacterMovement()->bSnapToPlaneAtStart = true;


	//now the camera and camera boom is managed in the MainCameraComp.
	
	//new camera
	TopDownCameraComp = CreateDefaultSubobject<UTopDownCameraComp>(TEXT("TopDownCameraComp"));
	TopDownCameraComp->SetupAttachment(RootComponent);//temp attatchement-> it should follow the owner with lag
	TopDownCameraComp->InitializeCompRequirements();
	TopDownCameraComp->SetAbsolute(true, true, true);

	/* === 경로 설정 인덱스 초기화  === */
	CurrentPathIndex = INDEX_NONE;

	/* === 팀 변수 초기화  === */
	TeamID = ETeamType::None;

	bIsAttackMoving = false;

	// 26.01.29. mpyi
	// 미니맵을 위한 씬 컴포넌트 2D <- 차후 '카메라' 시스템으로 이동할 예정
	MinimapCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCaptureComponent"));
	MinimapCaptureComponent->SetupAttachment(RootComponent);

	// 미니맵 캡처 기본 설정
	MinimapCaptureComponent->SetAbsolute(false, true, false); // 순서대로: 위치, 회전, 스케일
	// 위치는 캐릭터를 따라다녀야 함으로 앱솔루트 ㄴㄴ
	MinimapCaptureComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 1000.0f));
	MinimapCaptureComponent->SetRelativeRotation(FRotator(-90.0f, 0.0f, 0.0f));
	MinimapCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	MinimapCaptureComponent->OrthoWidth = 2048.0f; // 이거로 미니맵 확대/축소 조절

	/// 최적화 필요시 아래 플래그 조절해가면서 해결해 보기
	//MinimapCaptureComponent->ShowFlags.SetDynamicShadows(false); // 동적 그림자
	//MinimapCaptureComponent->ShowFlags.SetGlobalIllumination(false); // 루멘
	//MinimapCaptureComponent->ShowFlags.SetMotionBlur(false); // 잔상 제거용
	//MinimapCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_BaseColor; // 포스트 프로세싱 무효화
}

void ABaseCharacter::BeginPlay()
{
	Super::BeginPlay();

	SetActorTickEnabled(false);

	// 클라이언트 초기화
	if (HeroData)
	{
		InitVisuals();
	}
}

void ABaseCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	/// UI TEST를 위함 지우진 말아주세요!! {차후 제거할 예정}
	/// mpyi
	//{
	//	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	//	UBaseAttributeSet* AS = Cast<UBaseAttributeSet>(GetPlayerState<ABasePlayerState>()->GetAttributeSet());
	//	if (ASC && AS) // 둘 다 존재해야 함
	//	{
	//		ASC->ApplyModToAttribute(AS->GetHealthAttribute(), EGameplayModOp::Additive, -1.0f);
	//		float CurrentHP = AS->GetHealth();
	//		// UE_LOG(LogTemp, Warning, TEXT("명령 전송 완료 | 현재 실제 HP 수치: %f"), CurrentHP);
	//	}
	//	else
	//	{
	//		UE_LOG(LogTemp, Error, TEXT("ASC(%s) 또는 AS(%s)가 NULL입니다!"),
	//			ASC ? TEXT("OK") : TEXT("NULL"),
	//			AS ? TEXT("OK") : TEXT("NULL"));
	//	}
	//}
	
	// 사망 상태일 시, Tick 관련 이동 및 공격 실행 중지
	if (AbilitySystemComponent.IsValid() && AbilitySystemComponent->HasMatchingGameplayTag(ProjectER::State::Life::Death))
	{
		return; // 죽었으면 아무것도 안 함
	}
	
	if (IsLocallyControlled() || HasAuthority())
	{
		// Tick 활성화 시 경로 탐색 (서버)
		UpdatePathFollowing();
	
		// 타겟이 유효할 때 추적/공격 수행
		if (TargetActor)
		{
			CheckCombatTarget(DeltaTime);
		}
	}	
	
	if (HasAuthority() && bIsAttackMoving && !TargetActor)
	{
		ScanForEnemiesWhileMoving();
	}
}

void ABaseCharacter::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// ASC 초기화 (서버)
	InitAbilitySystem();

	// 비주얼 초기화 (서버)
	if (HeroData)
	{
		InitVisuals();
	}

	if (TopDownCameraComp)//disable the tick for the server
	{
		TopDownCameraComp->SetComponentTickEnabled(false);
	}

	// UI 초기화
	InitUI();
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseCharacter, HeroData);
	DOREPLIFETIME(ABaseCharacter, TargetActor);
	DOREPLIFETIME(ABaseCharacter, TeamID);
}

ETeamType ABaseCharacter::GetTeamType() const
{
	AER_PlayerState* PS = GetPlayerState<AER_PlayerState>();
	return PS->TeamType;
}

bool ABaseCharacter::IsTargetable() const
{
	if (AbilitySystemComponent.IsValid())
	{
		if (AbilitySystemComponent->HasMatchingGameplayTag(ProjectER::State::Life::Death))
		{
			return false;
		}
	}
	
	return !IsHidden(); // 숨어있지 않고 살아있으면 true
}

void ABaseCharacter::OnRep_TeamID()
{
	FString Team = (TeamID == ETeamType::Team_A) ? TEXT("Team_A") : 
						(TeamID == ETeamType::Team_B) ? TEXT("Team_B") : 
							(TeamID == ETeamType::Team_C) ? TEXT("Team_C") : TEXT("None");
	
	FString Message = FString::Printf(TEXT("[%s] Team Changed to: %s"), *GetName(), *Team);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);
}

void ABaseCharacter::Server_SetTeamID_Implementation(ETeamType NewTeamID)
{
	TeamID = NewTeamID;
	OnRep_TeamID();
}

UAbilitySystemComponent* ABaseCharacter::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent.Get();
}

void ABaseCharacter::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// ASC 초기화 (클라이언트)
	InitAbilitySystem();
	// UI 초기화
	InitUI();

	//Camera Setting for local player pawn
	if (TopDownCameraComp)
	{
		if (IsLocallyControlled())
		{
			TopDownCameraComp->Activate();
			TopDownCameraComp->SetComponentTickEnabled(true);
		}
		else
		{
			TopDownCameraComp->Deactivate();
			TopDownCameraComp->SetComponentTickEnabled(false);
		}
	}
}

void ABaseCharacter::HandleLevelUp()
{
	// 서버 권한 확인
	if (!HasAuthority()) return;

	// 스탯 재계산 (변경된 Level을 기준으로 CurveTable 값을 다시 읽어옴)
	// InitAttributes 내부에서 GetCharacterLevel()을 호출하는데, 
	// 이미 AttributeSet에서 Level을 올렸으므로 오른 레벨의 스탯이 적용됩니다.
	InitAttributes();
    
	// 레벨업 시 체력/마나 회복
	if (GetAbilitySystemComponent())
	{
		// AttributeSet을 가져와서 직접 채워주거나 GameplayEffect 적용
		if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
		{
			if (UBaseAttributeSet* AS = PS->GetAttributeSet())
			{
				AS->SetHealth(AS->GetMaxHealth());
				AS->SetStamina(AS->GetMaxStamina());
			}
		}
	}

	// 이펙트 및 UI 처리 (Multicast)
	// Multicast_LevelUpVFX(); 
    
	UE_LOG(LogTemp, Warning, TEXT("[LevelUp] New Level: %f"), GetCharacterLevel());
}

float ABaseCharacter::GetCharacterLevel() const
{
	/*if (const UBaseAttributeSet* BaseSet = GetPlayerState<ABasePlayerState>() ? GetPlayerState<ABasePlayerState>()->GetAttributeSet() : nullptr)
	{
		return BaseSet->GetLevel();
	}*/

	if (const AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>())
	{
		if (const UBaseAttributeSet* AS = ERPS->GetAttributeSet())
		{
			return AS->GetLevel();
		}
	}
	else if (const ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
	{
		if (const UBaseAttributeSet* AS = PS->GetAttributeSet())
		{
			return AS->GetLevel();
		}
	}

	// 기본값(1레벨 시작) 반환
	return 1.0f;
}

float ABaseCharacter::GetAttackRange() const
{
	if (const AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>())
	{
		// [전민성] - MVP 병합 시 else문 삭제 필요
		if (const UBaseAttributeSet* AS = ERPS->GetAttributeSet())
		{
			return AS->GetAttackRange();
		}
	}
	else if(const ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
	{
		if (const UBaseAttributeSet* AS = PS->GetAttributeSet())
		{
			return AS->GetAttackRange();
		}
	}

	// 기본값(150) 반환
	return 150.0f;
}

void ABaseCharacter::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		// MovementComp->MaxWalkSpeed = Data.NewValue;
        
#if WITH_EDITOR
		if (bShowDebug)
		{
			UE_LOG(LogTemp, Warning, TEXT("[%s] 이동 속도 변경됨: %f"), *GetName(), Data.NewValue);
		}
#endif
	}
}

void ABaseCharacter::OnRep_HeroData()
{
	// 비주엃 초기화 (클라이언트)
	InitVisuals();
}

void ABaseCharacter::InitAbilitySystem()
{
	// [전민성] - 원본 코드
	//ABasePlayerState* PS = GetPlayerState<ABasePlayerState>();
	//if (!PS) return;
	// 
	//UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	//if (!ASC) return;
	// 
	//// ASC 캐싱 / Actor Info 설정
	//AbilitySystemComponent = ASC;
	//ASC->InitAbilityActorInfo(PS, this);

	// [전민성] - MVP 병합 시 ERPS로 통합 필요
	ABasePlayerState* PS = GetPlayerState<ABasePlayerState>();

	AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>();

	if (!PS && !ERPS) return;

	UAbilitySystemComponent* ASC;
	if (!PS)
	{
		ASC = ERPS->GetAbilitySystemComponent();
		AbilitySystemComponent = ASC;
		ASC->InitAbilityActorInfo(ERPS, this);
	}
	else
	{
		ASC = PS->GetAbilitySystemComponent();
		AbilitySystemComponent = ASC;
		ASC->InitAbilityActorInfo(PS, this);
	}
	
	// 스탯 변경 감지 델리게이트 연결
	if (AbilitySystemComponent.IsValid())
	{
		// AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &ABaseCharacter::OnMoveSpeedChanged);
	}

	if (HasAuthority() && HeroData)
	{
		for (TSoftObjectPtr<USkillDataAsset> SkillDataAsset : HeroData->SkillDataAsset)
		{
			if (SkillDataAsset.LoadSynchronous())
			{
				FGameplayAbilitySpec Spec = SkillDataAsset->MakeSpec();
				ASC->GiveAbility(Spec);
			}
		}

		// Ability 부여
		for (const auto& AbilityPair : HeroData->Abilities)
		{
			FGameplayTag InputTag = AbilityPair.Key;
			TSoftClassPtr<UGameplayAbility> AbilityPtr = AbilityPair.Value;
			
			if (UClass* AbilityClass = AbilityPtr.LoadSynchronous())
			{
				// Spec 생성 (레벨 1, InputID는 태그의 해시값 등을 사용하거나 별도 매핑 필요)
				// 여기서는 예시로 1레벨 부여
				FGameplayAbilitySpec Spec(AbilityClass, 1);

				// 동적 Input Tag깅 (Enhanced Input과 연동하기 위해 Spec에 태그 추가 가능)
				Spec.GetDynamicSpecSourceTags().AddTag(InputTag);

				ASC->GiveAbility(Spec);
			}
		}

		// 전민성 추가
		FGameplayAbilitySpec Spec(OpenAbilityClass, 1);
		ASC->GiveAbility(Spec);
		
		if (AliveStateEffectClass)
		{
			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddSourceObject(this);
			FGameplayEffectSpecHandle EffectSpec = AbilitySystemComponent->MakeOutgoingSpec(AliveStateEffectClass, 1.0f, Context);
			if (EffectSpec.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*EffectSpec.Data.Get());
			}
		}

		// Attribute Set 초기화
		InitAttributes();
	}
}

void ABaseCharacter::InitAttributes()
{
	if (!AbilitySystemComponent.IsValid() || !HeroData || !InitStatusEffectClass) return;

	// 커브 테이블 로드
	UCurveTable* CurveTable = HeroData->StatCurveTable.LoadSynchronous();
	if (!CurveTable) return;

	float Level = GetCharacterLevel();
	if (Level <= 0.f) Level = 1.0f;

	FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
	Context.AddSourceObject(this);

	FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(InitStatusEffectClass, Level, Context);

	if (SpecHandle.IsValid())
	{
		auto SetStat = [&](FGameplayTag AttributeTag, FString RowSuffix)
			{
				FName RowName = FName(*HeroData->StatusRowName.ToString().Append(RowSuffix));

				FRealCurve* Curve = CurveTable->FindCurve(RowName, FString());
				if (Curve)
				{
					float Value = Curve->Eval(Level);

					// GE Spec 값 주입 (SetByCaller)
					SpecHandle.Data->SetSetByCallerMagnitude(AttributeTag, Value);
				}
				else
				{
					// UE_LOG(LogTemp, Warning, TEXT("Curve Row Not Found: %s"), *RowName.ToString());
				}
			};

		// 레벨 설정
		SpecHandle.Data->SetSetByCallerMagnitude(ProjectER::Status::Level, Level);

		// 스탯별 값 주입 실행
		SetStat(ProjectER::Status::MaxLevel, "_MaxLevel");
		SetStat(ProjectER::Status::MaxXP, "_MaxXp");
		SetStat(ProjectER::Status::MaxHealth, "_MaxHealth");
		SetStat(ProjectER::Status::HealthRegen, "_HealthRegen");
		SetStat(ProjectER::Status::MaxStamina, "_MaxStamina");
		SetStat(ProjectER::Status::StaminaRegen, "_StaminaRegen");
		SetStat(ProjectER::Status::AttackPower, "_AttackPower");
		SetStat(ProjectER::Status::AttackSpeed, "_AttackSpeed");
		SetStat(ProjectER::Status::AttackRange, "_AttackRange");
		SetStat(ProjectER::Status::SkillAmp, "_SkillAmp");
		SetStat(ProjectER::Status::CritChance, "_CritChance");
		SetStat(ProjectER::Status::CritDamage, "_CritDamage");
		SetStat(ProjectER::Status::Defense, "_Defense");
		SetStat(ProjectER::Status::MoveSpeed, "_MoveSpeed");
		SetStat(ProjectER::Status::CooldownReduction, "_CooldownReduction");
		SetStat(ProjectER::Status::Tenacity, "_Tenacity");

		// 적용
		AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
	}
	
	// 최초 초기화 시 MaxXP 커브 데이터 캐싱
	if (HeroData)
	{
		UCurveTable* Table = HeroData->StatCurveTable.LoadSynchronous(); 
		if (Table)
		{
			// MaxXP 커브 찾기
			FString RowNameStr = HeroData->StatusRowName.ToString() + TEXT("_MaxXp");
			FName RowName = FName(*RowNameStr);
			
			if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
			{
				if (UBaseAttributeSet* AS = PS->GetAttributeSet())
				{
					AS->SetMaxXPCurve(Table, RowName);
				}
			}
			// 혹은 AER_PlayerState 사용 시
			else if (AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>())
			{
				if (UBaseAttributeSet* AS = ERPS->GetAttributeSet())
				{
					AS->SetMaxXPCurve(Table, RowName);
				}
			}
		}
	}
}

void ABaseCharacter::InitVisuals()
{
	if (!HeroData) return;

	// 스켈레탈 메시 로드 및 설정
	// TSoftObjectPtr을 동기 로드(LoadSynchronous)합니다.
	// 최적화 Tip: AssetManager를 통해 게임 시작 전(로딩 화면)에 미리 AsyncLoad 해두면 
	// 여기서 LoadSynchronous를 호출해도 딜레이가 0입니다.
	if (!HeroData->Mesh.IsNull())
	{
		if (USkeletalMesh* LoadedMesh = HeroData->Mesh.LoadSynchronous())
		{
			GetMesh()->SetSkeletalMesh(LoadedMesh);

			// 메시 크기에 맞춰 캡슐 컴포넌트 조정 (필요 시)
			// GetCapsuleComponent()->SetCapsuleSize(...);

			// 메시 위치 조정 (데이터에 오프셋이 있다면 적용)
			// GetMesh()->SetRelativeLocation(...);
		}
	}

	// ABP 로드 및 설정
	if (!HeroData->AnimClass.IsNull())
	{
		if (UClass* LoadedAnimClass = HeroData->AnimClass.LoadSynchronous())
		{
			GetMesh()->SetAnimInstanceClass(LoadedAnimClass);
		}
	}
	
	if (!HeroData->DeathMontage.IsNull())
	{
		// 동기 로드 (LoadSynchronous)하여 변수에 저장
		DeadAnimMontage = HeroData->DeathMontage.LoadSynchronous();
	}
}

void ABaseCharacter::Server_MoveToLocation_Implementation(FVector TargetLocation)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (NavSys == nullptr) return;

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(this, GetActorLocation(), TargetLocation);

	if (NavPath != nullptr && NavPath->PathPoints.Num() > 1)
	{
		PathPoints = NavPath->PathPoints;
		CurrentPathIndex = 1;
		SetActorTickEnabled(true);
		
		if (AbilitySystemComponent.IsValid() && MovingStateEffectClass)
		{
			FGameplayTag MoveTag = FGameplayTag::RequestGameplayTag(FName("State.Action.Move"));
			if (!AbilitySystemComponent->HasMatchingGameplayTag(MoveTag))
			{
				FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
				Context.AddSourceObject(this);
                
				FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(MovingStateEffectClass, 1.0f, Context);
				if (SpecHandle.IsValid())
				{
					MovingEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}
		}
	}
	else
	{
		// 경로 탐색 실패 시 중지
		// but, 타겟이 있을 경우 정지 X
		if (TargetActor == nullptr) 
		{
			StopPathFollowing();
		}
	}
}

bool ABaseCharacter::Server_MoveToLocation_Validate(FVector TargetLocation)
{
	return true;
}

void ABaseCharacter::Server_StopMove_Implementation()
{
	StopMove();
}

void ABaseCharacter::MoveToLocation(FVector TargetLocation)
{
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return;

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(this, GetActorLocation(), TargetLocation);

	if (NavPath && NavPath->PathPoints.Num() > 1)
	{
		PathPoints = NavPath->PathPoints;
		CurrentPathIndex = 1;
		SetActorTickEnabled(true);
		
		if (AbilitySystemComponent.IsValid() && MovingStateEffectClass)
		{
			FGameplayTag MoveTag = FGameplayTag::RequestGameplayTag(FName("State.Action.Move"));
			if (!AbilitySystemComponent->HasMatchingGameplayTag(MoveTag))
			{
				FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
				Context.AddSourceObject(this);
                
				FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(MovingStateEffectClass, 1.0f, Context);
				if (SpecHandle.IsValid())
				{
					// Handle 저장
					MovingEffectHandle = AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}	
		}
	}
	else
	{
		// 경로 탐색 실패 시 중지
		// but, 타겟이 있을 경우 정지 X
		if (TargetActor == nullptr) 
		{
			StopPathFollowing();
		}
	}

	if (!HasAuthority())
	{
		Server_MoveToLocation(TargetLocation);
	}
}

void ABaseCharacter::StopMove()
{
	// 이동 정지
	StopPathFollowing();
	GetCharacterMovement()->StopMovementImmediately();
	
	if (AbilitySystemComponent.IsValid())
	{
		// 일반 공격 정지
		FGameplayTagContainer CancelTags;
		CancelTags.AddTag(FGameplayTag::RequestGameplayTag(FName("Ability.Action.AutoAttack")));
		
		AbilitySystemComponent->CancelAbilities(&CancelTags);
		
		// 이동 정지 시 GE_Moving 제거
		if (HasAuthority() && MovingEffectHandle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(MovingEffectHandle);
			MovingEffectHandle.Invalidate(); // 핸들 초기화
		}
	}
	
	// 서버 동기화
	if (!HasAuthority())
	{
		Server_StopMove();
	}
	
	bIsAttackMoving = false;
}

void ABaseCharacter::UpdatePathFollowing()
{
	if (CurrentPathIndex == INDEX_NONE || CurrentPathIndex >= PathPoints.Num())
	{
		StopPathFollowing();
		return;
	}

	FVector CurrentLocation = GetActorLocation();
	FVector TargetPoint = PathPoints[CurrentPathIndex];

	FVector Direction = (TargetPoint - CurrentLocation);
	Direction.Z = 0.f;

	if (Direction.SizeSquared() < ArrivalDistanceSq)
	{
		CurrentPathIndex++;
		if (CurrentPathIndex >= PathPoints.Num())
		{
			StopPathFollowing();
			return;
		}

		TargetPoint = PathPoints[CurrentPathIndex];
		Direction = (TargetPoint - CurrentLocation);
		Direction.Z = 0.f;
	}

	if (!Direction.IsNearlyZero())
	{
		FVector NormalDirection = Direction.GetSafeNormal();

		// 캐릭터 이동 입력 
		AddMovementInput(NormalDirection, 1.0f);

		// 캐릭터 회전
		FRotator TargetRot = NormalDirection.Rotation();
		FRotator CurrentRot = GetActorRotation();

		FRotator NewRotation = FMath::RInterpTo(CurrentRot, TargetRot, GetWorld()->GetDeltaSeconds(), 20.0f);

		SetActorRotation(NewRotation);
	}

#if WITH_EDITOR
	// [디버깅] 액터 Rotation 체크
	/*FRotator MyRot = GetActorRotation();
	UE_LOG(LogTemp, Warning, TEXT("Rotation Check -> Pitch: %f | Yaw: %f"), MyRot.Pitch, MyRot.Yaw);*/

	// [디버깅] 경로 및 이동 방향 시각화
	if (bShowDebug)
	{
		// 전체 경로 그리기 (초록색 선)
		for (int32 i = 0; i < PathPoints.Num() - 1; ++i)
		{
			DrawDebugLine(
				GetWorld(),
				PathPoints[i],
				PathPoints[i + 1],
				FColor::Green,
				false, -1.0f, 0, 3.0f // 두께 3.0
			);
		}

		// 현재 목표 지점 (빨간색 구체)
		if (PathPoints.IsValidIndex(CurrentPathIndex))
		{
			DrawDebugSphere(
				GetWorld(),
				PathPoints[CurrentPathIndex],
				30.0f, // 반지름
				12,
				FColor::Red,
				false, -1.0f, 0, 2.0f
			);

			// 내 위치에서 목표 지점까지 연결선 (노란색 점선)
			DrawDebugLine(
				GetWorld(),
				GetActorLocation(),
				PathPoints[CurrentPathIndex],
				FColor::Yellow,
				false, -1.0f, 0, 1.5f
			);
		}

		// 실제 이동 방향 (파란색 화살표)
		FVector Velocity = GetVelocity();
		if (!Velocity.IsNearlyZero())
		{
			DrawDebugDirectionalArrow(
				GetWorld(),
				GetActorLocation(),
				GetActorLocation() + Velocity.GetSafeNormal() * 100.0f, // 1m 길이
				50.0f, // 화살표 크기
				FColor::Blue,
				false, -1.0f, 0, 5.0f // 두께
			);
		}
	}
#endif
}

void ABaseCharacter::StopPathFollowing()
{
	PathPoints.Empty();
	CurrentPathIndex = INDEX_NONE;
	
	// 타겟이 있을 시 이동 및 공격을 위해 Tick 유지
	// CheckCombatTarget() 유지 용도
	if (TargetActor == nullptr)
	{
		SetActorTickEnabled(false);
	}
	
	// 목적지 도착 시 GE_Moving 제거
	if (HasAuthority() && AbilitySystemComponent.IsValid())
	{
		if (MovingEffectHandle.IsValid())
		{
			AbilitySystemComponent->RemoveActiveGameplayEffect(MovingEffectHandle);
			MovingEffectHandle.Invalidate();
		}
	}
}

FRotator ABaseCharacter::GetCombatGazeRotation(FName SocketName)
{
	// 시작점 (Muzzle)
	FVector StartPos = GetMuzzleLocation(SocketName);
    
	// 목표점 (Target)
	FVector TargetPos;
    
	if (TargetActor) 
	{
		// 단순 ActorLocation은 발밑(Root)일 수 있으므로, 
		// 캡슐 컴포넌트의 중간이나 특정 뼈를 노리는 보정 로직 추가
		TargetPos = TargetActor->GetActorLocation();
        
		// [디테일] 타겟의 키 절반만큼 위를 조준 (가슴팍)
		// ACharacter로 캐스팅 가능하다면 Capsule HalfHeight를 더해줌
		if (ACharacter* TargetChar = Cast<ACharacter>(TargetActor))
		{
			TargetPos.Z += TargetChar->GetCapsuleComponent()->GetScaledCapsuleHalfHeight() * 0.7f; 
		}
	}
	else
	{
		// 타겟이 없으면 내 정면 10m 앞
		TargetPos = StartPos + (GetActorForwardVector() * 1000.0f);
	}
    
	// 각도 계산 (Unreal Math Library)
	return UKismetMathLibrary::FindLookAtRotation(StartPos, TargetPos);
}

FVector ABaseCharacter::GetMuzzleLocation(FName SocketName)
{
	if (GetMesh())
	{
		return GetMesh()->GetSocketLocation(SocketName);
	}
	return GetActorLocation();
}

void ABaseCharacter::SetTarget(AActor* NewTarget)
{
	if (TargetActor == NewTarget) return;
	
	// 서버에서 설정
	if (HasAuthority())
	{
		TargetActor = NewTarget;
		OnRep_TargetActor(); // 서버 OnRep 수동 호출
	}
	else
	{
		// 클라이언트에서도 반영(Prediction) 후 서버 요청
		TargetActor = NewTarget;
		OnRep_TargetActor(); 
		
		// 클라이언트에서 서버에 동기화 요청
		Server_SetTarget(NewTarget);
	}
}

void ABaseCharacter::CheckCombatTarget(float DeltaTime)
{
	if (!IsValid(TargetActor))
	{
		SetTarget(nullptr); // 타겟이 소멸 시 지정 해제
		return;
	}
	
	if (ITargetableInterface* TargetObj = Cast<ITargetableInterface>(TargetActor))
	{
		// 타겟이 타겟팅 불가능 상태(사망, 은신 등)라면?
		if (!TargetObj->IsTargetable())
		{
			// 타겟 해제
			SetTarget(nullptr);
            
			// 이동 및 공격 행위 중단 (공격 선딜레이 캔슬)
			StopMove(); 
            
			return;
		}
	}

	float Distance = GetDistanceTo(TargetActor);
	float AttackRange = GetAttackRange();
	
	float Tolerance = 20.0f; // 사거리 보정 값 유사 시 사용
	float CheckRange = HasAuthority() ? (AttackRange + Tolerance) : (AttackRange - Tolerance); // 보정된 사거리
	
	// [디버깅용 로그 추가] 현재 거리와 사거리 비교
#if WITH_EDITOR
	if (bShowDebug)
	{
		static float LogTimer = 0.0f;
		LogTimer += DeltaTime;
		if (LogTimer > 0.5f)
		{
			LogTimer = 0.0f;
			/*UE_LOG(LogTemp, Warning, TEXT("[%s] Dist: %.2f / Range: %.2f"), 
				*GetName(), Distance, GetAttackRange());*/
		}
	}
#endif
	
	if (Distance <= AttackRange) // 사거리 내 진입 시
	{
		// 이동 정지
		StopPathFollowing();
		GetCharacterMovement()->StopMovementImmediately();
        
		// 회전 - 공격할 때는 적을 바라봐야 함
		FVector Direction = TargetActor->GetActorLocation() - GetActorLocation();
		Direction.Z = 0.0f; // 높이 무시
		SetActorRotation(Direction.Rotation());
		
		// 공격 어빌리티 실행 (GAS) : 서버 판정 우선
		if (HasAuthority() && AbilitySystemComponent.IsValid())
		{
			// 공격 제한 태그 검사
			static const FGameplayTag CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting")); 
			static const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));
            
			if (AbilitySystemComponent->HasMatchingGameplayTag(CastingTag) || 
				AbilitySystemComponent->HasMatchingGameplayTag(ActiveTag))
			{
				return; // 스킬 캐스팅 혹은 시전 중일 시 일반 공격 시도 종료
			}
			
			FGameplayTag AttackTag = FGameplayTag::RequestGameplayTag(FName("Ability.Action.AutoAttack"));
			
			TArray<FGameplayAbilitySpec*> Specs;
			AbilitySystemComponent->GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(AttackTag), Specs);

			bool bHasAbility = (Specs.Num() > 0);
			bool bWasActivated = false;
			
			if (bHasAbility)
			{
				bWasActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(AttackTag));
			}
			
			if (bWasActivated)
			{
#if WITH_EDITOR
				if (bShowDebug)
				{
					UE_LOG(LogTemp, Warning, TEXT("[%s] Tag: %s / Found Ability: %s / Activated: %s"),
							*GetName(),
							*AttackTag.ToString(),
							bHasAbility ? TEXT("YES") : TEXT("NO (Check DataAsset!)"), // 여기가 NO라면 데이터에셋)문제
							bWasActivated ? TEXT("True") : TEXT("False (Check Cooldown/Cost/State)") // 여기가 False라면 조건 문제
							);
				}
#endif	
			}
		}
		return;
	}
	else // 사거리 밖일 시
	{
		PathfindingTimer += DeltaTime;
		if (PathfindingTimer > 0.1f)
		{
			PathfindingTimer = 0.0f;
			MoveToLocation(TargetActor->GetActorLocation());
		}	
	}
}

void ABaseCharacter::Server_AttackMoveToLocation_Implementation(FVector TargetLocation)
{
	// 기존 타겟 해제
	SetTarget(nullptr);
    
	// 이동 상태를 '공격 명령'으로 설정
	bIsAttackMoving = true;

	// 이동 시작
	MoveToLocation(TargetLocation);
}

void ABaseCharacter::OnRep_TargetActor()
{
	// 타겟 유무에 따라 Tick 활성화/비활성화
	SetActorTickEnabled(TargetActor != nullptr);
    
	// 경로 탐색 타이머 초기화 
	if (TargetActor)
	{
		PathfindingTimer = 0.0f;
	}

#if WITH_EDITOR
	if (bShowDebug)
	{
		UE_LOG(LogTemp, Warning, TEXT("[%s] Set Target Actor -> %s"),
			*GetName(),
			TargetActor ? *TargetActor->GetName() : TEXT("None"));
	}
#endif
}

void ABaseCharacter::ScanForEnemiesWhileMoving()
{
	// 주변 적 검색 로직 (SphereOverlap)
	FVector MyLoc = GetActorLocation();
	float ScanRange = GetAttackRange(); // 내 사거리만큼 검색 (혹은 시야거리)

	TArray<FOverlapResult> OverlapResults;
	FCollisionQueryParams QueryParams(NAME_None, false, this);
    
	bool bResult = GetWorld()->OverlapMultiByChannel(
		OverlapResults,
		MyLoc,
		FQuat::Identity,
		ECC_Pawn, // Pawn 채널 검색 (적 캐릭터)
		FCollisionShape::MakeSphere(ScanRange),
		QueryParams
	);

	if (bResult)
	{
		AActor* ClosestEnemy = nullptr;
		float MinDistSq = FLT_MAX;

		for (const FOverlapResult& Result : OverlapResults)
		{
			AActor* OtherActor = Result.GetActor();
			if (!OtherActor || OtherActor == this) continue;

			if (ITargetableInterface* TargetObj = Cast<ITargetableInterface>(OtherActor))
			{
				// 적군이고 타겟팅 가능한지 확인
				if (TargetObj->GetTeamType() != GetTeamType() && TargetObj->IsTargetable())
				{
					float DistSq = FVector::DistSquared(MyLoc, OtherActor->GetActorLocation());
					if (DistSq < MinDistSq)
					{
						MinDistSq = DistSq;
						ClosestEnemy = OtherActor;
					}
				}
			}
		}

		// 적을 찾았으면 타겟으로 설정하고 이동 중지 (자동 공격 시작됨)
		if (ClosestEnemy)
		{
			SetTarget(ClosestEnemy);
			bIsAttackMoving = false; // 어택 땅 모드 종료 -> 전투 모드 전환
            
			// 이동 멈춤 (Target이 생겼으므로 CheckCombatTarget에서 알아서 처리함)
			StopPathFollowing(); 
			GetCharacterMovement()->StopMovementImmediately();
		}
	}
}

void ABaseCharacter::Server_Revive_Implementation(FVector RespawnLocation)
{
	Revive(RespawnLocation);
}

void ABaseCharacter::HandleDeath()
{
	if (HasAuthority())
	{
		if (AbilitySystemComponent.IsValid()) // 중복 사망 방지
		{
			FGameplayTag DeathTag = ProjectER::State::Life::Death;
			if (AbilitySystemComponent->HasMatchingGameplayTag(DeathTag))
			{
				return; // 중복 사망 방지
			}
			
			// [추가] GE_State_Dead 적용 방식
			if (DeathStateEffectClass)
			{
				FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
				Context.AddSourceObject(this);
                
				FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DeathStateEffectClass, 1.0f, Context);
				if (SpecHandle.IsValid())
				{
					AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
				}
			}
			
			AbilitySystemComponent->CancelAllAbilities();
		}

		// 타겟 지정 해제 (나를 노리는 적들에게 "나 죽었어" 알림)
		// (이 부분은 AI나 타겟팅 시스템에 따라 추가 구현 필요)
		SetTarget(nullptr);
		// 사망 알림 델리게이트
		OnDeath.Broadcast();
		// 모든 클라이언트에게 연출 실행 명령
		Multicast_Death();
	}
}

void ABaseCharacter::Revive(FVector RespawnLocation)
{
	if (!HasAuthority()) return;
	
	// 빈사 상태(Down) 제거 (추가 예정)
	// 태그로 찾아서 GE 제거
	// AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(FGameplayTagContainer(ProjectER::State::Life::Down));
	
	if (AbilitySystemComponent.IsValid())
	{
		// [상태 초기화] 사망(Death) 또는 빈사(Down) 태그를 가진 모든 GE 제거
		FGameplayTagContainer BadStateTags;
		BadStateTags.AddTag(ProjectER::State::Life::Death);
		BadStateTags.AddTag(ProjectER::State::Life::Down);
		
		AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(BadStateTags);

		// (안전장치) 혹시 Loose Tag로 남아있을 경우를 대비해 직접 제거 (기존 코드 호환용)
		AbilitySystemComponent->RemoveLooseGameplayTag(ProjectER::State::Life::Death);
		AbilitySystemComponent->RemoveLooseGameplayTag(ProjectER::State::Life::Down);

		//  Alive GE 적용
		if (AliveStateEffectClass)
		{
			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddSourceObject(this);
			
			FGameplayEffectSpecHandle Spec = AbilitySystemComponent->MakeOutgoingSpec(AliveStateEffectClass, 1.0f, Context);
			if (Spec.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
			}
		}
	}

	// 스탯 초기화
	// (AER_PlayerState 대응 추가)
	UBaseAttributeSet* AS = nullptr;
	
	if (AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>())
	{
		AS = ERPS->GetAttributeSet();
	}
	else if (ABasePlayerState* BasePS = GetPlayerState<ABasePlayerState>())
	{
		AS = BasePS->GetAttributeSet();
	}
	
	if (AS)
	{
		AS->SetHealth(AS->GetMaxHealth());
		AS->SetStamina(AS->GetMaxStamina());
		
		UE_LOG(LogTemp, Warning, TEXT("[Revive] HP Recovered: %f"), AS->GetHealth());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("[Revive] AttributeSet is NULL! Check PlayerState Class."));
	}
    
	// 타겟 초기화
	SetTarget(nullptr);

	// 클라이언트 동기화 (위치 이동 및 비주얼/물리 복구)
	Multicast_Revive(RespawnLocation);
}

void ABaseCharacter::HandleDown()
{
	if (HasAuthority())
	{
		if (AbilitySystemComponent.IsValid())
		{
			// 기존 진행 중인 모든 어빌리티 취소 (공격, 캐스팅 등)
			AbilitySystemComponent->CancelAllAbilities();

			// GE_DownStatus 적용
			if (DownStateEffectClass)
			{
				FGameplayEffectContextHandle EffectContext = AbilitySystemComponent->MakeEffectContext();
				EffectContext.AddSourceObject(this);
                
				FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DownStateEffectClass, GetCharacterLevel(), EffectContext);
				if (SpecHandle.IsValid())
				{
					AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
					UE_LOG(LogTemp, Warning, TEXT("[%s] Apply GE_State_Down Success!"), *GetName());
				}
			}
		}
        
		// 타겟 해제 
		SetTarget(nullptr); 
	}
	
	// 클라이언트 연출 동기화
	Multicast_HandleDown();
}

void ABaseCharacter::Multicast_Revive_Implementation(FVector RespawnLocation)
{
	// 위치 이동
	SetActorLocation(RespawnLocation);
	SetActorRotation(FRotator::ZeroRotator);

	// 애니메이션 초기화 
	StopAnimMontage();
	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Stop(0.0f);
	}

	// 캡슐 콜리전 복구
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	}

	// 이동 컴포넌트 복구
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		GetCharacterMovement()->StopMovementImmediately();
		// HandleDeath에서 DisableMovement()를 했으므로 다시 활성화 필요할 수 있음
		// 보통 SetMovementMode(Walking)으로 해결되지만, 안 된다면 아래 코드 추가
		// GetCharacterMovement()->Activate(); 
	}
	
	SetActorTickEnabled(true);
    
	// (선택) 부활 이펙트 재생
	// UNiagaraFunctionLibrary::SpawnSystemAtLocation(...)
}

void ABaseCharacter::Multicast_Death_Implementation()
{
	// 사망 애니메이션 몽타주 재생
	if (DeadAnimMontage && GetMesh() && GetMesh()->GetAnimInstance())
	{
		PlayAnimMontage(DeadAnimMontage);
	}

	// Capsule 비활성화 
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 이동 정지 및 기능 비활성화
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}

	/* // 입력 차단 
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		PC->DisableInput(PC);
        
		// (선택) UI 띄우기 호출
		// if (ABasePlayerController* BasePC = Cast<ABasePlayerController>(PC))
		// {
		//     BasePC->Client_SetDead(); 
		// }
	} */
    
	// 틱 비활성화 (불필요한 연산 방지)
	SetActorTickEnabled(false);
}

void ABaseCharacter::Server_SetTarget_Implementation(AActor* NewTarget)
{
	SetTarget(NewTarget);
}

void ABaseCharacter::Multicast_HandleDown_Implementation()
{
	StopAnimMontage();
	if (GetMesh() && GetMesh()->GetAnimInstance())
	{
		GetMesh()->GetAnimInstance()->Montage_Stop(0.0f);
	}

	// (선택) 캡슐 콜리전 처리
	// 기어다닐 때 다른 유저의 길을 막지 않게 하려면 여기서 Collision Response를 수정할 수 있습니다.
	// GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	UE_LOG(LogTemp, Warning, TEXT("[%s] 빈사 상태(Down) 진입!"), *GetName());
}

void ABaseCharacter::InitUI()
{
	ABasePlayerController* PC = Cast<ABasePlayerController>(GetController());
	if (IsValid(PC) && PC->IsLocalController())
	{
		AHUD* GenericHUD = PC->GetHUD();
		if (GenericHUD == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("!!! HUD NONE CREATED!!!"));
			return;
		}

		// [전민성] - MVP 병합 시 else문 삭제 필요
		if (AUI_HUDFactory* HUD = Cast<AUI_HUDFactory>(GenericHUD))
		{
			if (AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>())
			{
				HUD->InitOverlay(PC, GetPlayerState(), GetAbilitySystemComponent(), ERPS->GetAttributeSet());
			}
			else
			{
				HUD->InitOverlay(PC, GetPlayerState(), GetAbilitySystemComponent(), GetPlayerState<ABasePlayerState>()->GetAttributeSet());
			}
			HUD->InitMinimapComponent(MinimapCaptureComponent);
			HUD->InitHeroDataFactory(HeroData);
			HUD->InitASCFactory(GetAbilitySystemComponent());
			PC->setMainHud(HUD->getMainHUD());
			
			UE_LOG(LogTemp, Warning, TEXT("HUD InitOverlay Success!"));
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("!!! HUD Casting Fail! address : %s !!!"), *GenericHUD->GetName());
		}

	}

	/// 미니맵 설정
	if (!IsLocallyControlled())
	{
		/// '나' 이외는 캡쳐 컴포넌트를 꺼서 성능 최적화~
		MinimapCaptureComponent->Deactivate();
	}
}
