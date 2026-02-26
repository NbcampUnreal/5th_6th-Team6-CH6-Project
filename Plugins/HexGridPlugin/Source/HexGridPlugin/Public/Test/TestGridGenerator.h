// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Test/HexGridComponent.h"
#include "TestGridGenerator.generated.h"

UCLASS()
class HEXGRIDPLUGIN_API ATestGridGenerator : public AActor
{
	GENERATED_BODY()

public:
	ATestGridGenerator();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	UHexGridComponent* HexGridComponent;

public:
	// Convenience passthrough so Blueprint can still call generate/clear on the actor
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void GenerateGrid()
	{
		if (HexGridComponent)
		HexGridComponent->GenerateGrid(GetActorLocation());
	}

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ClearGrid()
	{
		if (HexGridComponent)
			HexGridComponent->ClearGrid();
	}
};
/*
public:
	ATestGridGenerator();

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	UHexGridComponent* HexGridComponent;
	
public:
	// Convenience passthrough so Blueprint can still call generate/clear on the actor
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void GenerateGrid() { if (HexGridComponent) HexGridComponent->GenerateGrid(); }

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ClearGrid() { if (HexGridComponent) HexGridComponent->ClearGrid(); }
	
	// Grid parameters
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	int32 GridRadius = 5;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	UStaticMesh* HexMesh;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	float HexSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	EHexOrientation Orientation = EHexOrientation::PointyTop;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Hex Grid")
	UInstancedStaticMeshComponent* ISMComponent;

	UPROPERTY()
	UHexGridGenerator* Generator;
	
public:
	// --- Public BP-callable functions for each grid/mask type ---
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void GenerateHexGrid_NoMask();
	
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void GenerateHexGrid_Circular(const FVector& CenterLocation, float Radius);
	
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void GenerateHexGrid_Polygon(const TArray<FVector2D>& PolygonPoints);
	
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void GenerateHexGrid_Texture(UTexture2D* MaskTexture, float Threshold);
	
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void GenerateHexGrid_Square(const FVector& CenterLocation, float X_HalfWidth, float Y_HalfHeight);
	
	// clear previous instances
	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ClearHexGrid();
	
private:
	// --- Shared internal function ---
	void GenerateHexGrid_Internal(FHexGrid& Grid, const FVector& GridCenter);
	
	// --- Helper for layout origin offset ---
	FHexGridLayout CreateLayoutAtCenter(const FVector& Center);
};
*/
