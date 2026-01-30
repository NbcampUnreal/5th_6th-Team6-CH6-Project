#include "ItemSystem/W_LootingPopup.h"
#include "ItemSystem/BaseItemData.h"
#include "GameFramework/PlayerController.h"

void UW_LootingPopup::InitPopup(AActor* InTargetBox, float InMaxDistance)
{
    TargetBox = InTargetBox;
    MaxDistance = InMaxDistance;
}

void UW_LootingPopup::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
    Super::NativeTick(MyGeometry, InDeltaTime);

    if (TargetBox)
    {
        APawn* OwningPawn = GetOwningPlayerPawn();
        if (OwningPawn)
        {
            float CurrentDistance = OwningPawn->GetDistanceTo(TargetBox);

            // 거리가 멀어지면 팝업만 제거
            if (CurrentDistance > MaxDistance)
            {
                // [수정] PC 조작 코드 삭제: 팝업이 사라져도 마우스 커서 설정은 유지됨
                RemoveFromParent();

                UE_LOG(LogTemp, Warning, TEXT("[W_LootingPopup] 거리 이탈로 인한 UI 제거"));
            }
        }
    }
}