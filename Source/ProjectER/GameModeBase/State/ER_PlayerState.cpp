#include "ER_PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

AER_PlayerState::AER_PlayerState()
{
	bReplicates = true;

	// ASC 생성 및 설정
	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>("AbilitySystemComponent");
	AbilitySystemComponent->SetIsReplicated(true); // ASC 상태 복제
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed); // ReplicationMode 설정

	// Attribute Set 생성
	AttributeSet = CreateDefaultSubobject<UBaseAttributeSet>("AttributeSet"); // Attribute Set 생성

	SetNetUpdateFrequency(100.0f); // 네트워크(갱신 주기) 최적화
}

void AER_PlayerState::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);

	DOREPLIFETIME(AER_PlayerState, PlayerStateName);
	DOREPLIFETIME(AER_PlayerState, bIsReady);
	DOREPLIFETIME(AER_PlayerState, bIsLose);
	DOREPLIFETIME(AER_PlayerState, bIsWin);
	DOREPLIFETIME(AER_PlayerState, bIsDead);
	DOREPLIFETIME(AER_PlayerState, TeamType);
	DOREPLIFETIME(AER_PlayerState, RespawnTime);
	DOREPLIFETIME(AER_PlayerState, KillCount);
	DOREPLIFETIME(AER_PlayerState, DeathCount);
	DOREPLIFETIME(AER_PlayerState, AssistCount);

}

void AER_PlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	AER_PlayerState* PS = Cast<AER_PlayerState>(PlayerState);
	if (PS)
	{
		PS->PlayerStateName = PlayerStateName;
		PS->TeamType = TeamType;

	}
}

UAbilitySystemComponent* AER_PlayerState::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AER_PlayerState::AddDamageContributor(APlayerState* AttackerPS, float Damage, float Now)
{
	if (!AttackerPS || Damage <= 0.f)
        return;

    FDamageContrib& E = DamageContribMap.FindOrAdd(AttackerPS);
    E.AttackerPS = AttackerPS;
    E.LastHitTime = Now;
    E.TotalDamage += Damage;
}

void AER_PlayerState::GetAssists(float Now, float WindowSec, APlayerState* KillerPS, TArray<APlayerState*>& OutAssists) const
{
	OutAssists.Reset();

	for (const auto& Pair : DamageContribMap)
	{
		const FDamageContrib& E = Pair.Value;
		APlayerState* PS = E.AttackerPS.Get();
		if (!PS) continue;

		if (PS == KillerPS) continue;

		if (Now - E.LastHitTime <= WindowSec)
		{
			// 옵션: 최소 누적 데미지 조건
			if (E.TotalDamage > 0.f)
				OutAssists.Add(PS);
		}
	}
}

void AER_PlayerState::ResetDamageContrib()
{
	DamageContribMap.Reset();
}

