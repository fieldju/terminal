// Copyright (c) Microsoft Corporation.
// Licensed under the MIT license.
//
// Module Name:
// - CompositionEffects.h
//
// Abstract:
// - Minimal IGraphicsEffect implementations for the handful of Direct2D effects
//   we feed to Windows.UI.Composition. The compositor only needs an effect ID,
//   its properties and its sources (IGraphicsEffectD2D1Interop), so this avoids
//   pulling in Win2D for three tiny effect descriptions.

#pragma once

#include <winrt/Windows.Graphics.Effects.h>
#include <windows.graphics.effects.interop.h>

namespace Microsoft::Terminal::Control::Effects
{
    namespace abi_effects = ABI::Windows::Graphics::Effects;

    // Direct2D built-in effect CLSIDs (d2d1effects.h). Declared locally so we
    // don't need to link dxguid.lib just for three GUIDs.
    inline constexpr GUID CLSID_GaussianBlur{ 0x1feb6d69, 0x2fe6, 0x4ac9, { 0x8c, 0x58, 0x1d, 0x7f, 0x93, 0xe7, 0xa6, 0xa5 } };
    inline constexpr GUID CLSID_Flood{ 0x61c23c20, 0xae69, 0x4d8e, { 0x94, 0xcf, 0x50, 0x07, 0x8d, 0xf6, 0x38, 0xf2 } };
    inline constexpr GUID CLSID_Composite{ 0x48fc9f51, 0xf6ac, 0x48f1, { 0x8b, 0x58, 0x3b, 0x28, 0xac, 0x46, 0xf7, 0x6d } };

    template<typename D>
    struct EffectBase : winrt::implements<D,
                                          winrt::Windows::Graphics::Effects::IGraphicsEffect,
                                          winrt::Windows::Graphics::Effects::IGraphicsEffectSource,
                                          abi_effects::IGraphicsEffectD2D1Interop>
    {
        winrt::hstring Name() const { return _name; }
        void Name(const winrt::hstring& value) { _name = value; }

        // IGraphicsEffectD2D1Interop
        HRESULT __stdcall GetSource(UINT index, abi_effects::IGraphicsEffectSource** source) noexcept override
        {
            if (!source)
            {
                return E_POINTER;
            }
            *source = nullptr;
            const auto& sources = static_cast<D*>(this)->_sources();
            if (index >= sources.size())
            {
                return E_INVALIDARG;
            }
            winrt::copy_to_abi(sources[index], *reinterpret_cast<void**>(source));
            return S_OK;
        }

        HRESULT __stdcall GetSourceCount(UINT* count) noexcept override
        {
            if (!count)
            {
                return E_POINTER;
            }
            *count = gsl::narrow_cast<UINT>(static_cast<D*>(this)->_sources().size());
            return S_OK;
        }

    protected:
        static HRESULT _returnValue(const winrt::Windows::Foundation::IInspectable& value, ABI::Windows::Foundation::IPropertyValue** result) noexcept
        try
        {
            *result = value.as<ABI::Windows::Foundation::IPropertyValue>().detach();
            return S_OK;
        }
        CATCH_RETURN()

        winrt::hstring _name;
    };

    // CLSID_D2D1GaussianBlur. Properties: 0 = StandardDeviation (float),
    // 1 = Optimization (enum), 2 = BorderMode (enum).
    struct GaussianBlurEffect : EffectBase<GaussianBlurEffect>
    {
        GaussianBlurEffect(const winrt::Windows::Graphics::Effects::IGraphicsEffectSource& source, float blurAmount) :
            _source{ source }, _blurAmount{ blurAmount } {}

        HRESULT __stdcall GetEffectId(GUID* id) noexcept override
        {
            *id = CLSID_GaussianBlur;
            return S_OK;
        }

        HRESULT __stdcall GetNamedPropertyMapping(LPCWSTR name, UINT* index, abi_effects::GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) noexcept override
        {
            if (_wcsicmp(name, L"BlurAmount") == 0)
            {
                *index = 0;
                *mapping = abi_effects::GRAPHICS_EFFECT_PROPERTY_MAPPING_DIRECT;
                return S_OK;
            }
            return E_INVALIDARG;
        }

        HRESULT __stdcall GetPropertyCount(UINT* count) noexcept override
        {
            *count = 3;
            return S_OK;
        }

