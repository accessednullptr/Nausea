// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/RichTextBlockDecorator.h"
#include "VerticalRichTextBlockDecorator.generated.h"

class URichTextBlock;

/**
 * 
 */
UCLASS()
class NAUSEA_API UVerticalRichTextBlockDecorator : public URichTextBlockDecorator
{
	GENERATED_BODY()
	
//~ Begin URichTextBlockDecorator Interface
public:
	virtual TSharedPtr<ITextDecorator> CreateDecorator(URichTextBlock* InOwner) override;
//~ End URichTextBlockDecorator Interface
};

class NAUSEA_API FVerticalRichTextDecorator : public ITextDecorator
{
public:
	FVerticalRichTextDecorator(URichTextBlock* InOwner);

	virtual ~FVerticalRichTextDecorator() {}

//~ Begin ITextDecorator Interface
public:
	virtual TSharedRef<ISlateRun> Create(const TSharedRef<class FTextLayout>& TextLayout, const FTextRunParseResults& RunParseResult, const FString& OriginalText, const TSharedRef< FString >& InOutModelText, const ISlateStyle* Style) override final;
	virtual bool Supports(const FTextRunParseResults& RunParseResult, const FString& Text) const override;
//~ End ITextDecorator Interface

protected:
	void CreateDecoratorText(const FTextRunInfo& RunInfo, FTextBlockStyle& InOutTextStyle, FString& InOutString) const;

protected:
	URichTextBlock* Owner;
};