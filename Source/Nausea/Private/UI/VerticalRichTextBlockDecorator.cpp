// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/VerticalRichTextBlockDecorator.h"
#include "UObject/SoftObjectPtr.h"
#include "Rendering/DrawElements.h"
#include "Framework/Text/SlateTextRun.h"
#include "Framework/Text/SlateWidgetRun.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"
#include "Components/RichTextBlock.h"

TSharedPtr<ITextDecorator> UVerticalRichTextBlockDecorator::CreateDecorator(URichTextBlock* InOwner)
{
	return TSharedPtr<FVerticalRichTextDecorator>(new FVerticalRichTextDecorator(InOwner));
}

FVerticalRichTextDecorator::FVerticalRichTextDecorator(URichTextBlock* InOwner)
	: Owner(InOwner)
{

}

TSharedRef<ISlateRun> FVerticalRichTextDecorator::Create(const TSharedRef<class FTextLayout>& TextLayout, const FTextRunParseResults& RunParseResult, const FString& OriginalText, const TSharedRef< FString >& InOutModelText, const ISlateStyle* Style)
{
	FTextRange ModelRange;
	ModelRange.BeginIndex = InOutModelText->Len();
	*InOutModelText = OriginalText;
	ModelRange.EndIndex = InOutModelText->Len();

	FTextRunInfo RunInfo(RunParseResult.Name, FText::FromString(OriginalText.Mid(RunParseResult.ContentRange.BeginIndex, RunParseResult.ContentRange.EndIndex - RunParseResult.ContentRange.BeginIndex)));
	for (const TPair<FString, FTextRange>& Pair : RunParseResult.MetaData)
	{
		RunInfo.MetaData.Add(Pair.Key, OriginalText.Mid(Pair.Value.BeginIndex, Pair.Value.EndIndex - Pair.Value.BeginIndex));
	}

	const FTextBlockStyle& TextStyle = Owner->GetCurrentDefaultTextStyle();

	TSharedPtr<ISlateRun> SlateRun;

	// Assume there's a text handler if widget is empty, if there isn't one it will just display an empty string
	FTextBlockStyle TempStyle = TextStyle;
	CreateDecoratorText(RunInfo, TempStyle, *InOutModelText);

	ModelRange.EndIndex = InOutModelText->Len();
	SlateRun = FSlateTextRun::Create(RunInfo, InOutModelText, TempStyle, ModelRange);

	return SlateRun.ToSharedRef();
}

bool FVerticalRichTextDecorator::Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const
{
	return true;
}

void FVerticalRichTextDecorator::CreateDecoratorText(const FTextRunInfo& RunInfo, FTextBlockStyle& InOutTextStyle, FString& InOutString) const
{
	FString MutatedString;
	MutatedString.Reserve((InOutString.Len() * 2) + 1);

	for (const TCHAR& Character : InOutString)
	{
		MutatedString.AppendChar(Character);
		MutatedString.AppendChar(*TEXT("\r\n"));
	}

	MutatedString += TEXT('\u200B');

	InOutString = MutatedString;
}