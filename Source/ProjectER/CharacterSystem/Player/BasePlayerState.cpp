#include "BasePlayerState.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

ABasePlayerState::ABasePlayerState()
{
	// ASC 생성 및 설정
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true); // ASC 상태 복제
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed); // ReplicationMode 설정
	
	// Attribute Set 생성
	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>("AttributeSet"); // Attribute Set 생성
	
	SetNetUpdateFrequency(100.0f); // 네트워크(갱신 주기) 최적화
}

UAbilitySystemComponent* ABasePlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}