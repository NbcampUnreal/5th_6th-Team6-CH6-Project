#include "LevelAreaBridgeVolume.h"
#include "Components/BoxComponent.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"
#include "LogHelper/DebugLogHelper.h"

ALevelAreaBridgeVolume::ALevelAreaBridgeVolume()
{
    PrimaryActorTick.bCanEverTick = false;

    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

    OverlapVolume = CreateDefaultSubobject<UBoxComponent>(TEXT("OverlapVolume"));
    OverlapVolume->SetupAttachment(RootComponent);
    OverlapVolume->SetCollisionProfileName(TEXT("Trigger"));
}

void ALevelAreaBridgeVolume::BeginPlay()
{
    Super::BeginPlay();

    OverlapVolume->OnComponentBeginOverlap.AddDynamic(
        this, &ALevelAreaBridgeVolume::OnOverlapBegin);

    OverlapVolume->OnComponentEndOverlap.AddDynamic(
        this, &ALevelAreaBridgeVolume::OnOverlapEnd);
}


/* =====================================================================
   Overlap
   ===================================================================== */

void ALevelAreaBridgeVolume::OnOverlapBegin(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex,
    bool bFromSweep,
    const FHitResult& SweepResult)
{
    if (!OtherActor) return;

    ULevelAreaTrackerComponent* Tracker =
        OtherActor->FindComponentByClass<ULevelAreaTrackerComponent>();

    if (!Tracker) return;

    Tracker->StartAreaUpdateLoop();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ALevelAreaBridgeVolume >> %s entered bridge — loop started"),
        *OtherActor->GetName());
}

void ALevelAreaBridgeVolume::OnOverlapEnd(
    UPrimitiveComponent* OverlappedComp,
    AActor* OtherActor,
    UPrimitiveComponent* OtherComp,
    int32 OtherBodyIndex)
{
    if (!OtherActor) return;

    ULevelAreaTrackerComponent* Tracker =
        OtherActor->FindComponentByClass<ULevelAreaTrackerComponent>();

    if (!Tracker) return;

    Tracker->StopAreaUpdateLoop();

    UE_LOG(LevelAreaGraphManagement, Log,
        TEXT("ALevelAreaBridgeVolume >> %s left bridge — loop stopped"),
        *OtherActor->GetName());
}