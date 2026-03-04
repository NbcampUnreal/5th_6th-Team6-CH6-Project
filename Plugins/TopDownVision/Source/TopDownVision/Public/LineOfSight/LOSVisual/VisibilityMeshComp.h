// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "VisibilityMeshComp.generated.h"

TOPDOWNVISION_API DECLARE_LOG_CATEGORY_EXTERN(VisibilityMeshComp, Log, All);

class UMaterialInstanceDynamic;
class USkeletalMeshComponent;
class UStaticMeshComponent;


UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TOPDOWNVISION_API UVisibilityMeshComp : public UActorComponent
{
    GENERATED_BODY()

public:
    UVisibilityMeshComp();

public:
    /** Called in-editor to find tagged mesh components on the actor
     *  and populate MeshTargets. */
    UFUNCTION(BlueprintCallable, Category="Visibility")
    void FindMeshesByTag();

    /** Manually register a mesh outside the actor's direct hierarchy.
     *  Call before Initialize. */
    void AddMesh(TSoftObjectPtr<USkeletalMeshComponent> Mesh);
    void AddMesh(TSoftObjectPtr<UStaticMeshComponent> Mesh);

    /** Called by VisionTargetComp::BeginPlay to create MIDs from MeshTargets. */
    void Initialize();

    /** Called every fade tick by VisionTargetComp with the current alpha. */
    UFUNCTION(BlueprintCallable, Category="Visibility")
    void UpdateVisibility(float Alpha);

protected:
    /** Tag to filter mesh components on the actor.
     *  Add this tag to any mesh component in the editor to include it. */
    UPROPERTY(EditAnywhere, Category="Visibility")
    FName VisibilityMeshTag = TEXT("VisibilityMesh");

    /** Resolved soft pointers populated by FindMeshesByName or AddMesh.
     *  Do not edit manually. */
    UPROPERTY(VisibleAnywhere, Category="Visibility")
    TArray<TSoftObjectPtr<USkeletalMeshComponent>> SkeletalMeshTargets;

    UPROPERTY(VisibleAnywhere, Category="Visibility")
    TArray<TSoftObjectPtr<UStaticMeshComponent>> StaticMeshTargets;

    /** Scalar parameter name in the materials that controls visibility (0~1). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Visibility")
    FName VisibilityParam = TEXT("VisibilityAlpha");

private:
    /** Flat list of all MIDs across all tracked meshes and their slots.
     *  All receive the same alpha. Created at Initialize. */
    UPROPERTY(Transient)
    TArray<UMaterialInstanceDynamic*> MIDs;
};
