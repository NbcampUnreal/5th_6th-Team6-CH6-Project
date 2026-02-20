// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_RespawnSubsystem.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/ER_OutGamePlayerController.h"
#include "GameModeBase/Subsystem/Phase/ER_PhaseSubsystem.h"
#include "CharacterSystem/Player/BasePlayerController.h"
#include "CharacterSystem/Character/BaseCharacter.h"


void UER_RespawnSubsystem::HandlePlayerDeath(AER_PlayerState& PS, AER_GameState& GS, AER_PlayerState* KillerPS, const TArray<APlayerState*>& Assists)
{
	if (!GS.HasAuthority())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start HandlePlayerDeath"));


	if (PS.bIsDead)
		return;

	PS.bIsDead = true;
	PS.AddDeathCount();
	PS.ForceNetUpdate();
	UE_LOG(LogTemp, Warning, TEXT("Death.PS  K : %d, D : %d, A : %d"), PS.GetKillCount(), PS.GetDeathCount(), PS.GetAssistCount());
	//PS.FlushNetDormancy();
	if (KillerPS != nullptr)
	{
		KillerPS->AddKillCount();
		KillerPS->ForceNetUpdate();
		UE_LOG(LogTemp, Warning, TEXT("Kill.PS K : %d, D : %d, A : %d"), KillerPS->GetKillCount(), KillerPS->GetDeathCount(), KillerPS->GetAssistCount());
	}

	

	for (auto& AssistPS : Assists)
	{
		AER_PlayerState* AssistERPS = Cast<AER_PlayerState>(AssistPS);
		if (AssistERPS && AssistERPS != KillerPS)
		{
			AssistERPS->AddAssistCount();
			AssistERPS->ForceNetUpdate();
			UE_LOG(LogTemp, Warning, TEXT("Kill.PS K : %d, D : %d, A : %d"), AssistERPS->GetKillCount(), AssistERPS->GetDeathCount(), AssistERPS->GetAssistCount());
		}
	}

	UE_LOG(LogTemp, Warning, TEXT("PS.bIsDead = %s"), PS.bIsDead ? TEXT("True") : TEXT("False"));
}

bool UER_RespawnSubsystem::EvaluateTeamElimination(AER_PlayerState& PS, AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return false;

	// 전멸 방지 페이즈인지 확인


	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start EvaluateTeamElimination"));

	int32 TeamIdx = static_cast<int32>(PS.TeamType);

	// 전멸 판정이면 팀 index 리턴, 아니면 -1리턴
	return GS.GetTeamEliminate(TeamIdx);

}

void UER_RespawnSubsystem::SetTeamLose(AER_GameState& GS, int32 TeamIdx)
{
	for (auto& player : GS.GetTeamArray(TeamIdx))
	{
		ABasePlayerController* PC = Cast<ABasePlayerController>(player->GetOwner());

		PC->Client_SetLose();
	}
}

void UER_RespawnSubsystem::SetTeamWin(AER_GameState& GS, int32 TeamIdx)
{
	for (auto& player : GS.GetTeamArray(TeamIdx))
	{
		ABasePlayerController* PC = Cast<ABasePlayerController>(player->GetOwner());

		PC->Client_SetWin();
	}
}

void UER_RespawnSubsystem::StartRespawnTimer(AER_PlayerState& PS, AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return;

	if (!PS.bIsDead)
		return;

	if (GS.GetCurrentPhase() == 5)
		return;


	const int32 PlayerId = PS.GetPlayerId();

	// 리스폰 시간 계산 -> 추후에 페이즈, 레벨에 따라서 리스폰 시간 계산
	// 이터널 리턴 -> 1~6레벨 3~8초, 7레벨 10초, 8~11레벨 25~30초, 12레벨 35초, 13레벨 이상 40초
	// 롤 ->  1~6레벨 레벨 * 2 + 4, 7레벨 16 + 5 , 8~18 레벨 * 2.5 + 7.5
	float RespawnTime = 5.f;
	switch (GS.GetCurrentPhase())
	{
		case 1:
			RespawnTime = 5.f;
			break;
		case 2:
			RespawnTime = 10.f;
			break;
		case 3:
			RespawnTime = 15.f;
			break;
		case 4:
			RespawnTime = 20.f;
			break;
		case 5:
			RespawnTime = 25.f;
			break;
		default: 
			RespawnTime = 999.f;
			break;
	}

	PS.RespawnTime = GS.GetServerWorldTimeSeconds() + RespawnTime;
	PS.ForceNetUpdate();

	TWeakObjectPtr<AER_PlayerState> WeakPS(&PS);
	// 리스폰 UI 출력
	if (ABasePlayerController* PC = Cast<ABasePlayerController>(PS.GetOwner()))
	{
		PC->Client_StartRespawnTimer();
		PC->UI_RespawnStart(RespawnTime);
	}

	// 리스폰 타이머 시작
	FTimerHandle& Handle = RespawnMap.FindOrAdd(PlayerId);

	GetWorld()->GetTimerManager().ClearTimer(Handle);

	GetWorld()->GetTimerManager().SetTimer(
		Handle,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakPS]()
			{
				if (!WeakPS.IsValid())
					return;

				AER_PlayerState* PS_ = WeakPS.Get();
				if (AController* C = PS_->GetOwner<AController>())
				{
					if (APawn* Pawn = C->GetPawn())
					{
						if (ABaseCharacter* Char = Cast<ABaseCharacter>(Pawn))
						{
							Char->Server_Revive(Char->GetActorLocation());
						}
					}
				}

				PS_->bIsDead = false;
				PS_->ForceNetUpdate();

				UE_LOG(LogTemp, Log, TEXT("[RSS] : Player(%s) Respawned"), *PS_->GetPlayerName());
			}),
		RespawnTime,
		false
	);
}

void UER_RespawnSubsystem::StopResapwnTimer(AER_GameState& GS, int32 TeamIdx)
{
	if (!GS.HasAuthority())
		return;

	// 사출 당한 팀의 배열을 순회
	for (auto& player : GS.GetTeamArray(TeamIdx))
	{
		// 플레이어의 id를 받아와 리스폰 map에 접근 후 타이머 정지
		const int32 PlayerId = player->GetPlayerId();
		FTimerHandle& Handle = RespawnMap.FindOrAdd(PlayerId);
		GetWorld()->GetTimerManager().ClearTimer(Handle);

		ABasePlayerController* PC = Cast<ABasePlayerController>(player->GetOwner());
		// 사망한 팀원의 리스폰 UI 제거
		PC->Client_StopRespawnTimer();
	}
}

void UER_RespawnSubsystem::RespawnPlayer()
{
	// 추후 캐릭터 부활처리 추가
}

int32 UER_RespawnSubsystem::CheckIsLastTeam(AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return -1;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start CheckIsLastTeam"));

	// -1 이면 실패 혹은 마지막 팀이 아님
	return GS.GetLastTeamIdx();

}

void UER_RespawnSubsystem::InitializeRespawnMap(AER_GameState& GS)
{
	// 리스폰 map 초기화
	RespawnMap.Reset();

	for (APlayerState* player : GS.PlayerArray)
	{
		if (!player)
			continue;

		RespawnMap.FindOrAdd(player->GetPlayerId());
	}
}




