#include "Monster/BaseMonster.h"

//
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet/BaseMonsterAttributeSet.h"
#include "Components/StateTreeComponent.h"

#include "Components/CapsuleComponent.h"

#include "Monster/MonsterRangeComponent.h"
#include "Components/WidgetComponent.h"

ABaseMonster::ABaseMonster()
	:TargetPlayer(nullptr),
	StartLocation(FVector::ZeroVector),
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

	//이동속도 조절필요
	GetMovementComponent();
}

UAbilitySystemComponent* ABaseMonster::GetAbilitySystemComponent() const
{
	return ASC;
}

void ABaseMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(ABaseMonster, TargetPlayer);
	DOREPLIFETIME(ABaseMonster, StartLocation);
	DOREPLIFETIME(ABaseMonster, bIsCombat);
	DOREPLIFETIME(ABaseMonster, bIsDead);
}

void ABaseMonster::PossessedBy(AController* newController)
{
	Super::PossessedBy(newController);

	UE_LOG(LogTemp, Warning, TEXT("%s : PossessedBy"), *GetName());

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
		InitGiveAbilities();

		//ASC->AddLooseGameplayTag(AliveStateTag);

		AttributeSet->OnMonsterHit.AddDynamic(this, &ABaseMonster::OnMonterHitHandle);
		AttributeSet->OnMonsterDeath.AddDynamic(this, &ABaseMonster::OnMonterDeathHandle);
		MonsterRangeComp->OnPlayerCountOne.AddDynamic(this, &ABaseMonster::OnPlayerCountOneHandle);
		MonsterRangeComp->OnPlayerCountZero.AddDynamic(this, &ABaseMonster::OnPlayerCountZeroHandle);
	}
	else if (!HasAuthority())
	{
		// UI 변경
		AttributeSet->OnHealthChanged.AddDynamic(this, &ABaseMonster::OnHealthChangedHandle);
	}
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("%s : BeginPlay"), *GetName());

	if (HasAuthority())
	{
		StartLocation = GetActorLocation();
		StateTreeComp->StartLogic();
	}
}

void ABaseMonster::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void ABaseMonster::InitGiveAbilities()
{
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitGiveAbilities - ASC is Not Valid!"));
		return;
	}

	if (DefaultAbilities.IsEmpty())
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::InitGiveAbilities - DefaultAbilities is Empty!"));
		return;
	}

	for (TSubclassOf<UGameplayAbility> Ability : DefaultAbilities)
	{
		ASC->GiveAbility(
			FGameplayAbilitySpec(Ability, 1, INDEX_NONE, this)
		);
	}
}

void ABaseMonster::OnHealthChangedHandle(float OldValue, float NewValue)
{
	// UI 
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

	// Target에게 보상 지급

	//
}

void ABaseMonster::OnPlayerCountOneHandle()
{ 
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountOneHandle : Not ASC"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("%s : Awake"), *GetName());
	FGameplayEventData* Payload = new FGameplayEventData();
	ASC->HandleGameplayEvent(FGameplayTag(BeginSearchEventTag), Payload);
	StateTreeComp->SendStateTreeEvent(FStateTreeEvent(BeginSearchEventTag));
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
		UE_LOG(LogTemp, Warning, TEXT("%s : Sit"), *GetName());
		FGameplayEventData* Payload = new FGameplayEventData();
		ASC->HandleGameplayEvent(FGameplayTag(EndSearchEventTag), Payload);
		StateTreeComp->SendStateTreeEvent(FStateTreeEvent(EndSearchEventTag));
	}
}

void ABaseMonster::SendAttackRangeEvent(float AttackRange)
{
	const float Distance = 
		FVector::DistSquared(
			TargetPlayer->GetActorLocation(), 
			GetActorLocation()
		);

	if (Distance <= AttackRange * AttackRange)
	{
		// 공격가능
		UE_LOG(LogTemp, Warning, TEXT("Can Attack"));
		StateTreeComp->SendStateTreeEvent(FGameplayTag(TargetOnEventTag));
	}
	else
	{
		// 공격불가능
		UE_LOG(LogTemp, Warning, TEXT("Can't Attack"));
		StateTreeComp->SendStateTreeEvent(FGameplayTag(TargetOffEventTag));
	}
}

void ABaseMonster::SendReturnSuccessEvent()
{
	UE_LOG(LogTemp, Warning, TEXT("Return Success"));
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

