#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "BaseCharacterAnimInstance.generated.h"

class ABaseCharacter;
class UCharacterMovementComponent;

UCLASS()
class PROJECTER_API UBaseCharacterAnimInstance : public UAnimInstance
{
	GENERATED_BODY()
	
public:
	UBaseCharacterAnimInstance();
	
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

protected:
	UPROPERTY(BlueprintReadOnly, Category = "Ref")
	TObjectPtr<ABaseCharacter> Character;

	UPROPERTY(BlueprintReadOnly, Category = "Ref")
	TObjectPtr<UCharacterMovementComponent> MovementComponent;
	
	// 이동 속도 (걷기/뛰기 블렌딩용)
	UPROPERTY(BlueprintReadOnly, Category = "State")
	float GroundSpeed;

	// 움직임 여부 
	UPROPERTY(BlueprintReadOnly, Category = "State")
	uint8 bShouldMove : 1;

	// 공중에 뜸 여부 (에어본 CC기 등)
	UPROPERTY(BlueprintReadOnly, Category = "State")
	uint8 bIsFalling : 1;
	
	// 사망 상태 확인용 변수
	UPROPERTY(BlueprintReadOnly, Category = "State")
	uint8 bIsDead : 1;
	
	// 사망 상태 확인용 변수
	UPROPERTY(BlueprintReadOnly, Category = "State")
	uint8 bIsDown : 1;
};
