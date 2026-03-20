#include "ER_PlayerState.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"
#include "CharacterSystem/Data/CharacterData.h"

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
	DOREPLIFETIME(AER_PlayerState, StartPoint);
	DOREPLIFETIME(AER_PlayerState, SelectedCharacterData);

}

void AER_PlayerState::CopyProperties(APlayerState* PlayerState)
{
	Super::CopyProperties(PlayerState);

	AER_PlayerState* PS = Cast<AER_PlayerState>(PlayerState);
	if (PS)
	{
		PS->PlayerStateName = PlayerStateName;
		PS->TeamType = TeamType;
		PS->StartPoint = StartPoint;
		// Seamless Travel 시 캐릭터 선택 데이터 보존
		PS->SelectedCharacterData = SelectedCharacterData;
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

void AER_PlayerState::Server_SetStartPoint_Implementation(int32 idx)
{
	SetStartPoint(idx);
}

void AER_PlayerState::SetSelectedCharacterData(TSoftObjectPtr<UCharacterData> InData)
{
	// 서버에서 직접 세팅되거나 권한이 있을 때 데이터 할당
	SelectedCharacterData = InData;
	
	// 서버(호스트) 본인의 UI를 즉시 갱신하기 위해 명시적 호출
	if (HasAuthority())
	{
		OnCharacterDataChanged.Broadcast(SelectedCharacterData);
	}
}

void AER_PlayerState::OnRep_SelectedCharacterData()
{
	// 서버로부터 데이터 복제가 올 때 자동 호출됨
	// 로컬 UI에게 "내 데이터가 바뀌었으니 화면 갱신해라"라고 알림
	OnCharacterDataChanged.Broadcast(SelectedCharacterData);
}

void AER_PlayerState::SetReadyState(bool bNewReadyState)
{
	bIsReady = bNewReadyState;
	
	// 서버 로컬 UI 갱신용
	if (HasAuthority())
	{
		OnReadyStateChanged.Broadcast(bIsReady);
	}
}

void AER_PlayerState::OnRep_bIsReady()
{
	// 클라이언트 UI 갱신용
	OnReadyStateChanged.Broadcast(bIsReady);
}
