// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/Management/VisionGameStateComp.h"

#include "GameFramework/PlayerState.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "Net/UnrealNetwork.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"

DEFINE_LOG_CATEGORY(VisionGameStateComp);

// -------------------------------------------------------------------------- //
//  FastArray Callbacks — fire on clients
// -------------------------------------------------------------------------- //

void FVisibleActorArray::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
    if (!OwnerComp)
        return;

    for (int32 Idx : AddedIndices)
    {
        if (Items.IsValidIndex(Idx))
            OwnerComp->OnTargetBecameVisible(Items[Idx].Target, Items[Idx].TeamID);
    }
}

void FVisibleActorArray::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
    // Fires BEFORE item is removed — Items[Idx] is still valid here
    if (!OwnerComp)
        return;

    for (int32 Idx : RemovedIndices)
    {
        if (Items.IsValidIndex(Idx))
            OwnerComp->OnTargetBecameHidden(Items[Idx].Target, Items[Idx].TeamID);
    }
}

void FVisibleActorArray::PostReplicatedChange(const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
    // Entries are only added or removed — no change events expected
}

// -------------------------------------------------------------------------- //
//  Component
// -------------------------------------------------------------------------- //

UVisionGameStateComp::UVisionGameStateComp()
{
    PrimaryComponentTick.bCanEverTick = false;
    SetIsReplicatedByDefault(true);
}

void UVisionGameStateComp::BeginPlay()
{
    Super::BeginPlay();
    VisibleActors.OwnerComp = this;
}

void UVisionGameStateComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(UVisionGameStateComp, VisibleActors);
}

// -------------------------------------------------------------------------- //
//  Server API
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::SetActorVisibleToTeam(AActor* Target, EVisionChannel Team)
{
    if (!Target)
    {
        UE_LOG(VisionGameStateComp, Warning,
            TEXT("SetActorVisibleToTeam >> Null target"));
        return;
    }

    if (IsActorVisibleToTeam(Target, Team))
        return;

    FVisibleActorEntry& NewEntry = VisibleActors.Items.AddDefaulted_GetRef();
    NewEntry.Target = Target;
    NewEntry.TeamID = (uint8)Team;
    VisibleActors.MarkItemDirty(NewEntry);

    UE_LOG(VisionGameStateComp, Log,
        TEXT("SetActorVisibleToTeam >> %s visible to team %d"),
        *Target->GetName(), (uint8)Team);
}

void UVisionGameStateComp::ClearActorVisibleToTeam(AActor* Target, EVisionChannel Team)
{
    if (!Target)
    {
        UE_LOG(VisionGameStateComp, Warning,
            TEXT("ClearActorVisibleToTeam >> Null target"));
        return;
    }

    const uint8 TeamID = (uint8)Team;

    for (int32 i = VisibleActors.Items.Num() - 1; i >= 0; --i)
    {
        const FVisibleActorEntry& Entry = VisibleActors.Items[i];
        if (Entry.Target == Target && Entry.TeamID == TeamID)
        {
            VisibleActors.Items.RemoveAt(i);
            VisibleActors.MarkArrayDirty();

            UE_LOG(VisionGameStateComp, Log,
                TEXT("ClearActorVisibleToTeam >> %s hidden from team %d"),
                *Target->GetName(), TeamID);
            return;
        }
    }

    UE_LOG(VisionGameStateComp, Verbose,
        TEXT("ClearActorVisibleToTeam >> %s was not visible to team %d, nothing to clear"),
        *Target->GetName(), TeamID);
}

bool UVisionGameStateComp::IsActorVisibleToTeam(AActor* Target, EVisionChannel Team) const
{
    const uint8 TeamID = (uint8)Team;

    for (const FVisibleActorEntry& Entry : VisibleActors.Items)
    {
        if (Entry.Target == Target && Entry.TeamID == TeamID)
            return true;
    }

    return false;
}

// -------------------------------------------------------------------------- //
//  Client callbacks — fired by FastArray
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::OnTargetBecameVisible(AActor* Target, uint8 TeamID)
{
    if (!Target)
        return;

    // Get local player's VisionPlayerStateComp
    APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
    if (!PC || !PC->PlayerState)
        return;

    UVisionPlayerStateComp* VisionPS = PC->PlayerState->FindComponentByClass<UVisionPlayerStateComp>();
    if (!VisionPS)
        return;

    // Filter — only reveal if local player can see this team
    if (!VisionPS->CanSeeTeam((EVisionChannel)TeamID))
    {
        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("OnTargetBecameVisible >> %s skipped — local player cannot see team %d"),
            *Target->GetName(), TeamID);
        return;
    }

    UE_LOG(VisionGameStateComp, Log,
        TEXT("OnTargetBecameVisible >> %s visible to team %d"),
        *Target->GetName(), TeamID);

    if (UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>())
        VisualComp->SetVisible(true);
}

void UVisionGameStateComp::OnTargetBecameHidden(AActor* Target, uint8 TeamID)
{
    if (!Target)
        return;

    APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
    if (!PC || !PC->PlayerState)
        return;

    UVisionPlayerStateComp* VisionPS = PC->PlayerState->FindComponentByClass<UVisionPlayerStateComp>();
    if (!VisionPS)
        return;

    if (!VisionPS->CanSeeTeam((EVisionChannel)TeamID))
    {
        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("OnTargetBecameHidden >> %s skipped — local player cannot see team %d"),
            *Target->GetName(), TeamID);
        return;
    }

    UE_LOG(VisionGameStateComp, Log,
        TEXT("OnTargetBecameHidden >> %s hidden from team %d"),
        *Target->GetName(), TeamID);

    if (UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>())
        VisualComp->SetVisible(false);
}