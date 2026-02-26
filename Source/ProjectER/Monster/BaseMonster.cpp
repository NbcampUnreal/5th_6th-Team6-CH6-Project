#include "Monster/BaseMonster.h"

#include "Monster/GAS/AttributeSet/BaseMonsterAttributeSet.h"
#include "Monster/Data/MonsterDataAsset.h"
#include "Monster/Data/BaseMonsterTableRow.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "SkillSystem/SkillDataAsset.h"

#include "Components/StateTreeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/BoxComponent.h"
#include "Components/WidgetComponent.h"
#include "Monster/MonsterRangeComponent.h"
#include "Components/ProgressBar.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "ItemSystem/Component/LootableComponent.h"

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
	GetCapsuleComponent()->SetCollisionProfileName(TEXT("Spectator"));

	HitBoxComp = CreateDefaultSubobject<UBoxComponent>(TEXT("HitBoxComponent"));
	HitBoxComp->SetupAttachment(RootComponent);
	HitBoxComp->SetCollisionProfileName(TEXT("Spectator"));

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

	//ItemBox
	LootableComp = CreateDefaultSubobject<ULootableComponent>(TEXT("LootableComponent"));
}

UAbilitySystemComponent* ABaseMonster::GetAbilitySystemComponent() const
{
	return ASC;
}

void ABaseMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseMonster, bIsCombat);
	DOREPLIFETIME(ABaseMonster, bIsDead);
	DOREPLIFETIME(ABaseMonster, TeamID);
	DOREPLIFETIME(ABaseMonster, MonsterId);
	DOREPLIFETIME(ABaseMonster, MonsterLevel);
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
		AttributeSet->OnMoveSpeedChanged.AddDynamic(this, &ABaseMonster::OnMoveSpeedChangedHandle);
		MonsterRangeComp->OnPlayerCountOne.AddDynamic(this, &ABaseMonster::OnPlayerCountOneHandle);
		MonsterRangeComp->OnPlayerCountZero.AddDynamic(this, &ABaseMonster::OnPlayerCountZeroHandle);
		MonsterRangeComp->OnPlayerOut.AddDynamic(this, &ABaseMonster::OnTargetLostHandle);
		
	}
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();
	
	if (HasAuthority())
	{
		StartLocation = GetActorLocation();
		StartRotator = GetActorRotation();
	}
	else
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
	MonsterId = MonsterAssetId;
	MonsterLevel = Level;
	InitMonsterDataLoading(MonsterAssetId, Level);
}

void ABaseMonster::InitMonsterDataLoading(FPrimaryAssetId MonsterAssetId, float Level)
{
	UAssetManager::Get().LoadPrimaryAsset(MonsterAssetId,
		TArray<FName>(),
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
		UE_LOG(LogTemp, Error, TEXT("ABaseMonster::InitMonsterData - MonsterData is Not Valid!"));
	}

	InitVisuals();
	InitCollision();
	if (HasAuthority())
	{
		ASC->AddLooseGameplayTag(MonsterData->AttackType);
		InitAttributes(Level);
		InitGiveAbilities();
		InitStateTree();
	}
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
	//행동
	for (auto& AbilityPtr : MonsterData->DefaultAbilities)
	{
		if (IsValid(AbilityPtr) && ASC)
		{
			ASC->GiveAbility(FGameplayAbilitySpec(AbilityPtr.Get(), 1, 0));
		}
	}
	//스킬
	for (TObjectPtr<USkillDataAsset> SkillDataAsset : MonsterData->SkillDataAssets)
	{
		if (IsValid(SkillDataAsset) && ASC)
		{
			FGameplayAbilitySpec Spec = SkillDataAsset->MakeSpec();
			ASC->GiveAbility(Spec);
		}
	}
}

