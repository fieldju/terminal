// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#pragma once

#include <winrt/Windows.UI.Composition.h>

#include "BlurredAcrylicBrush.g.h"

namespace winrt::Microsoft::Terminal::Control::implementation
{
    struct BlurredAcrylicBrush : BlurredAcrylicBrushT<BlurredAcrylicBrush>
    {
        // The blur the stock AcrylicBrush uses.
        static constexpr float DefaultBlurAmount = 30.0f;

        BlurredAcrylicBrush() = default;

        float BlurAmount() const noexcept { return _blurAmount; }
        void BlurAmount(float value);

        // IXamlCompositionBrushBaseOverrides. We deliberately don't call the
        // base AcrylicBrush implementation: we supply the composition brush.
        void OnConnected();
        void OnDisconnected();

    private:
        void _rebuild();
        void _updateTint();
        winrt::Windows::UI::Color _effectiveTint() const;

        float _blurAmount{ DefaultBlurAmount };
        bool _connected{ false };
        winrt::Windows::UI::Composition::CompositionEffectBrush _effectBrush{ nullptr };
        int64_t _tintColorToken{ 0 };
        int64_t _tintOpacityToken{ 0 };
        int64_t _fallbackColorToken{ 0 };
    };
}

namespace winrt::Microsoft::Terminal::Control::factory_implementation
{
    BASIC_FACTORY(BlurredAcrylicBrush);
}
