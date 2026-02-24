#include "GameModeBase/Subsystem/Object/ER_ObjectSubsystem.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/PointActor/ER_PointActor.h"
#include "Kismet/GameplayStatics.h"


void UER_ObjectSubsystem::InitializeObjectPoints(TMap<FName, FObjectClassConfig>& ObjectClass)
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (World->GetNetMode() == NM_Client)
        return;

    if (bIsInitialized)
        return;

    UE_LOG(LogTemp, Log, TEXT("[OSS] InitializeObjectPoints Start Points Count : %d"), Points.Num());

    const FName ObjectTag(TEXT("Object"));
    const FName BossTag(TEXT("Monster"));
    const FName SupplyTag(TEXT("Supply"));

    SupplyPointsByRegion.Reset();
    BossPoints.Reset();

    for (auto& Point : Points)
    {
        AActor* PointActor = Point.Get();

        if (!IsValid(PointActor))
            continue;

        const FObjectClassConfig* Picked = nullptr;
        FName DAName;
        FName TagName;

        // 보스 이니셜라이즈
        if (PointActor->ActorHasTag(BossTag))
        {
            for (const FName& Tag : PointActor->Tags)
            {
                if (Tag == BossTag)
                    continue;

                if (const FObjectClassConfig* Found = ObjectClass.Find(Tag))
                {
                    TagName = BossTag;
                    Picked = Found;
                    DAName = Tag;
                    break;
                }
            }

            if (!Picked || !Picked->Class)
            {
                UE_LOG(LogTemp, Warning, TEXT("[OSS] No Class mapping for %s"), *PointActor->GetName());
                continue;
            }

            FObjectInfo Info;
            Info.SpawnPoint = PointActor;
            Info.ObjectClass = Picked->Class;
            Info.DAName = DAName;
            Info.bIsSpawned = false;

            BossPoints.Add(Info);
        }
        
        // 보급 이니셜라이즈
        else if (PointActor->ActorHasTag(SupplyTag))
        {
            for (const FName& Tag : PointActor->Tags)
            {
                if (Tag == SupplyTag)
                    continue;

                if (const FObjectClassConfig* Found = ObjectClass.Find(Tag))
                {
                    TagName = SupplyTag;
                    Picked = Found;
                    //DAName = Tag;
                    break;
                }
            }

            if (!Picked || !Picked->Class)
            {
                UE_LOG(LogTemp, Warning, TEXT("[OSS] No Class mapping for %s"), *PointActor->GetName());
                continue;
            }
            AER_PointActor* PA = Cast<AER_PointActor>(PointActor);
            if (!PA)
                continue;

            FObjectInfo Info;
            Info.SpawnPoint = PointActor;
            Info.ObjectClass = Picked->Class;
            Info.bIsSpawned = false;
            Info.RegionType = PA->RegionType;

            SupplyPointsByRegion.FindOrAdd(Info.RegionType).Add(Info);
        }
        
    }
    bIsInitialized = true;
    UE_LOG(LogTemp, Log, TEXT("[OSS] InitializeObjectPoints End. SupplyPoint Count : %d , BossPoint Count : %d"), SupplyPointsByRegion.Num(), BossPoints.Num());
}

void UER_ObjectSubsystem::PickSupplySpawnIndex()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
        return;

    if (!bIsInitialized)
        return;

    PendingSupplyPicks.Reset();

    for (auto& Pair : SupplyPointsByRegion)
    {
        ERegionType Region = Pair.Key;
        TArray<FObjectInfo>& Infos = Pair.Value;

        TArray<int32> Candidates;
        Candidates.Reserve(Infos.Num());

        for (int32 i = 0; i < Infos.Num(); ++i)
        {
            // 이미 스폰 됐거나, 예약 상태라면 제외
            if (!Infos[i].bIsSpawned && !Infos[i].bIsReserved
                && Infos[i].ObjectClass
                && IsValid(Infos[i].SpawnPoint.Get()))
            {
                Candidates.Add(i);
            }
        }

        if (Candidates.Num() == 0)
            continue;

        const int32 PickIdx = Candidates[FMath::RandRange(0, Candidates.Num() - 1)];

        FSupplySpawnPick Info;
        Info.Region = Region;
        Info.Index = PickIdx;

        // 예약으로 변경
        Infos[PickIdx].bIsReserved = true;
        PendingSupplyPicks.Add(Info);

        if (AER_PointActor* PA = Cast<AER_PointActor>(Infos[PickIdx].SpawnPoint.Get()))
        {
            PA->SetSelectedVisual(true);
        }
    }
}

void UER_ObjectSubsystem::SpawnSupplyOjbect()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
        return;

    if (!bIsInitialized)
        return;

    if (PendingSupplyPicks.Num() == 0)
        return;

    for (int32 p = PendingSupplyPicks.Num() - 1; p >= 0; --p)
    {
        const FSupplySpawnPick Pick = PendingSupplyPicks[p];

        TArray<FObjectInfo>* InfosPtr = SupplyPointsByRegion.Find(Pick.Region);
        if (!InfosPtr || !InfosPtr->IsValidIndex(Pick.Index))
        {
            PendingSupplyPicks.RemoveAtSwap(p);
            continue;
        }

        FObjectInfo& Info = (*InfosPtr)[Pick.Index];

        // 스폰 직전 재검증
        if (Info.bIsSpawned || !Info.ObjectClass || !IsValid(Info.SpawnPoint.Get()))
        {
            // 실패 시 예약 해제
            Info.bIsReserved = false;
            PendingSupplyPicks.RemoveAtSwap(p);
            continue;
        }

        const FTransform SpawnTM = Info.SpawnPoint->GetActorTransform();

        AActor* Spawned = World->SpawnActorDeferred<AActor>(
            Info.ObjectClass,
            SpawnTM,
            nullptr,
            nullptr,
            ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn
        );

        if (!Spawned)
        {
            Info.bIsReserved = false;
            PendingSupplyPicks.RemoveAtSwap(p);
            continue;
        }

        UGameplayStatics::FinishSpawningActor(Spawned, SpawnTM);

        // 스폰 완료 시 예약 해제
        Info.bIsSpawned = true;
        Info.bIsReserved = false;

        if (AER_PointActor* PA = Cast<AER_PointActor>(Info.SpawnPoint.Get()))
        {
            PA->SetSelectedVisual(false);
        }

        PendingSupplyPicks.RemoveAtSwap(p);
    }
}

void UER_ObjectSubsystem::RegisterPoint(AActor* Point)
{
	Points.AddUnique(Point);
}

void UER_ObjectSubsystem::UnregisterPoint(AActor* Point)
{
	Points.Remove(Point);
}
