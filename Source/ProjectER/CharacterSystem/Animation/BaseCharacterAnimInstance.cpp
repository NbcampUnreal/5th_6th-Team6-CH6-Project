#include "CharacterSystem/Animation/BaseCharacterAnimInstance.h"
#include "CharacterSystem/Character/BaseCharacter.h"

#include "GameFramework/CharacterMovementComponent.h"

UBaseCharacterAnimInstance::UBaseCharacterAnimInstance()
{
	GroundSpeed = 0.0f;
	bIsFalling = false;
	bShouldMove = false;
}

void UBaseCharacterAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();
	
	Character = Cast<ABaseCharacter>(TryGetPawnOwner());
	if (IsValid(Character) == true)
	{
		MovementComponent = Character->GetCharacterMovement();
	}
}

void UBaseCharacterAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);
	
	if (!Character || !MovementComponent)
	{
		return;
	}

	// 속력 계산 (Z축 제외, XY평면 속도)
	FVector Velocity = MovementComponent->Velocity;
	GroundSpeed = Velocity.Size2D();
	
	// 이동 여부 판별
	bool bHasAcceleration = MovementComponent->GetCurrentAcceleration().SizeSquared() > KINDA_SMALL_NUMBER;
	bShouldMove = (GroundSpeed > 3.f) && bHasAcceleration;

	// 공중에 뜸 판별
	bIsFalling = MovementComponent->IsFalling();
}
