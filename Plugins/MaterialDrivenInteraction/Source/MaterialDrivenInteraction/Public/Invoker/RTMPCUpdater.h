#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "RTMPCUpdater.generated.h"

struct FRTPoolEntry;
class URTPoolManager;
class UMaterialParameterCollection;
class UMaterialParameterCollectionInstance;
class UMaterialInstanceDynamic;
class UTextureRenderTarget2D;

static constexpr int32 FRT_MAX_POOL_SLOTS = 16;

UCLASS(ClassGroup = "Foliage RT", meta = (BlueprintSpawnableComponent))
class MATERIALDRIVENINTERACTION_API URTMPCUpdater : public UActorComponent
{
	GENERATED_BODY()

public:

	URTMPCUpdater();

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
							   FActorComponentTickFunction* ThisTickFunction) override;

	/** MPC asset receiving cell-origin vectors and cell-size each tick. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Foliage RT|MPC")
	TObjectPtr<UMaterialParameterCollection> ParameterCollection;

	/** Returns the MPC instance for this world. */
	UFUNCTION(BlueprintPure, Category = "Foliage RT|MPC")
	UMaterialParameterCollectionInstance* GetMPCInstance() const;

private:

	void PushVectorDataToMPC();
	void BuildParameterNames();

	UFUNCTION()
	void OnCellAssigned(FIntPoint CellIndex, int32 SlotIndex);

	UFUNCTION()
	void OnCellReclaimed(FIntPoint CellIndex, int32 SlotIndex);

	UPROPERTY()
	TObjectPtr<URTPoolManager> PoolManager;

	TArray<FName> ParamName_SlotData;
	TArray<FName> ParamName_ImpulseRT;
	TArray<FName> ParamName_ContinuousRT;

	static const FName PN_CellSize;
	static const FName PN_ActiveSlotCount;

	/*UPROPERTY(VisibleAnywhere, Category = "Foliage RT|Debug")
	TArray<FRTPoolEntry> DebugPool;*/
};