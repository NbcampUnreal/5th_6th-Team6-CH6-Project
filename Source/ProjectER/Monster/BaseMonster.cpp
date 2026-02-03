#include "Monster/BaseMonster.h"

//
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AttributeSet/BaseMonsterAttributeSet.h"
#include "Components/StateTreeComponent.h"

#include "Components/CapsuleComponent.h"

#include "Monster/MonsterRangeComponent.h"
#include "Components/WidgetComponent.h"


#include "GameModeBase/GameMode/ER_InGameMode.h"

ABaseMonster::ABaseMonster()
	:TargetPlayer(nullptr),
	StartLocation(FVector::ZeroVector),
	bIsCombat(false),
	bIsDead(false),
	bIsReturn(false)
{
	//액터 복제
	SetReplicates(true);
	//위치 / 회전 / 속도 복제
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

	// AttributeSet는 ASC가 복제될 때 같이 복제
	AttributeSet = CreateDefaultSubobject<UBaseMonsterAttributeSet>(TEXT("AttributeSet"));
	
	// StateTree은 각 클라에서 돌아가면서 동기화되는 데이터를 가지고 상태를 변경하는듯
	StateTreeComp = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	StateTreeComp->SetAutoActivate(false);

	// 주변 플레이어 감지용 컴포넌트
	MonsterRangeComp = CreateDefaultSubobject<UMonsterRangeComponent>(TEXT("MonsterRangeComponent"));	

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
	if (IsValid(StateTreeComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::PossessedBy : Not StateTreeComp"), *GetName());
		return;
	}

	ASC->InitAbilityActorInfo(this, this);
	// UI 변경
	AttributeSet->OnHealthChanged.AddDynamic(this, &ABaseMonster::OnHealthChangedHandle);

	if (HasAuthority())
	{
		InitGiveAbilities();

		ASC->AddLooseGameplayTag(AliveStateTag);

		AttributeSet->OnMonsterHit.AddDynamic(this, &ABaseMonster::OnMonterHitHandle);
		AttributeSet->OnMonsterDeath.AddDynamic(this, &ABaseMonster::OnMonterDeathHandle);
		MonsterRangeComp->OnPlayerCountOne.AddDynamic(this, &ABaseMonster::OnPlayerCountOneHandle);
		MonsterRangeComp->OnPlayerCountZero.AddDynamic(this, &ABaseMonster::OnPlayerCountZeroHandle);
	}
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("%s : BeginPlay"), *GetName());
	
	if (HasAuthority())
	{
		StartLocation = GetActorLocation();
		//StateTreeComp->StartLogic();
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
	UE_LOG(LogTemp, Warning, TEXT("%s : Target On ( %s )"), *GetName(), *Target->GetName());
	
	// hit 이벤트
	UStateTreeComponent* STComp = GetStateTreeComponent();
	if (IsValid(STComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not StateTree"));
		return;
	}
	if (HitEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterHitHandle : Not HitEventTag"));
		return;
	}

	STComp->SendStateTreeEvent(FStateTreeEvent(HitEventTag));
}

void ABaseMonster::OnMonterDeathHandle(AActor* Target)
{
	SetbIsDead(true);
	SetTargetPlayer(nullptr);
	SetbIsCombat(false);
	UE_LOG(LogTemp, Warning, TEXT("%s : Die"), *GetName());

	if (DeathStateTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterDeathHandle : Not DeathStateTag"));
		return;
	}

	// Deaht 이벤트
	UStateTreeComponent* STComp = GetStateTreeComponent();
	if (IsValid(STComp) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterDeathHandle : Not StateTree"));
		return;
	}
	if (DeathEventTag.IsValid() == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnMonterDeathHandle : Not DeathEventTag"));
		return;
	}
	STComp->SendStateTreeEvent(FStateTreeEvent(DeathEventTag));

	// [전민성] - 사망 시 gamemode에 알림 추가
	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
	InGameMode->NotifyNeutralDied(this);
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
}

void ABaseMonster::OnPlayerCountZeroHandle()
{
	if (IsValid(ASC) == false)
	{
		UE_LOG(LogTemp, Warning, TEXT("ABaseMonster::OnPlayerCountZeroHandle : Not ASC"));
		return;
	}
	UE_LOG(LogTemp, Warning, TEXT("%s : Sit"), *GetName());
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



/// [전민성 추가분]
void ABaseMonster::Death()
{
	if (!HasAuthority())
		return;

	auto InGameMode = Cast<AER_InGameMode>(GetWorld()->GetAuthGameMode());
	InGameMode->NotifyNeutralDied(this);

	SetLifeSpan(0.1f);
}
