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

UENUM(BlueprintType)
enum class ERegionType : uint8
{
	None,
	Region1,
	Region2,
	Region3,
	Region4,
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

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPointActorType PointType = EPointActorType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERegionType RegionType = ERegionType::None;

};