void ABaseMonster::InitAttributes(float Level)
{
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
	
	FBaseMonsterTableRow* MonsterRow = 
		MonsterData->MonsterDataTable
		->FindRow<FBaseMonsterTableRow>(MonsterData->TableRowName, TEXT("MonsterData"));

	if (MonsterRow)
	{
		
		AttributeSet->SetLevel(1.f);
		AttributeSet->SetMaxLevel(MonsterRow->BaseMaxLevel);
		AttributeSet->SetXP(0.f);
		FRealCurve* MaxHealth = MonsterData->MonsterCurveTable->FindCurve(FName("MaxHealth"), TEXT("MonsterCurve"));
		if (MaxHealth)
		{
			AttributeSet->SetMaxHealth(MonsterRow->BaseMaxHealth + MaxHealth->Eval(Level));
			AttributeSet->SetHealth(MonsterRow->BaseMaxHealth + MaxHealth->Eval(Level));
		}
		AttributeSet->SetHealthRegen(MonsterRow->BaseHealthRegen);
		AttributeSet->SetStamina(MonsterRow->BaseMaxStamina);
		AttributeSet->SetMaxStamina(MonsterRow->BaseMaxStamina);
		AttributeSet->SetStaminaRegen(MonsterRow->BaseStaminaRegen);
		FRealCurve* AttackPower = MonsterData->MonsterCurveTable->FindCurve(FName("AttackPower"), TEXT("MonsterCurve"));
		if (AttackPower)
		{
			AttributeSet->SetAttackPower(MonsterRow->BaseAttackPower + AttackPower->Eval(Level));
		}
		FRealCurve* SkillAmp = MonsterData->MonsterCurveTable->FindCurve(FName("SkillAmp"), TEXT("MonsterCurve"));
		AttributeSet->SetAttackSpeed(MonsterRow->BaseAttackSpeed);
		if (SkillAmp)
		{
			AttributeSet->SetSkillAmp(MonsterRow->BaseSkillAmp + SkillAmp->Eval(Level));
		}
		AttributeSet->SetAttackRange(MonsterRow->BaseAttackRange);
		AttributeSet->SetCriticalChance(MonsterRow->BaseCriticalChance);
		AttributeSet->SetCriticalDamage(MonsterRow->BaseCriticalDamage);
		FRealCurve* Defense = MonsterData->MonsterCurveTable->FindCurve(FName("Defense"), TEXT("MonsterCurve"));
		if (Defense)
		{
			AttributeSet->SetDefense(MonsterRow->BaseDefense + Defense->Eval(Level));
		}
		AttributeSet->SetMoveSpeed(MonsterRow->BaseMoveSpeed);
		AttributeSet->SetCooldownReduction(MonsterRow->BaseCooldownReduction);
		AttributeSet->SetTenacity(MonsterRow->BaseTenacity);
	}
}

void ABaseMonster::InitVisuals()
{
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

void ABaseMonster::InitCollision()
{
	GetMesh()->SetRelativeScale3D(MonsterData->MeshScale);
	GetCapsuleComponent()->SetCapsuleSize(MonsterData->CollisionRadius, MonsterData->CapsuleHalfHeight);
	GetCapsuleComponent()->SetCollisionProfileName("MonsterObjectCollision");
	HitBoxComp->SetBoxExtent(MonsterData->HitBoxExtent);
	HitBoxComp->SetCollisionProfileName("MonsterTraceCollision");
}

void ABaseMonster::InitStateTree()
{
	StateTreeComp->SetStateTree(MonsterData->MonsterStateTree);
	StateTreeComp->StartLogic();
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

void ABaseMonster::OnRep_MonsterData()
{
	InitMonsterDataLoading(MonsterId, MonsterLevel);
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

void ABaseMonster::Multicast_SetCollisionProfileName_Implementation(FName ProfileName)
{
	GetCapsuleComponent()->SetCollisionProfileName(ProfileName);
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

	if (bIsPhaseTrigger == false && AttributeSet->GetHPPersent() <= 0.5f)
	{
		bIsPhaseTrigger = true;
		SendStateTreeEvent(MonsterTags.Phase2EventTag);
	}

	ABaseCharacter* BC = Cast<ABaseCharacter>(Target);
	if (!BC->OnDeath.IsAlreadyBound(this, &ABaseMonster::OnTargetLostHandle))
	{
		BC->OnDeath.AddDynamic(this, &ABaseMonster::OnTargetLostHandle);
	}
	
	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not StateTree"));
		return;
	}
	if (MonsterTags.HitEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not HitEventTag"));
		return;
	}

	StateTreeComp->SendStateTreeEvent(FStateTreeEvent(MonsterTags.HitEventTag));
}

void ABaseMonster::OnMonterDeathHandle(AActor* Target)
{
	if (bIsDead)
	{
		UE_LOG(LogTemp, Warning, TEXT("Monster is already dead"));
		return;
	}

	ABaseCharacter* BC = Cast<ABaseCharacter>(Target);
	if (BC->OnDeath.IsAlreadyBound(this, &ABaseMonster::OnTargetLostHandle))
	{
		BC->OnDeath.RemoveDynamic(this, &ABaseMonster::OnTargetLostHandle);
	}

	Death(); 
	//아이템 박스 초기화;
	LootableComp->InitializeWithItems(MonsterData->ItemList);
	//보상 지급
	GameplayEffectSetByCaller(Target, XPRewardEffect, MonsterTags.IncomingXPTag, MonsterData->Exp);
}

void ABaseMonster::GameplayEffectSetByCaller(AActor* Player, TSubclassOf<UGameplayEffect> GE, FGameplayTag Tag, float Amount)
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
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(GE, 1, ContextHandle);
	UAbilitySystemComponent* TargetASC =
		UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Player);

	SpecHandle.Data->SetSetByCallerMagnitude(
		Tag,
		Amount
	);

	TargetASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data);
}

void ABaseMonster::TryActivateByDynamicTag(FGameplayTag InputTag)
{
	for (FGameplayAbilitySpec& Spec : ASC->GetActivatableAbilities())
	{
		if (Spec.DynamicAbilityTags.HasTagExact(InputTag))
		{
			ASC->TryActivateAbility(Spec.Handle);
			break;
		}
	}
}

