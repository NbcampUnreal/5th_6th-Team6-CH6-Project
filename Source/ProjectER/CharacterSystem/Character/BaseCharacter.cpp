#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/Player/BasePlayerState.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h"
#include "CharacterSystem/Data/CharacterData.h"
#include "CharacterSystem/Player/BasePlayerController.h"
#include "ItemSystem/Component/LootableComponent.h" // [김현수 추가분]
#include "ItemSystem/Component/BaseInventoryComponent.h" // [김현수 추가분] 빈사/사망 전환 시 food 효과 정리용

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
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"
#include "DrawDebugHelpers.h"
#include "Camera/TopDownCameraComp.h"

#include "UI/UI_HUDFactory.h" // UI시스템 관리자
#include "UI/UI_MainHUD.h" // Main UI

#include "SkillSystem/SkillDataAsset.h"

#include "Components/SceneCaptureComponent2D.h" // 미니맵용
#include "Components/StaticMeshComponent.h" // 미니맵용
#include "Materials/MaterialInstanceDynamic.h" // 미니맵용
#include "Engine/StaticMesh.h" // 미니맵용
#include "GameFramework/Volume.h" // 시야 체크 볼륨 예외처리용

#include "Components/WidgetComponent.h" // HP바 위젯용
#include "GameModeBase/State/ER_GameState.h"
#include "UI/UI_HP_Bar.h" // HP바 위젯용

#include "GameModeBase/State/ER_PlayerState.h"
#include "LineOfSight/MainVisionRTManager.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"


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
	//TopDownCameraComp->InitializeCompRequirements();// this should not be done in the constructor!!!
	TopDownCameraComp->SetAbsolute(true, true, true);

	/* === 경로 설정 인덱스 초기화  === */
	CurrentPathIndex = INDEX_NONE;

	/* === 팀 변수 초기화  === */
	TeamID = ETeamType::None;

	bIsAttackMoving = false;

	// 26.01.29. mpyi
	// 미니맵을 위한 씬 컴포넌트 2D <- 차후 '카메라' 시스템으로 이동할 예정
	MinimapCaptureComponent = CreateDefaultSubobject<USceneCaptureComponent2D>(TEXT("MinimapCaptureComponent"));
	MinimapCaptureComponent->SetupAttachment(TopDownCameraComp);

	// 미니맵 캡처 기본 설정
	MinimapCaptureComponent->SetAbsolute(false, true, false); // 순서대로: 위치, 회전, 스케일
	// 위치는 캐릭터를 따라다녀야 함으로 앱솔루트 ㄴㄴ
	MinimapCaptureComponent->SetRelativeLocation(FVector(0.0f, 0.0f, 2000.0f));
	MinimapCaptureComponent->SetRelativeRotation(FRotator(-90.0f, 45.0f, 0.0f));
	MinimapCaptureComponent->ProjectionType = ECameraProjectionMode::Orthographic;
	MinimapCaptureComponent->OrthoWidth = 4500; // 이거로 미니맵 확대/축소 조절
	MinimapCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;	// 투명도 반영

	// 미니맵용 아이콘 만들기
	MinimapIconMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MinimapIcon"));
	MinimapIconMesh->SetupAttachment(RootComponent);
	MinimapLineMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MinimapIconLine"));
	MinimapLineMesh->SetupAttachment(RootComponent);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh(TEXT("/Engine/BasicShapes/Plane"));
	if (PlaneMesh.Succeeded())
	{
		MinimapIconMesh->SetStaticMesh(PlaneMesh.Object);
	}
	static ConstructorHelpers::FObjectFinder<UStaticMesh> PlaneMesh2(TEXT("/Engine/BasicShapes/Plane"));
	if (PlaneMesh2.Succeeded())
	{
		MinimapLineMesh->SetStaticMesh(PlaneMesh2.Object);
	}

	// 아이콘이 항상 하늘을 향하게 배치 (캐릭터 머리 위)
	MinimapIconMesh->SetRelativeLocation(FVector(0.f, 0.f, 500.0f));	// 미니맵 카메라가 1000이니까 그보다 아래로
	MinimapIconMesh->SetRelativeRotation(FRotator(0.f, 90.f, 0.f));
	MinimapIconMesh->SetRelativeScale3D(FVector(5.0f, 5.0f, 5.0f));	// 얼굴 아이콘 크기 조절
	MinimapIconMesh->SetAbsolute(false, true, false); // 회전값 고정 (중요함....)
	MinimapIconMesh->SetCastShadow(false);	// 그림자 없애기

	MinimapLineMesh->SetRelativeLocation(FVector(0, 0, 450.0f));	// 미니맵 아이콘이 500이니까 그보다 아래로
	MinimapLineMesh->SetRelativeScale3D(FVector(6.0f, 6.0f, 6.0f));	// 얼굴 아이콘 크기 조절
	MinimapLineMesh->SetAbsolute(false, true, false); // 회전값 고정 (중요함....)
	MinimapLineMesh->SetCastShadow(false);	// 그림자 없애기

	// 캐릭터 메쉬는 미니맵에 안보이게
	GetMesh()->SetHiddenInSceneCapture(true);
	// 미니맵 아이콘은 미니맵에 보이게
	MinimapIconMesh->SetVisibleInSceneCaptureOnly(true);
	MinimapLineMesh->SetVisibleInSceneCaptureOnly(true);

	MinimapIconMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	MinimapLineMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	// HP Bar 생성
	HP_MP_BarWidget = CreateDefaultSubobject<UWidgetComponent>(TEXT("HPBarWidget"));
	HP_MP_BarWidget->SetupAttachment(GetMesh());
	// 	HP_MP_BarWidget->SetDrawSize(FVector2D(250.0f, 130.0f));
	HP_MP_BarWidget->SetRelativeLocation(FVector(0.0f, 0.0f, 300.0f));
	HP_MP_BarWidget->SetWidgetSpace(EWidgetSpace::Screen);

	/// 콜리전 없애기
	HP_MP_BarWidget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	HP_MP_BarWidget->SetCollisionResponseToAllChannels(ECR_Ignore);
	HP_MP_BarWidget->SetGenerateOverlapEvents(false);

	HP_MP_BarWidget->SetDrawAtDesiredSize(true);

	/// 최적화 필요시 아래 플래그 조절해가면서 해결해 보기
	
	MinimapCaptureComponent->ShowFlags.SetDynamicShadows(false); // 동적 그림자
	MinimapCaptureComponent->ShowFlags.SetGlobalIllumination(false); // 루멘
	//MinimapCaptureComponent->ShowFlags.SetMotionBlur(false); // 잔상 제거용
	//MinimapCaptureComponent->CaptureSource = ESceneCaptureSource::SCS_BaseColor; // 포스트 프로세싱 무효화

	//Additional flag de-initialize
	MinimapCaptureComponent->ShowFlags.SetAtmosphere(false);
	MinimapCaptureComponent->ShowFlags.SetFog(false);
	MinimapCaptureComponent->ShowFlags.SetBloom(false);
	MinimapCaptureComponent->ShowFlags.SetAmbientOcclusion(false);
	MinimapCaptureComponent->ShowFlags.SetAntiAliasing(false);
	MinimapCaptureComponent->ShowFlags.SetMotionBlur(false);
	MinimapCaptureComponent->ShowFlags.SetVolumetricFog(false);
	
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


	/*EVisionChannel CurrentVisionChannel=GetVisionChannelFromPlayerStateComp();
	TeamID;//test-> did the team id been settled?
	
	if (TopDownCameraComp)
	{
		TopDownCameraComp->InitializeComponent();
	}*/
	
	PreloadMontages();
}

void ABaseCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	/*//Minimap update timer deactivation
	GetWorld()->GetTimerManager().ClearTimer(MinimapCaptureTimerHandle);
	MinimapCaptureComponent->Deactivate();*/
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
	
	if (HasAuthority())
	{
		UBaseAttributeSet* AS = nullptr;
		
		if (AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>())
		{
			AS = ERPS->GetAttributeSet();
		}
		else if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
		{
			AS = PS->GetAttributeSet();
		}

		if (AS)
		{
			AS->SetHealth(AS->GetMaxHealth());
			AS->SetStamina(AS->GetMaxStamina());
		}
	}
	
	// 비주얼 초기화 (서버)
	if (HeroData)
	{
		InitVisuals();
	}

	bool bIsServer=HasAuthority();
	
	// 서버 (리슨 서버 호스트 포함)에서 빙의된 직후 바로 초기화 진행
	InitPlayer();
}

void ABaseCharacter::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseCharacter, HeroData);
	DOREPLIFETIME(ABaseCharacter, TargetActor);
	DOREPLIFETIME(ABaseCharacter, TeamID);
}

void ABaseCharacter::InitWeapons()
{
	// 기존 무기 정리
	DetachAllWeapons();
	
	for (const FWeaponVisualData& WeaponData : HeroData->DefaultWeapons)
	{
		AttachWeapon(WeaponData); // 무기 장착
	}
}

void ABaseCharacter::AttachWeapon(const FWeaponVisualData& WeaponData)
{
	if (WeaponData.WeaponMesh.IsNull()) return;
	
	UStaticMesh* LoadedMesh = WeaponData.WeaponMesh.LoadSynchronous();
	if (!LoadedMesh) return;
	
	// 동적 컴포넌트 생성
	// NewObject + RegisterComponent() 런타임 동적 생성
	// 생성자에서 CreateDefaultSubobject와 달리, 게임 진행 중 자유롭게 추가/제거 가능
	UStaticMeshComponent* WeaponComp = NewObject<UStaticMeshComponent>(this);
	WeaponComp->SetStaticMesh(LoadedMesh);
	WeaponComp->SetRelativeTransform(WeaponData.AttachOffset);
	WeaponComp->SetWorldScale3D(WeaponData.WeaponScale);
	WeaponComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponComp->RegisterComponent();
	
	// 소켓에 부착
	WeaponComp->AttachToComponent(
		GetMesh(),
		FAttachmentTransformRules::SnapToTargetNotIncludingScale,
		WeaponData.AttachSocketName
	);
	
	WeaponMeshComponents.Add(WeaponComp);
}

void ABaseCharacter::DetachAllWeapons()
{
	for (UStaticMeshComponent* Comp : WeaponMeshComponents)
	{
		if (IsValid(Comp))
		{
			Comp->DestroyComponent();
		}
	}
	
	WeaponMeshComponents.Empty();
}

