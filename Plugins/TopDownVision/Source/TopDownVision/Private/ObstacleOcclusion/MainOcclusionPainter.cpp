// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleOcclusion/MainOcclusionPainter.h"


// Sets default values for this component's properties
UMainOcclusionPainter::UMainOcclusionPainter()
{
	// Set this component to be initialized when the game starts, and to be ticked every frame.  You can turn these features
	// off to improve performance if you don't need them.
	PrimaryComponentTick.bCanEverTick = true;

	// ...
}


// Called when the game starts
void UMainOcclusionPainter::BeginPlay()
{
	Super::BeginPlay();

	// ...
	
}

void UMainOcclusionPainter::UpdateOcclusionRT()
{
}

void UMainOcclusionPainter::DrawProviderArea()
{
}



