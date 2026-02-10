// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModeBase/Subsystem/NeutralSpawn/ER_NeutralSpawnSubsystem.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_GameState.h"
#include "EngineUtils.h"
#include "GameModeBase/TEMPNeutral.h"
#include "Monster/BaseMonster.h"

void UER_NeutralSpawnSubsystem::InitializeSpawnPoints(TMap<FName, FNeutralClassConfig>& NeutralClass)
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (World->GetNetMode() == NM_Client)
        return;

    const FName SpawnTag(TEXT("Monster"));

    UE_LOG(LogTemp, Log, TEXT("[NSS] InitializeSpawnPoints Start"));

    NeutralSpawnMap.Reset();

    // 레벨에 있는 액터 순회
    for (AActor* Actor : FActorRange(World))
    {
        if (!IsValid(Actor))
            continue;
        // SpawnTag 태그를 가졌는지 확인
        if (!Actor->ActorHasTag(SpawnTag))
            continue;

        const FNeutralClassConfig* Picked = nullptr;
        // SpawnTag 태그가 아닌 다른 태그 확인
        FName DAName;
        for (const FName& Tag : Actor->Tags)
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
            UE_LOG(LogTemp, Warning, TEXT("[NSS] No Class mapping for %s"), *Actor->GetName());
            continue;
        }

        const int32 Key = Actor->GetUniqueID();

        // FNeutralInfo 작성
        FNeutralInfo Info;
        Info.SpawnPoint = Actor;
        Info.NeutralActorClass = Picked->Class;
        Info.RespawnDelay = Picked->RespawnDelay;
        Info.DAName = DAName;
        //Info.bIsSpawned = false;

        // Map에 추가
        NeutralSpawnMap.Add(Key, Info);
    }
    UE_LOG(LogTemp, Log, TEXT("[NSS] InitializeSpawnPoints End"));
}

void UER_NeutralSpawnSubsystem::StartRespawnNeutral(const int32 SpawnPointIdx)
{
    // 몬스터가 가진 SpawnPoint 값을 받아와 Map을 검색
    UE_LOG(LogTemp, Log, TEXT("[NSS] StartRespawnNeutral Start"));
    FNeutralInfo* Info = NeutralSpawnMap.Find(SpawnPointIdx);
    if (!Info)
        return;

    GetWorld()->GetTimerManager().ClearTimer(Info->RespawnTimer);
    GetWorld()->GetTimerManager().SetTimer(
        Info->RespawnTimer,
        FTimerDelegate::CreateWeakLambda(this, [this, SpawnPointIdx]()
            {
                UE_LOG(LogTemp, Log, TEXT("[NSS] StartRespawnNeutral Timer End"));
                FNeutralInfo* Info = NeutralSpawnMap.Find(SpawnPointIdx);
                // 리스폰 함수
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride =
                    ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                ABaseMonster* Spawned = GetWorld()->SpawnActor<ABaseMonster>(
                    Info->NeutralActorClass,
                    Info->SpawnPoint->GetActorTransform(),
                    Params
                );
                FPrimaryAssetId MonsterAssetId(TEXT("Monster"), Info->DAName);
                AER_GameState* ERGS = GetWorld()->GetAuthGameMode()->GetGameState<AER_GameState>();
                Spawned->InitMonsterData(MonsterAssetId, ERGS->GetCurrentPhase());
                Spawned->SetSpawnPoint(SpawnPointIdx);

                Info->SpawnedActor = Spawned;
                UE_LOG(LogTemp, Log, TEXT("[NSS] Complete NeutralSpawn DA_Name : %s , Phase : %d"), *Info->DAName.ToString(), ERGS->GetCurrentPhase());
            }),
        Info->RespawnDelay,
        false
    );
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

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride =
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        ABaseMonster* Spawned = World->SpawnActor<ABaseMonster>(
            Info.NeutralActorClass,
            Info.SpawnPoint->GetActorTransform(),
            Params
        );
        FPrimaryAssetId MonsterAssetId(TEXT("Monster"), TEXT("DA_Monster_Orc"));
        Spawned->InitMonsterData(MonsterAssetId, 1);
        Spawned->SetSpawnPoint(Pair.Key);

        if (!Spawned)
            continue;

        Info.SpawnedActor = Spawned;

        //Spawned->OnDestroyed.AddDynamic(this, &ThisClass::OnNeutralDestroyed);
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
        if (ABaseMonster* N = Cast<ABaseMonster>(Info.SpawnedActor.Get()))
        {
            N->Death();
        }
    }
}