void ABaseMonster::OnCooldown(FGameplayTag CooldownTag, float Cooldown)
{
	AddCooldownTag(CooldownTag);
	FTimerHandle& TimerHandle = CooldownTimerMap.FindOrAdd(CooldownTag);

	GetWorld()->GetTimerManager().SetTimer(
		TimerHandle,
		FTimerDelegate::CreateLambda([this, CooldownTag]()
			{
				RemoveCooldownTag(CooldownTag);
			}),
		Cooldown,
		false
	);
}

void ABaseMonster::AddCooldownTag(FGameplayTag CooldownTag)
{
	ASC->AddLooseGameplayTag(CooldownTag);
}

void ABaseMonster::RemoveCooldownTag(FGameplayTag CooldownTag)
{
	ASC->RemoveLooseGameplayTag(CooldownTag);
}

void ABaseMonster::OnTargetLostHandle()
{
	ABaseCharacter* BC = Cast<ABaseCharacter>(TargetPlayer);
	if (BC->OnDeath.IsAlreadyBound(this, &ABaseMonster::OnTargetLostHandle))
	{
		BC->OnDeath.RemoveDynamic(this, &ABaseMonster::OnTargetLostHandle);
	}
	StateTreeComp->SendStateTreeEvent(FGameplayTag(MonsterTags.TargetOffEventTag));
	TargetPlayer = nullptr;
}

void ABaseMonster::OnPlayerCountOneHandle()
{ 
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountOneHandle : Not ASC"));
		return;
	}
	if (StateTreeComp->IsRunning() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountOneHandle : StateTree Loading..."));
		return;
	}
	FGameplayEventData* Payload = new FGameplayEventData();
	ASC->HandleGameplayEvent(MonsterTags.BeginSearchEventTag, Payload);
	StateTreeComp->SendStateTreeEvent(MonsterTags.BeginSearchEventTag);
}  

void ABaseMonster::OnPlayerCountZeroHandle()
{
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountZeroHandle : Not ASC"));
		return;
	}

	if (StateTreeComp->IsRunning() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountOneHandle : StateTree Loading..."));
		return;
	}
	if (bIsCombat == false)
	{
		FGameplayEventData* Payload = new FGameplayEventData();
		ASC->HandleGameplayEvent(FGameplayTag(MonsterTags.EndSearchEventTag), Payload);
		StateTreeComp->SendStateTreeEvent(FStateTreeEvent(MonsterTags.EndSearchEventTag));
	}
}

void ABaseMonster::SendAttackRangeEvent(float AttackRange)
{
	if (IsValid(TargetPlayer) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::SendAttackRangeEvent : Not Player"));
		return;
	}
	
	const float Distance = FVector::DistSquared(
			TargetPlayer->GetActorLocation(), GetActorLocation());

	if (Distance <= AttackRange * AttackRange)
	{
		// 공격
		StateTreeComp->SendStateTreeEvent(FGameplayTag(MonsterTags.AttackEventTag));
	}
	else
	{
		// 다시 체이스
		StateTreeComp->SendStateTreeEvent(FGameplayTag(MonsterTags.TargetOnEventTag));
	}
}

bool ABaseMonster::HasASCTag(FGameplayTag Tag)
{
	return ASC && ASC->HasMatchingGameplayTag(Tag);
}

void ABaseMonster::SendStateTreeEvent(FGameplayTag InputTag)
{
	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::SendStateTreeEvent : Not StateTreeComp"));
		return;
	}
	if (StateTreeComp->IsRunning() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::SendStateTreeEvent : Not StateTreeComp Running"));
		return;
	}
	StateTreeComp->SendStateTreeEvent(InputTag);
}

UStateTreeComponent* ABaseMonster::GetStateTreeComponent()
{
	return StateTreeComp;
}

void ABaseMonster::SetTargetPlayer(AActor* Target)
{
	//if (TargetPlayer == nullptr)
	//{
	//	TargetPlayer = Target;
	//}
	//else
	//{
	//	const float OldTargetDistance = FVector::DistSquared(
	//		TargetPlayer->GetActorLocation(), GetActorLocation());

	//	const float NewTargetDistance = FVector::DistSquared(
	//		Target->GetActorLocation(), GetActorLocation());

	//	if (OldTargetDistance > NewTargetDistance)
	//	{
	//		TargetPlayer = Target;
	//	}
	//}
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
	if (bIsDead)
	{
		return false;
	}

	// if (IsValid(ASC))
	// {
	// 	if (ASC->HasMatchingGameplayTag(MonsterTags.DeathStateTag))
	// 	{
	// 		return false;
	// 	}
	// }

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

void ABaseMonster::Death()
{
	if (!HasAuthority())
		return;

	SetbIsDead(true);
	SetTargetPlayer(nullptr);
	SetbIsCombat(false);

	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
	InGameMode->NotifyNeutralDied(this);

	SetLifeSpan(20.f);

	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterDeathHandle : Not StateTree"));
		return;
	}
	if (MonsterTags.DeathEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterDeathHandle : Not DeathEventTag"));
		return;
	}
	StateTreeComp->SendStateTreeEvent(FStateTreeEvent(MonsterTags.DeathEventTag));
}
