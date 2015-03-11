#pragma once

#include "cinder/Cinder.h"

#if defined(CINDER_MSW)

#include "cinder/msw/CinderMsw.h"
#include "cinder/msw/CinderMswCom.h"
#include "cinder/evr/IPlayer.h"
#include "cinder/evr/MediaFoundationVideo.h"

namespace cinder {
	namespace msw {
		namespace video {

			class MediaFoundationPlayer : public IMFAsyncCallback, public IPlayer {
			public:
				typedef enum PlayerState {
					Closed = 0,
					Ready,
					OpenPending,
					Started,
					Paused,
					Stopped,
					Closing
				};

			public:
				MediaFoundationPlayer( HRESULT &hr, HWND hwnd );
				~MediaFoundationPlayer();

				// IPlayer methods
				//bool CreateSharedTexture( int w, int h, int textureID ) override { return mPresenterPtr->CreateSharedTexture( w, h, textureID ); }
				//void ReleaseSharedTexture( int textureID ) override { mPresenterPtr->ReleaseSharedTexture( textureID ); }
				//bool LockSharedTexture( int *pTextureID, int *pFreeTextures ) override { return mPresenterPtr->LockSharedTexture( pTextureID, pFreeTextures ); }
				//bool UnlockSharedTexture( int textureID ) override { return mPresenterPtr->UnlockSharedTexture( textureID ); }

			protected:
				// IUnknown methods
				STDMETHODIMP QueryInterface( REFIID iid, void** ppv ) override;
				STDMETHODIMP_( ULONG ) AddRef() override;
				STDMETHODIMP_( ULONG ) Release() override;

				// IMFAsyncCallback methods
				STDMETHODIMP  GetParameters( DWORD*, DWORD* ) override { return E_NOTIMPL; } // Implementation of this method is optional.
				STDMETHODIMP  Invoke( IMFAsyncResult* pAsyncResult ) override;

				// IPlayer methods
				HRESULT OpenFile( PCWSTR pszFileName ) override;
				HRESULT Close() override;

				HRESULT Play() override;
				HRESULT Pause() override;
				HRESULT Stop() override;

				HRESULT HandleEvent( UINT_PTR pEventPtr ) override;

				BOOL    IsPlaying() const override { return ( mState == Started ); }
				BOOL    IsPaused() const override { return ( mState == Paused && !mHasEnded ); }
				BOOL    IsStopped() const override { return ( mState == Stopped || ( mState == Paused && mHasEnded ) ); }
				BOOL    IsLooping() const override { return ( mIsLoopEnabled ); }

				UINT32  GetWidth() const override { return mWidth; }
				UINT32  GetHeight() const override { return mHeight; }

				float   GetTime() const override;
				float   GetDuration() const override;
				float   GetFrameRate() const override;
				UINT32  GetNumFrames() const override;

				void    SetLoop( BOOL loop = TRUE ) override { mIsLoopEnabled = loop; }
				void    SeekToTime( FLOAT seconds ) override { SkipToPosition( (MFTIME) ( seconds * 1e7 ) ); }

				//! Returns the latest frame as an OpenGL texture, if available. Returns an empty texture if no (new) frame is available.
				ci::gl::Texture2dRef GetTexture() override { return mPresenterPtr->GetTexture(); }

				//
				HRESULT SkipToPosition( MFTIME seekTime );

				HRESULT CreateSession();
				HRESULT CloseSession();
				HRESULT CreatePartialTopology( IMFPresentationDescriptor *pPD );// { return E_NOTIMPL; }
				HRESULT SetMediaInfo( IMFPresentationDescriptor *pPD );

				HRESULT CorrectAspectRatio( UINT32 *pWidth, UINT32 *pHeight, UINT32 pixelAspectNumerator, UINT32 pixelAspectDenominator );

				HRESULT HandleSessionTopologySetEvent( IMFMediaEvent *pEvent );
				HRESULT HandleSessionTopologyStatusEvent( IMFMediaEvent *pEvent );
				HRESULT HandleEndOfPresentationEvent( IMFMediaEvent *pEvent );
				HRESULT HandleNewPresentationEvent( IMFMediaEvent *pEvent );
				HRESULT HandleSessionEvent( IMFMediaEvent *pEvent, MediaEventType eventType );

				static HRESULT CreateMediaSource( LPCWSTR pUrl, IMFMediaSource **ppSource );
				static HRESULT CreatePlaybackTopology( IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, HWND hVideoWnd, IMFTopology **ppTopology, IMFVideoPresenter *pVideoPresenter );
				static HRESULT CreateMediaSinkActivate( IMFStreamDescriptor *pSourceSD, HWND hVideoWindow, IMFActivate **ppActivate, IMFVideoPresenter *pVideoPresenter, IMFMediaSink **ppMediaSink );
				static HRESULT AddSourceNode( IMFTopology *pTopology, IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, IMFStreamDescriptor *pSD, IMFTopologyNode **ppNode );
				static HRESULT AddOutputNode( IMFTopology *pTopology, IMFStreamSink *pStreamSink, IMFTopologyNode **ppNode );
				static HRESULT AddOutputNode( IMFTopology *pTopology, IMFActivate *pActivate, DWORD dwId, IMFTopologyNode **ppNode );
				static HRESULT AddBranchToPartialTopology( IMFTopology *pTopology, IMFMediaSource *pSource, IMFPresentationDescriptor *pPD, DWORD iStream, HWND hVideoWnd, IMFVideoPresenter *pVideoPresenter );

			protected:
				BOOL                     mIsInitialized;
				BOOL                     mIsLoopEnabled;
				BOOL                     mHasEnded;

				UINT32                   mWidth, mHeight;
				UINT32                   mPixelAspectNumerator, mPixelAspectDenominator;
				UINT32                   mFrameRateNumerator, mFrameRateDenominator;
				float                    mCurrentVolume;

				IMFMediaSession*         mMediaSessionPtr;
				IMFSequencerSource*      mSequencerSourcePtr;
				IMFMediaSource*          mMediaSourcePtr;
				IMFVideoDisplayControl*  mVideoDisplayControlPtr;
				IMFAudioStreamVolume*    mAudioStreamVolumePtr;

				EVRCustomPresenter*      mPresenterPtr;

				HWND                     mHwnd;
				PlayerState              mState;
				HANDLE                   mOpenEventHandle;
				HANDLE                   mCloseEventHandle;

				volatile long            mRefCount;
			};

		} // namespace video
	} // namespace msw
} // namespace cinder

#endif // CINDER_MSW