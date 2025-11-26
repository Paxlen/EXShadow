// Fill out your copyright notice in the Description page of Project Settings.


#include "EXShadowActorComponent.h"

#include "Engine/DirectionalLight.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetRenderingLibrary.h"
#include "Math/Matrix.h"


// Sets default values for this component's properties
UEXShadowActorComponent::UEXShadowActorComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	bTickInEditor = true;
	bAutoActivate = true;
}

// Called when the game starts
void UEXShadowActorComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UEXShadowActorComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	/*
	 * I'd prefer to keep this as an ActorComponent that can be easily added to any existing Actor without the user
	 * needing to do any additional setup work on the Actor itself.
	 * BUT, BeginPlay doesn't run in the level editor (unless you start a PIE session) and ActorComponents don't
	 * have an equivalent to AActor's OnConstruction() so I'm doing this hacky setup work on tick until I find
	 * a better solution to do one-time setup when I know the scene is valid.
	 * If there is no clean solution then I can either have the user call a setup function from the owner Actor's
	 * construction script or just make this an AActor that handles it.
	 */
	
	if (!bSetupComplete)
	{
		bSetupComplete = Setup();
	}
	
	UpdateShadowDepthRT();
}

void UEXShadowActorComponent::FindDirectionalLight()
{
	/*
	 * Currently just support DirectionalLights.
	 */
	
	AActor* FoundLight = UGameplayStatics::GetActorOfClass(GetOwner(), ADirectionalLight::StaticClass());
	
	if (FoundLight)
	{
		DirectionalLight = Cast<ADirectionalLight>(FoundLight);
	} 
}

void UEXShadowActorComponent::BuildMIDCache()
{
	/*
	 * Find all the materials on the owner actor and create a set of MaterialInstanceDynamic materials so
	 * we can set shadow parameters on them.
	 * @TODO: Update when meshes on the owner change 
	 */
	
	TArray<UMeshComponent*> MeshComps;
	
	GetOwner()->GetComponents<UMeshComponent>(MeshComps, true);
	
	for (UMeshComponent* MeshComp : MeshComps)
	{
		TArray<UMaterialInterface*> Mats = MeshComp->GetMaterials();
		
		for (int i = 0; i < Mats.Num(); ++i)
		{
			MIDCache.Add(MeshComp->CreateDynamicMaterialInstance(i, Mats[i]));
		}
	}
}

void UEXShadowActorComponent::CreateShadowRenderTarget()
{
	/*
	 * Create and configure a render target to write scene depth to. 
	 */
	
	ShadowDepthRenderTarget =  UKismetRenderingLibrary::CreateRenderTarget2D(
		this,
		EXShadowRTSize,
		EXShadowRTSize,
		ETextureRenderTargetFormat::RTF_R16f,
		FLinearColor::Black,
		false,
		false);
}

void UEXShadowActorComponent::UpdateSceneCapturePosition()
{
	/*
	 *  Match the SceneCaptureComponent's position to the 
	 */
	
	if (!DirectionalLight) return;

	const FVector NewLoc = (
		(DirectionalLight->GetActorForwardVector() * -1) * CaptureLength)
		+ GetOwner()->GetActorLocation();

	const FRotator LookAtRot = UKismetMathLibrary::FindLookAtRotation(NewLoc, GetOwner()->GetActorLocation());
	
	SceneCaptureComp->SetWorldLocationAndRotation(NewLoc, LookAtRot);
}

void UEXShadowActorComponent::UpdateShadowDepthRT()
{
	/*
	 * Capture the scene depth and update related dynamic material parameters 
	 */
	
	if (!SceneCaptureComp || MIDCache.IsEmpty()) return;
	
	UpdateSceneCapturePosition();
	
	SceneCaptureComp->CaptureScene();
	
	UpdateMaterialParams();
}