ETeamType ABaseCharacter::GetTeamType() const
{
	if (AER_PlayerState* PS = GetPlayerState<AER_PlayerState>())
	{
		return PS->TeamType;
	}
	
	return TeamID;
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

void ABaseCharacter::HighlightActor(bool bIsHighlight, int32 StencilValue)
{
	if (USkeletalMeshComponent* MyMesh = GetMesh())
	{
		MyMesh->SetRenderCustomDepth(bIsHighlight);
		
		if (bIsHighlight)
		{
			MyMesh->SetCustomDepthStencilValue(StencilValue);
		}
	}
}

void ABaseCharacter::OnRep_TeamID()
{
	FString Team = (TeamID == ETeamType::Team_A) ? TEXT("Team_A") : 
						(TeamID == ETeamType::Team_B) ? TEXT("Team_B") : 
							(TeamID == ETeamType::Team_C) ? TEXT("Team_C") : TEXT("None");
	
	FString Message = FString::Printf(TEXT("[%s] Team Changed to: %s"), *GetName(), *Team);
	
	if (GEngine)
	{
		// GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);
	}
}

void ABaseCharacter::Server_SetTeamID_Implementation(ETeamType NewTeamID)
{
	TeamID = NewTeamID;
	OnRep_TeamID();
}

EVisionChannel ABaseCharacter::ConvertTeamToVisionChannel(ETeamType InTeamType)
{
	switch (InTeamType)
	{
		case ETeamType::None:
		return EVisionChannel::None;
		
		case ETeamType::Team_A:
		return EVisionChannel::TeamA;
		
		case ETeamType::Team_B:
		return EVisionChannel::TeamB;
		
		case ETeamType::Team_C:
		return EVisionChannel::TeamC;

		default:
		return EVisionChannel::None;
	}
}

EVisionChannel ABaseCharacter::GetVisionChannelFromPlayerStateComp()
{
	if (const AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>())
	{
		return ConvertTeamToVisionChannel( ERPS->GetTeamType());
	}
	
	//failed to get the vision channel -> return none
	return EVisionChannel::None;
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
	
	// [길 2] 클라이언트는 서버가 PlayerState를 복제해준 순간 초기화
	InitPlayer();
}

bool ABaseCharacter::IsLocalPlayerPawn()
{
	return IsLocallyControlled();
}

void ABaseCharacter::HandleLevelUp()
{
	if (!HasAuthority()) return;
	
	UBaseAttributeSet* AS = nullptr;
	
	if (AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>())
	{
		AS = ERPS->GetAttributeSet();
	}
	else if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
	{
		AS = PS->GetAttributeSet();
	}

	float OldMaxHealth = 0.0f;
	float OldMaxStamina = 0.0f;

	if (AS)
	{
		OldMaxHealth = AS->GetMaxHealth();
		OldMaxStamina = AS->GetMaxStamina();
	}
	
	InitAttributes();
    
	// 레벨업 시 체력/마나 회복
	if (AS)
	{
		// 스탯 상승량 계산
		float HealthIncrease = AS->GetMaxHealth() - OldMaxHealth;
		float StaminaIncrease = AS->GetMaxStamina() - OldMaxStamina;

		// 상승량이 0보다 크면 현재 스탯에 더해줌
		if (HealthIncrease > 0.0f)
		{
			AS->SetHealth(AS->GetHealth() + HealthIncrease);
		}
		
		if (StaminaIncrease > 0.0f)
		{
			AS->SetStamina(AS->GetStamina() + StaminaIncrease);
		}
		
		// 스킬 포인트 지급
		AS->SetSkillPoint(AS->GetSkillPoint() + 1.0f);
	}

	// 레벨업 이펙트 GC 호출
	if (AbilitySystemComponent.IsValid() && HasAuthority()) // 서버에서 실행
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = GetActorLocation(); 
		CueParams.Instigator = this;
		CueParams.EffectCauser = this;
		
		AbilitySystemComponent->ExecuteGameplayCue(
			ProjectER::GameplayCue::Combat::LevelUp, 
			CueParams
		);
	}
}

float ABaseCharacter::GetCharacterLevel() const
{
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

UAnimMontage* ABaseCharacter::GetCharacterMontageByTag(FGameplayTag MontageTag)
{
	if (CachedMontages.Contains(MontageTag))
	{
		return CachedMontages[MontageTag];
	}
	
	return nullptr;
}

void ABaseCharacter::PreloadMontages()
{
	if (!HeroData) return;
	
	for (const auto& Pair : HeroData->CharacterMontages)
	{
		if (UAnimMontage* LoadedMontage = Pair.Value.LoadSynchronous())
		{
			// 로드된 몽타주를 CachedMontages에 저장하여 메모리에서 날아가지 않게 꽉 붙잡아둡니다 (Caching)
			CachedMontages.Add(Pair.Key, LoadedMontage);
		}
	}
	
	if (!HeroData->BasicHitVFX.IsNull())
	{
		CachedBasicHitVFX = HeroData->BasicHitVFX.LoadSynchronous();
	}
}

void ABaseCharacter::Server_UpgradeSkill_Implementation(FGameplayTag SkillTag)
{
	if (!AbilitySystemComponent.IsValid())
	{
		return;
	}

	// 스킬 포인트 여부 확인
	UBaseAttributeSet* AS = nullptr;
	if (AER_PlayerState* ERPS = GetPlayerState<AER_PlayerState>())
	{
		AS = ERPS->GetAttributeSet();
	}
	else if (ABasePlayerState* PS = GetPlayerState<ABasePlayerState>())
	{
		AS = PS->GetAttributeSet();
	}

	if (!AS)
	{
		return;
	}

	if (AS->GetSkillPoint() <= 0.0f)
	{
		return; 
	}

	// 전달받은 태그에 해당하는 어빌리티 스펙(Spec) 찾기
	FGameplayAbilitySpec* TargetSpec = nullptr;
	
	for (FGameplayAbilitySpec& Spec : AbilitySystemComponent->GetActivatableAbilities())
	{
		// 스킬 식별용 태그(Dynamic Tags 혹은 AbilityTags)와 일치 여부 검사
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(SkillTag) || 
			Spec.Ability->GetAssetTags().HasTagExact(SkillTag))
		{
			TargetSpec = &Spec;
			break;
		}
	}

	// 결과 처리
	if (TargetSpec)
	{
		if (TargetSpec->Level >= 5) 
		{
			return;
		}

		TargetSpec->Level += 1;
		AbilitySystemComponent->MarkAbilitySpecDirty(*TargetSpec);
		AS->SetSkillPoint(AS->GetSkillPoint() - 1.0f);
	}
}

void ABaseCharacter::OnMoveSpeedChanged(const FOnAttributeChangeData& Data)
{
	if (UCharacterMovementComponent* MovementComp = GetCharacterMovement())
	{
		MovementComp->MaxWalkSpeed = Data.NewValue;
	}
}

void ABaseCharacter::OnRep_HeroData()
{
	// 비주엃 초기화 (클라이언트)
	InitVisuals();
}

