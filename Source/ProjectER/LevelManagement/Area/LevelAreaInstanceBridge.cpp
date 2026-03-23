#include "LevelAreaInstanceBridge.h"

#include "GameFramework/GameStateBase.h"
#include "LevelInstance/LevelInstanceActor.h"
#include "LevelManagement/LevelGraphManager/LevelAreaGameStateComp/LevelAreaGameStateComponent.h"
#include "LevelManagement/LevelGraphManager/LevelSequenceManager/LevelSequenceManagerSubsystem.h"
#include "LogHelper/DebugLogHelper.h"

ALevelAreaInstanceBridge::ALevelAreaInstanceBridge()
{
	PrimaryActorTick.bCanEverTick = false;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

void ALevelAreaInstanceBridge::BeginPlay()
{
	Super::BeginPlay();

	UWorld* World = GetWorld();
	if (!World) return;

	ULevelSequenceManagerSubsystem* Sub =
		World->GetSubsystem<ULevelSequenceManagerSubsystem>();
	if (!Sub) return;

	// Register self into the bridge map
	Sub->RegisterBridge(this);

	// Register all owned sequences
	for (const TPair<FName, TObjectPtr<ALevelSequenceActor>>& Pair : Sequences)
	{
		if (Pair.Value)
			Sub->RegisterSequence(Pair.Key, Pair.Value);
	}
}

void ALevelAreaInstanceBridge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	Super::EndPlay(EndPlayReason);

	UWorld* World = GetWorld();
	if (!World) return;

	ULevelSequenceManagerSubsystem* Sub =
		World->GetSubsystem<ULevelSequenceManagerSubsystem>();
	if (!Sub) return;

	Sub->UnregisterBridge(this);

	for (const TPair<FName, TObjectPtr<ALevelSequenceActor>>& Pair : Sequences)
	{
		Sub->UnregisterSequence(Pair.Key);
	}
}

void ALevelAreaInstanceBridge::NotifyHazardStateChanged(EAreaHazardState NewState)
{
	UE_LOG(LevelAreaGraphManagement, Log,
		TEXT("ALevelAreaInstanceBridge >> NotifyHazardStateChanged: Node %d → %s"),
		NodeID, *UEnum::GetValueAsString(NewState));

	OnHazardStateChanged.Broadcast(NewState);
}

ALevelSequenceActor* ALevelAreaInstanceBridge::GetSequenceByName(FName SequenceName) const
{
	TObjectPtr<ALevelSequenceActor> const* Found = Sequences.Find(SequenceName);

	if (!Found)
	{
		UE_LOG(LevelAreaGraphManagement, Warning,
			TEXT("ALevelAreaInstanceBridge >> GetSequenceByName: '%s' not found on Node %d"),
			*SequenceName.ToString(), NodeID);
		return nullptr;
	}

	return *Found;
}

ALevelInstance* ALevelAreaInstanceBridge::GetOwningLevelInstance() const
{
	ULevel* MyLevel = GetLevel();
	if (!MyLevel) return nullptr;

	return Cast<ALevelInstance>(MyLevel->GetOuter());
}
