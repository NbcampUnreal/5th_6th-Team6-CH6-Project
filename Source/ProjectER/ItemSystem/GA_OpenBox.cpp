#include "ItemSystem/GA_OpenBox.h"
#include "Abilities/Tasks/AbilityTask_WaitDelay.h"
#include "ItemSystem/BaseBoxActor.h"
#include "ItemSystem/W_LootingPopup.h"
#include "Blueprint/UserWidget.h"
#include "GameFramework/PlayerController.h"
#include "Engine/OverlapResult.h"
#include "Engine/World.h"

UGA_OpenBox::UGA_OpenBox()
{
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
}

void UGA_OpenBox::ActivateAbility(const FGameplayAbilitySpecHandle Handle, const FGameplayAbilityActorInfo* ActorInfo, const FGameplayAbilityActivationInfo ActivationInfo, const FGameplayEventData* TriggerEventData)
{
	if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
	{
		EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
		return;
	}

	UAbilityTask_WaitDelay* WaitDelayTask = UAbilityTask_WaitDelay::WaitDelay(this, OpenTime);
	if (WaitDelayTask)
	{
		WaitDelayTask->OnFinish.AddDynamic(this, &UGA_OpenBox::OnFinishOpen);
		WaitDelayTask->ReadyForActivation();
	}
}

void UGA_OpenBox::OnFinishOpen()
{
	AActor* Avatar = GetAvatarActorFromActorInfo();
	if (!Avatar)
	{
		EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
		return;
	}

	APlayerController* PC = Cast<APlayerController>(Cast<APawn>(Avatar)->GetController());
	ABaseBoxActor* TargetBox = nullptr;

	// 주변 상자 검색
	TArray<FOverlapResult> OverlapResults;
	FCollisionShape Sphere = FCollisionShape::MakeSphere(300.f);
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(Avatar);

	if (GetWorld()->OverlapMultiByChannel(OverlapResults, Avatar->GetActorLocation(), FQuat::Identity, ECC_Visibility, Sphere, QueryParams))
	{
		for (const FOverlapResult& Result : OverlapResults)
		{
			TargetBox = Cast<ABaseBoxActor>(Result.GetActor());
			if (TargetBox) break;
		}
	}

	// UI 생성 및 초기화
	if (PC && TargetBox && LootWidgetClass)
	{
		UW_LootingPopup* LootWidget = CreateWidget<UW_LootingPopup>(PC, LootWidgetClass);
		if (LootWidget)
		{
			LootWidget->AddToViewport(10);
			LootWidget->UpdateLootingSlots(TargetBox->GetCurrentLoot());

			// 팝업에 상자 정보를 넘겨 거리 체크를 시작하게 함
			LootWidget->InitPopup(TargetBox, 350.f);

			PC->bShowMouseCursor = true;
			FInputModeGameAndUI InputMode;
			InputMode.SetHideCursorDuringCapture(false);
			PC->SetInputMode(InputMode);

			UE_LOG(LogTemp, Warning, TEXT("[GA_OpenBox] 루팅 UI 표시 및 거리 체크 시작: %s"), *TargetBox->GetName());
		}
	}

	EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
}