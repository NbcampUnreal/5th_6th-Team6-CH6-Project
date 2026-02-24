#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "GameplayTagContainer.h"
#include "AnimNotify_SendStateTreeEventTag.generated.h"

UCLASS()
class PROJECTER_API UAnimNotify_SendStateTreeEventTag : public UAnimNotify
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
