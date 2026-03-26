#include "GameModeBase/Subsystem/Phase/ER_PhaseSubsystem.h"
#include "GameModeBase/State/ER_GameState.h"
#include "GameModeBase/GameMode/ER_InGameMode.h"
#include "GameModeBase/State/ER_PlayerState.h"
#include "LevelManagement/LevelGraphManager/LevelAreaGameStateComp/LevelAreaGameStateComponent.h"
#include "LevelManagement/LevelAreaTrackerComponent.h"
#include "Kismet/GameplayStatics.h"

// GAS Includes
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "CharacterSystem/GAS/AttributeSet/BaseAttributeSet.h"

void UER_PhaseSubsystem::StartPhaseTimer(AER_GameState& GS, float Duration)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    ClearPhaseTimer();
    GS.PhaseServerTime = GS.GetServerWorldTimeSeconds();
    GS.PhaseDuration = Duration;
    GS.ForceNetUpdate();

    // 1초 주기로 사용될 판정 로직을 위해 GameState를 캐싱합니다.
    CachedGameState = &GS;

    GetWorld()->GetTimerManager().SetTimer(
        PhaseTimer,
        this,
        &UER_PhaseSubsystem::OnPhaseTimeUp,
        Duration,
        false
    );

    GetWorld()->GetTimerManager().SetTimer(
        PeriodicCheckTimer,
        this,
        &UER_PhaseSubsystem::OnPeriodicCheckTick,
        1.0f,
        true
    );
}

void UER_PhaseSubsystem::StartNoticeTimer(float Duration)
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

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
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    World->GetTimerManager().ClearTimer(PhaseTimer);
    World->GetTimerManager().ClearTimer(PeriodicCheckTimer);
}

void UER_PhaseSubsystem::OnPhaseTimeUp()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    // 다음 페이즈 세팅(HandlePhaseTimeUp)이 시작되기 전에 기존 타이머를 먼저 비워줍니다.
    // 이 코드가 HandlePhaseTimeUp() 뒤에 있으면, 새로 갱신된(SetTimer) 타이머를 바로 지워버리게 됩니다.
    World->GetTimerManager().ClearTimer(PeriodicCheckTimer);

    if (AER_InGameMode* GM = Cast<AER_InGameMode>(World->GetAuthGameMode()))
    {
        GM->HandlePhaseTimeUp();
    }
}

void UER_PhaseSubsystem::OnNoticeTimeUp()
{
    UWorld* World = GetWorld();
    if (!World || World->GetNetMode() == NM_Client)
    {
        return;
    }

    if (AER_InGameMode* GM = Cast<AER_InGameMode>(World->GetAuthGameMode()))
    {
        GM->HandleObjectNoticeTimeUp();
    }
}

void UER_PhaseSubsystem::OnPeriodicCheckTick()
{
    const UWorld* World = GetWorld();
    if (World == nullptr || World->GetNetMode() == NM_Client)
    {
        return;
    }

    if (AER_GameState* ERGS = CachedGameState.Get())
    {
        ULevelAreaGameStateComponent* AreaGSComp = ERGS->GetComponentByClass<ULevelAreaGameStateComponent>();
        AER_InGameMode* GM = Cast<AER_InGameMode>(World->GetAuthGameMode());

        if (AreaGSComp == nullptr || GM == nullptr)
        {
            return;
        }

        for (APlayerState* PS : ERGS->PlayerArray)
        {
            if (AER_PlayerState* ERPS = Cast<AER_PlayerState>(PS))
            {
                if (APawn* Pawn = ERPS->GetPawn())
                {
                    if (ULevelAreaTrackerComponent* Tracker = Pawn->FindComponentByClass<ULevelAreaTrackerComponent>())
                    {
                        // 활성화되는 금지구역 수량 제한 (Phase * HazardsPerPhase)
                        if (Tracker->CurrentHazardState == EAreaHazardState::Hazard)
                        {
                            // apply damage
                            if (ERPS->CurrentRestrictedTime <= 1.0f && !ERPS->bIsDead)
                            {
                                if (UAbilitySystemComponent* ASC = ERPS->GetAbilitySystemComponent())
                                {
                                    // GAS 방식을 사용하여 IncomingDamage(메타 어트리뷰트)에 즉사급 데미지를 가합니다.
                                    // 이를 통해 BaseAttributeSet의 PostGameplayEffectExecute가 정상적으로 호출되며 체력 차감 및 사망 프로세스를 탑니다.
                                    UGameplayEffect* DamageEffect = NewObject<UGameplayEffect>(GetTransientPackage(), FName(TEXT("HazardDamage")));
                                    DamageEffect->DurationPolicy = EGameplayEffectDurationType::Instant;

                                    FGameplayModifierInfo ModInfo;
                                    ModInfo.ModifierMagnitude = FScalableFloat(999999.0f);
                                    ModInfo.ModifierOp = EGameplayModOp::Additive;
                                    ModInfo.Attribute = UBaseAttributeSet::GetIncomingDamageAttribute();
                                    DamageEffect->Modifiers.Add(ModInfo);

                                    FGameplayEffectContextHandle EffectContext = ASC->MakeEffectContext();
                                    // 환경(Phase) 데미지가 자신(Player)의 데미지로 판정되어 자살로 인해 킬이 오르는 것을 막기 위해 Instigator를 변경
                                    EffectContext.AddInstigator(ERGS, ERGS);

                                    ASC->ApplyGameplayEffectToSelf(DamageEffect, 1.0f, EffectContext);
                                    ERPS->CurrentRestrictedTime = 0.0f;
                                }
                            }
                            if (ERPS->CurrentRestrictedTime >= 1.0f)
                            {
                                ERPS->CurrentRestrictedTime -= 1.0f;
                                ERPS->setUI_RestrictedTime();   // << UI 처리
                                UE_LOG(LogTemp, Log, TEXT("[PS] CurrentRestrictedTime: %f"), ERPS->CurrentRestrictedTime);
                            }
                            else
                            {
                                ERPS->setUI_RestrictedTime(); // 0초처리용
                            }
                        }

                    }
                }
            }
        }
    }
}
