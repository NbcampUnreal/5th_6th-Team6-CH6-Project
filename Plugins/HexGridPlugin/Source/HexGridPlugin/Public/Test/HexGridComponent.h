#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Hex/HexagonData.h"
#include "Wrappers/HexGridGenerator.h"
#include "HexGridComponent.generated.h"

HEXGRIDPLUGIN_API DECLARE_LOG_CATEGORY_EXTERN(HexGridComponentLog, Log, All)

UCLASS(ClassGroup=(HexGrid), meta=(BlueprintSpawnableComponent))
class HEXGRIDPLUGIN_API UHexGridComponent : public USceneComponent
{
	GENERATED_BODY()

public:
	UHexGridComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	bool bSnapToWorldGrid = false;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	int32 GridRadius = 1;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	float HexSize = 100.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	EHexOrientation Orientation = EHexOrientation::PointyTop;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Hex Grid")
	UStaticMesh* HexMesh;

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void GenerateGrid(FVector WorldOrigin);

	UFUNCTION(BlueprintCallable, Category = "Hex Grid")
	void ClearGrid();

	UFUNCTION(BlueprintPure, Category = "Hex Grid")
	const FHexGridWrapper& GetGridWrapper() const { return CurrentWrapper; }

private:
	UPROPERTY()
	UInstancedStaticMeshComponent* ISMComponent;

	UPROPERTY()
	UHexGridGenerator* Generator;

	FHexGridWrapper CurrentWrapper;

	// MakeLayout has two modes driven by bSnapToWorldGrid:
	// - free:  origin = WorldOrigin (grid centered on actor)
	// - snap:  origin = ZeroVector  (global lattice, caller handles snapping)
	FHexGridLayout MakeLayout(const FVector& WorldOrigin) const;
	void SpawnInstances(const FVector& ResolvedOrigin, const FHexGridLayout& Layout);
};