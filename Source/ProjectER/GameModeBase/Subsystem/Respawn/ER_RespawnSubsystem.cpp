// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_RespawnSubsystem.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/ER_OutGamePlayerController.h"
#include "CharacterSystem/Player/BasePlayerController.h"


void UER_RespawnSubsystem::HandlePlayerDeath(AER_PlayerState& PS, AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return;

	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start HandlePlayerDeath"));


	if (PS.bIsDead)
		return;

	PS.bIsDead = true;
	PS.ForceNetUpdate();
	PS.FlushNetDormancy();
	UE_LOG(LogTemp, Warning, TEXT("PS.bIsDead = %s"), PS.bIsDead ? TEXT("True") : TEXT("False"));
}

bool UER_RespawnSubsystem::EvaluateTeamElimination(AER_PlayerState& PS, AER_GameState& GS)
{
	if (!GS.HasAuthority())
		return false;

	// 전멸 방지 페이즈인지 확인


	UE_LOG(LogTemp, Warning, TEXT("[RSS] : Start EvaluateTeamElimination"));

	int32 TeamIdx = static_cast<int32>(PS.Team);

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

	const int32 PlayerId = PS.GetPlayerId();

	// 리스폰 시간 계산 -> 추후에 페이즈, 레벨에 따라서 리스폰 시간 계산
	float RespawnTime = 5.f;
	PS.RespawnTime = GS.GetServerWorldTimeSeconds() + RespawnTime;
	PS.ForceNetUpdate();

	TWeakObjectPtr<AER_PlayerState> WeakPS(&PS);

	// 리스폰 타이머 시작
	FTimerHandle& Handle = RespawnMap.FindOrAdd(PlayerId);

	GetWorld()->GetTimerManager().ClearTimer(Handle);

	GetWorld()->GetTimerManager().SetTimer(
		Handle,
		FTimerDelegate::CreateWeakLambda(this, [this, WeakPS]()
			{
				if (!WeakPS.IsValid())
					return;

				// 여기서 바로 리스폰 처리 로직을 작성
				AER_PlayerState* PS_ = WeakPS.Get();

				PS_->bIsDead = false;
				PS_->ForceNetUpdate();

				UE_LOG(LogTemp, Log, TEXT("[RSS] : Player(%s) Respawned"), *PS_->GetPlayerName());
			}),
		RespawnTime,
		false
	);

	// 리스폰 UI 출력
	if (ABasePlayerController* PC = Cast<ABasePlayerController>(PS.GetOwner()))
	{
		PC->Client_StartRespawnTimer();
	}
	
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




