#include "Monster/BaseMonster.h"

#include "AbilitySystemComponent.h"
#include "AttributeSet/BaseMonsterAttributeSet.h"
#include "Components/StateTreeComponent.h"

ABaseMonster::ABaseMonster()
{
	PrimaryActorTick.bCanEverTick = false;

	ASC = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	ASC->SetIsReplicated(true);
	ASC->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UBaseMonsterAttributeSet>(TEXT("AttributeSet"));
	StateTreeComp = CreateDefaultSubobject<UStateTreeComponent>(TEXT("StateTree"));
}

UAbilitySystemComponent* ABaseMonster::GetAbilitySystemComponent() const
{
	return ASC;
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

void ABaseMonster::SetPlayerCount(int32 Amount)
{
	PlayerCount += Amount;
}

int32 ABaseMonster::GetPlayerCount()
{
	return PlayerCount;
}

UStateTreeComponent* ABaseMonster::GetStateTreeComponent()
{
	return StateTreeComp;
}

