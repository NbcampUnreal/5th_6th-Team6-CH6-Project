#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "ER_TeleportComponent.generated.h"

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class PROJECTER_API UER_TeleportComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UER_TeleportComponent();

protected:
	virtual void BeginPlay() override;

public:	
	// Interacted by player controller
	void Interact(class ABasePlayerController* PC);
};
