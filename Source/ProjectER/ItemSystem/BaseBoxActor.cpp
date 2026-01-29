#include "ItemSystem/BaseBoxActor.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"

ABaseBoxActor::ABaseBoxActor()
{
    BoxMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BoxMesh"));
    SetRootComponent(BoxMesh);

    // 마우스 클릭이 인식되도록 콜리전 설정
    BoxMesh->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

    bReplicates = true;
}

void ABaseBoxActor::BeginPlay()
{
    Super::BeginPlay();

    // 서버에서만 아이템을 미리 생성
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

void ABaseBoxActor::PickupItem(APawn* InHandler)
{
    if (!InHandler) return;

    if (!HasAuthority()) return;

    UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(InHandler);
    if (ASC && OpenAbilityClass)
    {
        // 테스트용 임시 능력 부여 및 실행 ( 테스트용으로 추후에 캐릭터에 ASC 추가 필요 )
        FGameplayAbilitySpec Spec(OpenAbilityClass, 1);

        ASC->GiveAbilityAndActivateOnce(Spec);

        UE_LOG(LogTemp, Warning, TEXT("상자: 능력 부여 및 즉시 실행"));
    }
}