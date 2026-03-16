#include "ObstacleOcclusion/Usage/Roof/BuildingGateVolume.h"
#include "ObstacleOcclusion/Usage/Roof/BuildingTrackerComponent.h"
#include "Components/BoxComponent.h"
#include "TopDownVisionDebug.h"

DEFINE_LOG_CATEGORY(BuildingGateVolume);

ABuildingGateVolume::ABuildingGateVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

    OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
    OverlapVolume->SetupAttachment(RootComponent);
    OverlapVolume->SetCollisionProfileName(TEXT("Trigger"));
}

void ABuildingGateVolume::BeginPlay()
{
    Super::BeginPlay();

    OverlapVolume->OnComponentBeginOverlap.AddDynamic(
        this, &ABuildingGateVolume::OnOverlapBegin);

    OverlapVolume->OnComponentEndOverlap.AddDynamic(
        this, &ABuildingGateVolume::OnOverlapEnd);
}

// ── Overlap ───────────────────────────────────────────────────────────────────

void ABuildingGateVolume::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!IsValid(OtherActor)) return;

    UBuildingTrackerComponent* Tracker =
        OtherActor->FindComponentByClass<UBuildingTrackerComponent>();

    if (!Tracker || !Tracker->IsInitialized()) return;

    Tracker->StartTrackerLoop();

    UE_LOG(BuildingGateVolume, Log,
        TEXT("ABuildingGateVolume::OnOverlapBegin>> %s entered gate %s"),
        *OtherActor->GetName(), *GetName());
}

void ABuildingGateVolume::OnOverlapEnd(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!IsValid(OtherActor)) return;

    UBuildingTrackerComponent* Tracker =
        OtherActor->FindComponentByClass<UBuildingTrackerComponent>();

    if (!Tracker || !Tracker->IsInitialized()) return;

    Tracker->StopTrackerLoop();

    UE_LOG(BuildingGateVolume, Log,
        TEXT("ABuildingGateVolume::OnOverlapEnd>> %s exited gate %s"),
        *OtherActor->GetName(), *GetName());
}