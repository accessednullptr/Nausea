// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "System/CoreGameUserSettings.h"

UCoreGameUserSettings::UCoreGameUserSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UCoreInputSettings::UCoreInputSettings(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UCoreInputSettings* UCoreInputSettings::GetCoreInputSettings()
{
	return GetMutableDefault<UCoreInputSettings>();
}

bool UCoreInputSettings::SetMouseXSensitivity(float InSensitivity)
{
	FInputAxisConfigEntry* MouseXAxis = nullptr;
	float MouseYSensitivity = -1.f;

	TArray<FInputAxisConfigEntry>& InputAxisConfig = GetInputSettings()->AxisConfig;
	for (FInputAxisConfigEntry& Axis : InputAxisConfig)
	{
		if (MouseXAxis != nullptr && MouseYSensitivity != -1.f)
		{
			break;
		}

		if (Axis.AxisKeyName == EKeys::MouseX)
		{
			MouseXAxis = &Axis;
			continue;
		}

		if (Axis.AxisKeyName == EKeys::MouseY)
		{
			MouseYSensitivity = Axis.AxisProperties.Sensitivity;
			continue;
		}
	}

	if (MouseXAxis == nullptr || MouseYSensitivity == -1.f)
	{
		return false;
	}

	const float PreviousMouseXSensitivity = MouseXAxis->AxisProperties.Sensitivity;
	MouseXAxis->AxisProperties.Sensitivity = InSensitivity;
	if (!SetMouseYSensitivityMultiplier((MouseYSensitivity / PreviousMouseXSensitivity)))
	{
		GetInputSettings()->ForceRebuildKeymaps();
		GetInputSettings()->SaveConfig();
		GetInputSettings()->UpdateDefaultConfigFile();
	}

	return true;
}

bool UCoreInputSettings::SetMouseYSensitivityMultiplier(float InSensitivityMultiplier)
{
	FInputAxisConfigEntry* MouseYAxis = nullptr;
	float MouseXSensitivity = -1.f;

	TArray<FInputAxisConfigEntry>& InputAxisConfig = GetInputSettings()->AxisConfig;
	for (FInputAxisConfigEntry& Axis : InputAxisConfig)
	{
		if (MouseYAxis != nullptr && MouseXSensitivity != -1.f)
		{
			break;
		}

		if (Axis.AxisKeyName == EKeys::MouseY)
		{
			MouseYAxis = &Axis;
			continue;
		}

		if (Axis.AxisKeyName == EKeys::MouseX)
		{
			MouseXSensitivity = Axis.AxisProperties.Sensitivity;
			continue;
		}
	}

	if (MouseYAxis == nullptr || MouseXSensitivity == -1.f)
	{
		return false;
	}

	const float MouseYSensitivity = MouseXSensitivity * InSensitivityMultiplier;

	if (MouseYAxis->AxisProperties.Sensitivity == MouseYSensitivity)
	{
		return false;
	}

	MouseYAxis->AxisProperties.Sensitivity = MouseYSensitivity;

	GetInputSettings()->ForceRebuildKeymaps();
	GetInputSettings()->SaveConfig();
	GetInputSettings()->UpdateDefaultConfigFile();
	return true;
}

void UCoreInputSettings::SetAimSensitivity(float InSensitivity)
{
	GetCoreInputSettings()->AimSensitivity = InSensitivity;
}

float UCoreInputSettings::GetMouseXSensitivity()
{
	const TArray<FInputAxisConfigEntry>& InputAxisConfig = GetInputSettings()->AxisConfig;
	for (const FInputAxisConfigEntry& Axis : InputAxisConfig)
	{
		if (Axis.AxisKeyName == EKeys::MouseX)
		{
			return Axis.AxisProperties.Sensitivity;
		}
	}

	return -1.f;
}

float UCoreInputSettings::GetMouseYSensitivityMultiplier()
{
	const TArray<FInputAxisConfigEntry>& InputAxisConfig = GetInputSettings()->AxisConfig;
	for (const FInputAxisConfigEntry& MouseYAxis : InputAxisConfig)
	{
		if (MouseYAxis.AxisKeyName == EKeys::MouseY)
		{
			for (const FInputAxisConfigEntry& MouseXAxis : InputAxisConfig)
			{
				if (MouseXAxis.AxisKeyName == EKeys::MouseX)
				{
					return MouseYAxis.AxisProperties.Sensitivity / MouseXAxis.AxisProperties.Sensitivity;
				}
			}

			break;
		}
	}

	return -1.f;
}

float UCoreInputSettings::GetAimSensitivity()
{
	return GetCoreInputSettings()->AimSensitivity;
}