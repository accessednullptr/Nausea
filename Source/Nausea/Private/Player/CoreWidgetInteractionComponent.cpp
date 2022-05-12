// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "Player/CoreWidgetInteractionComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Components/InputComponent.h"
#include "Components/WidgetComponent.h"
#include "Player/PlayerOwnershipInterface.h"
#include "Player/CorePlayerController.h"

UCoreWidgetInteractionComponent::UCoreWidgetInteractionComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	
	InteractionDistance = 200.f;
	InteractionSource = EWidgetInteractionSource::Mouse;
}

void UCoreWidgetInteractionComponent::OnRegister()
{
	bAutoActivate = false;
	Super::OnRegister();
}

void UCoreWidgetInteractionComponent::SetupInputComponent(UInputComponent* InputComponent)
{
	check(InputComponent);

	IPlayerOwnershipInterface* PlayerOwnershipInterface = Cast<IPlayerOwnershipInterface>(GetOwner());

	if (!PlayerOwnershipInterface)
	{
		return;
	}

	OwningPlayerController = PlayerOwnershipInterface->GetOwningController<ACorePlayerController>();

	if (!OwningPlayerController)
	{
		return;
	}

	InputComponent->BindAction("WidgetInteract", IE_Pressed, this, &UCoreWidgetInteractionComponent::MouseButtonPressed);
	InputComponent->BindAction("WidgetInteract", IE_Released, this, &UCoreWidgetInteractionComponent::MouseButtonReleased);
	Activate(true);
}