        HRESULT __stdcall GetProperty(UINT index, ABI::Windows::Foundation::IPropertyValue** value) noexcept override
        {
            using winrt::Windows::Foundation::PropertyValue;
            switch (index)
            {
            case 0:
                return _returnValue(PropertyValue::CreateSingle(_blurAmount), value);
            case 1:
                return _returnValue(PropertyValue::CreateUInt32(1 /* D2D1_GAUSSIANBLUR_OPTIMIZATION_BALANCED */), value);
            case 2:
                return _returnValue(PropertyValue::CreateUInt32(1 /* D2D1_BORDER_MODE_HARD */), value);
            default:
                return E_INVALIDARG;
            }
        }

        std::array<winrt::Windows::Graphics::Effects::IGraphicsEffectSource, 1> _sources() const { return { _source }; }

    private:
        winrt::Windows::Graphics::Effects::IGraphicsEffectSource _source;
        float _blurAmount;
    };

    // CLSID_D2D1Flood. Property 0 = Color (vector4). Exposed to the compositor
    // as "Color" so it can be updated through the brush's property set.
    struct ColorSourceEffect : EffectBase<ColorSourceEffect>
    {
        explicit ColorSourceEffect(winrt::Windows::UI::Color color) :
            _color{ color } {}

        HRESULT __stdcall GetEffectId(GUID* id) noexcept override
        {
            *id = CLSID_Flood;
            return S_OK;
        }

        HRESULT __stdcall GetNamedPropertyMapping(LPCWSTR name, UINT* index, abi_effects::GRAPHICS_EFFECT_PROPERTY_MAPPING* mapping) noexcept override
        {
            if (_wcsicmp(name, L"Color") == 0)
            {
                *index = 0;
                *mapping = abi_effects::GRAPHICS_EFFECT_PROPERTY_MAPPING_COLOR_TO_VECTOR4;
                return S_OK;
            }
            return E_INVALIDARG;
        }

        HRESULT __stdcall GetPropertyCount(UINT* count) noexcept override
        {
            *count = 1;
            return S_OK;
        }

        HRESULT __stdcall GetProperty(UINT index, ABI::Windows::Foundation::IPropertyValue** value) noexcept override
        {
            if (index != 0)
            {
                return E_INVALIDARG;
            }
            const std::array<float, 4> rgba{ _color.R / 255.0f, _color.G / 255.0f, _color.B / 255.0f, _color.A / 255.0f };
            return _returnValue(winrt::Windows::Foundation::PropertyValue::CreateSingleArray(rgba), value);
        }

        std::array<winrt::Windows::Graphics::Effects::IGraphicsEffectSource, 0> _sources() const { return {}; }

    private:
        winrt::Windows::UI::Color _color;
    };

    // CLSID_D2D1Composite in SOURCE_OVER mode: draws `source` on top of
    // `destination`.
    struct CompositeEffect : EffectBase<CompositeEffect>
    {
        CompositeEffect(const winrt::Windows::Graphics::Effects::IGraphicsEffectSource& destination,
                        const winrt::Windows::Graphics::Effects::IGraphicsEffectSource& source) :
            _destination{ destination }, _source{ source } {}

        HRESULT __stdcall GetEffectId(GUID* id) noexcept override
        {
            *id = CLSID_Composite;
            return S_OK;
        }

        HRESULT __stdcall GetNamedPropertyMapping(LPCWSTR, UINT*, abi_effects::GRAPHICS_EFFECT_PROPERTY_MAPPING*) noexcept override
        {
            return E_INVALIDARG;
        }

        HRESULT __stdcall GetPropertyCount(UINT* count) noexcept override
        {
            *count = 1;
            return S_OK;
        }

        HRESULT __stdcall GetProperty(UINT index, ABI::Windows::Foundation::IPropertyValue** value) noexcept override
        {
            if (index != 0)
            {
                return E_INVALIDARG;
            }
            return _returnValue(winrt::Windows::Foundation::PropertyValue::CreateUInt32(0 /* D2D1_COMPOSITE_MODE_SOURCE_OVER */), value);
        }

        std::array<winrt::Windows::Graphics::Effects::IGraphicsEffectSource, 2> _sources() const { return { _destination, _source }; }

    private:
        winrt::Windows::Graphics::Effects::IGraphicsEffectSource _destination;
        winrt::Windows::Graphics::Effects::IGraphicsEffectSource _source;
    };
}
