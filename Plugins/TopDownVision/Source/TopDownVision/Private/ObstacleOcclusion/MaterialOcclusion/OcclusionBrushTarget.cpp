// Fill out your copyright notice in the Description page of Project Settings.


#include "ObstacleOcclusion/MaterialOcclusion/OcclusionBrushTarget.h"

#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"


void FOcclusionBrushTarget::InitializeMID(UObject* Outer, UMaterialInterface* FallbackMaterial)
{
	if (!Outer) return;

	// Use per-target material if set, otherwise fall back to painter default
	UMaterialInterface* MatToUse = BrushMaterial ? BrushMaterial.Get() : FallbackMaterial;

	if (!MatToUse) return;

	BrushMID = UMaterialInstanceDynamic::Create(MatToUse, Outer);
}