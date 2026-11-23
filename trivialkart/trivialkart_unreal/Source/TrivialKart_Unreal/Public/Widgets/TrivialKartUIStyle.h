/*
* Copyright 2026 The Android Open Source Project
 *
 *   Licensed under the Apache License, Version 2.0 (the "License");
 *   you may not use this file except in compliance with the License.
 *   You may obtain a copy of the License at
 *
 *       https://www.apache.org/licenses/LICENSE-2.0
 *
 *   Unless required by applicable law or agreed to in writing, software
 *   distributed under the License is distributed on an "AS IS" BASIS,
 *   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *   See the License for the specific language governing permissions and
 *   limitations under the License.
 */

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/WidgetTree.h"
#include "Components/HorizontalBox.h"
#include "Components/Image.h"
#include "Components/Overlay.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Styling/SlateBrush.h"
#include "Styling/SlateColor.h"

namespace TrivialKartUIStyle
{
	inline void NormalizeWidgetTreeHitTesting(UWidgetTree* WidgetTree, const UWidget* ExcludedWidget = nullptr)
	{
		if (!WidgetTree)
		{
			return;
		}

		WidgetTree->ForEachWidget([ExcludedWidget](UWidget* Widget)
		{
			if (!Widget || Widget == ExcludedWidget || Widget->GetVisibility() != ESlateVisibility::Visible)
			{
				return;
			}
			if (Cast<UImage>(Widget) || Cast<UTextBlock>(Widget))
			{
				Widget->SetVisibility(ESlateVisibility::HitTestInvisible);
			}
			else if (Cast<UOverlay>(Widget) || Cast<UHorizontalBox>(Widget) || Cast<UVerticalBox>(Widget))
			{
				Widget->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			}
		});
	}

	inline FSlateBrush MakeRoundedHighlightBrush(
		float CornerRadius,
		float OutlineWidth,
		const FLinearColor& TintColor,
		const FLinearColor& OutlineColor,
		const FVector2D& ImageSize = FVector2D::ZeroVector)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		if (!ImageSize.IsZero())
		{
			Brush.ImageSize = ImageSize;
		}
		Brush.TintColor = FSlateColor(TintColor);
		Brush.OutlineSettings.CornerRadii = FVector4(CornerRadius, CornerRadius, CornerRadius, CornerRadius);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.Color = FSlateColor(OutlineColor);
		Brush.OutlineSettings.Width = OutlineWidth;
		return Brush;
	}

	inline const FSlateBrush& GetEmptyBrush()
	{
		static const FSlateBrush EmptyBrush = []()
		{
			FSlateBrush Brush;
			Brush.DrawAs = ESlateBrushDrawType::NoDrawType;
			return Brush;
		}();
		return EmptyBrush;
	}

	inline FSlateBrush MakeButtonFocusBrush(bool bIsPressed, float CornerRadius = 12.0f)
	{
		return MakeRoundedHighlightBrush(
			CornerRadius,
			6.0f,
			bIsPressed ? FLinearColor(1.0f, 0.35f, 0.0f, 0.68f) : FLinearColor(1.0f, 0.45f, 0.0f, 0.48f),
			FLinearColor(1.0f, 0.9f, 0.0f, 1.0f));
	}
}