void ABaseCharacter::InitAbilitySystem()
{
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
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UBaseAttributeSet::GetMoveSpeedAttribute()).AddUObject(this, &ABaseCharacter::OnMoveSpeedChanged);
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
		if (OpenAbilityClass)
		{
			FGameplayAbilitySpec Spec(OpenAbilityClass, 1);
			ASC->GiveAbility(Spec);
		}
		
		if (TeleportAbilityClass)
		{
			FGameplayAbilitySpec Spec(TeleportAbilityClass, 1);
			ASC->GiveAbility(Spec);
		}
		
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
		
		if (RegenEffectClass)
		{
			FGameplayEffectContextHandle Context = AbilitySystemComponent->MakeEffectContext();
			Context.AddSourceObject(this);
		
			FGameplayEffectSpecHandle RegenSpec = AbilitySystemComponent->MakeOutgoingSpec(RegenEffectClass, 1.0f, Context);
			if (RegenSpec.IsValid())
			{
				AbilitySystemComponent->ApplyGameplayEffectSpecToSelf(*RegenSpec.Data.Get());
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
	
	// BaseAttackSpeed 캐싱 — CurveTable Level 1 기준값 자동 추출
	if (HeroData)
	{
		UCurveTable* ASTable = HeroData->StatCurveTable.LoadSynchronous();
		if (ASTable)
		{
			FName ASRowName = FName(*HeroData->StatusRowName.ToString().Append("_AttackSpeed"));
			if (FRealCurve* ASCurve = ASTable->FindCurve(ASRowName, FString()))
			{
				CachedBaseAttackSpeed = ASCurve->Eval(1.0f);
			}
		}
	}
}

void ABaseCharacter::InitVisuals()
{
	if (!HeroData) return;

	// 스켈레탈 메시 로드 및 설정
	if (!HeroData->Mesh.IsNull())
	{
		if (USkeletalMesh* LoadedMesh = HeroData->Mesh.LoadSynchronous())
		{
			GetMesh()->SetSkeletalMesh(LoadedMesh);
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
	
	// 무기 장착
	InitWeapons();
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
	if (AbilitySystemComponent.IsValid())
	{
		static const FGameplayTag CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
		static const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));

		if (AbilitySystemComponent->HasMatchingGameplayTag(CastingTag) || 
			AbilitySystemComponent->HasMatchingGameplayTag(ActiveTag))
		{
			return; // 아무것도 하지 않고 함수 종료 (이동, 회전 차단)
		}
	}
	
	UNavigationSystemV1* NavSys = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());
	if (!NavSys) return;

	UNavigationPath* NavPath = NavSys->FindPathToLocationSynchronously(this, GetActorLocation(), TargetLocation);

	if (NavPath && NavPath->PathPoints.Num() > 1)
	{
		PathPoints = NavPath->PathPoints;
		CurrentPathIndex = 1;
		SetActorTickEnabled(true);
		
		// UE_LOG(LogTemp, Warning, TEXT("길찾기 성공! 포인트 갯수: %d"), NavPath->PathPoints.Num());
		
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
		// UE_LOG(LogTemp, Error, TEXT("길찾기 실패! 시작점과 도착점이 끊어져있거나 NavMesh 범위 밖입니다."));
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
	// [디버깅] 경로 및 이동 방향 시각화
	// 1. 전체 경로 (초록색 선)
	for (int32 i = 0; i < PathPoints.Num() - 1; ++i)
	{
		DrawDebugLine(GetWorld(), PathPoints[i], PathPoints[i + 1],
			FColor::Green, false, -1.0f, 0, 3.0f);
	}
	// 2. 현재 목표 웨이포인트 (빨간색 구체)
	if (PathPoints.IsValidIndex(CurrentPathIndex))
	{
		DrawDebugSphere(GetWorld(), PathPoints[CurrentPathIndex],
			30.0f, 12, FColor::Red, false, -1.0f, 0, 2.0f);
		// 3. 내 위치 → 목표 웨이포인트 (노란색 선)
		DrawDebugLine(GetWorld(), GetActorLocation(), PathPoints[CurrentPathIndex],
			FColor::Yellow, false, -1.0f, 0, 1.5f);
	}
	// 4. 실제 이동 방향 (파란색 화살표)
	FVector Velocity = GetVelocity();
	if (!Velocity.IsNearlyZero())
	{
		DrawDebugDirectionalArrow(GetWorld(),
			GetActorLocation(),
			GetActorLocation() + Velocity.GetSafeNormal() * 100.0f,
			50.0f, FColor::Blue, false, -1.0f, 0, 5.0f);
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
		TargetPos = TargetActor->GetActorLocation();
        
		// 타겟의 키 절반만큼 위를 조준 (가슴팍)
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
	
	if (Distance <= AttackRange) // 사거리 내 진입 시
	{
		if (AbilitySystemComponent.IsValid())
		{
			static const FGameplayTag CastingTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Casting"));
			static const FGameplayTag ActiveTag = FGameplayTag::RequestGameplayTag(FName("Skill.Animation.Active"));
			
			if (AbilitySystemComponent->HasMatchingGameplayTag(CastingTag) || 
				AbilitySystemComponent->HasMatchingGameplayTag(ActiveTag))
			{
				return; 
			}
		}
		
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
			
			if (bHasAbility && CanAttack())
			{
				MarkAttackExecuted();
				bWasActivated = AbilitySystemComponent->TryActivateAbilitiesByTag(FGameplayTagContainer(AttackTag));
			}
			
			if (bWasActivated)
			{
#if WITH_EDITOR
				/*if (bShowDebug)
				{
					UE_LOG(LogTemp, Warning, TEXT("[%s] Tag: %s / Found Ability: %s / Activated: %s"),
							*GetName(),
							*AttackTag.ToString(),
							bHasAbility ? TEXT("YES") : TEXT("NO (Check DataAsset!)"), // 여기가 NO라면 데이터에셋)문제
							bWasActivated ? TEXT("True") : TEXT("False (Check Cooldown/Cost/State)") // 여기가 False라면 조건 문제
							);
				}*/
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

FName ABaseCharacter::GetNextAutoAttackSectionName()
{
	FName SectionName;

	// 현재 인덱스에 맞춰 재생할 몽타주 섹션 이름 결정
	switch (AutoAttackIndex)
	{
	case 0: SectionName = FName("AttackA"); break;
	case 1: SectionName = FName("AttackB"); break;
	case 2: SectionName = FName("AttackC"); break;
	default: SectionName = FName("AttackA"); break;
	}

	// 다음 평타를 위해 인덱스 증가 (0, 1, 2 무한 순환)
	AutoAttackIndex = (AutoAttackIndex + 1) % 3;

	return SectionName;
}

float ABaseCharacter::GetAttackPlayRate() const
{
	if (CachedBaseAttackSpeed <= 0.0f) return 1.0f;
	
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return 1.0f;
	
	// 현재 APS (= CurveTable + 아이템/버프가 반영된 최종 AttackSpeed 값)
	float CurrentAPS = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackSpeedAttribute());
	if (CurrentAPS <= 0.0f) return 1.0f;
	
	// PlayRate = 현재APS / 기본APS
	// 기본 상태에서는 1.0x, 공격속도가 올라갈수록 비례 증가
	return CurrentAPS / CachedBaseAttackSpeed;
}

float ABaseCharacter::GetAttackCooldown() const
{
	UAbilitySystemComponent* ASC = GetAbilitySystemComponent();
	if (!ASC) return 1.0f;
	
	float CurrentAPS = ASC->GetNumericAttribute(UBaseAttributeSet::GetAttackSpeedAttribute());
	if (CurrentAPS <= 0.0f) return 1.0f;
	
	// 공격 주기 = 1 / APS
	return 1.0f / CurrentAPS;
}

// float ABaseCharacter::GetCurrentAttackSectionDuration(UAnimMontage* Montage, FName SectionName) const
// {
// 	if (!Montage) return 1.0f;
	
// 	int32 SectionIndex = Montage->GetSectionIndex(SectionName);
// 	if (SectionIndex == INDEX_NONE) return 1.0f;
	
// 	// 해당 섹션의 원본 재생 시간(초) 반환
// 	return Montage->GetSectionLength(SectionIndex);
// }

bool ABaseCharacter::CanAttack() const
{
	if (!GetWorld()) return false;
	
	float CurrentTime = GetWorld()->GetTimeSeconds();
	float Cooldown = GetAttackCooldown();
	
	// 마지막 공격으로부터 공격 주기가 경과했는지 확인
	return (CurrentTime - LastAttackExecuteTime) >= Cooldown;
}

void ABaseCharacter::MarkAttackExecuted()
{
	if (GetWorld())
	{
		LastAttackExecuteTime = GetWorld()->GetTimeSeconds();
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
		// [김현수 추가분] 사망 진입 시 진행 중인 food 회복 효과와 대기 큐를 즉시 제거
		if (UBaseInventoryComponent* InvComp = FindComponentByClass<UBaseInventoryComponent>())
		{
			InvComp->ClearFoodHealEffects();
		}

		if (AbilitySystemComponent.IsValid()) // 중복 사망 방지
		{
			FGameplayTag DeathTag = ProjectER::State::Life::Death;
			if (AbilitySystemComponent->HasMatchingGameplayTag(DeathTag))
			{
				return; // 중복 사망 방지
			}
			
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

		
		SetTarget(nullptr);
		
		// [김현수 추가분]
		// LootableComponent 초기화: 플레이어의 인벤토리 아이템을 루팅 가능하게 설정
		if (ULootableComponent* LootComp = FindComponentByClass<ULootableComponent>())
		{
			TArray<UBaseItemData*> LootItems;
			TArray<int32> LootCounts;

			// 플레이어의 인벤토리에서 아이템 추출
			if (UBaseInventoryComponent* InvComp = FindComponentByClass<UBaseInventoryComponent>())
			{
				for (int32 i = 0; i < InvComp->MaxSlots; ++i)
				{
					if (UBaseItemData* Item = InvComp->GetItemAt(i))
					{
						LootItems.Add(Item);
						LootCounts.Add(InvComp->GetStackCountAt(i));
					}
				}
			}

			// LootableComponent에 아이템 초기화
			LootComp->InitializeWithItemStacks(LootItems, LootCounts);
		}
		
		OnDeath.Broadcast();
		
		if (AbilitySystemComponent.IsValid() && HasAuthority()) // 서버에서 실행
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = GetActorLocation(); // 현재 캐릭터 위치
			CueParams.Instigator = this;
			CueParams.EffectCauser = this;
			
			AbilitySystemComponent->ExecuteGameplayCue(
				ProjectER::GameplayCue::Combat::Death, 
				CueParams
			);
		}
		
		Multicast_Death();
	}
}

void ABaseCharacter::Revive(FVector RespawnLocation)
{
	if (!HasAuthority()) return;
	
	if (AbilitySystemComponent.IsValid())
	{
		// 사망(Death) 또는 빈사(Down) 태그를 가진 모든 GE 제거
		FGameplayTagContainer BadStateTags;
		BadStateTags.AddTag(ProjectER::State::Life::Death);
		BadStateTags.AddTag(ProjectER::State::Life::Down);
		
		AbilitySystemComponent->RemoveActiveEffectsWithGrantedTags(BadStateTags);

		// Loose Tag로 남아있을 경우를 대비 직접 제거 (기존 코드 호환용)
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
		ERPS->bIsDead = false;
		ERPS->CurrentRestrictedTime = 10.0f;
		ERPS->setUI_RestrictedTime();
		ERPS->ForceNetUpdate();

	}
	else if (ABasePlayerState* BasePS = GetPlayerState<ABasePlayerState>())
	{
		AS = BasePS->GetAttributeSet();
	}
	
	if (AS)
	{
		AS->SetHealth(AS->GetMaxHealth() * 0.3f);
		AS->SetStamina(AS->GetMaxStamina() * 0.3f);
	}
    
	// 타겟 초기화
	SetTarget(nullptr);

	// 클라이언트 동기화 (위치 이동 및 비주얼/물리 복구)
	Multicast_Revive(RespawnLocation);
	
	// 부활 이펙트 GC 호출 (멀티캐스트로 모든 클라이언트에 재생 요청)
	if (AbilitySystemComponent.IsValid())
	{
		// 부활 대상 정보, 위치 Context
		FGameplayCueParameters CueParams;
		CueParams.Location = RespawnLocation;
		CueParams.Instigator = this;
		CueParams.EffectCauser = this;

		AbilitySystemComponent->ExecuteGameplayCue(
			ProjectER::GameplayCue::Combat::Revive, 
			CueParams
		);
	}
}

void ABaseCharacter::HandleDown()
{
	if (HasAuthority())
	{
		// [김현수 추가분] 빈사 진입 시 진행 중인 food 회복 효과와 대기 큐를 즉시 제거
		if (UBaseInventoryComponent* InvComp = FindComponentByClass<UBaseInventoryComponent>())
		{
			InvComp->ClearFoodHealEffects();
		}

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

	// [김현수 추가분]
	// 메시 콜리전 복구
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
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
	
	// [김현수 추가분]
	// LootableComponent 초기화 (부활 후 루팅 불가능하도록)
	if (ULootableComponent* LootComp = FindComponentByClass<ULootableComponent>())
	{
		LootComp->InitializeWithItems(TArray<UBaseItemData*>());
	}
	
	SetActorTickEnabled(true);
}

void ABaseCharacter::Multicast_Death_Implementation()
{
	// 사망 애니메이션 몽타주 재생
	FGameplayTag DeathTag = FGameplayTag::RequestGameplayTag(FName("Montage.Common.Death"));
	if (UAnimMontage* DeathMontage = GetCharacterMontageByTag(DeathTag))
	{
		PlayAnimMontage(DeathMontage);
	}

	// Capsule 비활성화
	if (GetCapsuleComponent())
	{
		GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// [김현수 추가분]
	// 메시 콜리전을 QueryOnly로 설정 (우클릭/raycast감지 가능, 플레이어 통과 가능)
	if (GetMesh())
	{
		GetMesh()->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
		GetMesh()->SetCollisionObjectType(ECC_Pawn);
		GetMesh()->SetCollisionResponseToAllChannels(ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
		GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);  // 플레이어 통과 가능
		GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);  // 커서 클릭 가능
	}

	// 이동 정지 및 기능 비활성화
	if (GetCharacterMovement())
	{
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->DisableMovement();
	}
    
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
}

void ABaseCharacter::InitUI()
{
	ABasePlayerController* PC = Cast<ABasePlayerController>(GetController());
	if (IsValid(PC) && PC->IsLocalController())
	{
		AHUD* GenericHUD = PC->GetHUD();
		if (GenericHUD == nullptr)
		{
			// UE_LOG(LogTemp, Error, TEXT("!!! HUD NONE CREATED!!!"));
			return;
		}

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
			PC->OnInventoryUpdated();
			
			// 얼굴 아이콘 설정
			HUD->getMainHUD()->SetMyFaceIcon(HeroData->CharacterIcon);
		
			// 팀원 UI 오픈
			ETeamType MyTeam = GetTeamType();

			if (AGameStateBase* GS = GetWorld()->GetGameState())
			{
				AER_GameState* ERGS = Cast<AER_GameState>(GS);
				if (IsValid(ERGS))
				{
					//for (APlayerState* PS : GS->PlayerArray)
					//{
					//	AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS);
					//	if (IsValid(ERPS))
					//	{
					//		if (MyTeam == ERPS->TeamType)
					//		{
					//			countTeam++;
					//		}
					//	}
					//}

					int32 WidgetIndex = 0;

					for (APlayerState* PS : ERGS->PlayerArray)
					{
						AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS);

						if (ERPS && ERPS != this->GetPlayerState() && ERPS->TeamType == MyTeam)
						{
							if (APawn* TeamPawn = ERPS->GetPawn())
							{
								ABaseCharacter* TeamChar = Cast<ABaseCharacter>(TeamPawn);
								if (TeamChar)
								{
									if (UAbilitySystemComponent* TargetASC = ERPS->GetAbilitySystemComponent())
									{
										HUD->getMainHUD()->SetTeamMemberData(WidgetIndex, TargetASC, TeamChar->HeroData->CharacterIcon);
										HUD->getMainHUD()->SetTeamWidgetVisible(WidgetIndex, true);

										WidgetIndex++;
									}
								}
							}
						}
					}
				}
			}
			HUD->getMainHUD()->InitTeamData();
		}
		else
		{
			// UE_LOG(LogTemp, Error, TEXT("!!! HUD Casting Fail! address : %s !!!"), *GenericHUD->GetName());
		}

		if (HP_MP_BarWidget)
		{
			if (!HP_MP_BarWidget->GetUserWidgetObject())
			{
				HP_MP_BarWidget->InitWidget();
			}
			HPBarWidgetInstance = Cast<UUI_HP_Bar>(HP_MP_BarWidget->GetUserWidgetObject());
		}

	}

	/// 미니맵 설정
	if (!IsLocallyControlled())
	{
		/// '나' 이외는 캡쳐 컴포넌트를 꺼서 성능 최적화~
		MinimapCaptureComponent->Deactivate();
	}
	else  //  no more tick update
	{
		MinimapCaptureComponent->bCaptureEveryFrame = false;
		MinimapCaptureComponent->bCaptureOnMovement = false;

		MinimapCaptureComponent->Activate();

		GetWorld()->GetTimerManager().SetTimer(
			MinimapCaptureTimerHandle,
			this,
			&ABaseCharacter::UpdateMinimapCapture,
			MinimapUpdateRate, true);
	}

	// 방장(Listen Server)과 클라이언트 모두 HP Bar 및 미니맵 아이콘 초기화 필요
	UpdateOverheadUI();

	// mpyi _ 미니맵용 얼굴 아이콘 마테리얼 인스턴스 초기화

	if (MinimapIconMesh && HeroData && HeroData->CharacterIcon)
	{
		UMaterialInterface* IconMasterMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/MPYI/Material/M_MinimapIcon.M_MinimapIcon"));
		UMaterialInterface* LineMasterMat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/MPYI/Material/M_MinimapLine.M_MinimapLine"));
		if (IconMasterMat)
		{
			MinimapIconMaterial = MinimapIconMesh->CreateDynamicMaterialInstance(0, IconMasterMat);
			MinimapIconMaterial->SetTextureParameterValue(FName("CharacterTexture"), HeroData->CharacterIcon);
		}
		if (LineMasterMat)
		{
			MinimapLineMaterial = MinimapLineMesh->CreateDynamicMaterialInstance(0, LineMasterMat);

			FLinearColor teamColor;
			if (IsLocallyControlled())
			{
				teamColor = FLinearColor::Green;
			}
			else
			{
				FLinearColor TeamColorA = FLinearColor::Red;
				FLinearColor TeamColorB = FLinearColor::Blue;
				FLinearColor TeamColorC = FLinearColor::Yellow;

				AER_PlayerState* MyPS = Cast<AER_PlayerState>(GetPlayerState());
				if (IsValid(MyPS))
				{
					switch (MyPS->TeamType)
					{
					case ETeamType::Team_A:
						teamColor = TeamColorA;
						break;
					case ETeamType::Team_B:
						teamColor = TeamColorB;
						break;
					case ETeamType::Team_C:
						teamColor = TeamColorC;
						break;
					default:
						teamColor = FLinearColor::Gray; // 예외 처리용
						break;
					}
				}
			}
			UpdateMinimapVisuals(teamColor);
		}
	}
	
}

void ABaseCharacter::UpdateOverheadUI()
{
	if (!HPBarWidgetInstance && HP_MP_BarWidget)
	{
		if (!HP_MP_BarWidget->GetUserWidgetObject())
		{
			HP_MP_BarWidget->InitWidget();
		}
		HPBarWidgetInstance = Cast<UUI_HP_Bar>(HP_MP_BarWidget->GetUserWidgetObject());
	}

	if (HPBarWidgetInstance && GetAbilitySystemComponent())
	{
		float CurrentHP = GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
		float MaxHP = GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());
		float CurrentMP = GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetStaminaAttribute());
		float MaxMP = GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetMaxStaminaAttribute());

		// HPBarWidgetInstance->Update_HP_bar(CurrentHP, MaxHP, team);
		// HPBarWidgetInstance->Update_MP_bar(CurrentMP, MaxMP);
		// HPBarWidgetInstance->Update_LV_bar(1);
		OnHealthChanged();
		OnStaminaChanged();
		OnLevelChanged();

		if (HeroData)
		{
			HPBarWidgetInstance->Update_HeadIcon(HeroData->CharacterIcon);
		}
		
		AER_PlayerState* TargetPS = GetPlayerState<AER_PlayerState>();
		if (IsValid(TargetPS))
		{
			FText PlayerName = FText::FromString(TargetPS->GetPlayerName());
			HPBarWidgetInstance->Update_PlayerName(PlayerName);
		}
		

		
	}
}

