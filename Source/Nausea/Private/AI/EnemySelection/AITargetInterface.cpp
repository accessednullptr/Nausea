// Copyright 2020-2021 Heavy Mettle Interactive. All Rights Reserved.


#include "AI/EnemySelection/AITargetInterface.h"

UAITargetInterface::UAITargetInterface(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

UAITargetStatics::UAITargetStatics(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{

}

bool UAITargetStatics::IsTargetable(TScriptInterface<IAITargetInterface> Target, const AActor* Targeter)
{
	return TSCRIPTINTERFACE_CALL_FUNC_RET(Target, IsTargetable, K2_IsTargetable, false, Targeter);
}

void UAITargetStatics::OnBecomeTarget(TScriptInterface<IAITargetInterface> Target, AActor* Targeter)
{
	TSCRIPTINTERFACE_CALL_FUNC(Target, OnBecomeTarget, K2_OnBecomeTarget, Targeter);
}

void UAITargetStatics::OnEndTarget(TScriptInterface<IAITargetInterface> Target, AActor* Targeter)
{
	TSCRIPTINTERFACE_CALL_FUNC(Target, OnEndTarget, K2_OnEndTarget, Targeter);
}