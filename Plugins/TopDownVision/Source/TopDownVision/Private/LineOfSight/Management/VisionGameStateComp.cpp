// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/Management/VisionGameStateComp.h"

#include "GameFramework/PlayerState.h"
#include "GameFramework/GameStateBase.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
#include "Net/UnrealNetwork.h"
#include "LineOfSight/VisionComps/Vision_VisualComp.h"

DEFINE_LOG_CATEGORY(VisionGameStateComp);

// -------------------------------------------------------------------------- //
//  Helpers
// -------------------------------------------------------------------------- //

/** Finds the local player's VisionPlayerStateComp.
 *  First tries PC->PlayerState (fast path, valid after initial setup).
 *  Falls back to iterating GameState->PlayerArray (works during early BeginPlay
 *  when PC->PlayerState is not yet assigned). */
static UVisionPlayerStateComp* GetLocalVisionPS(UWorld* World)
{
    if (!World)
        return nullptr;

    APlayerController* PC = GEngine->GetFirstLocalPlayerController(World);
    if (!PC)
        return nullptr;

    // Fast path — works after PC has its PlayerState assigned
    if (PC->PlayerState)
    {
        if (UVisionPlayerStateComp* VisionPS = PC->PlayerState->FindComponentByClass<UVisionPlayerStateComp>())
            return VisionPS;
    }

    // Fallback — iterate PlayerArray on GameState.
    // PC->PlayerState is null during early BeginPlay but PlayerArray is populated sooner.
    if (AGameStateBase* GS = World->GetGameState())
    {
        for (APlayerState* PS : GS->PlayerArray)
        {
            if (PS && PS->GetOwningController() == PC)
            {
                if (UVisionPlayerStateComp* VisionPS = PS->FindComponentByClass<UVisionPlayerStateComp>())
                {
                    return VisionPS;
                }
            }
        }
    }

    return nullptr;
}

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
            OwnerComp->OnTargetBecameVisible(Items[Idx].Target, Items[Idx].TeamChannel);
    }
}

void FVisibleActorArray::PreReplicatedRemove(const TArrayView<int32>& RemovedIndices, int32 FinalSize)
{
    if (!OwnerComp)
        return;

    for (int32 Idx : RemovedIndices)
    {
        if (Items.IsValidIndex(Idx))
            OwnerComp->OnTargetBecameHidden(Items[Idx].Target, Items[Idx].TeamChannel);
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
    NewEntry.TeamChannel = Team;
    VisibleActors.MarkItemDirty(NewEntry);

    // Fire locally — PostReplicatedAdd only fires on remote clients
    OnTargetBecameVisible(Target, Team);

    UE_LOG(VisionGameStateComp, Log,
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
    {
        if (Entry.Target == Target && Entry.TeamChannel == Team)
            return true;
    }

    return false;
}

//  Client callbacks

void UVisionGameStateComp::OnTargetBecameVisible(AActor* Target, EVisionChannel Team)
{
    if (!Target)
        return;

    UVisionPlayerStateComp* VisionPS = GetLocalVisionPS(GetWorld());

    if (!VisionPS)
    {
        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("OnTargetBecameVisible >> VisionPS not ready, deferring reveal of %s (team [%s])"),
            *Target->GetName(), *UEnum::GetValueAsString(Team));
        return;
    }

    if (!VisionPS->CanSeeTeam(Team))
    {
        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("OnTargetBecameVisible >> %s skipped — local player cannot see team [%s]"),
            *Target->GetName(), *UEnum::GetValueAsString(Team));
        return;
    }

    UE_LOG(VisionGameStateComp, Log,
        TEXT("OnTargetBecameVisible >> %s visible to team [%s]"),
        *Target->GetName(), *UEnum::GetValueAsString(Team));

    if (UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>())
        VisualComp->SetVisible(true);
}

void UVisionGameStateComp::OnTargetBecameHidden(AActor* Target, EVisionChannel Team)
{
    if (!Target)
        return;

    UVisionPlayerStateComp* VisionPS = GetLocalVisionPS(GetWorld());

    if (!VisionPS)
        return;

    if (!VisionPS->CanSeeTeam(Team))
    {
        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("OnTargetBecameHidden >> %s skipped — local player cannot see team [%s]"),
            *Target->GetName(), *UEnum::GetValueAsString(Team));
        return;
    }

    UE_LOG(VisionGameStateComp, Log,
        TEXT("OnTargetBecameHidden >> %s hidden from team [%s]"),
        *Target->GetName(), *UEnum::GetValueAsString(Team));

    if (UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>())
        VisualComp->SetVisible(false);
}

// -------------------------------------------------------------------------- //
//  Provider registration callback
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::OnProviderRegistered(UVision_VisualComp* NewProvider, EVisionChannel Channel)
{
    if (!NewProvider || !NewProvider->GetOwner() || Channel == EVisionChannel::None)
        return;

    ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem)
        return;

    TArray<UVision_VisualComp*> TeamProviders = Subsystem->GetProvidersForTeam(Channel);

    for (UVision_VisualComp* Existing : TeamProviders)
    {
        if (!Existing || !Existing->GetOwner())
            continue;

        SetActorVisibleToTeam(Existing->GetOwner(), Channel);

        UE_LOG(VisionGameStateComp, Log,
            TEXT("OnProviderRegistered >> Revealed %s to channel [%s]"),
            *Existing->GetOwner()->GetName(), *UEnum::GetValueAsString(Channel));
    }

    // If VisionPS was found via PlayerArray fallback, refresh now.
    // If still null, VisionPlayerStateComp::BeginPlay will call RefreshVisibility
    // on next tick once the PC->PlayerState link is established.
    UVisionPlayerStateComp* VisionPS = GetLocalVisionPS(GetWorld());
    if (VisionPS)
    {
        UE_LOG(VisionGameStateComp, Log,
            TEXT("OnProviderRegistered >> VisionPS found, calling RefreshVisibility"));
        VisionPS->RefreshVisibility();
    }
    else
    {
        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("OnProviderRegistered >> VisionPS still null, deferring to VisionPlayerStateComp::BeginPlay"));
    }
}
