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
		FName RightName,
		FName ForwardName,
		FName CurveXName,
		FName CurveYName,
		FName BendWeightName);
	
	UFUNCTION(BlueprintCallable, Category = "Curved World")
	void UpdateCameraParameters(
		const FVector& InOrigin,
		const FVector& InRightVector,
		const FVector& InForwardVector);

	UFUNCTION(BlueprintCallable, Category = "Curved World")
	void UpdateCurveParameters(
		float InCurveX,
		float InCurveY,
		float InBendWeight);

	UFUNCTION(BlueprintPure, Category = "Curved World")
	FVector CalculateOffset(const FVector& WorldPos) const;

	UFUNCTION(BlueprintCallable, Category = "Curved World")
	void CalculateTransform(const FVector& WorldPos, FVector& OutOffset, FRotator& OutRotation) const;

private:
	// Sync parameters to Material Parameter Collection
	void SyncToMaterialParameterCollection();

protected://variables

	//Param Values
	//continuous update
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FVector Origin = FVector::ZeroVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FVector RightVector = FVector::RightVector;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FVector ForwardVector = FVector::ForwardVector;

	//update when only needed
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	float CurveX = 0.001f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	float CurveY = 0.001f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	float BendWeight = 0.05f;

	//MPC
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	TObjectPtr<UMaterialParameterCollection> MaterialParameterCollection;
	UPROPERTY(Transient)
	TObjectPtr<UMaterialParameterCollectionInstance>MPCInstance;
	//Param Names
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_Origin=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_RightVector=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_ForwardVector=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_CurveX=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_CurveY=NAME_None;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Curved World")
	FName MPC_Param_BendWeight=NAME_None;
};
