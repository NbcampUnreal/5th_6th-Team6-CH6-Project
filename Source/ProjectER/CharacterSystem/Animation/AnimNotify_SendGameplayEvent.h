#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h" 
#include "AnimNotify_SendGameplayEvent.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTER_API UAnimNotify_SendGameplayEvent : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	UAnimNotify_SendGameplayEvent();
	
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayAbility")
	FGameplayTag EventTag;
	
	// 이벤트 강도 (예: 1.0 = 일반타격, 2.0 = 치명타 모션 등 활용 가능)
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "GameplayAbility")
	float EventMagnitude = 1.0f;
};
