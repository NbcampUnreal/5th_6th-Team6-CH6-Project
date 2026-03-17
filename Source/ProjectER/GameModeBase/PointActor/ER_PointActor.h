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

//16개
UENUM(BlueprintType)
enum class ERegionType : uint8
{
	None = 0,
	Alley = 1,
	Temple = 2,
	Downtown = 3,
	Pond = 4,
	Hospital = 5,
	ArcheryRange = 6,
	School = 7,
	Hotel = 8,
	Forest = 9,
	Cathedral = 10,
	Cemetery = 11,
	Factory = 12,
	Dock = 13,
	Uptown = 14,
	Beach = 15,
	ResearchCenter = 16
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

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

	// ✅ Rep 되면 클라에서 데칼 토글
	UFUNCTION()
	void OnRep_Selected();

public:
	UFUNCTION(BlueprintCallable)
	void SetSelectedVisual(bool bOn);

public:	

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EPointActorType PointType = EPointActorType::None;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	ERegionType RegionType = ERegionType::None;


private:
	// 루트
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<USceneComponent> Root;

	// ✅ 선택 표시 데칼
	UPROPERTY(VisibleAnywhere)
	TObjectPtr<UDecalComponent> SelectedDecal;

	// ✅ 선택 상태(Rep)
	UPROPERTY(ReplicatedUsing = OnRep_Selected)
	bool bSelected = false;
};