UWidgetInteractionComponent::FWidgetTraceResult UCoreWidgetInteractionComponent::PerformTrace() const
{
	FWidgetTraceResult TraceResult;

	if (!OwningPlayerController)
	{
		return TraceResult;
	}

	EWidgetInteractionSource UsedInteractionSource = InteractionSource;
	switch (InteractionSource)
	{
	case EWidgetInteractionSource::Mouse:
	case EWidgetInteractionSource::CenterScreen:
		UsedInteractionSource = OwningPlayerController->bShowMouseCursor ? EWidgetInteractionSource::Mouse : EWidgetInteractionSource::CenterScreen;
		break;
	}

	TArray<FHitResult> MultiHits;

	FVector WorldDirection;

	switch (UsedInteractionSource)
	{
	case EWidgetInteractionSource::World:
	{
		const FVector WorldLocation = GetComponentLocation();
		const FTransform WorldTransform = GetComponentTransform();
		WorldDirection = WorldTransform.GetUnitAxis(EAxis::X);

		TArray<UPrimitiveComponent*> PrimitiveChildren;
		GetRelatedComponentsToIgnoreInAutomaticHitTesting(PrimitiveChildren);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(WidgetInteractionComponentTrace));
		Params.AddIgnoredComponents(PrimitiveChildren);

		TraceResult.LineStartLocation = WorldLocation;
		TraceResult.LineEndLocation = WorldLocation + (WorldDirection * InteractionDistance);

		GetWorld()->LineTraceMultiByChannel(MultiHits, TraceResult.LineStartLocation, TraceResult.LineEndLocation, TraceChannel, Params);
		break;
	}
	case EWidgetInteractionSource::Mouse:
	case EWidgetInteractionSource::CenterScreen:
	{
		TArray<UPrimitiveComponent*> PrimitiveChildren;
		GetRelatedComponentsToIgnoreInAutomaticHitTesting(PrimitiveChildren);

		FCollisionQueryParams Params(SCENE_QUERY_STAT(WidgetInteractionComponentTrace));
		Params.AddIgnoredComponents(PrimitiveChildren);

		ULocalPlayer* LocalPlayer = OwningPlayerController->GetLocalPlayer();

		if (LocalPlayer && LocalPlayer->ViewportClient)
		{
			if (UsedInteractionSource == EWidgetInteractionSource::Mouse)
			{
				FVector2D MousePosition;
				if (LocalPlayer->ViewportClient->GetMousePosition(MousePosition))
				{
					FVector WorldOrigin;
					if (UGameplayStatics::DeprojectScreenToWorld(OwningPlayerController, MousePosition, WorldOrigin, WorldDirection) == true)
					{
						TraceResult.LineStartLocation = WorldOrigin;
						TraceResult.LineEndLocation = WorldOrigin + WorldDirection * InteractionDistance;

						GetWorld()->LineTraceMultiByChannel(MultiHits, TraceResult.LineStartLocation, TraceResult.LineEndLocation, TraceChannel, Params);
					}
				}
			}
			else if (UsedInteractionSource == EWidgetInteractionSource::CenterScreen)
			{
				FVector2D ViewportSize;
				LocalPlayer->ViewportClient->GetViewportSize(ViewportSize);

				FVector WorldOrigin;
				if (UGameplayStatics::DeprojectScreenToWorld(OwningPlayerController, ViewportSize * 0.5f, WorldOrigin, WorldDirection) == true)
				{
					TraceResult.LineStartLocation = WorldOrigin;
					TraceResult.LineEndLocation = WorldOrigin + WorldDirection * InteractionDistance;

					GetWorld()->LineTraceMultiByChannel(MultiHits, WorldOrigin, WorldOrigin + WorldDirection * InteractionDistance, TraceChannel, Params);
				}
			}
		}
		break;
	}
	case EWidgetInteractionSource::Custom:
	{
		WorldDirection = (CustomHitResult.TraceEnd - CustomHitResult.TraceStart).GetSafeNormal();
		TraceResult.HitResult = CustomHitResult;
		TraceResult.bWasHit = CustomHitResult.bBlockingHit;
		TraceResult.LineStartLocation = CustomHitResult.TraceStart;
		TraceResult.LineEndLocation = CustomHitResult.TraceEnd;
		break;
	}
	}

	// If it's not a custom interaction, we do some custom filtering to ignore invisible widgets.
	if (UsedInteractionSource != EWidgetInteractionSource::Custom)
	{
		for (const FHitResult& HitResult : MultiHits)
		{
			if (UWidgetComponent* HitWidgetComponent = Cast<UWidgetComponent>(HitResult.GetComponent()))
			{
				if (HitWidgetComponent->IsVisible())
				{
					TraceResult.bWasHit = true;
					TraceResult.HitResult = HitResult;
					break;
				}
			}
			else if (HitResult.bBlockingHit)
			{
				// If we hit something that wasn't a widget component, we're done.
				break;
			}
		}
	}

	// Resolve trace to location on widget.
	if (TraceResult.bWasHit)
	{
		TraceResult.HitWidgetComponent = Cast<UWidgetComponent>(TraceResult.HitResult.GetComponent());
		if (TraceResult.HitWidgetComponent)
		{
			// @todo WASTED WORK: GetLocalHitLocation() gets called in GetHitWidgetPath();

			if (TraceResult.HitWidgetComponent->GetGeometryMode() == EWidgetGeometryMode::Cylinder)
			{
				TTuple<FVector, FVector2D> CylinderHitLocation = TraceResult.HitWidgetComponent->GetCylinderHitLocation(TraceResult.HitResult.ImpactPoint, WorldDirection);
				TraceResult.HitResult.ImpactPoint = CylinderHitLocation.Get<0>();
				TraceResult.LocalHitLocation = CylinderHitLocation.Get<1>();
			}
			else
			{
				ensure(TraceResult.HitWidgetComponent->GetGeometryMode() == EWidgetGeometryMode::Plane);
				TraceResult.HitWidgetComponent->GetLocalHitLocation(TraceResult.HitResult.ImpactPoint, TraceResult.LocalHitLocation);
			}
			TraceResult.HitWidgetPath = FindHoveredWidgetPath(TraceResult);
		}
	}

	return TraceResult;
}

void UCoreWidgetInteractionComponent::MouseButtonPressed()
{
	PressPointerKey(EKeys::LeftMouseButton);
}

void UCoreWidgetInteractionComponent::MouseButtonReleased()
{
	ReleasePointerKey(EKeys::LeftMouseButton);
}