// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/TextureRenderTarget2D.h"
#include "Components/SceneCaptureComponent2D.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EXShadowActorComponent.generated.h"

class ADirectionalLight;

/*
 * ActorComponent that generates a custom shadow depth buffer for the owner Actor and passes off the necessary
 * material parameters needed to sample the buffer and generate a custom shadowmap. 
 */

UCLASS( ClassGroup=(Custom), meta=(BlueprintSpawnableComponent) )
class UEXShadowActorComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	// Sets default values for this component's properties
	UEXShadowActorComponent();

protected:
	// Called when the game starts
	virtual void BeginPlay() override;

public:	
	//Called every frame
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// The square dimension of the shadowmap render target
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="EXShadow")
	int32 EXShadowRTSize{2048};

	// How far away the SceneCapture component is from the target actor
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="EXShadow")
	double CaptureLength{1000.0f};

	// Scaler on the SceneCapture OrthoWidth which is determined by the target's bounding box
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="EXShadow")
	float OrthoWidthScale{0.25f};

private:
	// DirectionalLight that will be treated as the shadow caster
	UPROPERTY()
	TObjectPtr<ADirectionalLight> DirectionalLight;
	
	UPROPERTY()
	TObjectPtr<USceneCaptureComponent2D> SceneCaptureComp;

	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> ShadowDepthRenderTarget;

	// Cache of all the MaterialInstanceDynamic objects that will be created from any existing mesh materials
	// on the parent actor
	UPROPERTY()
	TArray<UMaterialInstanceDynamic*> MIDCache;

	bool bSetupComplete = false;
	
	void FindDirectionalLight();

	void BuildMIDCache();

	void CreateShadowRenderTarget();

	void UpdateSceneCapturePosition();

	void UpdateShadowDepthRT();

	void UpdateMaterialParams();

	void CreateCaptureComponent();

	bool Setup();
};
