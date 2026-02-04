#include "ItemSystem/BaseBoxActor.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "BaseInventoryComponent.h"
#include "ItemSystem/W_LootingPopup.h"
#include "Blueprint/WidgetBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"

ABaseBoxActor::ABaseBoxActor()
{
	BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
	SetRootComponent(BoxMesh);
	BoxMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	bReplicates = true;
}

void ABaseBoxActor::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
	DOREPLIFETIME(ABaseBoxActor, CurrentLoot);
}

void ABaseBoxActor::BeginPlay()
{
	Super::BeginPlay();

	if (HasAuthority())
	{
		// 1. 먼저 10칸을 빈 값(nullptr)으로 채움
		CurrentLoot.Init(nullptr, 10);

		// 2. 랜덤한 개수만큼 랜덤 위치에 배치
		if (ItemPool.Num() > 0)
		{
			int32 LootCount = FMath::RandRange(MinLootCount, MaxLootCount);
			for (int32 i = 0; i < LootCount; ++i)
			{
				int32 RandomSlot = FMath::RandRange(0, 9);
				// 중복 위치 방지 (이미 있으면 패스)
				if (CurrentLoot[RandomSlot] == nullptr)
				{
					CurrentLoot[RandomSlot] = ItemPool[FMath::RandRange(0, ItemPool.Num() - 1)];
				}
			}
		}
	}
}

void ABaseBoxActor::RemoveItemFromBox(UBaseItemData* ItemToRemove)
{
	if (!HasAuthority() || !ItemToRemove) return;

	int32 FoundIndex = CurrentLoot.Find(ItemToRemove);
	if (FoundIndex != INDEX_NONE)
	{
		// [핵심] 배열 크기를 줄이지 않고, 해당 칸만 비움 (위치 유지)
		CurrentLoot[FoundIndex] = nullptr;
		OnRep_CurrentLoot();
	}
}

void ABaseBoxActor::OnRep_CurrentLoot()
{
	TArray<UUserWidget*> FoundWidgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(GetWorld(), FoundWidgets, UW_LootingPopup::StaticClass());

	for (UUserWidget* Widget : FoundWidgets)
	{
		UW_LootingPopup* LootUI = Cast<UW_LootingPopup>(Widget);
		if (LootUI && LootUI->GetTargetBox() == this)
		{
			LootUI->UpdateLootingSlots(CurrentLoot);
		}
	}
}

void ABaseBoxActor::PickupItem(APawn* InHandler)
{
	if (!InHandler || !HasAuthority()) return;
	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InHandler);
	if (ASC && OpenAbilityClass)
	{
		FGameplayAbilitySpec Spec(OpenAbilityClass, 1);
		ASC->GiveAbilityAndActivateOnce(Spec);
	}
}