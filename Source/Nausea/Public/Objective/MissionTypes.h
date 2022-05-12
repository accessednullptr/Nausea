#pragma once

UENUM(BlueprintType)
enum class EMissionStatus : uint8
{
	Inactive,
	Active,
	Completed,
	Failed
};

UENUM(BlueprintType)
enum class EObjectiveState : uint8
{
	Inactive,
	Active,
	Completed,
	Failed
};