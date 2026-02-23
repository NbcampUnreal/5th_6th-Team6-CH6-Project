#include "GameModeBase/Subsystem/Phase/ER_PhaseSubsystem.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"

void UER_PhaseSubsystem::StartPhaseTimer(AER_GameState& GS, float Duration)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (World->GetNetMode() == NM_Client)
        return;

    ClearPhaseTimer();
    GS.PhaseServerTime = GS.GetServerWorldTimeSeconds();
    GS.PhaseDuration = Duration;
    GS.ForceNetUpdate();

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimer,
        this,
        &UER_PhaseSubsystem::OnPhaseTimeUp,
        Duration,
        false
    );
}

void UER_PhaseSubsystem::StartNoticeTimer(float Duration)
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (World->GetNetMode() == NM_Client)
        return;

    float HalfDuration = Duration / 2;

    GetWorld()->GetTimerManager().SetTimer(
        NoticeTimer,
        this,
        &UER_PhaseSubsystem::OnNoticeTimeUp,
        HalfDuration,
        false
    );
}

void UER_PhaseSubsystem::ClearPhaseTimer()
{
    if (GetWorld()->GetNetMode() == NM_Client)
        return;

    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(PhaseTimer);
    }
}

void UER_PhaseSubsystem::OnPhaseTimeUp()
{
    UWorld* World = GetWorld();
    if (!World) 
        return;

    if (World->GetNetMode() == NM_Client)
        return;

    if (AGameModeBase* GM = World->GetAuthGameMode())
    {
        Cast<AER_InGameMode>(GM)->HandlePhaseTimeUp();
    }
}

void UER_PhaseSubsystem::OnNoticeTimeUp()
{
    UWorld* World = GetWorld();
    if (!World)
        return;

    if (World->GetNetMode() == NM_Client)
        return;

    if (AGameModeBase* GM = World->GetAuthGameMode())
    {
        Cast<AER_InGameMode>(GM)->HandleObjectNoticeTimeUp();
    }
}
