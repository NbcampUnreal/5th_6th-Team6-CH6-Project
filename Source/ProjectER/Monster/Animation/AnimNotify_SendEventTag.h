#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_SendEventTag.generated.h"

UCLASS()
class PROJECTER_API UAnimNotify_SendEventTag : public UAnimNotify
{
	GENERATED_BODY()
	
public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "GAS|Tag")
	FGameplayTag EventTag;

	virtual void Notify(
		USkeletalMeshComponent* MeshComp, 
		UAnimSequenceBase* Animation, const 
		FAnimNotifyEventReference& EventReference
	)override;
};
