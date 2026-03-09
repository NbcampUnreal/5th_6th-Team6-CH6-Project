#include "LineOfSight/Management/VisionGameStateComp.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"

DEFINE_LOG_CATEGORY(VisionGameStateComp);

// -------------------------------------------------------------------------- //
//  FastArray callbacks — fire on clients only
// -------------------------------------------------------------------------- //

void FVisibleActorArray::PostReplicatedAdd(const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
    if (!OwnerComp) return;
    for (int32 Idx : AddedIndices)
        if (Items.IsValidIndex(Idx))
            OwnerComp->OnTargetBecameVisible(Items[Idx].Target, Items[Idx].TeamChannel);
}

void FVisibleActorArray::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
    if (!OwnerComp) return;
    for (int32 Idx : RemovedIndices)
        if (Items.IsValidIndex(Idx))
            OwnerComp->OnTargetBecameHidden(Items[Idx].Target, Items[Idx].TeamChannel);
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
        UE_LOG(VisionGameStateComp, Warning, TEXT("SetActorVisibleToTeam >> Null target"));
        return;
    }

    if (IsActorVisibleToTeam(Target, Team))
        return;

    FVisibleActorEntry& Entry = VisibleActors.Items.AddDefaulted_GetRef();
    Entry.Target      = Target;
    Entry.TeamChannel = Team;
    VisibleActors.MarkItemDirty(Entry);

    // Fire locally — PostReplicatedAdd only runs on remote clients
    OnTargetBecameVisible(Target, Team);

    UE_LOG(VisionGameStateComp, Log,
        TEXT("SetActorVisibleToTeam >> %s visible to team [%s]"),
        *Target->GetName(), *UEnum::GetValueAsString(Team));
}

void UVisionGameStateComp::ClearActorVisibleToTeam(AActor* Target, EVisionChannel Team)
{
    if (!Target)
    {
        UE_LOG(VisionGameStateComp, Warning, TEXT("ClearActorVisibleToTeam >> Null target"));
        return;
    }

    for (int32 i = VisibleActors.Items.Num() - 1; i >= 0; --i)
    {
        const FVisibleActorEntry& Entry = VisibleActors.Items[i];
        if (Entry.Target == Target && Entry.TeamChannel == Team)
        {
            VisibleActors.Items.RemoveAt(i);
            VisibleActors.MarkArrayDirty();

            OnTargetBecameHidden(Target, Team);

            UE_LOG(VisionGameStateComp, Log,
                TEXT("ClearActorVisibleToTeam >> %s hidden from team [%s]"),
                *Target->GetName(), *UEnum::GetValueAsString(Team));
            return;
        }
    }

    UE_LOG(VisionGameStateComp, Verbose,
        TEXT("ClearActorVisibleToTeam >> %s was not visible to team [%s], nothing to clear"),
        *Target->GetName(), *UEnum::GetValueAsString(Team));
}

bool UVisionGameStateComp::IsActorVisibleToTeam(AActor* Target, EVisionChannel Team) const
{
    for (const FVisibleActorEntry& Entry : VisibleActors.Items)
        if (Entry.Target == Target && Entry.TeamChannel == Team)
            return true;

    return false;
}

// -------------------------------------------------------------------------- //
//  Client callbacks — push to PlayerStateComp; queue if not ready yet
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::OnTargetBecameVisible(AActor* Target, EVisionChannel Team)
{
    if (!Target) return;

    UVisionPlayerStateComp* VisionPS = ULOSVisionSubsystem::GetLocalVisionPS(GetWorld());
    if (!VisionPS)
    {
        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("OnTargetBecameVisible >> VisionPS not ready, queuing reveal of %s"),
            *Target->GetName());
        PendingReveals.Add({ Target, Team, true });
        return;
    }

    UE_LOG(VisionGameStateComp, Log,
        TEXT("OnTargetBecameVisible >> Pushing visible [%s] to VisionPS"),
        *Target->GetName());

    VisionPS->ApplyActorVisibility(Target, Team, true);
}

void UVisionGameStateComp::OnTargetBecameHidden(AActor* Target, EVisionChannel Team)
{
    if (!Target) return;

    UVisionPlayerStateComp* VisionPS = ULOSVisionSubsystem::GetLocalVisionPS(GetWorld());
    if (!VisionPS)
    {
        PendingReveals.Add({ Target, Team, false });
        return;
    }

    UE_LOG(VisionGameStateComp, Log,
        TEXT("OnTargetBecameHidden >> Pushing hidden [%s] to VisionPS"),
        *Target->GetName());

    VisionPS->ApplyActorVisibility(Target, Team, false);
}

// -------------------------------------------------------------------------- //
//  Pending queue drain — called by VisionPlayerStateComp::RefreshVisibility
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::FlushPendingReveals(UVisionPlayerStateComp* VisionPS)
{
    if (!VisionPS || PendingReveals.IsEmpty())
        return;

    UE_LOG(VisionGameStateComp, Log,
        TEXT("FlushPendingReveals >> Flushing %d queued entries"), PendingReveals.Num());

    for (const FPendingVisibilityEntry& Entry : PendingReveals)
    {
        if (Entry.Target.IsValid())
            VisionPS->ApplyActorVisibility(Entry.Target.Get(), Entry.Team, Entry.bVisible);
    }

    PendingReveals.Empty();
}