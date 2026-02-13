#include "ItemSystem/Ability/GA_OpenBox.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "ItemSystem/Actor/BaseBoxActor.h"
#include "ItemSystem/UI/W_LootingPopup.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

#include "CharacterSystem/Player/BasePlayerController.h"

UGA_OpenBox::UGA_OpenBox()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
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
    //TargetBox.Reset();

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

    const float Dist = FVector::Dist(Avatar->GetActorLocation(), Box->GetActorLocation());
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