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

void FVisibleActorArray::PostReplicatedAdd(
    const TArrayView<int32>& AddedIndices, int32 FinalSize)
{
    if (!OwnerComp) return;
    for (int32 Idx : AddedIndices)
        if (Items.IsValidIndex(Idx))
            OwnerComp->OnTargetBecameVisible(Items[Idx].Target, Items[Idx].ObserverTeam);
}

void FVisibleActorArray::PreReplicatedRemove(
    const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
    if (!OwnerComp) return;

    // NOTE: at the point PreReplicatedRemove fires, the items are still in
    // the array (they are removed after this callback returns).
    // ReevaluateTargetVisibility must therefore NOT count on the array
    // already reflecting the removal.  We pass Team so the evaluator can
    // exclude this specific entry if needed — but for correctness the
    // simpler path is to call OnTargetBecameHidden after removal, which is
    // what happens on the server side.  On clients we fire early, but since
    // ReevaluateTargetVisibility scans for CanSeeTeam across ALL entries it
    // finds at scan time, any mis-timing is harmless: worst case the actor
    // stays visible for one extra frame.
    for (int32 Idx : RemovedIndices)
        if (Items.IsValidIndex(Idx))
            OwnerComp->OnTargetBecameHidden(Items[Idx].Target, Items[Idx].ObserverTeam);
}

void FVisibleActorArray::PostReplicatedChange(
    const TArrayView<int32>& ChangedIndices, int32 FinalSize)
{
    // Entries are only added or removed — no change events expected.
}

// -------------------------------------------------------------------------- //
//  Component lifecycle
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

void UVisionGameStateComp::GetLifetimeReplicatedProps(
    TArray<FLifetimeProperty>& OutLifetimeProps) const
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

    FVisibleActorEntry& Entry = VisibleActors.Items.AddDefaulted_GetRef();
    Entry.Target       = Target;
    Entry.ObserverTeam = Team;
    VisibleActors.MarkItemDirty(Entry);

    // Fire locally — PostReplicatedAdd only runs on remote clients.
    OnTargetBecameVisible(Target, Team);

    UE_LOG(VisionGameStateComp, Verbose,
        TEXT("SetActorVisibleToTeam >> %s visible to team [%s]"),
        *Target->GetName(), *UEnum::GetValueAsString(Team));
}

void UVisionGameStateComp::ClearActorVisibleToTeam(AActor* Target, EVisionChannel Team)
{
    if (!Target)
    {
        UE_LOG(VisionGameStateComp, Warning,
            TEXT("ClearActorVisibleToTeam >> Null target"));
        return;
    }

    for (int32 i = VisibleActors.Items.Num() - 1; i >= 0; --i)
    {
        const FVisibleActorEntry& Entry = VisibleActors.Items[i];
        if (Entry.Target != Target || Entry.ObserverTeam != Team)
            continue;

        VisibleActors.Items.RemoveAt(i);
        VisibleActors.MarkArrayDirty();

        // Entry is already gone from the array before we call this,
        // so ReevaluateTargetVisibility will correctly find only the
        // remaining entries when it scans.
        OnTargetBecameHidden(Target, Team);

        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("ClearActorVisibleToTeam >> %s hidden from team [%s]"),
            *Target->GetName(), *UEnum::GetValueAsString(Team));
        return;
    }

    UE_LOG(VisionGameStateComp, Verbose,
        TEXT("ClearActorVisibleToTeam >> %s was not visible to team [%s], nothing to clear"),
        *Target->GetName(), *UEnum::GetValueAsString(Team));
}

bool UVisionGameStateComp::IsActorVisibleToTeam(AActor* Target, EVisionChannel Team) const
{
    for (const FVisibleActorEntry& Entry : VisibleActors.Items)
    {
        if (Entry.Target != Target)
            continue;

        if (Entry.ObserverTeam == EVisionChannel::AlwaysVisible)
            return true;

        if (Entry.ObserverTeam == Team)
            return true;
    }
    return false;
}

EVisionChannel UVisionGameStateComp::GetLocalPlayerTeamChannel() const
{
    APlayerController* LocalPC = GEngine->GetFirstLocalPlayerController(GetWorld());
    if (!LocalPC)
        return EVisionChannel::None;

    AGameStateBase* GS = GetWorld()->GetGameState();
    if (!GS)
        return EVisionChannel::None;

    for (APlayerState* PS : GS->PlayerArray)
    {
        if (!PS || PS->GetOwningController() != LocalPC)
            continue;

        if (UVisionPlayerStateComp* VisionPS =
            PS->FindComponentByClass<UVisionPlayerStateComp>())
        {
            return VisionPS->GetTeamChannel();
        }
    }

    return EVisionChannel::None;
}

// -------------------------------------------------------------------------- //
//  Client callbacks — both now call ReevaluateTargetVisibility
//
//  OnTargetBecameVisible: a new entry arrived — re-eval may reveal the actor.
//  OnTargetBecameHidden:  an entry was removed — re-eval checks whether ANY
//                         remaining entry is still visible to this player.
//                         If TeamA's entry exists, the actor stays visible.
//                         This is the fix for the shared-vision corruption bug.
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::OnTargetBecameVisible(AActor* Target, EVisionChannel Team)
{
    if (!Target) return;

    UVisionPlayerStateComp* VisionPS = ULOSVisionSubsystem::GetLocalVisionPS(GetWorld());
    if (!VisionPS)
    {
        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("OnTargetBecameVisible >> VisionPS not ready, queuing %s"),
            *Target->GetName());
        PendingReveals.Add({ Target, Team });
        return;
    }

    VisionPS->ReevaluateTargetVisibility(Target);
}

void UVisionGameStateComp::OnTargetBecameHidden(AActor* Target, EVisionChannel Team)
{
    if (!Target) return;

    UVisionPlayerStateComp* VisionPS = ULOSVisionSubsystem::GetLocalVisionPS(GetWorld());
    if (!VisionPS)
    {
        PendingReveals.Add({ Target, Team });
        return;
    }

    // Do NOT call SetVisible(false) here.
    // Re-evaluate from scratch — if another team's entry still exists for
    // this target and this player can see that team, it stays visible.
    VisionPS->ReevaluateTargetVisibility(Target);
}

// -------------------------------------------------------------------------- //
//  Pending queue drain
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::FlushPendingReveals(UVisionPlayerStateComp* VisionPS)
{
    if (!VisionPS || PendingReveals.IsEmpty())
        return;

    UE_LOG(VisionGameStateComp, Verbose,
        TEXT("FlushPendingReveals >> Flushing %d queued entries"),
        PendingReveals.Num());

    // Deduplicate — multiple pending entries for the same target collapse to
    // one reeval call, same as RefreshVisibility does.
    TSet<AActor*> Evaluated;
    for (const FPendingVisibilityEntry& Entry : PendingReveals)
    {
        if (!Entry.Target.IsValid() || Evaluated.Contains(Entry.Target.Get()))
            continue;

        Evaluated.Add(Entry.Target.Get());
        VisionPS->ReevaluateTargetVisibility(Entry.Target.Get());
    }

    PendingReveals.Empty();
}