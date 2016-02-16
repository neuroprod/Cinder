/*
Copyright (c) 2015, The Barbarian Group
All rights reserved.

Copyright (c) Microsoft Open Technologies, Inc. All rights reserved.

This code is intended for use with the Cinder C++ library: http://libcinder.org

Redistribution and use in source and binary forms, with or without modification, are permitted provided that
the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and
the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and
the following disclaimer in the documentation and/or other materials provided with the distribution.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED
WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
POSSIBILITY OF SUCH DAMAGE.
*/

#pragma once

#include "cinder/msw/CinderMsw.h"
#include "cinder/msw/ScopedPtr.h"

#include <Mferror.h>
#include <d3d9.h> // Required for dxva2api.h
#include <dxva2api.h>
#include <mfapi.h>
#include <mfobjects.h>

namespace cinder {
namespace wmf {

//------------------------------------------------------------------------------------

//! Converts a fixed-point to a float.
inline float MFOffsetToFloat( const MFOffset& offset )
{
	return (float)offset.value + ( (float)offset.value / 65536.0f );
}

inline RECT MFVideoAreaToRect( const MFVideoArea area )
{
	float left = MFOffsetToFloat( area.OffsetX );
	float top = MFOffsetToFloat( area.OffsetY );

	RECT rc = {
		int( left + 0.5f ),
		int( top + 0.5f ),
		int( left + area.Area.cx + 0.5f ),
		int( top + area.Area.cy + 0.5f )
	};

	return rc;
}

inline MFOffset MFMakeOffset( float v )
{
	MFOffset offset;
	offset.value = short( v );
	offset.fract = WORD( 65536 * ( v - offset.value ) );
	return offset;
}

inline MFVideoArea MFMakeArea( float x, float y, DWORD width, DWORD height )
{
	MFVideoArea area;
	area.OffsetX = MFMakeOffset( x );
	area.OffsetY = MFMakeOffset( y );
	area.Area.cx = width;
	area.Area.cy = height;
	return area;
}

//------------------------------------------------------------------------------------

template <class Q>
HRESULT MFGetEventObject( IMFMediaEvent* pEvent, Q** ppObject )
{
	*ppObject = NULL;

	PROPVARIANT var;
	HRESULT     hr = pEvent->GetValue( &var );
	if( SUCCEEDED( hr ) ) {
		if( var.vt == VT_UNKNOWN ) {
			hr = var.punkVal->QueryInterface( ppObject );
		}
		else {
			hr = MF_E_INVALIDTYPE;
		}
		PropVariantClear( &var );
	}

	return hr;
}

//------------------------------------------------------------------------------------

inline HRESULT GetDXVA2ExtendedFormat( IMFMediaType* pMediaType, DXVA2_ExtendedFormat* pFormat )
{
	if( pFormat == NULL )
		return E_POINTER;

	if( pMediaType == NULL )
		return E_POINTER;

	HRESULT hr = S_OK;

	do {
		MFVideoInterlaceMode interlace = (MFVideoInterlaceMode)MFGetAttributeUINT32( pMediaType, MF_MT_INTERLACE_MODE, MFVideoInterlace_Unknown );

		if( interlace == MFVideoInterlace_MixedInterlaceOrProgressive ) {
			pFormat->SampleFormat = DXVA2_SampleFieldInterleavedEvenFirst;
		}
		else {
			pFormat->SampleFormat = (UINT)interlace;
		}

		pFormat->VideoChromaSubsampling = MFGetAttributeUINT32( pMediaType, MF_MT_VIDEO_CHROMA_SITING, MFVideoChromaSubsampling_Unknown );
		pFormat->NominalRange = MFGetAttributeUINT32( pMediaType, MF_MT_VIDEO_NOMINAL_RANGE, MFNominalRange_Unknown );
		pFormat->VideoTransferMatrix = MFGetAttributeUINT32( pMediaType, MF_MT_YUV_MATRIX, MFVideoTransferMatrix_Unknown );
		pFormat->VideoLighting = MFGetAttributeUINT32( pMediaType, MF_MT_VIDEO_LIGHTING, MFVideoLighting_Unknown );
		pFormat->VideoPrimaries = MFGetAttributeUINT32( pMediaType, MF_MT_VIDEO_PRIMARIES, MFVideoPrimaries_Unknown );
		pFormat->VideoTransferFunction = MFGetAttributeUINT32( pMediaType, MF_MT_TRANSFER_FUNCTION, MFVideoTransFunc_Unknown );
	} while( FALSE );

	return hr;
}

inline HRESULT ConvertToDXVAType( IMFMediaType* pMediaType, DXVA2_VideoDesc* pDesc )
{
	if( pDesc == NULL )
		return E_POINTER;

	if( pMediaType == NULL )
		return E_POINTER;

	HRESULT hr = S_OK;

	ZeroMemory( pDesc, sizeof( *pDesc ) );

	do {
		GUID guidSubType = GUID_NULL;
		hr = pMediaType->GetGUID( MF_MT_SUBTYPE, &guidSubType );
		BREAK_ON_FAIL( hr );

		UINT32 width = 0, height = 0;
		hr = MFGetAttributeSize( pMediaType, MF_MT_FRAME_SIZE, &width, &height );
		BREAK_ON_FAIL( hr );

		UINT32 fpsNumerator = 0, fpsDenominator = 0;
		hr = MFGetAttributeRatio( pMediaType, MF_MT_FRAME_RATE, &fpsNumerator, &fpsDenominator );
		BREAK_ON_FAIL( hr );

		pDesc->Format = (D3DFORMAT)guidSubType.Data1;
		pDesc->SampleWidth = width;
		pDesc->SampleHeight = height;
		pDesc->InputSampleFreq.Numerator = fpsNumerator;
		pDesc->InputSampleFreq.Denominator = fpsDenominator;

		hr = GetDXVA2ExtendedFormat( pMediaType, &pDesc->SampleFormat );
		BREAK_ON_FAIL( hr );

		pDesc->OutputFrameFreq = pDesc->InputSampleFreq;
		if( ( pDesc->SampleFormat.SampleFormat == DXVA2_SampleFieldInterleavedEvenFirst ) || ( pDesc->SampleFormat.SampleFormat == DXVA2_SampleFieldInterleavedOddFirst ) ) {
			pDesc->OutputFrameFreq.Numerator *= 2;
		}
	} while( FALSE );

	return hr;
}

//------------------------------------------------------------------------------------
}
} // namespace ci::wmf