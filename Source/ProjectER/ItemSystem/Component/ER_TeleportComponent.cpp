#include "ItemSystem/Component/ER_TeleportComponent.h"
#include "CharacterSystem/Player/BasePlayerController.h"

UER_TeleportComponent::UER_TeleportComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UER_TeleportComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UER_TeleportComponent::Interact(ABasePlayerController* PC)
{
	if (PC)
	{
		PC->Client_OpenTeleportUI(GetOwner());
	}
}