void ABaseCharacter::OnHealthChanged()
{
	if (!HPBarWidgetInstance && HP_MP_BarWidget)
	{
		if (!HP_MP_BarWidget->GetUserWidgetObject())
		{
			HP_MP_BarWidget->InitWidget();
		}
		HPBarWidgetInstance = Cast<UUI_HP_Bar>(HP_MP_BarWidget->GetUserWidgetObject());
	}

	if (HPBarWidgetInstance && GetAbilitySystemComponent())
	{
		float CurrentHP = GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetHealthAttribute());
		float MaxHP = GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetMaxHealthAttribute());

		int32 team = 2; 

		if (IsLocallyControlled())
		{
			team = 0; // 나 자신
		}
		else
		{
			APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
			if (LocalPC)
			{
				AER_PlayerState* MyPS = LocalPC->GetPlayerState<AER_PlayerState>();
				AER_PlayerState* TargetPS = GetPlayerState<AER_PlayerState>();

				if (MyPS && TargetPS)
				{
					if (MyPS->TeamType == TargetPS->TeamType)
					{
						team = 1; // 아군
					}
					else
					{
						team = 2; // 적군
					}
				}
			}
		}


		HPBarWidgetInstance->Update_HP_bar(CurrentHP, MaxHP, team);
	}
}

