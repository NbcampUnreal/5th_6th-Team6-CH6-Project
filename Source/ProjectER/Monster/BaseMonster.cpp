#include "Monster/BaseMonster.h"

#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"
#include "Monster/Data/MonsterDataAsset.h"
#include "Monster/Data/BaseMonsterTableRow.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"

#include "Components/StateTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/WidgetComponent.h"
#include "Monster/MonsterRangeComponent.h"
#include "Components/ProgressBar.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

ABaseMonster::ABaseMonster()
	:TargetPlayer(nullptr),
	StartLocation(FVector::ZeroVector),
	StartRotator(FRotator::ZeroRotator),
	bIsCombat(false),
	bIsDead(false)
{
	//액터 복제, UPROPERTY(Replicated)멤버 동기화
	SetReplicates(true);
	//위치 / 회전 / 속도 동기화
	SetReplicateMovement(true);

	//Tick 설정
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	// Collision 설정
	GetMesh()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::PhysicsOnly);
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

	// ASC 복제, 데이터 Minimal로 되는지 확인 필요
	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);
	
	AttributeSet = CreateDefaultSubobject<UBaseMonsterAttributeSet>(TEXT("AttributeSet"));
	
	// StateTree은 각 클라에서 실행
	StateTreeComp = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	StateTreeComp->SetStartLogicAutomatically(false);

	// 주변 플레이어 감지용 컴포넌트
	MonsterRangeComp = CreateDefaultSubobject<UMonsterRangeComponent>(TEXT("MonsterRangeComponent"));	
	MonsterRangeComp->SetIsReplicated(true);

	//UI Component
	HPBarWidgetComp = CreateDefaultSubobject<UWidgetComponent>(TEXT("WidgetComponent"));
	HPBarWidgetComp->SetupAttachment(GetMesh());
	HPBarWidgetComp->SetWidgetSpace(EWidgetSpace::Screen); // 체력바 크기가 일정할거같으니까?
	HPBarWidgetComp->SetVisibility(false);

	TeamID = ETeamType::Neutral;
}

UAbilitySystemComponent* ABaseMonster::GetAbilitySystemComponent() const
{
	return ASC;
}

void ABaseMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseMonster, MonsterData);
	DOREPLIFETIME(ABaseMonster, TargetPlayer);
	DOREPLIFETIME(ABaseMonster, StartLocation);
	DOREPLIFETIME(ABaseMonster, StartRotator);
	DOREPLIFETIME(ABaseMonster, bIsCombat);
	DOREPLIFETIME(ABaseMonster, bIsDead);
}

void ABaseMonster::PossessedBy(AController* newController)
{
	Super::PossessedBy(newController);

	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::PossessedBy : Not ASC"), *GetName());
		return;
	}
	if (IsValid(AttributeSet) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::PossessedBy : Not AttributeSet"), *GetName());
		return;
	}

	ASC->InitAbilityActorInfo(this, this);

	if (HasAuthority())
	{
		AttributeSet->OnMonsterHit.AddDynamic(this, &ABaseMonster::OnMonterHitHandle);
		AttributeSet->OnMonsterDeath.AddDynamic(this, &ABaseMonster::OnMonterDeathHandle);
		MonsterRangeComp->OnPlayerCountOne.AddDynamic(this, &ABaseMonster::OnPlayerCountOneHandle);
		MonsterRangeComp->OnPlayerCountZero.AddDynamic(this, &ABaseMonster::OnPlayerCountZeroHandle);
		AttributeSet->OnMoveSpeedChanged.AddDynamic(this, &ABaseMonster::OnMoveSpeedChangedHandle);
	}
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		//테스트용 스폰
		FPrimaryAssetId MonsterAssetId(TEXT("Monster"), TEXT("DA_Monster_Orc"));
		InitMonsterData(MonsterAssetId, 1);
		StartLocation = GetActorLocation();
		StartRotator = GetActorRotation();
	}
	if (!HasAuthority())
	{
		// UI 로직
		InitHPBar();
		AttributeSet->OnHealthChanged.AddDynamic(this, &ABaseMonster::OnHealthChangedHandle);
	}
}

void ABaseMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseMonster::InitMonsterData(FPrimaryAssetId MonsterAssetId, float Level)
{
	UAssetManager::Get().LoadPrimaryAsset(MonsterAssetId,
		TArray<FName>({ "Init" }),
		FStreamableDelegate::CreateUObject(
			this,
			&ABaseMonster::OnMonsterDataLoaded,
			MonsterAssetId,
			Level
		));
}

void ABaseMonster::OnMonsterDataLoaded(FPrimaryAssetId MonsterAssetId, float Level)
{
	MonsterData = Cast<UMonsterDataAsset>(
		UAssetManager::Get().GetPrimaryAssetObject(MonsterAssetId)
	);
	if (IsValid(MonsterData) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitMonsterData - MonsterData is Not Valid!"));
	}

	InitVisuals();
	InitAttributes(Level);
	InitGiveAbilities();
	// 데이터 로드 완료 후 실행
	StateTreeComp->StartLogic();
}

void ABaseMonster::InitGiveAbilities()
{
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitGiveAbilities - ASC is Not Valid!"));
		return;
	}

	if (MonsterData->DefaultAbilities.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitGiveAbilities - DefaultAbilities is Empty!"));
		return;
	}

	for (auto& AbilityPtr : MonsterData->DefaultAbilities)
	{
		//AbilityPtr.LoadSynchronous();

		if (IsValid(AbilityPtr) && ASC)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(AbilityPtr.Get(), 1, 0));
		}
	}
}

void ABaseMonster::InitAttributes(float Level)
{
	//MonsterData->MonsterDataTable.LoadSynchronous();
	//MonsterData->MonsterCurveTable.LoadSynchronous();

	if (IsValid(MonsterData->MonsterDataTable) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitAttributes : MonsterDataTable Not"));
		return;
	}
	if (IsValid(MonsterData->MonsterCurveTable) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitAttributes : MonsterCurveTable Not"));
		return;
	}
	//Level로 CurveTable 적용 
	
	FBaseMonsterTableRow* MonsterRow = MonsterData->MonsterDataTable->FindRow<FBaseMonsterTableRow>(MonsterData->TableRowName, TEXT("MonsterData"));

	if (MonsterRow)
	{
		
		AttributeSet->SetLevel(1.f);
		AttributeSet->SetMaxLevel(MonsterRow->BaseMaxLevel);
		AttributeSet->SetXP(0.f);
		FRealCurve* MaxHealth = MonsterData->MonsterCurveTable->FindCurve(FName("MaxHealth"), TEXT("MonsterCurve"));
		if (MaxHealth)
		{
			AttributeSet->SetMaxHealth(MonsterRow->BaseMaxHealth + MaxHealth->Eval(Level));
		}
		AttributeSet->SetHealth(MonsterRow->BaseMaxHealth);
		AttributeSet->SetHealthRegen(MonsterRow->BaseHealthRegen);
		AttributeSet->SetStamina(MonsterRow->BaseMaxStamina);
		AttributeSet->SetMaxStamina(MonsterRow->BaseMaxStamina);
		AttributeSet->SetStaminaRegen(MonsterRow->BaseStaminaRegen);
		FRealCurve* AttackPower = MonsterData->MonsterCurveTable->FindCurve(FName("AttackPower"), TEXT("MonsterCurve"));
		if (AttackPower)
		{
			AttributeSet->SetAttackPower(MonsterRow->BaseAttackPower + AttackPower->Eval(Level));
		}
		AttributeSet->SetAttackRange(MonsterRow->BaseAttackRange);
		AttributeSet->SetCriticalChance(MonsterRow->BaseCriticalChance);
		AttributeSet->SetCriticalDamage(MonsterRow->BaseCriticalDamage);
		AttributeSet->SetMoveSpeed(MonsterRow->BaseMoveSpeed);
		AttributeSet->SetCooldownReduction(MonsterRow->BaseCooldownReduction);
		AttributeSet->SetTenacity(MonsterRow->BaseTenacity);
		AttributeSet->SetAttackSpeed(MonsterRow->BaseAttackSpeed);
		FRealCurve* SkillAmp = MonsterData->MonsterCurveTable->FindCurve(FName("SkillAmp"), TEXT("MonsterCurve"));
		if (SkillAmp)
		{
			AttributeSet->SetSkillAmp(MonsterRow->BaseSkillAmp + SkillAmp->Eval(Level));
		}
		FRealCurve* Defense = MonsterData->MonsterCurveTable->FindCurve(FName("Defense"), TEXT("MonsterCurve"));
		if (Defense)
		{
			AttributeSet->SetDefense(MonsterRow->BaseDefense + Defense->Eval(Level));
		}
	}
}

