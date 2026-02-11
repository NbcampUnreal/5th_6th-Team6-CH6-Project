#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ER_PointActor.generated.h"

UENUM(BlueprintType)
enum class EPointActorType : uint8
{
	None,
	SpawnPoint,
	RespawnPoint,
	ObjectPoint,
};


UCLASS()
class PROJECTER_API AER_PointActor : public AActor
{
	GENERATED_BODY()
	
public:	
	AER_PointActor();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;

public:	

	UPROPERTY(BlueprintReadWrite)
	EPointActorType PointType = EPointActorType::None;

};
