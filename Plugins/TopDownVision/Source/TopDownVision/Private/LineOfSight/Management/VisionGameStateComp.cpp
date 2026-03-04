// Fill out your copyright notice in the Description page of Project Settings.

#include "LineOfSight/Management/VisionGameStateComp.h"

#include "GameFramework/PlayerState.h"
#include "LineOfSight/Management/VisionPlayerStateComp.h"
#include "LineOfSight/Management/Subsystem/LOSVisionSubsystem.h"
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
            OwnerComp->OnTargetBecameVisible(Items[Idx].Target, Items[Idx].TeamChannel);
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

            UE_LOG(VisionGameStateComp, Log,
             TEXT("ClearActorVisibleToTeam >> %s hidden from team [%s]"),
             *Target->GetName(), *UEnum::GetValueAsString(Team));
            return;
        }
    }

    OnTargetBecameHidden(Target, Team);
    
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

// -------------------------------------------------------------------------- //
//  Client callbacks — fired by FastArray
// -------------------------------------------------------------------------- //

void UVisionGameStateComp::OnTargetBecameVisible(AActor* Target, EVisionChannel TeamID)
{
    if (!Target)
        return;

    APlayerController* PC = GEngine->GetFirstLocalPlayerController(GetWorld());
    UVisionPlayerStateComp* VisionPS = PC && PC->PlayerState
        ? PC->PlayerState->FindComponentByClass<UVisionPlayerStateComp>()
        : nullptr;

    // PlayerState not ready yet (early BeginPlay) — fall back to direct channel check
    if (!VisionPS)
    {
        if (UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>())
        {
            const bool bSameTeam =
                VisualComp->GetVisionChannel() == TeamID ||
                VisualComp->GetVisionChannel() == EVisionChannel::SharedVision;

            if (bSameTeam)
                VisualComp->SetVisible(true);
        }
        return;
    }

    if (!VisionPS->CanSeeTeam((EVisionChannel)TeamID))
    {
        UE_LOG(VisionGameStateComp, Verbose,
            TEXT("OnTargetBecameVisible >> %s skipped — local player cannot see team [%s]"),
            *Target->GetName(), *UEnum::GetValueAsString(TeamID));
        return;
    }

    UE_LOG(VisionGameStateComp, Log,
        TEXT("OnTargetBecameVisible >> %s visible to team [%s]"),
        *Target->GetName(), *UEnum::GetValueAsString(TeamID));

    if (UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>())
        VisualComp->SetVisible(true);
}

void UVisionGameStateComp::OnTargetBecameHidden(AActor* Target, EVisionChannel TeamID)
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
            TEXT("OnTargetBecameHidden >> %s skipped — local player cannot see team [%s]"),
            *Target->GetName(), *UEnum::GetValueAsString(TeamID));
        return;
    }

    UE_LOG(VisionGameStateComp, Log,
        TEXT("OnTargetBecameHidden >> %s hidden from team [%s]"),
        *Target->GetName(), *UEnum::GetValueAsString(TeamID));

    if (UVision_VisualComp* VisualComp = Target->FindComponentByClass<UVision_VisualComp>())
        VisualComp->SetVisible(false);
}

void UVisionGameStateComp::OnProviderRegistered(UVision_VisualComp* NewProvider, EVisionChannel Channel)
{
    if (!NewProvider || !NewProvider->GetOwner() || Channel == EVisionChannel::None)
        return;

    ULOSVisionSubsystem* Subsystem = GetWorld()->GetSubsystem<ULOSVisionSubsystem>();
    if (!Subsystem)
        return;

    // Get all providers already registered on this channel (includes the new one)
    TArray<UVision_VisualComp*> TeamProviders = Subsystem->GetProvidersForTeam(Channel);

    for (UVision_VisualComp* Existing : TeamProviders)
    {
        if (!Existing || !Existing->GetOwner())
            continue;

        // Reveal existing providers to the new provider's team
        SetActorVisibleToTeam(Existing->GetOwner(), Channel);

        UE_LOG(VisionGameStateComp, Log,
            TEXT("OnProviderRegistered >> Revealed %s to channel [%s]"),
            *Existing->GetOwner()->GetName(), *UEnum::GetValueAsString(Channel));
    }
}

