#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ItemTestPlayerController.generated.h"

UCLASS()
class PROJECTER_API AItemTestPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AItemTestPlayerController();

protected:
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

	void OnRightClick();

private:
	UPROPERTY()
	AActor* Target;

	void TickInteract();
	void MoveTo(const FVector& Dest);
};