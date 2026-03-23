#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "LevelManagement/Requirements/LevelAreaData.h"
#include "LevelSequence.h"
#include "LevelAreaInstanceBridge.generated.h"

class ALevelSequenceActor;
class ALevelInstance;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(
	FOnLevelAreaHazardStateChanged,
	EAreaHazardState, NewState);

USTRUCT(BlueprintType)
struct FLevelSequenceList
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Sequences")
	TArray<TObjectPtr<ALevelSequenceActor >> Sequences;
};

UCLASS()
class PROJECTER_API ALevelAreaInstanceBridge : public AActor
{
	GENERATED_BODY()

public:

	ALevelAreaInstanceBridge();

protected:
	virtual void BeginPlay() override;

	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
public:

	// Must match the NodeID of the corresponding ALevelAreaActor
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Bridge")
	int32 NodeID = INDEX_NONE;

	// Sequences to play 
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Bridge|Sequences")
	TMap<FName, TObjectPtr<ALevelSequenceActor>> Sequences;

	// Bind in Blueprint to react to hazard state changes
	UPROPERTY(BlueprintAssignable, Category="Bridge|Events")
	FOnLevelAreaHazardStateChanged OnHazardStateChanged;

	// Called by ULevelAreaGameStateComponent after hazards are applied
	UFUNCTION(BlueprintCallable, Category="Bridge|Hazard")
	void NotifyHazardStateChanged(EAreaHazardState NewState);

	// Get a sequence actor by name for Blueprint playback
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Bridge|Sequences")
	ALevelSequenceActor* GetSequenceByName(FName SequenceName) const;

	// Finds the ALevelInstance that contains this bridge at runtime
	UFUNCTION(BlueprintCallable, BlueprintPure, Category="Bridge")
	ALevelInstance* GetOwningLevelInstance() const;
};