// Fill out your copyright notice in the Description page of Project Settings.


#include "CurvedWorldSubsystem.h"

#include "FCurvedWorldUtil.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"


//Log
DEFINE_LOG_CATEGORY(CurvedWorldSubsystem);

void UCurvedWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
	
	UE_LOG(CurvedWorldSubsystem, Log,
		TEXT("UCurvedWorldSubsystem::Initialize >> CurvedWorldSubsystem initialized"));
}

void UCurvedWorldSubsystem::Deinitialize()
{
	Super::Deinitialize();

	UE_LOG(CurvedWorldSubsystem, Log,
		TEXT("UCurvedWorldSubsystem::Deinitialize >> CurvedWorldSubsystem deinitialize"));
}

bool UCurvedWorldSubsystem::SetCurvedWorldMPC(UMaterialParameterCollection* InMPC, FName OriginName, FName ForwardName,
	FName RightName, FName UpName, FName CurveXName, FName CurveYName, FName BendWeightName)
{
	if (!InMPC)
	{
		UE_LOG(LogTemp, Warning, TEXT("CurvedWorldSubsystem: Attempted to set null MPC"));
		return false;
	}
	//MPC Setting
	MaterialParameterCollection = InMPC;
	
	UWorld* World = GetWorld();
	if (!World)
	{
		return false;
	}
	// Get MPC instance
	MPCInstance = World->GetParameterCollectionInstance(MaterialParameterCollection);
	if (!MPCInstance)
	{
		UE_LOG(LogTemp, Warning,
			TEXT("UCurvedWorldSubsystem::SyncToMaterialParameterCollection >> Failed to get MPC instance"));
		return false;
	}

	//set the mpc Param Names
	//Location
	MPC_Param_Origin=OriginName;
	//Directions
	
	MPC_Param_ForwardVector = ForwardName;
	MPC_Param_RightVector = RightName;
	MPC_Param_UpVector = UpName;
	
	//curves
	MPC_Param_CurveX=CurveXName;
	MPC_Param_CurveY=CurveYName;
	MPC_Param_BendWeight=BendWeightName;
    
	// Sync current values to the newly set MPC
	SyncToMaterialParameterCollection();
    
	UE_LOG(LogTemp, Log, TEXT("CurvedWorldSubsystem: MPC set to %s"), *InMPC->GetName());
	return true;
}

void UCurvedWorldSubsystem::UpdateCameraParameters(const FVector& InOrigin, const FVector& InForwardVector,
	const FVector& InRightVector, const FVector& InUpVector)
{
	Camera_Origin = InOrigin;
	
	Camera_ForwardVector = InForwardVector;
	Camera_RightVector = InRightVector;
	Camera_UpVector = InUpVector;
	

	// Sync to MPC for materials
	SyncToMaterialParameterCollection();
}

void UCurvedWorldSubsystem::UpdateCurveParameters(float InCurveX, float InCurveY, float InBendWeight)
{
	CurveX = InCurveX;
	CurveY = InCurveY;
	BendWeight = InBendWeight;

	// Sync to MPC for materials
	SyncToMaterialParameterCollection();
}

void UCurvedWorldSubsystem::SyncToMaterialParameterCollection()
{
	if (!MaterialParameterCollection)
	{
		return;
	}

	// Update MPC parameters (adjust parameter names to match your MPC)
	MPCInstance->SetVectorParameterValue(MPC_Param_Origin, Camera_Origin);
	
	MPCInstance->SetVectorParameterValue(MPC_Param_ForwardVector, Camera_ForwardVector);
	MPCInstance->SetVectorParameterValue(MPC_Param_RightVector, Camera_RightVector);
	MPCInstance->SetVectorParameterValue(MPC_Param_UpVector, Camera_UpVector);
	
	MPCInstance->SetScalarParameterValue(MPC_Param_CurveX, CurveX);
	MPCInstance->SetScalarParameterValue(MPC_Param_CurveY, CurveY);
	MPCInstance->SetScalarParameterValue(MPC_Param_BendWeight, BendWeight);
}
