#include "ItemSystem/BaseBoxActor.h"
#include "Net/UnrealNetwork.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
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

    if (HasAuthority() && ItemPool.Num() > 0)
    {
        int32 LootCount = FMath::RandRange(MinLootCount, MaxLootCount);
        for (int32 i = 0; i < LootCount; ++i)
        {
            int32 RandomIndex = FMath::RandRange(0, ItemPool.Num() - 1);
            CurrentLoot.Add(ItemPool[RandomIndex]);
        }
    }
}

void ABaseBoxActor::RemoveItemFromBox(UBaseItemData* ItemToRemove)
{
    // 서버에서만 실행
    if (!HasAuthority()) return;

    if (ItemToRemove && CurrentLoot.Contains(ItemToRemove))
    {
        CurrentLoot.Remove(ItemToRemove);
        // 서버 배열에서 삭제되면 클라이언트로 자동 복제됨
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