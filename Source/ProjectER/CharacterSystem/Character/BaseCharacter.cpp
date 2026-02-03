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
#include "NavigationSystem.h"
#include "NavigationPath.h"
#include "GameplayEffect.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"
#include "DrawDebugHelpers.h"

#include "UI/UI_HUDFactory.h" // UI시스템 관리자

#include "SkillSystem/SkillDataAsset.h"

#include "Components/SceneCaptureComponent2D.h" // 미니맵용

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

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));

	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->SetUsingAbsoluteRotation(true);
	CameraBoom->TargetArmLength = 800.f;
	CameraBoom->SetRelativeRotation(FRotator(-60.f, 0.f, 0.f));
	CameraBoom->bDoCollisionTest = false;

	TopDownCameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("TopDownCamera"));

	TopDownCameraComponent->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	TopDownCameraComponent->bUsePawnControlRotation = false;

	/* === 경로 설정 인덱스 초기화  === */
	CurrentPathIndex = INDEX_NONE;

	/* === 팀 변수 초기화  === */
	TeamID = ETeamType::None;



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

	// [수정] 내 캐릭터이거나(Local), 서버(Authority)일 때만 로직 수행
	// 남의 캐릭터(Simulated Proxy)는 이 로직을 돌리면 안 됨 (RPC 권한 없음)
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

	// UI 초기화
	InitUI();
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseCharacter, HeroData);
	DOREPLIFETIME(ABaseCharacter, TargetActor);
}

ETeamType ABaseCharacter::GetTeamType() const
{
	return TeamID;
}

bool ABaseCharacter::IsTargetable() const
{
	return true;
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
}

float ABaseCharacter::GetCharacterLevel() const
{
	/*if (const UBaseAttributeSet* BaseSet = GetPlayerState<ABasePlayerState>() ? GetPlayerState<ABasePlayerState>()->GetAttributeSet() : nullptr)
	{
		return BaseSet->GetLevel();
	}*/

	if (const ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
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
	if (const ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
	{
		if (const UBaseAttributeSet* AS = PS->GetAttributeSet())
		{
			return AS->GetAttackRange();
		}
	}

	// 기본값(150) 반환
	return 150.0f;
}

void ABaseCharacter::OnRep_HeroData()
{
	// 비주엃 초기화 (클라이언트)
	InitVisuals();
}

void ABaseCharacter::InitAbilitySystem()
{
	ABasePlayerState* PS = GetPlayerState<ABasePlayerState>();
	if (!PS) return;

	UAbilitySystemComponent* ASC = PS->GetAbilitySystemComponent();
	if (!ASC) return;

	// ASC 캐싱 / Actor Info 설정
	AbilitySystemComponent = ASC;
	ASC->InitAbilityActorInfo(PS, this);

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
				Spec.DynamicAbilityTags.AddTag(InputTag);

				ASC->GiveAbility(Spec);
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
					UE_LOG(LogTemp, Warning, TEXT("Curve Row Not Found: %s"), *RowName.ToString());
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
	}
	
	// 서버 동기화
	if (!HasAuthority())
	{
		Server_StopMove();
	}
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
		SetTarget(nullptr); // 타겟이 사망 혹은 소멸 시 지정 해제
		return;
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
			UE_LOG(LogTemp, Warning, TEXT("[%s] Dist: %.2f / Range: %.2f"), 
				*GetName(), Distance, GetAttackRange());
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

void ABaseCharacter::Server_SetTarget_Implementation(AActor* NewTarget)
{
	SetTarget(NewTarget);
}

void ABaseCharacter::InitUI()
{
	APlayerController* PC = Cast<APlayerController>(GetController());
	if (IsValid(PC) && PC->IsLocalController())
	{
		AHUD* GenericHUD = PC->GetHUD();
		if (GenericHUD == nullptr)
		{
			UE_LOG(LogTemp, Error, TEXT("!!! HUD NONE CREATED!!!"));
			return;
		}

		if (AUI_HUDFactory* HUD = Cast<AUI_HUDFactory>(GenericHUD))
		{
			HUD->InitOverlay(PC, GetPlayerState(), GetAbilitySystemComponent(), GetPlayerState<ABasePlayerState>()->GetAttributeSet());
			HUD->InitMinimapComponent(MinimapCaptureComponent);
			HUD->InitHeroDataFactory(HeroData);
			HUD->InitASCFactory(GetAbilitySystemComponent());
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