void ABaseMonster::InitVisuals()
{
	//MonsterData->Mesh.LoadSynchronous();
	//MonsterData->Anim.LoadSynchronous();

	if (IsValid(MonsterData->Mesh) == false || !GetMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitVisuals : MonsterData->Mesh Not"));
		return;
	}
	GetMesh()->SetSkeletalMesh(MonsterData->Mesh.Get());

	if (IsValid(MonsterData->Anim) == false || !GetMesh())
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitVisuals : MonsterData->Anim Not"));
		return;
	}
	GetMesh()->SetAnimInstanceClass(MonsterData->Anim.Get());
}

void ABaseMonster::OnRep_IsCombat()
{
	if (HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(bIsCombat);
	}
}

void ABaseMonster::OnRep_IsDead()
{
	if (bIsDead && HPBarWidgetComp)
	{
		HPBarWidgetComp->SetVisibility(false);
	}
}

void ABaseMonster::OnHealthChangedHandle(float CurrentHP, float MaxHP)
{
	// UpdateHP
	UUserWidget* Widget = HPBarWidgetComp->GetUserWidgetObject();
	UProgressBar* HPBar = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("HealthBar")));
	HPBar->SetPercent(CurrentHP / MaxHP);
}

void ABaseMonster::OnMoveSpeedChangedHandle(float OldSpeed, float NewSpeed)
{
	GetCharacterMovement()->MaxWalkSpeed = NewSpeed;
}

void ABaseMonster::InitHPBar()
{
	UUserWidget* Widget = HPBarWidgetComp->GetUserWidgetObject();
	if (IsValid(Widget) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::BeginPlay : Not Widget"));
	}
	UProgressBar* HPBar = Cast<UProgressBar>(Widget->GetWidgetFromName(TEXT("HealthBar")));
	if (IsValid(HPBar) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::BeginPlay : Not HPBar"));
	}

	HPBar->SetPercent(1.f);
}



// 서버에서만
void ABaseMonster::OnMonterHitHandle(AActor* Target)
{
	SetTargetPlayer(Target);
	SetbIsCombat(true);

	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not StateTree"));
		return;
	}
	if (HitEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not HitEventTag"));
		return;
	}

	StateTreeComp->SendStateTreeEvent(FStateTreeEvent(HitEventTag));
}

void ABaseMonster::OnMonterDeathHandle(AActor* Target)
{
	SetbIsDead(true);
	SetTargetPlayer(nullptr);
	SetbIsCombat(false);

	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterDeathHandle : Not StateTree"));
		return;
	}
	if (DeathEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterDeathHandle : Not DeathEventTag"));
		return;
	}
	StateTreeComp->SendStateTreeEvent(FStateTreeEvent(DeathEventTag));

	// [전민성] - 사망 시 gamemode에 알림 추가
	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
	InGameMode->NotifyNeutralDied(this);
	// Target에게 보상 지급
   
	//BoxComp = 생성;
	//BoxComp->InitBox(DataAsset->ItemList);

	GiveRewardsToPlayer(Target);
}

