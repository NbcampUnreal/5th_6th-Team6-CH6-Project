#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "OcclusionInterface.generated.h"

UINTERFACE()
class UOcclusionInterface : public UInterface
{
	GENERATED_BODY()
};

class TOPDOWNVISION_API IOcclusionInterface
{
	GENERATED_BODY()

public:

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnOcclusionEnter(UObject* SourceTracer);

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void OnOcclusionExit(UObject* SourceTracer);

	// for locking the occluded state
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable)
	void ForceOcclude(bool bForce);
};