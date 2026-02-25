// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TopDownCameraComp.generated.h"

/**
 *  this is for camera comp. the location will be used for RT and also curved World Origin Param value
 */


//Forward
//Camera View
class USpringArmComponent;
class UCameraComponent;
// RT
class UCameraVisionManager;
class UCurvedWorldSubsystem;

PROJECTER_API DECLARE_LOG_CATEGORY_EXTERN(MainCameraComp, Log, All);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class PROJECTER_API UTopDownCameraComp : public USceneComponent
{
	GENERATED_BODY()

public:
	UTopDownCameraComp();

protected:
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;
	
	virtual void OnRegister() override;
	
public:
	// Input
	void AddKeyPanInput(FVector2D Input);
	void ToggleCameraMode();
	void SetFreeCamMode(bool bIsKeyPressed);
	void RecenterOnPawn();
    
	UFUNCTION(BlueprintCallable, Category="CameraComp")
	void InitializeCompRequirements();

	// Getters
	FVector GetCameraWorldLocation() const { return GetComponentLocation(); }
	UCameraComponent* GetCameraComp() const { return CameraComp; }
	USpringArmComponent* GetCameraBoomComp() const { return SpringArm; }
	bool IsCameraPanFreeMode() const { return bIsFreeCamMode; }

private:
	// Tick helpers
	void TickFollowMode(float DeltaTime);
	void TickFreeCamMode(float DeltaTime);

	// Init helpers
	void PrepareRequirements();
	void CacheCurveWorldSubsystem();

	// Edge scroll
	FVector2D GatherEdgeScrollInput() const;
	void ApplyPanInput(FVector2D Direction, float DeltaTime);

	// Getters
	APlayerController* GetOwnerPlayerController() const;

	// Curve world updates
	void UpdateCameraTransform();
	void UpdateCameraCurveValues(float RadialCurveStrength);

	// Net helper
	bool ShouldUseClientLogic();

protected:
	// Sub-components
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera|Components")
	TObjectPtr<USpringArmComponent> SpringArm;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera|Components")
	TObjectPtr<UCameraComponent> CameraComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera|Components")
	TObjectPtr<UCameraVisionManager> CameraVisionManager;

	// Cached subsystem
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera|Components")
	TObjectPtr<UCurvedWorldSubsystem> CurveSubSystem = nullptr;

	// Curve settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Curve")
	float CurveBendWeight = 1.5f;

	// Movement settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Settings")
	float ArmLength = 2000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Settings")
	float ArmPitch = -60.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Settings")
	float PanSpeed = 900.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Settings")
	float EdgeScrollSpeedMultiplier = 0.6f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Settings")
	float EdgeScrollMargin = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Camera|Settings")
	float FollowLagSpeed = 1.f;

	// State
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera|State")
	bool bIsFreeCamMode = false;

	// Is the camera hard-locked to the player (e.g. by pressing Y)?
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="Camera|State")
	bool bIsCameraLocked = true;

private:
	FVector2D PendingKeyInput = FVector2D::ZeroVector;
	FVector FreeCamPivotLocation = FVector::ZeroVector;
	FVector SmoothedFollowLocation = FVector::ZeroVector;
};