void UEXShadowActorComponent::UpdateMaterialParams()
{
	/*
	 *  Send the depth render target, and the projection matrix data we need to sample it, to the materials
	 */

	if (MIDCache.IsEmpty()) return;
	
	const float OrthoWidth = SceneCaptureComp->OrthoWidth;
	
	// Taken from SceneCaptureRendering.cpp
	// swap axis st. x=z,y=x,z=y (unreal coord space) so that z is up
	const FMatrix ViewTransformMatrix = SceneCaptureComp->GetComponentTransform().ToInverseMatrixWithScale() * FMatrix(
	FPlane(0,	0,	1,	0),
	FPlane(1,	0,	0,	0),
	FPlane(0,	1,	0,	0),
	FPlane(0,	0,	0,	1));

	const FMatrix OrthoProjMatrix = FMatrix(
		FPlane(2.f/OrthoWidth, 0, 0, 0),
		FPlane(0, -2.f/OrthoWidth, 0, 0),
		FPlane(0, 0, 1.f/1000, 0),
		FPlane(0, 0, 1.f/1000,1)
		);
	
	const FMatrix ViewProjMatrix = ViewTransformMatrix * OrthoProjMatrix;
	
	// Taken from SceneCaptureRendering.cpp
	FMatrix44f ViewProjMatrixf = FMatrix44f(ViewProjMatrix);
	const FLinearColor* MatrixVectors = (const FLinearColor*)&ViewProjMatrixf;

	for (UMaterialInstanceDynamic* Mat : MIDCache)
	{
		/*
		 *  The material editor doesn't seem to provide normal matrix operations
		 */
		Mat->SetTextureParameterValue(FName("ShadowDepthRT"), ShadowDepthRenderTarget);
		Mat->SetVectorParameterValue(FName("ShadowProjX"), MatrixVectors[0]);
		Mat->SetVectorParameterValue(FName("ShadowProjY"), MatrixVectors[1]);
		Mat->SetVectorParameterValue(FName("ShadowProjZ"), MatrixVectors[2]);
		Mat->SetVectorParameterValue(FName("ShadowProjOrigin"), MatrixVectors[3]);
	}
}

void UEXShadowActorComponent::CreateCaptureComponent()
{
	/*
	 * Create a capture component and configure it to capture orthographic scene depth as we want to generate
	 * shadows from a DirectionalLight.
	 */
	
	AActor* Owner = GetOwner();
	
	if (!Owner || !ShadowDepthRenderTarget) return;
	
	SceneCaptureComp = Cast<USceneCaptureComponent2D>(
		Owner->AddComponentByClass(USceneCaptureComponent2D::StaticClass(),
		false,
		Owner->GetTransform(),
		false)
		);
	
	FVector OwnerBoxExtent, OwnerOrigin;
	Owner->GetActorBounds(false, OwnerOrigin, OwnerBoxExtent);
	
	SceneCaptureComp->ProjectionType = ECameraProjectionMode::Orthographic;
	SceneCaptureComp->OrthoWidth = OwnerBoxExtent.Size() * OrthoWidthScale;
	SceneCaptureComp->bAutoCalculateOrthoPlanes = false;
	SceneCaptureComp->PrimitiveRenderMode = ESceneCapturePrimitiveRenderMode::PRM_RenderScenePrimitives;
	SceneCaptureComp->CaptureSource = ESceneCaptureSource::SCS_SceneDepth;
	SceneCaptureComp->bCaptureEveryFrame = false;
	SceneCaptureComp->bCaptureOnMovement = false;
	SceneCaptureComp->TextureTarget = ShadowDepthRenderTarget;
}

bool UEXShadowActorComponent::Setup()
{
	/*
	 * Repeatable setup that's going to run on tick. Will go away when I figure out a UActorComponent equivalent
	 * to OnConstruction() or just move this over to being a UActor. 
	 */
	
	if (!ShadowDepthRenderTarget)
	{
		CreateShadowRenderTarget();
		if (!ShadowDepthRenderTarget) return false;
	}

	if (!SceneCaptureComp)
	{
		CreateCaptureComponent();
		if (!SceneCaptureComp) return false;
	}

	if (!DirectionalLight)
	{
		FindDirectionalLight();
		if (!DirectionalLight) return false;
	}

	if (MIDCache.IsEmpty())
	{
		BuildMIDCache();
	}

	return true;
}
