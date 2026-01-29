#include "Monster/BaseMonster.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet/BaseMonsterAttributeSet.h"
#include "Components/StateTreeComponent.h"
#include "Monster/MonsterRangeComponent.h"
#include "Net/UnrealNetwork.h"
#include "Components/WidgetComponent.h"

ABaseMonster::ABaseMonster()
{
	bReplicates = true;
	PrimaryActorTick.bCanEverTick = false;

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UBaseMonsterAttributeSet>(TEXT("AttributeSet"));
	StateTreeComp = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
	MonsterRangeComp = CreateDefaultSubobject<UMonsterRangeComponent>(TEXT("MonsterRangeComponent"));	
}

UAbilitySystemComponent* ABaseMonster::GetAbilitySystemComponent() const
{
	return ASC;
}

void ABaseMonster::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	//DOREPLIFETIME(ABaseMonster, );
}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();
	
	UE_LOG(LogTemp, Warning, TEXT("%s : BeginPlay"), *GetName());



}

void ABaseMonster::PossessedBy(AController* newController)
{
	Super::PossessedBy(newController);

	UE_LOG(LogTemp, Warning, TEXT("%s : PossessedBy"), *GetName());

	InitAbilitySystem();

	ASC->GetGameplayAttributeValueChangeDelegate(AttributeSet->GetHealthAttribute())
		.AddUObject(this, &ABaseMonster::OnHealthChangedCallback);
}

void ABaseMonster::InitAbilitySystem()
{
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("%s : Not ASC"), *GetName());
		return;
	}

	ASC->InitAbilityActorInfo(this, this);
	InitGiveAbilities();
}

void ABaseMonster::InitGiveAbilities()
{
	//if (!HasAuthority()) { return; }
	
	for (TSubclassOf<UGameplayAbility> Ability : DefaultAbilities)
	{
		ASC->GiveAbility(
			FGameplayAbilitySpec(Ability, 1, INDEX_NONE, this)
		);
	}
}

void ABaseMonster::OnHealthChangedCallback(const FOnAttributeChangeData& Data) const
{
	OnHealthChanged.Broadcast(Data.NewValue, AttributeSet->GetMaxHealth());
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