void ABaseCharacter::OnStaminaChanged()
{
	if (!HPBarWidgetInstance && HP_MP_BarWidget)
	{
		if (!HP_MP_BarWidget->GetUserWidgetObject())
		{
			HP_MP_BarWidget->InitWidget();
		}
		HPBarWidgetInstance = Cast<UUI_HP_Bar>(HP_MP_BarWidget->GetUserWidgetObject());
	}

	if (HPBarWidgetInstance && GetAbilitySystemComponent())
	{
		float CurrentSP = GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetStaminaAttribute());
		float MaxSP = GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetMaxStaminaAttribute());

		HPBarWidgetInstance->Update_MP_bar(CurrentSP, MaxSP);
	}
}

void ABaseCharacter::OnLevelChanged()
{
	if (!HPBarWidgetInstance && HP_MP_BarWidget)
	{
		if (!HP_MP_BarWidget->GetUserWidgetObject())
		{
			HP_MP_BarWidget->InitWidget();
		}
		HPBarWidgetInstance = Cast<UUI_HP_Bar>(HP_MP_BarWidget->GetUserWidgetObject());
	}

	if (HPBarWidgetInstance && GetAbilitySystemComponent())
	{
		float CurrentLV = GetAbilitySystemComponent()->GetNumericAttribute(UBaseAttributeSet::GetLevelAttribute());

		HPBarWidgetInstance->Update_LV_bar(CurrentLV);
	}
}