void ABaseMonster::GiveRewardsToPlayer(AActor* Player)
{
	if (IsValid(Player) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::GiveRewardsToPlayer : Not Player"));
		return;
	}
	if (IsValid(MonsterData.Get()) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::GiveRewardsToPlayer : Not MonsterData"));
		return;
	}
		
	//타겟에게 GE를 이용해 경험치 전달
	FGameplayEffectContextHandle ContextHandle = ASC->MakeEffectContext();
	ContextHandle.AddSourceObject(this);
	ContextHandle.AddInstigator(this, nullptr);
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(XPRewardEffect, 1, ContextHandle);
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player);

	SpecHandle.Data->SetSetByCallerMagnitude(
		IncomingXPTag,
		MonsterData->Exp
	);
	UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::GiveRewardsToPlayer : XP %d"), MonsterData->Exp);
	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void ABaseMonster::OnPlayerCountOneHandle()
{ 
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountOneHandle : Not ASC"));
		return;
	}
	FGameplayEventData* Payload = new FGameplayEventData();
	ASC->HandleGameplayEvent(BeginSearchEventTag, Payload);
	StateTreeComp->SendStateTreeEvent(BeginSearchEventTag);
}  

void ABaseMonster::OnPlayerCountZeroHandle()
{
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountZeroHandle : Not ASC"));
		return;
	}

	if (bIsCombat == false)
	{
		FGameplayEventData* Payload = new FGameplayEventData();
		ASC->HandleGameplayEvent(FGameplayTag(EndSearchEventTag), Payload);
		StateTreeComp->SendStateTreeEvent(FStateTreeEvent(EndSearchEventTag));
	}
}

void ABaseMonster::SendAttackRangeEvent(float AttackRange)
{
	if (IsValid(TargetPlayer) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::SendAttackRangeEvent : Not Player"));
		return;
	}
	const float Distance = 
		FVector::DistSquared(
			TargetPlayer->GetActorLocation(), 
			GetActorLocation()
		);

	if (Distance <= AttackRange * AttackRange)
	{
		// 공격가능
		StateTreeComp->SendStateTreeEvent(FGameplayTag(TargetOnEventTag));
	}
	else
	{
		// 공격불가능
		StateTreeComp->SendStateTreeEvent(FGameplayTag(TargetOffEventTag));
	}
}

void ABaseMonster::SendReturnSuccessEvent()
{
	StateTreeComp->SendStateTreeEvent(FGameplayTag(ReturnEventTag));
}

UStateTreeComponent* ABaseMonster::GetStateTreeComponent()
{
	return StateTreeComp;
}

void ABaseMonster::SetTargetPlayer(AActor* Target)
{
	TargetPlayer = Target;
}

AActor* ABaseMonster::GetTargetPlayer()
{
	return TargetPlayer;
}

void ABaseMonster::SetbIsCombat(bool value)
{
	bIsCombat = value;
}

bool ABaseMonster::GetbIsCombat()
{
	return bIsCombat;
}

void ABaseMonster::SetbIsDead(bool value)
{
	bIsDead = value;
}

bool ABaseMonster::GetbIsDead()
{
	return bIsDead;
}


ETeamType ABaseMonster::GetTeamType() const
{
	return TeamID;
}

bool ABaseMonster::IsTargetable() const
{
	return true;
}

void ABaseMonster::Server_SetTeamID_Implementation(ETeamType NewTeamID)
{
	TeamID = NewTeamID;
	OnRep_TeamID();
}

void ABaseMonster::OnRep_TeamID()
{
	/*FString Team = (TeamID == ETeamType::Team_A) ? TEXT("Team_A") : 
						(TeamID == ETeamType::Team_B) ? TEXT("Team_B") : 
							(TeamID == ETeamType::Team_C) ? TEXT("Team_C") : TEXT("None");
	
	FString Message = FString::Printf(TEXT("[%s] Team Changed to: %s"), *GetName(), *Team);
	
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, Message);
	}
	
	UE_LOG(LogTemp, Warning, TEXT("%s"), *Message);*/
}

/// [전민성 추가분]
void ABaseMonster::Death()
{
	if (!HasAuthority())
		return;

	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
	InGameMode->NotifyNeutralDied(this);

	SetLifeSpan(0.1f);
}
