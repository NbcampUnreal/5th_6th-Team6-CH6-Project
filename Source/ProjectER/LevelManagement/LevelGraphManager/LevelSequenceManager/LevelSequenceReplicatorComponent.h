#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "LevelSequenceReplicatorComponent.generated.h"

UCLASS(ClassGroup=(LevelManagement), meta=(BlueprintSpawnableComponent))
class PROJECTER_API ULevelSequenceReplicatorComponent : public UActorComponent
{
	GENERATED_BODY()

public:

	ULevelSequenceReplicatorComponent();

	/* ---------- Playback ---------- */

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySequence(FName SequenceName);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_StopSequence(FName SequenceName);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PauseSequence(FName SequenceName);

	UFUNCTION(NetMulticast, Reliable)
	void Multicast_ResetSequence(FName SequenceName);

	// Carries server start time — used for both initial play and late join sync
	UFUNCTION(NetMulticast, Reliable)
	void Multicast_PlaySequenceAtTime(FName SequenceName, float ServerStartTime);

	/* ---------- Late Join Sync ---------- */

	// Called by a late joining client to get the correct loop position
	UFUNCTION(BlueprintCallable, Category="SequenceReplicator")
	float GetSequenceLoopPosition(FName SequenceName) const;

	// Returns INDEX_NONE if sequence was never started
	UFUNCTION(BlueprintCallable, Category="SequenceReplicator")
	bool IsSequencePlaying(FName SequenceName) const;

private:

	// Stores server world time at which each sequence was started
	TMap<FName, float> SequenceStartTimes;
};