void ABaseCharacter::UpdateMinimapCapture()
{
	if (MinimapCaptureComponent && MinimapCaptureComponent->IsActive())
		MinimapCaptureComponent->CaptureScene();
}

void ABaseCharacter::UpdateMinimapVisuals(FLinearColor n_teamColor)
{
	if (MinimapLineMaterial)
	{
		MinimapLineMaterial->SetVectorParameterValue(FName("TeamColor"), n_teamColor);
	}
}

EVisionChannel ABaseCharacter::GetVisionChannelFromVisionPlayerStateComp()
{
	if (APlayerState* PC=GetPlayerState())
	{
		if (UVisionPlayerStateComp* PVC=PC->FindComponentByClass<UVisionPlayerStateComp>())
		{
			return PVC->GetTeamChannel();
		}
	}

	//failed to get the vision channel
	return EVisionChannel::None;
}

void ABaseCharacter::InitPlayer()
{
	// UI 초기화 (로컬 플레이어 전용 로직이 내부에 있음)
	//InitUI();

	// 일단 임시로 2초 뒤에 로딩하도록
	GetWorld()->GetTimerManager().SetTimer(
	UILoadTimerHandle,
    this,
    &ABaseCharacter::InitUI,
    2.0f,
    false);

	// Camera Setting for local player pawn
	if (TopDownCameraComp)
	{
		UE_LOG(LogTemp, Log, TEXT("[BaseChar] InitPlayer: Setting up TopDownCameraComp"));
		
		// IsLocallyControlled() 대신 컨트롤러를 직접 가져와서 로컬 플레이어인지 체크합니다.
		if (APlayerController* PC = Cast<APlayerController>(GetController()))
		{
			if (PC->IsLocalController())
			{
				// [이 코드는 리슨 서버의 방장(Host)과 게임에 접속한 클라이언트(Client) 모두, 자신의 화면에 보이는 자기 캐릭터에서만 실행됩니다]
				UE_LOG(LogTemp, Log, TEXT("[BaseChar] InitPlayer: PC->IsLocalController() == TRUE. Activating Camera."));
				TopDownCameraComp->InitializeComponent();
				TopDownCameraComp->Activate();
				TopDownCameraComp->SetComponentTickEnabled(true);
			}
			else
			{
				// [이 코드는 남의 캐릭터 즉, 내 화면에 보이는 다른 유저의 캐릭터일 때 실행됩니다]
				UE_LOG(LogTemp, Log, TEXT("[BaseChar] InitPlayer: PC->IsLocalController() == FALSE. Deactivating Camera."));
				TopDownCameraComp->Deactivate();
				TopDownCameraComp->SetComponentTickEnabled(false);
			}
		}
		else
		{
			// 이 캐릭터에 컨트롤러가 아직 안 붙었다면 (AI거나 스폰 직후인 경우)
			UE_LOG(LogTemp, Log, TEXT("[BaseChar] InitPlayer: No PlayerController Attached Yet. Deactivating Camera."));
			TopDownCameraComp->Deactivate();
			TopDownCameraComp->SetComponentTickEnabled(false);
		}
	}
	
	OnPlayerStateChosen();
}

