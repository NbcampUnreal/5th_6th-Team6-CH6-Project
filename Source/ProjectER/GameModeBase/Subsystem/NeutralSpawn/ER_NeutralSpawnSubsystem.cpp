// Fill out your copyright notice in the Description page of Project Settings.


#include "GameModeBase/Subsystem/NeutralSpawn/ER_NeutralSpawnSubsystem.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_GameState.h"
#include "Monster/BaseMonster.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

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
                FNeutralInfo* Info = NeutralSpawnMap.Find(SpawnPointIdx);
                FActorSpawnParameters Params;
                Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

                // 액터 클래스 소환
                ABaseMonster* Spawned = GetWorld()->SpawnActor<ABaseMonster>(
                    Info->NeutralActorClass,
                    Info->SpawnPoint->GetActorTransform(),
                    Params
                );

                // FPrimaryAssetId의 값 지정
                FPrimaryAssetId MonsterAssetId(TEXT("Monster"), Info->DAName);
                AER_GameState* ERGS = GetWorld()->GetAuthGameMode()->GetGameState<AER_GameState>();

                // 현재 페이즈의 값을 GameState에서 받아와 페이즈 정보 전달
                Spawned->InitMonsterData(MonsterAssetId, ERGS->GetCurrentPhase());

                // 몬스터에게 Map의 Key값 전달
                Spawned->SetSpawnPoint(SpawnPointIdx);

                // FNeutralInfo 값 갱신
                Info->SpawnedActor = Spawned;
                Info->bIsSpawned = true;

                UE_LOG(LogTemp, Log, TEXT("[NSS] Complete Neutral Respawn DA_Name : %s , Phase : %d"), *Info->DAName.ToString(), ERGS->GetCurrentPhase());
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
            continue;

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

        // 액터 클래스 소환
        ABaseMonster* Spawned = World->SpawnActor<ABaseMonster>(
            Info.NeutralActorClass,
            Info.SpawnPoint->GetActorTransform(),
            Params
        );

        // FPrimaryAssetId의 값 지정
        FPrimaryAssetId MonsterAssetId(TEXT("Monster"), Info.DAName);
        AER_GameState* ERGS = GetWorld()->GetAuthGameMode()->GetGameState<AER_GameState>();

        // 현재 페이즈의 값을 GameState에서 받아와 페이즈 정보 전달
        Spawned->InitMonsterData(MonsterAssetId, ERGS->GetCurrentPhase());

        // 몬스터에게 Map의 Key값 전달
        Spawned->SetSpawnPoint(Pair.Key);

        // FNeutralInfo 값 갱신
        Info.SpawnedActor = Spawned;
        Info.bIsSpawned = true;
        UE_LOG(LogTemp, Log, TEXT("[NSS] Complete NeutralSpawn DA_Name : %s , Phase : %d"), *Info.DAName.ToString(), ERGS->GetCurrentPhase());

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

        FActorSpawnParameters Params;
        Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

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
        Info.bIsSpawned = false;
        if (ABaseMonster* N = Cast<ABaseMonster>(Info.SpawnedActor.Get()))
        {
            N->Death();
        }

    }
}

