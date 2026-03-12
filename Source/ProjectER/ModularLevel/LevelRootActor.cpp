// Fill out your copyright notice in the Description page of Project Settings.


#include "LevelRootActor.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

DEFINE_LOG_CATEGORY(LevelExtraction);

// Sets default values
ALevelRootActor::ALevelRootActor()
{
	// Set this actor to call Tick() every frame.  You can turn this off to improve performance if you don't need it.
	PrimaryActorTick.bCanEverTick = true;
}



bool ALevelRootActor::IsValidToRun(UWorld*& OutWorld)
{

#if WITH_EDITOR
	if (!GEditor)
	{
		UE_LOG(LevelExtraction, Error,
			TEXT("Invalid GEditor."));
		return false;
	}
#endif

	if (!TargetAsset)
	{
		UE_LOG(LevelExtraction, Error,
			TEXT("TargetAsset is null."));
		return false;
	}

	OutWorld = GetWorld();

	if (!OutWorld)
	{
		UE_LOG(LevelExtraction, Error,
			TEXT("Invalid World."));
		return false;
	}

	return true;
}

void ALevelRootActor::ExtractActorsToAsset()
{
	UWorld* World = nullptr;//catcher

	if (!IsValidToRun(World))
		return;

	TArray<AActor*> Collected;//catcher
	GetAttachedActors(Collected, true); // get even the attached actors of an actor attached with this actor

	const FTransform RootTransform = GetActorTransform();
	const FTransform RootInverse = RootTransform.Inverse();

	for (AActor* Actor : Collected)
	{
		if (!Actor)
			continue;

		FRoomActorData Data;

		Data.ActorClass = Actor->GetClass();

		const FTransform ActorTransform = Actor->GetActorTransform();

		Data.RelativeLocation =
			RootInverse.TransformPosition(ActorTransform.GetLocation());

		Data.RelativeRotation =
			(ActorTransform.GetRotation() * RootInverse.GetRotation()).Rotator();

		TargetAsset->Actors.Add(Data);
	}

	bool bDidMarkedDirty=TargetAsset->MarkPackageDirty();
	if (!bDidMarkedDirty)
	{
		UE_LOG(LevelExtraction, Error,
		TEXT("ALevelRootActor::ExtractActorsToAsset >> Failed to Mark changes."));
	}

	UE_LOG(LevelExtraction, Log,
		TEXT("ALevelRootActor::ExtractActorsToAsset >> Extracted %d actors."),
		TargetAsset->Actors.Num());
}

void ALevelRootActor::LoadActorsFromAsset()
{
	UWorld* World = nullptr;

	if (!IsValidToRun(World))
		return;

	const FTransform RootTransform = GetActorTransform();

	for (const FRoomActorData& Data : TargetAsset->Actors)
	{
		if (!Data.ActorClass.IsValid())
			continue;

		UClass* SpawnClass = Data.ActorClass.LoadSynchronous();

		if (!SpawnClass)
			continue;

		const FVector WorldLocation =
			RootTransform.TransformPosition(Data.RelativeLocation);

		const FRotator WorldRotation =
			(RootTransform.GetRotation() *
			 Data.RelativeRotation.Quaternion()).Rotator();

		FTransform SpawnTransform(WorldRotation, WorldLocation);

		AActor* Spawned =
			World->SpawnActor<AActor>(SpawnClass, SpawnTransform);

		if (Spawned)
		{
			Spawned->AttachToActor(this,
				FAttachmentTransformRules::KeepWorldTransform);
		}
	}

	UE_LOG(LevelExtraction, Log,
		TEXT("Actors loaded from asset."));
}

void ALevelRootActor::ClearAssetActors()	//clear all
{

	if (!TargetAsset)
		return;

	TargetAsset->Actors.Empty();

	bool bDidMarkedDirty=TargetAsset->MarkPackageDirty();
	if (!bDidMarkedDirty)
	{
		UE_LOG(LevelExtraction, Error,
		TEXT("ALevelRootActor::ClearAssetActors >> Failed to Mark changes."));
	}

	UE_LOG(LevelExtraction, Log,
		TEXT("Asset actors cleared."));
}

void ALevelRootActor::UpdateAssetActors()
{
	//Recreate
	ClearAssetActors();
	ExtractActorsToAsset();
}
