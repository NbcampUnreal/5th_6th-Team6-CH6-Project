// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModeBase/Subsystem/NeutralSpawn/ER_NeutralSpawnSubsystem.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_GameState.h"
#include "Monster/BaseMonster.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Engine/AssetManager.h"
#include "Monster/MonsterRangeComponent.h"

void UER_NeutralSpawnSubsystem::InitializeSpawnPoints(TMap<FName, FNeutralClassConfig>& NeutralClass)
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (World->GetNetMode() == NM_Client)
        return;

    if (bIsInitialized)
        return;

    const FName SpawnTag(TEXT("Monster"));

    UE_LOG(LogTemp, Log, TEXT("[NSS] InitializeSpawnPoints Start"));

    NeutralSpawnMap.Reset();

    // 레벨에 있는 액터 순회
    for (auto& Point : Points)
    {
        AActor* PointActor = Point.Get();
        if (!IsValid(PointActor))
            continue;

        // SpawnTag 태그를 가졌는지 확인
        if (!PointActor->ActorHasTag(SpawnTag))
            continue;

        const FNeutralClassConfig* Picked = nullptr;
        // SpawnTag 태그가 아닌 다른 태그 확인
        FName DAName;
        for (const FName& Tag : PointActor->Tags)
        {
            if (Tag == SpawnTag)
                continue;

            // 확인한 태그에 맞는 클래스 캐싱
            if (const FNeutralClassConfig* Found = NeutralClass.Find(Tag))
            {
                Picked = Found;
                DAName = Tag;
                break;
            }
        }

        if (!Picked || !Picked->Class || DAName.IsNone())
        {
            UE_LOG(LogTemp, Warning, TEXT("[NSS] No Class mapping for %s"), *PointActor->GetName());
            continue;
        }

        const int32 Key = PointActor->GetUniqueID();

        // FNeutralInfo 작성
        FNeutralInfo Info;
        Info.SpawnPoint = PointActor;
        Info.NeutralActorClass = Picked->Class;
        Info.RespawnDelay = Picked->RespawnDelay;
        Info.DAName = DAName;
        Info.bIsSpawned = false;

        // Map에 추가
        NeutralSpawnMap.Add(Key, Info);
    }
    bIsInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("[NSS] InitializeSpawnPoints End"));
}

void UER_NeutralSpawnSubsystem::StartRespawnNeutral(const int32 SpawnPointIdx)
{
    UE_LOG(LogTemp, Log, TEXT("[NSS] StartRespawnNeutral Start Key : %d"), SpawnPointIdx);
    // 몬스터가 가진 SpawnPoint 값 (SpawnPointIdx)을 받아와 Map을 검색
    FNeutralInfo* Info = NeutralSpawnMap.Find(SpawnPointIdx);
    if (!Info)
        return;

    if (Info->bIsSpawned)
        return;

    if (!bIsInitialized)
        return;

    // 타이머 시작 전 타이머 초기화
    GetWorld()->GetTimerManager().ClearTimer(Info->RespawnTimer);
    GetWorld()->GetTimerManager().SetTimer(
        Info->RespawnTimer,
        FTimerDelegate::CreateWeakLambda(this, [this, SpawnPointIdx]()
            {
                // 비동기로 실행하는 것이니 다시 Map에서 검색
                if (FNeutralInfo* DelayedInfo = NeutralSpawnMap.Find(SpawnPointIdx))
                {
                    SpawnMonsterInternal(*DelayedInfo, SpawnPointIdx);
                }
            }),
        Info->RespawnDelay,
        false
    );
}

void UER_NeutralSpawnSubsystem::FirstSpawnNeutral()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (World->GetNetMode() == NM_Client)
        return;

    if (!bIsInitialized)
        return;

    for (auto& Pair : NeutralSpawnMap)
    {
        FNeutralInfo& Info = Pair.Value;

        if (Info.bIsSpawned)
        {
            continue;
        }

        SpawnMonsterInternal(Info, Pair.Key);


    }
}

void UER_NeutralSpawnSubsystem::SetFalsebIsSpawned(const int32 SpawnPointIdx)
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (World->GetNetMode() == NM_Client)
        return;

    FNeutralInfo* Info = NeutralSpawnMap.Find(SpawnPointIdx);
    if (!Info)
        return;

    Info->bIsSpawned = false;
}

void UER_NeutralSpawnSubsystem::RegisterPoint(AActor* Point)
{
    Points.AddUnique(Point);
}

void UER_NeutralSpawnSubsystem::UnregisterPoint(AActor* Point)
{
    Points.Remove(Point);
}


