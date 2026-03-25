#include "ItemSystem/Ability/GA_OpenBox.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "ItemSystem/Actor/BaseBoxActor.h"
#include "ItemSystem/UI/W_LootingPopup.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

#include "CharacterSystem/Player/BasePlayerController.h"
#include "ItemSystem/Component/LootableComponent.h"

namespace
{
    static float GetDistanceToActorBounds2D(const AActor* TargetActor, const FVector& FromLocation)
    {
        if (!IsValid(TargetActor))
        {
            return TNumericLimits<float>::Max();
        }

        const FBox Bounds = TargetActor->GetComponentsBoundingBox(true);
        if (!Bounds.IsValid)
        {
            return FVector::Dist2D(FromLocation, TargetActor->GetActorLocation());
        }

        const FVector ClosestPoint(
            FMath::Clamp(FromLocation.X, Bounds.Min.X, Bounds.Max.X),
            FMath::Clamp(FromLocation.Y, Bounds.Min.Y, Bounds.Max.Y),
            FMath::Clamp(FromLocation.Z, Bounds.Min.Z, Bounds.Max.Z)
        );

        return FVector::Dist2D(FromLocation, ClosestPoint);
    }
}

UGA_OpenBox::UGA_OpenBox()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;

    AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Event.Interact.OpenBox")));
}

void UGA_OpenBox::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    UE_LOG(LogTemp, Log, TEXT("GA_OpenBox START"));

    const AActor* Box = nullptr;
    if (TriggerEventData)
    {
        Box = TriggerEventData->Target;
    }

    if (!Box)
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    TargetBox = Box;

    //  LootableComponent의 OnLootDepleted 델리게이트 바인딩
    if (ULootableComponent* LootComp = Box->FindComponentByClass<ULootableComponent>())
    {
        LootComp->OnLootDepleted.AddUObject(this, &UGA_OpenBox::OnBoxDepleted);
    }

    if (ABasePlayerController* PC = Cast<ABasePlayerController>(ActorInfo->PlayerController.Get()))
    {
        PC->Client_OpenLootUI(Box);
    }

    // 거리 체크/종료까지 GA가 맡을 거면 Task/Timer로 유지
    StartDistanceCheck(ActorInfo);
}

void UGA_OpenBox::EndAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, bool bReplicateEndAbility, bool bWasCancelled)
{
    StopDistanceCheck();

    // 델리게이트 언바인딩
    if (const AActor* Box = TargetBox.Get())
    {
        if (ULootableComponent* LootComp = Box->FindComponentByClass<ULootableComponent>())
        {
            LootComp->OnLootDepleted.RemoveAll(this);
        }
    }

    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

void UGA_OpenBox::StartDistanceCheck(const FGameplayAbilityActorInfo* ActorInfo)
{
    if (!ActorInfo)
        return;

    UE_LOG(LogTemp, Log, TEXT("StartDistanceCheck Start"));

    UWorld* World = GetWorld();
    if (!World)
        return;

    World->GetTimerManager().SetTimer(
        DistanceCheckTimer,
        this,
        &UGA_OpenBox::TickDistanceCheck,
        DistanceCheckInterval,
        true
    );
}

void UGA_OpenBox::StopDistanceCheck()
{
    if (UWorld* World = GetWorld())
    {
        UE_LOG(LogTemp, Log, TEXT("StopDistanceCheck Start"));
        World->GetTimerManager().ClearTimer(DistanceCheckTimer);
    }
}

void UGA_OpenBox::TickDistanceCheck()
{
    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (!ActorInfo)
        return;

    AActor* Avatar = ActorInfo->AvatarActor.Get();
    const AActor* Box = TargetBox.Get();

    if (!Avatar || !Box)
    {
        if (ABasePlayerController* PC = Cast<ABasePlayerController>(ActorInfo->PlayerController.Get()); PC && PC->IsLocalController())
        {
            PC->Client_CloseLootUI(); // Box가 null이면 Close 함수에서 null 처리해도 됨
        }
        UE_LOG(LogTemp, Log, TEXT("TickDistanceCheck !Avatar || !Box"));
        StopDistanceCheck();
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return;
    }

    const float Dist = GetDistanceToActorBounds2D(Box, Avatar->GetActorLocation());
    if (Dist > MaxLootDistance)
    {
        if (ABasePlayerController* PC = Cast<ABasePlayerController>(ActorInfo->PlayerController.Get()))
        {
            PC->Client_CloseLootUI();
        }
        UE_LOG(LogTemp, Log, TEXT("TickDistanceCheck Dist > MaxLootDistance"));
        StopDistanceCheck();
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
    }
}
// 루팅 완료 콜백
void UGA_OpenBox::OnBoxDepleted()
{
    UE_LOG(LogTemp, Log, TEXT("[GA_OpenBox] OnBoxDepleted - Box is now empty!"));

    const FGameplayAbilityActorInfo* ActorInfo = GetCurrentActorInfo();
    if (!ActorInfo)
        return;

    // UI 닫기
    if (ABasePlayerController* PC = Cast<ABasePlayerController>(ActorInfo->PlayerController.Get()))
    {
        PC->Client_CloseLootUI();
    }

    // Ability 종료
    StopDistanceCheck();
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}