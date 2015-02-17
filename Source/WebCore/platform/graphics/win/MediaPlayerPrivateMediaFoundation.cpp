/*
 * Copyright (C) 2014 Alex Christensen <achristensen@webkit.org>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "MediaPlayerPrivateMediaFoundation.h"

#include "CachedResourceLoader.h"
#include "FrameView.h"
#include "GraphicsContext.h"
#include "HostWindow.h"
#include "NotImplemented.h"

#if USE(MEDIA_FOUNDATION)

#include <wtf/MainThread.h>

namespace WebCore {

MediaPlayerPrivateMediaFoundation::MediaPlayerPrivateMediaFoundation(MediaPlayer* player) 
    : m_player(player)
    , m_visible(false)
    , m_loadingProgress(false)
    , m_paused(false)
    , m_hasAudio(false)
    , m_hasVideo(false)
    , m_hwndVideo(nullptr)
    , m_readyState(MediaPlayer::HaveNothing)
    , m_mediaSession(nullptr)
    , m_sourceResolver(nullptr)
    , m_mediaSource(nullptr)
    , m_topology(nullptr)
    , m_sourcePD(nullptr)
    , m_videoDisplay(nullptr)
{
    createSession();
    createVideoWindow();
}

MediaPlayerPrivateMediaFoundation::~MediaPlayerPrivateMediaFoundation()
{
    destroyVideoWindow();
    endSession();
}

PassOwnPtr<MediaPlayerPrivateInterface> MediaPlayerPrivateMediaFoundation::create(MediaPlayer* player)
{
    return adoptPtr(new MediaPlayerPrivateMediaFoundation(player));
}

void MediaPlayerPrivateMediaFoundation::registerMediaEngine(MediaEngineRegistrar registrar)
{
    if (isAvailable())
        registrar(create, getSupportedTypes, supportsType, 0, 0, 0, 0);
}

bool MediaPlayerPrivateMediaFoundation::isAvailable() 
{
    notImplemented();
    return true;
}

void MediaPlayerPrivateMediaFoundation::getSupportedTypes(HashSet<String>& types)
{
    types.add(String("video/mp4"));
}

MediaPlayer::SupportsType MediaPlayerPrivateMediaFoundation::supportsType(const MediaEngineSupportParameters& parameters)
{
    if (parameters.type.isNull() || parameters.type.isEmpty())
        return MediaPlayer::IsNotSupported;

    if (parameters.type == "video/mp4")
        return MediaPlayer::IsSupported;

    return MediaPlayer::IsNotSupported;
}

void MediaPlayerPrivateMediaFoundation::load(const String& url)
{
    startCreateMediaSource(url);
}

void MediaPlayerPrivateMediaFoundation::cancelLoad()
{
    notImplemented();
}

void MediaPlayerPrivateMediaFoundation::play()
{
    if (!m_mediaSession)
        return;

    PROPVARIANT varStart;
    PropVariantInit(&varStart);
    varStart.vt = VT_EMPTY;

    HRESULT hr = m_mediaSession->Start(nullptr, &varStart);
    ASSERT(SUCCEEDED(hr));

    PropVariantClear(&varStart);

    m_paused = !SUCCEEDED(hr);
}

void MediaPlayerPrivateMediaFoundation::pause()
{
    if (!m_mediaSession)
        return;

    m_paused = SUCCEEDED(m_mediaSession->Pause());
}

IntSize MediaPlayerPrivateMediaFoundation::naturalSize() const 
{
    return m_size;
}

bool MediaPlayerPrivateMediaFoundation::hasVideo() const
{
    return m_hasVideo;
}

bool MediaPlayerPrivateMediaFoundation::hasAudio() const
{
    return m_hasAudio;
}

void MediaPlayerPrivateMediaFoundation::setVisible(bool visible)
{
    m_visible = visible;
}

bool MediaPlayerPrivateMediaFoundation::seeking() const
{
    notImplemented();
    return false;
}

bool MediaPlayerPrivateMediaFoundation::paused() const
{
    return m_paused;
}

MediaPlayer::NetworkState MediaPlayerPrivateMediaFoundation::networkState() const
{ 
    notImplemented();
    return MediaPlayer::Empty;
}

MediaPlayer::ReadyState MediaPlayerPrivateMediaFoundation::readyState() const
{
    return m_readyState;
}

std::unique_ptr<PlatformTimeRanges> MediaPlayerPrivateMediaFoundation::buffered() const
{ 
    notImplemented();
    return PlatformTimeRanges::create();
}

bool MediaPlayerPrivateMediaFoundation::didLoadingProgress() const
{
    return m_loadingProgress;
}

void MediaPlayerPrivateMediaFoundation::setSize(const IntSize& size)
{
    m_size = size;

    if (!m_videoDisplay)
        return;

    LayoutSize scrollOffset;

    FrameView* view = nullptr;
    if (m_player && m_player->cachedResourceLoader() && m_player->cachedResourceLoader()->document())
        view = m_player->cachedResourceLoader()->document()->view();
    if (view)
        scrollOffset = view->scrollOffsetForFixedPosition();

    int xPos = -scrollOffset.width().toInt() + m_lastPaintRect.x();
    int yPos = -scrollOffset.height().toInt() + m_lastPaintRect.y();

    if (m_hwndVideo && !m_lastPaintRect.isEmpty())
        ::MoveWindow(m_hwndVideo, xPos, yPos, m_size.width(), m_size.height(), FALSE);

    RECT rc = { 0, 0, m_size.width(), m_size.height() };
    m_videoDisplay->SetVideoPosition(nullptr, &rc);
}

void MediaPlayerPrivateMediaFoundation::paint(GraphicsContext* context, const IntRect& rect)
{
    if (context->paintingDisabled()
        || !m_player->visible())
        return;

    m_lastPaintRect = rect;

    // We currently let Media Foundation handle the drawing, by providing a handle to the window to draw in.
    // We should instead read individual frames from the stream, and paint them into the graphics context here.

    notImplemented();
}

bool MediaPlayerPrivateMediaFoundation::createSession()
{
    if (FAILED(MFStartup(MF_VERSION, MFSTARTUP_FULL)))
        return false;

    if (FAILED(MFCreateMediaSession(nullptr, &m_mediaSession)))
        return false;

    // Get next event.
    AsyncCallback* callback = new AsyncCallback(this, true);
    HRESULT hr = m_mediaSession->BeginGetEvent(callback, nullptr);
    ASSERT(SUCCEEDED(hr));

    return true;
}

bool MediaPlayerPrivateMediaFoundation::endSession()
{
    if (m_mediaSession) {
        m_mediaSession->Shutdown();
        m_mediaSession->Release();
        m_mediaSession = nullptr;
    }

    HRESULT hr = MFShutdown();
    ASSERT(SUCCEEDED(hr));

    return true;
}

bool MediaPlayerPrivateMediaFoundation::startCreateMediaSource(const String& url)
{
    if (FAILED(MFCreateSourceResolver(&m_sourceResolver)))
        return false;

    COMPtr<IUnknown> cancelCookie;
    Vector<UChar> urlSource = url.charactersWithNullTermination();

    AsyncCallback* callback = new AsyncCallback(this, false);

    if (FAILED(m_sourceResolver->BeginCreateObjectFromURL(urlSource.data(), MF_RESOLUTION_MEDIASOURCE, nullptr, &cancelCookie, callback, nullptr)))
        return false;

    return true;
}

bool MediaPlayerPrivateMediaFoundation::endCreatedMediaSource(IMFAsyncResult* asyncResult)
{
    MF_OBJECT_TYPE objectType;
    COMPtr<IUnknown> source;

    HRESULT hr = m_sourceResolver->EndCreateObjectFromURL(asyncResult, &objectType, &source);
    if (FAILED(hr))
        return false;

    hr = source->QueryInterface(IID_PPV_ARGS(&m_mediaSource));
    if (FAILED(hr))
        return false;

    hr = asyncResult->GetStatus();
    m_loadingProgress = SUCCEEDED(hr);

    callOnMainThread([this] {
        onCreatedMediaSource();
    });

    return true;
}

bool MediaPlayerPrivateMediaFoundation::endGetEvent(IMFAsyncResult* asyncResult)
{
    COMPtr<IMFMediaEvent> event;

    // Get the event from the event queue.
    HRESULT hr = m_mediaSession->EndGetEvent(asyncResult, &event);
    if (FAILED(hr))
        return false;

    // Get the event type.
    MediaEventType mediaEventType;
    hr = event->GetType(&mediaEventType);
    if (FAILED(hr))
        return false;

    switch (mediaEventType) {
    case MESessionTopologySet:
        callOnMainThread([this] {
            onTopologySet();
        });
        break;

    case MESessionClosed:
        break;
    }

    if (mediaEventType != MESessionClosed) {
        // For all other events, ask the media session for the
        // next event in the queue.
        AsyncCallback* callback = new AsyncCallback(this, true);

        hr = m_mediaSession->BeginGetEvent(callback, nullptr);
        if (FAILED(hr))
            return false;
    }

    return true;
}

bool MediaPlayerPrivateMediaFoundation::createTopologyFromSource()
{
    // Create a new topology.
    if (FAILED(MFCreateTopology(&m_topology)))
        return false;

    // Create the presentation descriptor for the media source.
    if (FAILED(m_mediaSource->CreatePresentationDescriptor(&m_sourcePD)))
        return false;

    // Get the number of streams in the media source.
    DWORD sourceStreams = 0;
    if (FAILED(m_sourcePD->GetStreamDescriptorCount(&sourceStreams)))
        return false;

    // For each stream, create the topology nodes and add them to the topology.
    for (DWORD i = 0; i < sourceStreams; i++) {
        if (!addBranchToPartialTopology(i))
            return false;
    }

    return true;
}

bool MediaPlayerPrivateMediaFoundation::addBranchToPartialTopology(int stream)
{
    // Get the stream descriptor for this stream.
    COMPtr<IMFStreamDescriptor> sourceSD;
    BOOL selected = FALSE;
    if (FAILED(m_sourcePD->GetStreamDescriptorByIndex(stream, &selected, &sourceSD)))
        return false;

    // Create the topology branch only if the stream is selected.
    // Otherwise, do nothing.
    if (!selected)
        return true;

    // Create a source node for this stream.
    COMPtr<IMFTopologyNode> sourceNode;
    if (!createSourceStreamNode(sourceSD, sourceNode))
        return false;

    COMPtr<IMFTopologyNode> outputNode;
    if (!createOutputNode(sourceSD, outputNode))
        return false;

    // Add both nodes to the topology.
    if (FAILED(m_topology->AddNode(sourceNode.get())))
        return false;

    if (FAILED(m_topology->AddNode(outputNode.get())))
        return false;

    // Connect the source node to the output node.
    if (FAILED(sourceNode->ConnectOutput(0, outputNode.get(), 0)))
        return false;

    return true;
}

LRESULT CALLBACK MediaPlayerPrivateMediaFoundation::VideoViewWndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    return DefWindowProc(hWnd, message, wParam, lParam);
}

LPCWSTR MediaPlayerPrivateMediaFoundation::registerVideoWindowClass()
{
    const LPCWSTR kVideoWindowClassName = L"WebVideoWindowClass";

    static bool haveRegisteredWindowClass = false;
    if (haveRegisteredWindowClass)
        return kVideoWindowClassName;

    haveRegisteredWindowClass = true;

    WNDCLASSEX wcex;

    wcex.cbSize = sizeof(WNDCLASSEX);

    wcex.style = CS_DBLCLKS;
    wcex.lpfnWndProc = VideoViewWndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = nullptr;
    wcex.hIcon = nullptr;
    wcex.hCursor = ::LoadCursor(0, IDC_ARROW);
    wcex.hbrBackground = nullptr;
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = kVideoWindowClassName;
    wcex.hIconSm = nullptr;

    if (RegisterClassEx(&wcex))
        return kVideoWindowClassName;

    return nullptr;
}

void MediaPlayerPrivateMediaFoundation::createVideoWindow()
{
    HWND hWndParent = nullptr;
    FrameView* view = nullptr;
    if (!m_player || !m_player->cachedResourceLoader() || !m_player->cachedResourceLoader()->document())
        return;
    view = m_player->cachedResourceLoader()->document()->view();
    if (!view || !view->hostWindow())
        return;
    hWndParent = view->hostWindow()->platformPageClient();

    m_hwndVideo = CreateWindowEx(WS_EX_NOACTIVATE | WS_EX_TRANSPARENT, registerVideoWindowClass(), 0, WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | WS_CLIPCHILDREN,
        0, 0, 0, 0, hWndParent, 0, 0, 0);
}

void MediaPlayerPrivateMediaFoundation::destroyVideoWindow()
{
    if (m_hwndVideo) {
        DestroyWindow(m_hwndVideo);
        m_hwndVideo = nullptr;
    }
}

bool MediaPlayerPrivateMediaFoundation::createOutputNode(COMPtr<IMFStreamDescriptor> sourceSD, COMPtr<IMFTopologyNode>& node)
{
    if (!sourceSD)
        return false;

#ifndef NDEBUG
    // Get the stream ID.
    DWORD streamID = 0;
    sourceSD->GetStreamIdentifier(&streamID); // Just for debugging, ignore any failures.
#endif

    COMPtr<IMFMediaTypeHandler> handler;
    if (FAILED(sourceSD->GetMediaTypeHandler(&handler)))
        return false;

    GUID guidMajorType = GUID_NULL;
    if (FAILED(handler->GetMajorType(&guidMajorType)))
        return false;

    // Create a downstream node.
    if (FAILED(MFCreateTopologyNode(MF_TOPOLOGY_OUTPUT_NODE, &node)))
        return false;

    // Create an IMFActivate object for the renderer, based on the media type.
    COMPtr<IMFActivate> rendererActivate;
    if (MFMediaType_Audio == guidMajorType) {
        // Create the audio renderer.
        if (FAILED(MFCreateAudioRendererActivate(&rendererActivate)))
            return false;
        m_hasAudio = true;
    } else if (MFMediaType_Video == guidMajorType) {
        // Create the video renderer.
        if (FAILED(MFCreateVideoRendererActivate(m_hwndVideo, &rendererActivate)))
            return false;
        m_hasVideo = true;
    } else
        return false;

    // Set the IActivate object on the output node.
    if (FAILED(node->SetObject(rendererActivate.get())))
        return false;

    return true;
}

bool MediaPlayerPrivateMediaFoundation::createSourceStreamNode(COMPtr<IMFStreamDescriptor> sourceSD, COMPtr<IMFTopologyNode>& node)
{
    if (!m_mediaSource || !m_sourcePD || !sourceSD)
        return false;

    // Create the source-stream node.
    HRESULT hr = MFCreateTopologyNode(MF_TOPOLOGY_SOURCESTREAM_NODE, &node);
    if (FAILED(hr))
        return false;

    // Set attribute: Pointer to the media source.
    hr = node->SetUnknown(MF_TOPONODE_SOURCE, m_mediaSource.get());
    if (FAILED(hr))
        return false;

    // Set attribute: Pointer to the presentation descriptor.
    hr = node->SetUnknown(MF_TOPONODE_PRESENTATION_DESCRIPTOR, m_sourcePD.get());
    if (FAILED(hr))
        return false;

    // Set attribute: Pointer to the stream descriptor.
    hr = node->SetUnknown(MF_TOPONODE_STREAM_DESCRIPTOR, sourceSD.get());
    if (FAILED(hr))
        return false;

    return true;
}

void MediaPlayerPrivateMediaFoundation::onCreatedMediaSource()
{
    if (!createTopologyFromSource())
        return;

    // Set the topology on the media session.
    HRESULT hr = m_mediaSession->SetTopology(0, m_topology.get());
    ASSERT(SUCCEEDED(hr));
}

void MediaPlayerPrivateMediaFoundation::onTopologySet()
{
    if (FAILED(MFGetService(m_mediaSession.get(), MR_VIDEO_RENDER_SERVICE, IID_PPV_ARGS(&m_videoDisplay))))
        return;

    ASSERT(m_videoDisplay);

    RECT rc = { 0, 0, m_size.width(), m_size.height() };
    m_videoDisplay->SetVideoPosition(nullptr, &rc);

    m_readyState = MediaPlayer::HaveFutureData;

    ASSERT(m_player);
    m_player->readyStateChanged();

    play();
    m_player->playbackStateChanged();
}

MediaPlayerPrivateMediaFoundation::AsyncCallback::AsyncCallback(MediaPlayerPrivateMediaFoundation* mediaPlayer, bool event)
    : m_refCount(0)
    , m_mediaPlayer(mediaPlayer)
    , m_event(event)
{
}

MediaPlayerPrivateMediaFoundation::AsyncCallback::~AsyncCallback()
{
}

HRESULT MediaPlayerPrivateMediaFoundation::AsyncCallback::QueryInterface(REFIID riid, __RPC__deref_out void __RPC_FAR *__RPC_FAR *ppvObject)
{
    if (!IsEqualGUID(riid, IID_IMFAsyncCallback)) {
        *ppvObject = nullptr;
        return E_NOINTERFACE;
    }
    *ppvObject = this;
    AddRef();
    return S_OK;
}

ULONG STDMETHODCALLTYPE MediaPlayerPrivateMediaFoundation::AsyncCallback::AddRef()
{
    m_refCount++;
    return m_refCount;
}

ULONG STDMETHODCALLTYPE MediaPlayerPrivateMediaFoundation::AsyncCallback::Release()
{
    m_refCount--;
    ULONG refCount = m_refCount;
    if (!refCount)
        delete this;
    return refCount;
}

HRESULT STDMETHODCALLTYPE MediaPlayerPrivateMediaFoundation::AsyncCallback::GetParameters(__RPC__out DWORD *pdwFlags, __RPC__out DWORD *pdwQueue)
{
    // Returning E_NOTIMPL gives default values.
    return E_NOTIMPL;
}

HRESULT STDMETHODCALLTYPE MediaPlayerPrivateMediaFoundation::AsyncCallback::Invoke(__RPC__in_opt IMFAsyncResult *pAsyncResult)
{
    if (m_event)
        m_mediaPlayer->endGetEvent(pAsyncResult);
    else
        m_mediaPlayer->endCreatedMediaSource(pAsyncResult);

    return S_OK;
}

}

#endif
