// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.

#include "pch.h"
#include "BlurredAcrylicBrush.h"
#include "BlurredAcrylicBrush.g.cpp"

#include "CompositionEffects.h"

using namespace winrt::Windows::UI;
using namespace winrt::Windows::UI::Composition;
using namespace winrt::Windows::UI::Xaml;
using namespace winrt::Windows::UI::Xaml::Media;
namespace Effects = ::Microsoft::Terminal::Control::Effects;

namespace winrt::Microsoft::Terminal::Control::implementation
{
    void BlurredAcrylicBrush::BlurAmount(float value)
    {
        value = std::clamp(value, 0.0f, 250.0f);
        if (value == _blurAmount)
        {
            return;
        }
        _blurAmount = value;
        if (_connected)
        {
            // The blur amount is baked into the effect factory, so we need a
            // new brush rather than a property-set update.
            _rebuild();
        }
    }

    void BlurredAcrylicBrush::OnConnected()
    {
        _connected = true;

        // Track the AcrylicBrush properties we inherited, so callers can keep
        // using TintColor/TintOpacity/FallbackColor exactly like before.
        const auto onTintChanged = [weak = get_weak()](auto&&, auto&&) {
            if (auto self = weak.get())
            {
                self->_updateTint();
            }
        };
        _tintColorToken = RegisterPropertyChangedCallback(AcrylicBrush::TintColorProperty(), onTintChanged);
        _tintOpacityToken = RegisterPropertyChangedCallback(AcrylicBrush::TintOpacityProperty(), onTintChanged);
        _fallbackColorToken = RegisterPropertyChangedCallback(XamlCompositionBrushBase::FallbackColorProperty(), onTintChanged);

        _rebuild();
    }

    void BlurredAcrylicBrush::OnDisconnected()
    {
        _connected = false;

        UnregisterPropertyChangedCallback(AcrylicBrush::TintColorProperty(), _tintColorToken);
        UnregisterPropertyChangedCallback(AcrylicBrush::TintOpacityProperty(), _tintOpacityToken);
        UnregisterPropertyChangedCallback(XamlCompositionBrushBase::FallbackColorProperty(), _fallbackColorToken);

        _effectBrush = nullptr;
        if (const auto brush = CompositionBrush())
        {
            CompositionBrush(nullptr);
            brush.Close();
        }
    }

    Color BlurredAcrylicBrush::_effectiveTint() const
    {
        auto tint = TintColor();
        const auto opacity = std::clamp(TintOpacity(), 0.0, 1.0);
        tint.A = gsl::narrow_cast<uint8_t>(std::lround(tint.A * opacity));
        return tint;
    }

    void BlurredAcrylicBrush::_rebuild()
    {
        const auto compositor = Window::Current().Compositor();

        // Respect "Transparency effects" being off (and battery saver, etc.)
        // the same way AcrylicBrush does: draw the solid fallback color.
        if (!winrt::Windows::UI::ViewManagement::UISettings{}.AdvancedEffectsEnabled())
        {
            _effectBrush = nullptr;
            CompositionBrush(compositor.CreateColorBrush(FallbackColor()));
            return;
        }

        // Backdrop -> [Gaussian blur] -> tint drawn on top.
        CompositionEffectSourceParameter backdrop{ L"Backdrop" };
        winrt::Windows::Graphics::Effects::IGraphicsEffectSource blurred = backdrop;
        if (_blurAmount > 0.0f)
        {
            blurred = winrt::make<Effects::GaussianBlurEffect>(backdrop, _blurAmount);
        }

        auto tint = winrt::make_self<Effects::ColorSourceEffect>(_effectiveTint());
        tint->Name(L"Tint");

        const auto graph = winrt::make<Effects::CompositeEffect>(blurred, *tint);
        const auto factory = compositor.CreateEffectFactory(graph, winrt::single_threaded_vector<winrt::hstring>({ L"Tint.Color" }));

        _effectBrush = factory.CreateBrush();
        _effectBrush.SetSourceParameter(L"Backdrop", compositor.CreateBackdropBrush());
        CompositionBrush(_effectBrush);
    }

    void BlurredAcrylicBrush::_updateTint()
    {
        if (!_connected)
        {
            return;
        }
        if (_effectBrush)
        {
            _effectBrush.Properties().InsertColor(L"Tint.Color", _effectiveTint());
        }
        else
        {
            // We're showing the fallback color brush.
            _rebuild();
        }
    }
}
