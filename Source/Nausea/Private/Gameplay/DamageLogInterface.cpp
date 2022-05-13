// Copyright 2020-2022 Heavy Mettle Interactive. Published under the MIT License.


#include "Gameplay/DamageLogInterface.h"
#include "Internationalization/StringTableRegistry.h"
#include "NauseaHelpers.h"

UDamageLogInterface::UDamageLogInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UDamageLogStatics::UDamageLogStatics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UDamageLogStatics::ImplementsDamageLogInterface(TSubclassOf<UObject> ObjectClass)
{
	if (!ObjectClass)
	{
		return false;
	}

	TScriptInterface<IDamageLogInterface> TestInterface = TScriptInterface<IDamageLogInterface>(ObjectClass.GetDefaultObject());

	return TSCRIPTINTERFACE_IS_VALID(TestInterface);
}

TScriptInterface<IDamageLogInterface> UDamageLogStatics::GetDamageLogModifierInterface(TSubclassOf<UObject> ObjectClass)
{
	if (!ObjectClass)
	{
		return TScriptInterface<IDamageLogInterface>(nullptr);
	}

	if (!ObjectClass.GetDefaultObject() || !ObjectClass->Implements<UDamageLogInterface>())
	{
		return TScriptInterface<IDamageLogInterface>(nullptr);
	}

	return TScriptInterface<IDamageLogInterface>(ObjectClass.GetDefaultObject());
}

FText UDamageLogStatics::GetDamageLogInstigatorName(TScriptInterface<IDamageLogInterface> Target)
{
	return TSCRIPTINTERFACE_CALL_FUNC_RET(Target, GetDamageLogInstigatorName, K2_GetDamageLogInstigatorName, FText::GetEmpty());
}

inline UObject* GetCDOFromObjectOrClass(UObject* Object)
{
	if (!Object)
	{
		return nullptr;
	}

	if (UClass* Class = Cast<UClass>(Object))
	{
		return Class->GetDefaultObject(); //If this is a class, return the CDO.
	}
	else
	{
		return Object;
	}
}

FText UDamageLogStatics::GetDamageLogEventText(FDamageLogEvent DamageLogEvent)
{
	return GenerateDamageLogEventText(DamageLogEvent);
}

FText UDamageLogStatics::GetDamageLogModificationText(FDamageLogEventModifier DamageLogEventModifier)
{
	return GenerateDamageLogEventModifierText(DamageLogEventModifier);
}

static FText DamageLogEventText = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Event");
static FText DamageLogEventWithWeaponText = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Event_WithWeapon");
FText UDamageLogStatics::GenerateDamageLogEventText(const FDamageLogEvent& DamageLogEvent)
{
	TScriptInterface<IDamageLogInterface> InstigatorInterface = GetCDOFromObjectOrClass(DamageLogEvent.Instigator.Get());
	if (!TSCRIPTINTERFACE_IS_VALID(InstigatorInterface))
	{
		return FText::GetEmpty();
	}

	TScriptInterface<IDamageLogInterface> WeaponInterface = GetCDOFromObjectOrClass(DamageLogEvent.InstigatorWeaponClass.Get());
	if (TSCRIPTINTERFACE_IS_VALID(WeaponInterface))
	{
		return FText::FormatNamed(DamageLogEventWithWeaponText,
			TEXT("Instigator"), GetDamageLogInstigatorName(InstigatorInterface),
			TEXT("Weapon"), GetDamageLogInstigatorName(WeaponInterface),
			TEXT("Amount"), DamageLogEvent.DamageDealt,
			TEXT("InitialAmount"), DamageLogEvent.DamageInitial);
	}
	else
	{
		return FText::FormatNamed(DamageLogEventText,
			TEXT("Instigator"), GetDamageLogInstigatorName(InstigatorInterface),
			TEXT("Amount"), DamageLogEvent.DamageDealt,
			TEXT("InitialAmount"), DamageLogEvent.DamageInitial);
	}

	return FText::GetEmpty();
}

static FText DamageLogModificationIncreaseText = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Event_Modification_Increase");
static FText DamageLogModificationDecreaseText = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Event_Modification_Decrease");
static FText DamageLogModificationIncreaseWithInstigatorText = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Event_Modification_Increase_With_Instigator");
static FText DamageLogModificationDecreaseWithInstigatorText = LOCTABLE("/Game/Localization/DamageLogStringTable.DamageLogStringTable", "Event_Modification_Decrease_With_Instigator");
FText UDamageLogStatics::GenerateDamageLogEventModifierText(const FDamageLogEventModifier& DamageLogEventModifier)
{
	TScriptInterface<IDamageLogInterface> ModificationInterface = GetCDOFromObjectOrClass(DamageLogEventModifier.DataObject.Get());
	TScriptInterface<IDamageLogInterface> ModificationInstigatorInterface = DamageLogEventModifier.InstigatorObject.Get();
	if (!TSCRIPTINTERFACE_IS_VALID(ModificationInterface))
	{
		return FText::GetEmpty();
	}

	if (DamageLogEventModifier.Modifier == 1.f)
	{
		return FText::GetEmpty();
	}

	const FText InstigatorName = TSCRIPTINTERFACE_IS_VALID(ModificationInstigatorInterface) ? GetDamageLogInstigatorName(ModificationInstigatorInterface) : FText::GetEmpty();
	const int32 Percent = FMath::RoundToInt(FMath::Abs(DamageLogEventModifier.Modifier - 1.f) * 100.f);

	const FText ModifierText = DamageLogEventModifier.Modifier > 1.f ?
		(InstigatorName.IsEmpty() ? DamageLogModificationIncreaseText : DamageLogModificationIncreaseWithInstigatorText)
		: (InstigatorName.IsEmpty() ? DamageLogModificationDecreaseText : DamageLogModificationDecreaseWithInstigatorText);

	return FText::FormatNamed(ModifierText, TEXT("Instigator"), InstigatorName, TEXT("Modifier"), GetDamageLogInstigatorName(ModificationInterface), TEXT("Percent"), Percent);
}