void ABaseCharacter::Multicast_ToggleCraftingUI_Implementation(bool bShow)
{
	if (bShow)
	{
		if (CraftingWidgetClass && !CraftingWidgetComp)
		{
			CraftingWidgetComp = NewObject<UWidgetComponent>(this, TEXT("CraftingWidgetComp"));
			CraftingWidgetComp->SetWidgetSpace(EWidgetSpace::Screen);
			CraftingWidgetComp->SetWidgetClass(CraftingWidgetClass);
			CraftingWidgetComp->SetDrawSize(FVector2D(50.f, 50.f));
			CraftingWidgetComp->RegisterComponent();
			CraftingWidgetComp->AttachToComponent(GetMesh(), FAttachmentTransformRules::KeepRelativeTransform);
			CraftingWidgetComp->SetRelativeLocation(FVector(0.f, 0.f, 400.f)); // HP바(300.f)보다 더 높은 위치로 지정

			// 시야 판정 타이머 시작 (0.1초마다 검사)
			GetWorld()->GetTimerManager().SetTimer(
				CraftingUIVisibilityTimer,
				this,
				&ABaseCharacter::UpdateCraftingUIVisibility,
				0.3f,
				true
			);

			// 즉시 1회 실행하여 최초 상태 반영
			UpdateCraftingUIVisibility();
		}
	}
	else
	{
		if (CraftingWidgetComp)
		{
			CraftingWidgetComp->DestroyComponent();
			CraftingWidgetComp = nullptr;
		}

		// 시작했던 타이머 초기화 (파괴)
		GetWorld()->GetTimerManager().ClearTimer(CraftingUIVisibilityTimer);
	}
}

void ABaseCharacter::UpdateCraftingUIVisibility()
{
	if (!CraftingWidgetComp) return;

	APlayerController* LocalPC = GetWorld()->GetFirstPlayerController();
	if (!LocalPC) return;

	ABaseCharacter* LocalChar = Cast<ABaseCharacter>(LocalPC->GetPawn());
	if (!LocalChar) return;

	// 1. 내 자신이거나 우리 팀(아군)이면 무조건 보임
	if (this == LocalChar || this->GetTeamType() == LocalChar->GetTeamType())
	{
		CraftingWidgetComp->SetVisibility(true);
		return;
	}

	// 2. 적이거나 중립일 때 1000거리 이상이면 가림
	float Dist = FVector::Dist(this->GetActorLocation(), LocalChar->GetActorLocation());
	if (Dist > 1000.f)
	{
		CraftingWidgetComp->SetVisibility(false);
		return;
	}

	// 3. 거리 1000 안쪽이면 시야(장애물) 라인트레이스 확인 (월드 스태틱만 감지)
	TArray<FHitResult> HitResults;
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(this);
	QueryParams.AddIgnoredActor(LocalChar);

	FCollisionObjectQueryParams ObjectParams;
	ObjectParams.AddObjectTypesToQuery(ECC_WorldStatic);

	// 중간에 겹치는 투명 볼륨(PCGVolume 등)을 통과하기 위해 MultiTrace 사용
	bool bHit = GetWorld()->LineTraceMultiByObjectType(
		HitResults,
		LocalChar->GetActorLocation(),
		this->GetActorLocation(),
		ObjectParams,
		QueryParams
	);

	bool bBlockedByWall = false;

	if (bHit)
	{
		for (const FHitResult& Hit : HitResults)
		{
			AActor* HitActor = Hit.GetActor();
			
			// 충돌한 액터가 '볼륨(Volume)' 클래스 계열이면 무시하고 통과시킴
			if (HitActor && !HitActor->IsA<AVolume>())
			{
				bBlockedByWall = true;
				
				// 디버깅용 로그: 진짜 벽을 가린 녀석 출력
				FString HitName = HitActor->GetName();
				FString CompName = Hit.GetComponent() ? Hit.GetComponent()->GetName() : TEXT("UnknownComp");
				UE_LOG(LogTemp, Warning, TEXT("[Crafting UI Visibility] Blocked by Actor: %s / Component: %s"), *HitName, *CompName);
				
				break; // 하나라도 진짜 벽에 막혔으면 더 검사할 필요 없음
			}
		}
	}

	if (bBlockedByWall)
	{
		// 중간에 진짜 장애물(벽 등)이 있으면 가림
		CraftingWidgetComp->SetVisibility(false);
	}
	else
	{
		// 안 가려져 있으면 보임
		CraftingWidgetComp->SetVisibility(true);
	}
}
