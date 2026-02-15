// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CurvedWorldSubsystem.generated.h"

/**
 *  like how MPC is used for one true source of the param values, this is used for cpp class.
 *
 *  the value provider (in this case, camera root) update the param values and others use it
 *
 *  
 */

//LOG
WORLDBENDER_API DECLARE_LOG_CATEGORY_EXTERN(CurvedWorldSubsystem, Log, All);

UCLASS()
class WORLDBENDER_API UCurvedWorldSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UFUNCTION(BlueprintCallable, Category = "Curved World")
	bool SetCurvedWorldMPC(
		UMaterialParameterCollection* InMPC,
		FName OriginName,
		FName ForwardName,
		FName RightName,
		FName UpName,
		FName CurveXName,
		FName CurveYName,
		FName BendWeightName);
	
	UFUNCTION(BlueprintCallable, Category = "Curved World")
	void UpdateCameraParameters(
		const FVector& InOrigin,
		const FVector& InForwardVector,
		const FVector& InRightVector,
		const FVector& InUpVector);

	UFUNCTION(BlueprintCallable, Category = "Curved World")
	void UpdateCurveParameters(
		float InCurveX,
		float InCurveY,
		float InBendWeight);

private:
	// Sync parameters to Material Parameter Collection
	void SyncToMaterialParameterCollection();

public://variables

	//Param Values
	//continuous update
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FVector Camera_Origin = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FVector Camera_ForwardVector = FVector(1,0,0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FVector Camera_RightVector = FVector(0,1,0);
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FVector Camera_UpVector = FVector(0,0,1);

	//update when only needed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	float CurveX = 0.001f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	float CurveY = 0.001f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	float BendWeight = 0.05f;
	
protected:
	//MPC
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	TObjectPtr<UMaterialParameterCollection> MaterialParameterCollection;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollectionInstance>MPCInstance;
	
	//=== Param Names ===//
	
	//CurveOrigin Location
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_Origin=NAME_None;
	
	//Directions
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_ForwardVector=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_RightVector=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_UpVector=NAME_None;
	
	//Curve Values
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_CurveX=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_CurveY=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_BendWeight=NAME_None;
};
