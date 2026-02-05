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

	// 멀티플레이어 설정
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
		CurrentLoot.Empty();
		if (ItemPool.Num() > 0)
		{
			int32 LootCount = FMath::RandRange(MinLootCount, MaxLootCount);
			for (int32 i = 0; i < LootCount; ++i)
			{
				CurrentLoot.Add(ItemPool[FMath::RandRange(0, ItemPool.Num() - 1)]);
			}
		}
		// 상자 칸수 10칸 유지 (왼쪽 정렬 상태로 nullptr 채움)
		while (CurrentLoot.Num() < 10)
		{
			CurrentLoot.Add(nullptr);
		}
	}
}

// 상호작용(우클릭) 처리
void ABaseBoxActor::PickupItem(APawn* InHandler)
{
	if (!InHandler) return;

	// 서버권한이 있으면 즉시 실행 (능력 부여 및 실행)
	if (HasAuthority())
	{
		UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InHandler);
		if (ASC && OpenAbilityClass)
		{
			FGameplayAbilitySpec Spec(OpenAbilityClass, 1);
			ASC->GiveAbilityAndActivateOnce(Spec);
		}
	}
	else
	{
		// 클라이언트라면 서버에 나 이거 열었다고 보고(RPC 호출)
		Server_PickupItem(InHandler);
	}
}

// 상호작용 서버 RPC 구현
bool ABaseBoxActor::Server_PickupItem_Validate(APawn* InHandler) { return true; }
void ABaseBoxActor::Server_PickupItem_Implementation(APawn* InHandler)
{
	PickupItem(InHandler); // 서버에서 다시 실행하여 HasAuthority() 통과
}

// 아이템 제거 서버 RPC 구현
bool ABaseBoxActor::Server_RemoveItemFromBox_Validate(UBaseItemData* ItemToRemove) { return true; }
void ABaseBoxActor::Server_RemoveItemFromBox_Implementation(UBaseItemData* ItemToRemove)
{
	RemoveItemFromBox(ItemToRemove);
}

void ABaseBoxActor::RemoveItemFromBox(UBaseItemData* ItemToRemove)
{
	if (!HasAuthority() || !ItemToRemove) return;

	// 아이템 삭제 후 배열 정렬 (RemoveSingle은 중간을 지우고 뒤를 당김)
	CurrentLoot.RemoveSingle(ItemToRemove);

	// 부족한 칸 nullptr로 채워서 10칸 유지
	while (CurrentLoot.Num() < 10)
	{
		CurrentLoot.Add(nullptr);
	}

	// 클라이언트 UI 갱신 유도
	OnRep_CurrentLoot();
}

void ABaseBoxActor::OnRep_CurrentLoot()
{
	if (!GetWorld()) return;

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