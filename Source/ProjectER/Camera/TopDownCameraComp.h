// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneComponent.h"
#include "TopDownCameraComp.generated.h"

/**
 *  this is for camera comp. the location will be used for RT and also curved World Origin Param value
 */

class UCurvedWorldSubsystem;
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
	// Sets default values for this component's properties
	UTopDownCameraComp();

protected:
	
	virtual void TickComponent(
		float DeltaTime,
		ELevelTick TickType,
		FActorComponentTickFunction* ThisTickFunction) override;

	virtual void OnRegister() override;

public:
	//Control Inputs
	void AddKeyPanInput(FVector2D Input);
	void ToggleCameraMode();
	
	void RecenterOnPawn();// for snap back to pawn and enter follow mode

	FVector GetCameraWorldLocation() const { return GetComponentLocation(); }//return the comp root location

	UCameraComponent*GetCameraComp() const {return CameraComp;}
	USpringArmComponent* GetCameraBoomComp()const {return SpringArm;}

	bool IsCameraPanFreeMode()const {return bIsFreeCamMode;}

	
private:
	//internal tick helpers
	void TickFollowMode(float DeltaTime);
	void TickFreeCamMode(float DeltaTime);


	void PrepareRequirements();//wrapper for cacheing
	
	void CacheCurveWorldSubsystem();
	

	//EdgeHelper
	FVector2D GatherEdgeScrollInput() const;

	void ApplyPanInput(FVector2D Direction, float DeltaTime);

	//GetterHelper
	APlayerController* GetOwnerPlayerController() const;


	// update for curve world value

	void UpdateCameraTransform();

	void UpdateCameraCurveValues(float RadialCurveStrength);

	//ServerClient gate
	bool ShouldUseClientLogic();

	

protected:
	//SubComps
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Components")
	TObjectPtr<USpringArmComponent> SpringArm;
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Components")
	TObjectPtr<UCameraComponent> CameraComp;

	//RT
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Components")
	TObjectPtr<UCameraVisionManager> CameraVisionManager;
	

	//CurveWorldSubsystem
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|Components")
	UCurvedWorldSubsystem* CurveSubSystem=nullptr;

	//
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Curve")
	float CurveBendWeight=1.5f;
	
	//movement settings
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float ArmLength = 2000.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float ArmPitch = -60.f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float PanSpeed = 900.f;
	//EdgeScroll
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float EdgeScrollSpeedMultiplier = 0.6f;
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float EdgeScrollMargin = 20.f;

	//Lag
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Camera|Settings")
	float FollowLagSpeed = 1.f;


	//StateFlag
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Camera|State")
	bool bIsFreeCamMode = false;

private:
	FVector2D PendingKeyInput = FVector2D::ZeroVector;
	FVector FreeCamPivotLocation = FVector::ZeroVector;
	FVector SmoothedFollowLocation = FVector::ZeroVector;

	bool bInitialized = false;
};
