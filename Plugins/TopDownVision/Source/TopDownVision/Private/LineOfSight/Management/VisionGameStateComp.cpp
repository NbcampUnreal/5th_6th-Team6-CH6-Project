// Fill out your copyright notice in the Description page of Project Settings.


#include "LineOfSight/Management/VisionGameStateComp.h"
#include "Net/UnrealNetwork.h"
#include "Engine/World.h"
#include "LineOfSight/Management/VisionProviderInterface.h"

UVisionGameStateComp::UVisionGameStateComp()
{
	PrimaryComponentTick.bCanEverTick = false;
	SetIsReplicatedByDefault(true);
}

void UVisionGameStateComp::BeginPlay()
{
	Super::BeginPlay();
	
}

void UVisionGameStateComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	//TeamVisionData.Empty();
	Super::EndPlay(EndPlayReason);
}

void UVisionGameStateComp::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
	Super::GetLifetimeReplicatedProps(OutLifetimeProps);
}

void UVisionGameStateComp::RegisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider)
{
}

void UVisionGameStateComp::UnregisterVisionProvider(TScriptInterface<IVisionProviderInterface> Provider)
{
}