void UER_NeutralSpawnSubsystem::TEMP_SpawnNeutrals()
{
    UWorld* World = GetWorld();
    if (!World) 
        return;

    if (World->GetNetMode() == NM_Client) 
        return;

    UE_LOG(LogTemp, Log, TEXT("[NSS] TEMP_SpawnNeutrals Start"));

    for (auto& Pair : NeutralSpawnMap)
    {
        FNeutralInfo& Info = Pair.Value;

        if (Info.bIsSpawned)
            continue;

        SpawnMonsterInternal(Info, Pair.Key);
    }
}

void UER_NeutralSpawnSubsystem::TEMP_NeutralsALLDespawn()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (World->GetNetMode() == NM_Client)
        return;

    UE_LOG(LogTemp, Log, TEXT("[NSS] TEMP_NeutralsALLDespawn Start"));

    for (auto& it : NeutralSpawnMap)
    {
        FNeutralInfo& Info = it.Value;
        Info.bIsSpawned = false;
        if (ABaseMonster* N = Cast<ABaseMonster>(Info.SpawnedActor.Get()))
        {
            N->Death();
        }
    }
}

void UER_NeutralSpawnSubsystem::SpawnMonsterInternal(FNeutralInfo& Info, int32 SpawnPointIdx)
{
    if (!Info.SpawnPoint.IsValid()) 
    {
        return;
    }

    FPrimaryAssetId MonsterAssetId(TEXT("Monster"), Info.DAName);
    FTransform SpawnTM = Info.SpawnPoint->GetActorTransform();

    // 에셋이 로딩되어 있는지 확인 후, 로드완료 콜백을 호출해 스폰 진행
    UAssetManager::Get().LoadPrimaryAsset(
        MonsterAssetId,
        TArray<FName>(),
        FStreamableDelegate::CreateUObject(
            this,
            &UER_NeutralSpawnSubsystem::OnMonsterDataLoadedForSpawn,
            SpawnPointIdx,
            MonsterAssetId,
            SpawnTM
        )
    );
}

void UER_NeutralSpawnSubsystem::OnMonsterDataLoadedForSpawn(int32 SpawnPointIdx, FPrimaryAssetId AssetId, FTransform SpawnTM)
{
    // 로드 성공 여부 검사: 에셋을 찾지 못했다면 스폰을 취소합니다 (크래시 방지)
    UObject* LoadedData = UAssetManager::Get().GetPrimaryAssetObject(AssetId);
    if (!LoadedData)
    {
        UE_LOG(LogTemp, Error, TEXT("[NSS] Failed to Load MonsterAsset: %s. Aborting Spawn!"), *AssetId.ToString());
        return;
    }

    FNeutralInfo* InfoPtr = NeutralSpawnMap.Find(SpawnPointIdx);
    if (!InfoPtr || !InfoPtr->SpawnPoint.IsValid())
    {
        return;
    }

    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    // 비동기 로드 완료 후 스폰
    ABaseMonster* Spawned = GetWorld()->SpawnActorDeferred<ABaseMonster>(
        InfoPtr->NeutralActorClass,
        SpawnTM,
        nullptr,
        nullptr,
        Params.SpawnCollisionHandlingOverride
    );

    if (!Spawned) 
    {
        return;
    }

    AER_GameState* ERGS = GetWorld()->GetGameState<AER_GameState>();
    int32 Phase = (ERGS && ERGS->GetCurrentPhase() > 0) ? ERGS->GetCurrentPhase() : 1;

    // 1. 껍데기만 있는 몬스터의 스폰을 먼저 완료 (BeginPlay, PossessedBy 내부 동작 등을 처리)
    UGameplayStatics::FinishSpawningActor(Spawned, SpawnTM);

    // 2. 완전히 정상적인 형태를 갖춘 뒤에 데이터를 꽂아넣기
    Spawned->InitMonsterData(AssetId, Phase);
    Spawned->SetSpawnPoint(SpawnPointIdx);
    
    // 3. 몬스터 스폰 완료 후, 범위를 확인하여 그룹을 형성
    if (UMonsterRangeComponent* RangeComp = Spawned->GetMonsterRangeComp())
    {
        RangeComp->InitMonsterGroup();
    }

    InfoPtr->SpawnedActor = Spawned;
    InfoPtr->bIsSpawned = true;

    UE_LOG(LogTemp, Log, TEXT("[NSS] Neutral Spawned after Async Load! DA_Name: %s, Phase: %d"), *InfoPtr->DAName.ToString(), Phase);
}
