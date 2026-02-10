#include "ItemSystem/Component/BaseInventoryComponent.h"
#include "ItemSystem/Data/BaseItemData.h"
#include "Net/UnrealNetwork.h"

UBaseInventoryComponent::UBaseInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	MaxSlots = 20;
	SetIsReplicatedByDefault(true); // 컴포넌트 리플리케이션 활성화
}

void UBaseInventoryComponent::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(UBaseInventoryComponent, InventoryContents);
}

bool UBaseInventoryComponent::AddItem(UBaseItemData* Item)
{
	if (!Item || InventoryContents.Num() >= MaxSlots) return false;

	// 권한이 없으면 서버에 요청
	if (!GetOwner()->HasAuthority())
	{
		Server_AddItem(Item);
		return true;
	}

	InventoryContents.Add(Item);

	if (GEngine)
	{
		FString DebugMsg = FString::Printf(TEXT("가방에 추가됨: %s (현재 %d개)"),
			*Item->ItemName.ToString(), InventoryContents.Num());

		GEngine->AddOnScreenDebugMessage(-1, 3.f, FColor::Cyan, DebugMsg);
	}

	OnInventoryUpdated.Broadcast();
	return true;
}

bool UBaseInventoryComponent::Server_AddItem_Validate(UBaseItemData* InData) { return true; }

void UBaseInventoryComponent::Server_AddItem_Implementation(UBaseItemData* InData)
{
	AddItem(InData);
}

void UBaseInventoryComponent::OnRep_InventoryContents()
{
	OnInventoryUpdated.Broadcast();
}