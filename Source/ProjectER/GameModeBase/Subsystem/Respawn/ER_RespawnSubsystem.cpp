// Fill out your copyright notice in the Description page of Project Settings.


#include "ER_RespawnSubsystem.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/ER_OutGamePlayerController.h"
#include "GameModeBase/Subsystem/Phase/ER_PhaseSubsystem.h"
#include "CharacterSystem/Player/BasePlayerController.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "GameModeBase/PointActor/ER_PointActor.h"


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

		// mpyi _ 킬 카운트 체크를 위해 추가
		ABasePlayerController* KillerPC = Cast<ABasePlayerController>(KillerPS->GetPlayerController());
		if (IsValid(KillerPC))
		{
			KillerPC->UI_KillCountUpdate(KillerPS->GetKillCount());
		}		

		UE_LOG(LogTemp, Warning, TEXT("Kill.PS K : %d, D : %d, A : %d"), KillerPS->GetKillCount(), KillerPS->GetDeathCount(), KillerPS->GetAssistCount());
	}

	for (auto& AssistPS : Assists)
	{
		AER_PlayerState* AssistERPS = Cast<AER_PlayerState>(AssistPS);
		if (AssistERPS && AssistERPS != KillerPS)
		{
			AssistERPS->AddAssistCount();
			AssistERPS->ForceNetUpdate();


			// mpyi _ 어시 카운트 체크를 위해 추가
			ABasePlayerController* AssistPC = Cast<ABasePlayerController>(AssistERPS->GetPlayerController());
			if (IsValid(AssistPC))
			{
				AssistPC->UI_AssistCountUpdate(AssistERPS->GetAssistCount());
			}

			UE_LOG(LogTemp, Warning, TEXT("Kill.PS K : %d, D : %d, A : %d"), AssistERPS->GetKillCount(), AssistERPS->GetDeathCount(), AssistERPS->GetAssistCount());
		}
	}

	// mpyi _ 데스 카운트 체크를 위해 추가
	ABasePlayerController* PC = Cast<ABasePlayerController>(PS.GetOwner());
	if (IsValid(PC))
	{
		PC->UI_DeathCountUpdate(PS.GetDeathCount());
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

void UER_RespawnSubsystem::InitializeRespawnPoints()
{
	UWorld* World = GetWorld();
	if (!World)
		return;

	if (World->GetNetMode() == NM_Client)
		return;

	UE_LOG(LogTemp, Log, TEXT("[RSS] InitializeRespawnPoints Start Points Count : %d"), Points.Num());

	const FName RespawnTag(TEXT("Respawn"));

	RespawnPointsByRegion.Reset();

	for (auto& Point : Points)
	{
		AActor* PointActor = Point.Get();

		if (!IsValid(PointActor))
		{
			continue;
		}

		AER_PointActor* PA = Cast<AER_PointActor>(PointActor);
		if (!PA)
		{
			continue;
		}
		RespawnPointsByRegion.FindOrAdd(PA->RegionType).Add(PointActor);
			
	}
	UE_LOG(LogTemp, Log, TEXT("[RSS] InitializeRespawnPoints End. RespawnPoints Count : %d"), RespawnPointsByRegion.Num());
}

void UER_RespawnSubsystem::ResetAssignedSpawnPoints()
{
	AssignedSpawnIndices.Reset();
	UE_LOG(LogTemp, Log, TEXT("[RSS] ResetAssignedSpawnPoints: Spawn tracking cleared."));
}

FTransform UER_RespawnSubsystem::GetRespawnPointLocation(const int32 Index)
{
	UWorld* World = GetWorld();
	if (World == nullptr || World->GetNetMode() == NM_Client)
	{
		return FTransform::Identity;
	}

	const ERegionType RegionType = static_cast<ERegionType>(Index);
	const TArray<TWeakObjectPtr<AActor>>* RespawnPoints = RespawnPointsByRegion.Find(RegionType);
	if (RespawnPoints == nullptr || RespawnPoints->IsEmpty())
	{
		return FTransform::Identity;
	}

	int32 TotalPointsCount = RespawnPoints->Num();
	TArray<int32>& AssignedIndicesForRegion = AssignedSpawnIndices.FindOrAdd(RegionType);

	// 사용 가능한 인덱스들 찾기
	TArray<int32> AvailableIndices;
	for (int32 i = 0; i < TotalPointsCount; ++i)
	{
		if (!AssignedIndicesForRegion.Contains(i))
		{
			AvailableIndices.Add(i);
		}
	}

	int32 ChosenIndex = -1;

	// 여분 포인트가 있다면 그 중 하나를 무작위 할당
	if (AvailableIndices.Num() > 0)
	{
		ChosenIndex = AvailableIndices[FMath::RandRange(0, AvailableIndices.Num() - 1)];
		AssignedIndicesForRegion.Add(ChosenIndex);
	}
	else
	{
		// 이 지역의 모든 스폰 포인트가 할당되었다면, 무작위 포인트 하나를 다시 사용 (풀백)
		ChosenIndex = FMath::RandRange(0, TotalPointsCount - 1);
		UE_LOG(LogTemp, Warning, TEXT("[RSS] All spawn points in region %d assigned. Falling back to random reuse."), static_cast<int32>(RegionType));
	}

	const TWeakObjectPtr<AActor>& RespawnPoint = (*RespawnPoints)[ChosenIndex];
	
	if (RespawnPoint.IsValid() && RespawnPoint.Get() != nullptr)
	{
		FTransform SpawnTransform = RespawnPoint.Get()->ActorToWorld();
		
		// 스폰 포인트가 재사용되는 경우 (풀백), 겹침 현상을 방지하기 위해 랜덤 오프셋 추가
		if (AvailableIndices.Num() == 0)
		{
			FVector Location = SpawnTransform.GetLocation();
			// 반경 100~200 유닛 사이의 랜덤 오프셋 생성
			float RandomRadius = FMath::RandRange(100.f, 200.f);
			float RandomAngle = FMath::RandRange(0.f, 360.f);
			
			Location.X += RandomRadius * FMath::Cos(FMath::DegreesToRadians(RandomAngle));
			Location.Y += RandomRadius * FMath::Sin(FMath::DegreesToRadians(RandomAngle));
			
			SpawnTransform.SetLocation(Location);
		}

		return SpawnTransform;
	}

	return FTransform::Identity;
}

void UER_RespawnSubsystem::RegisterPoint(AActor* Point)
{
	Points.AddUnique(Point);
	UE_LOG(LogTemp, Log, TEXT("[RSS] RegisterPoint Count : %d"), Points.Num());
}

void UER_RespawnSubsystem::UnregisterPoint(AActor* Point)
{
	Points.Remove(Point);
}




