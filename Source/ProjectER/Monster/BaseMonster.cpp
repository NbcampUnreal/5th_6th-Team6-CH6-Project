#include "Monster/BaseMonster.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet/BaseMonsterAttributeSet.h"

ABaseMonster::ABaseMonster()
{
	PrimaryActorTick.bCanEverTick = false;

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UBaseMonsterAttributeSet>(TEXT("AttributeSet"));

}

void ABaseMonster::BeginPlay()
{
	Super::BeginPlay();
	
	InitAbilitySystem();
}

void ABaseMonster::PossessedBy(AController* newController)
{
	UE_LOG(LogTemp, Warning, TEXT("AI Controller Possessed"));
}

UAbilitySystemComponent* ABaseMonster::GetAbilitySystemComponent() const
{
	return ASC;
}

void ABaseMonster::InitAbilitySystem()
{
	if (!ASC)
	{
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