#include "CharacterSystem/Animation/BaseCharacterAnimInstance.h"
#include "CharacterSystem/Character/BaseCharacter.h"
#include "CharacterSystem/GameplayTags/GameplayTags.h" 

#include "AbilitySystemComponent.h" 
#include "AbilitySystemGlobals.h" 
#include "GameFramework/CharacterMovementComponent.h"

UBaseCharacterAnimInstance::UBaseCharacterAnimInstance()
{
	GroundSpeed = 0.0f;
	bIsFalling = false;
	bShouldMove = false;
	bIsDead = false;
	bIsDown = false;
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
	float TargetSpeed = Velocity.Size2D();
	
	// GroundSpeed = Velocity.Size2D();
	// FInterpTo 보간 적용으로 Ground Speed 를 점진적으로 조절
	GroundSpeed = FMath::FInterpTo(GroundSpeed, TargetSpeed, DeltaSeconds, 5.0f); 
	
	// 이동 여부 판별
	bool bHasAcceleration = MovementComponent->GetCurrentAcceleration().SizeSquared() > KINDA_SMALL_NUMBER;
	bShouldMove = (TargetSpeed > 3.f) && bHasAcceleration;

	// 공중에 뜸 판별
	bIsFalling = MovementComponent->IsFalling();
	
	// 사망 혹은 빈사 태그가 있는지 확인
	if (IAbilitySystemInterface* ASCInterface = Cast<IAbilitySystemInterface>(Character))
	{
		if (UAbilitySystemComponent* ASC = ASCInterface->GetAbilitySystemComponent())
		{
			bIsDead = ASC->HasMatchingGameplayTag(ProjectER::State::Life::Death);
			bIsDown = ASC->HasMatchingGameplayTag(ProjectER::State::Life::Down);
		}
	}
}
