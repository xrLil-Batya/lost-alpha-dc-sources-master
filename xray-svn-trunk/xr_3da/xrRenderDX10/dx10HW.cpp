// dx10HW.cpp: implementation of the DX10 specialisation of CHW.
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#pragma hdrstop

#pragma warning(disable:4995)
#include <d3dx9.h>
#pragma warning(default:4995)
#ifdef USE_DX11
#include <d3dx11.h>
#else
#include <d3dx10.h>
#endif
#include "../xrRender/HW.h"
#include "../XR_IOConsole.h"
#include "../../Include/xrAPI/xrAPI.h"

#include "StateManager\dx10SamplerStateCache.h"
#include "StateManager\dx10StateCache.h"

#ifndef _EDITOR
void	fill_vid_mode_list			(CHW* _hw);
void	free_vid_mode_list			();

void	fill_render_mode_list		();
void	free_render_mode_list		();
#else
void	fill_vid_mode_list			(CHW* _hw)	{}
void	free_vid_mode_list			()			{}
void	fill_render_mode_list		()			{}
void	free_render_mode_list		()			{}
#endif

#ifdef USE_DX11
#pragma comment (lib, "d3dx11.lib")
#else
#pragma comment (lib, "d3dx10.lib")
#endif

CHW			HW;

CHW::CHW() : 
	m_pAdapter(0),
	pDevice(NULL),
	m_move_window(true)
{
	Device.seqAppActivate.Add(this);
	Device.seqAppDeactivate.Add(this);
}

CHW::~CHW()
{
	Device.seqAppActivate.Remove(this);
	Device.seqAppDeactivate.Remove(this);
}
//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////
void CHW::CreateD3D()
{
	IDXGIFactory * pFactory;
	R_CHK( CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)(&pFactory)) );

	m_pAdapter = 0;
	m_bUsePerfhud = false;

#ifndef	MASTER_GOLD
	// Look for 'NVIDIA NVPerfHUD' adapter
	// If it is present, override default settings
	UINT i = 0;
	while(pFactory->EnumAdapters(i++, &m_pAdapter) != DXGI_ERROR_NOT_FOUND)
	{
		DXGI_ADAPTER_DESC desc;
		m_pAdapter->GetDesc(&desc);
		if(!wcscmp(desc.Description,L"NVIDIA PerfHUD"))
		{
			m_bUsePerfhud = true;
			break;
		}
		else
		{
			_RELEASE(m_pAdapter);
		}
	}
#endif	//	MASTER_GOLD

	if (!m_pAdapter)
	{
		HRESULT R = pFactory->EnumAdapters(0, &m_pAdapter);
		if (FAILED(R))
		{
			// Fatal error! Cannot get the primary graphics adapter AT STARTUP !!!
			Msg					("Failed to initialize graphics hardware.\n"
								 "Please try to restart the game.\n"
								 "EnumAdapters returned 0x%08x", R
								 );
			FlushLog			();
			MessageBox			(NULL,"Failed to initialize graphics hardware.\nPlease try to restart the game.","Error!",MB_OK|MB_ICONERROR);
			TerminateProcess	(GetCurrentProcess(),0);
		}
	}

	_RELEASE(pFactory);
}

void CHW::DestroyD3D()
{
	_SHOW_REF				("refCount:m_pAdapter",m_pAdapter);
	_RELEASE				(m_pAdapter);
}

#if defined(USE_DX10) || defined(USE_DX11)
#if defined(USE_DX11)
HRESULT CHW::CreateDeviceDX11()
	{
	HRESULT R;
	UINT createDeviceFlags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef DEBUG
	createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

	D3D_FEATURE_LEVEL pFeatureLevels[] = {
		D3D_FEATURE_LEVEL_11_0,
	   };

	R = D3D11CreateDeviceAndSwapChain(
		NULL,
		m_DriverType,
		NULL,
		createDeviceFlags,
		pFeatureLevels,
		sizeof(pFeatureLevels) / sizeof(pFeatureLevels[0]),
		D3D11_SDK_VERSION,
		&m_ChainDesc,
		&m_pSwapChain,
		&pDevice,
		&FeatureLevel,
		&pContext
	);

	return R;
	}
#else
HRESULT CHW::CreateDeviceDX10()
	{
	HRESULT R;
	UINT createDeviceFlags = D3D10_CREATE_DEVICE_BGRA_SUPPORT;
#ifdef DEBUG
	createDeviceFlags |= D3D10_CREATE_DEVICE_DEBUG;
#endif

	FeatureLevel = D3D_FEATURE_LEVEL_10_1;
	R = D3D10CreateDeviceAndSwapChain1(
		m_pAdapter,
		m_DriverType,
		NULL,
		createDeviceFlags,
		D3D10_FEATURE_LEVEL_10_1,
		D3D10_1_SDK_VERSION,
		&m_ChainDesc,
		&m_pSwapChain,
		&pDevice1
	);
	if (!SUCCEEDED(R))
		{
		FeatureLevel = D3D_FEATURE_LEVEL_10_0;
		R = D3D10CreateDeviceAndSwapChain1(
			m_pAdapter,
			m_DriverType,
			NULL,
			createDeviceFlags,
			D3D10_FEATURE_LEVEL_10_0,
			D3D10_1_SDK_VERSION,
			&m_ChainDesc,
			&m_pSwapChain,
			&pDevice1
		);
		}

	pDevice = pDevice1;
	pContext = pDevice;

	return R;
	}
#endif
#endif

void CHW::CreateDevice( HWND m_hWnd, bool move_window )
{
	m_move_window			= move_window;
	CreateD3D();

	// TODO: DX10: Create appropriate initialization

	// General - select adapter and device
	BOOL  bWindowed			= !psDeviceFlags.is(rsFullscreen);

	m_DriverType = (Caps.bForceGPU_REF || m_bUsePerfhud) ? 
		D3D_DRIVER_TYPE_REFERENCE : D3D_DRIVER_TYPE_HARDWARE;

	// Display the name of video board
	DXGI_ADAPTER_DESC Desc;
	R_CHK( m_pAdapter->GetDesc(&Desc) );
	//	Warning: Desc.Description is wide string
	Msg		("* GPU [vendor:%X]-[device:%X]: %S", Desc.VendorId, Desc.DeviceId, Desc.Description);
	Msg		("* GPU driver type: %s", (D3D_DRIVER_TYPE_HARDWARE == m_DriverType) ? "hardware" : "software reference");

	Caps.id_vendor	= Desc.VendorId;
	Caps.id_device	= Desc.DeviceId;

	// Select back-buffer & depth-stencil format
	DXGI_FORMAT&	fTarget	= Caps.fTarget;
	DXGI_FORMAT&	fDepth	= Caps.fDepth;
	
	// Set up the presentation parameters
	ZeroMemory(&m_ChainDesc, sizeof(DXGI_SWAP_CHAIN_DESC));
	selectResolution(m_ChainDesc.BufferDesc.Width, m_ChainDesc.BufferDesc.Height, bWindowed);
	m_ChainDesc.BufferCount = ps_r3_backbuffers_count;
	m_ChainDesc.BufferDesc.Format = selectFormat(true);
	m_ChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	m_ChainDesc.OutputWindow = m_hWnd;
	m_ChainDesc.SampleDesc.Count = 1;
	m_ChainDesc.SampleDesc.Quality = 0;
	m_ChainDesc.Windowed = bWindowed;
	m_ChainDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	if (ps_r3_backbuffers_count > 1)
		m_ChainDesc.SwapEffect = DXGI_SWAP_EFFECT_SEQUENTIAL;
	else
		m_ChainDesc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;
	m_ChainDesc.BufferDesc.RefreshRate = selectRefresh(m_ChainDesc.BufferDesc.Width, 
														m_ChainDesc.BufferDesc.Height, 
														m_ChainDesc.BufferDesc.Format);

	HRESULT R;
	pDevice = NULL;
	pContext = NULL;

#if defined(USE_DX10) || defined(USE_DX11)
#if defined(USE_DX11)
	R = CreateDeviceDX11();
#else
	R = CreateDeviceDX10();
#endif
#endif

   if (FAILED(R))
	   {
	   Msg("Failed to initialize graphics hardware.\n"
		   "Please try to restart the game.\n"
		   "CreateDevice returned 0x%08x", R
	   );
	   FlushLog();
	   MessageBox(NULL, "Failed to initialize graphics hardware.\nPlease try to restart the game.",
				  "Error!",
				  MB_OK | MB_ICONERROR);
	   TerminateProcess(GetCurrentProcess(), 0);
	   }
   
	R_CHK(R);

	_SHOW_REF	("* CREATE: DeviceREF:", HW.pDevice);

	//	Create backbuffer and depth-stencil views here
	UpdateViews();

	//u32	memory									= pDevice->GetAvailableTextureMem	();
	size_t	memory									= Desc.DedicatedVideoMemory;
	Msg		("*     Texture memory: %d M",		memory/(1024*1024));
	//Msg		("*          DDI-level: %2.1f",		float(D3DXGetDriverLevel(pDevice))/100.f);

	switch (FeatureLevel)
		{
		case D3D_FEATURE_LEVEL_10_0:
			Msg("*     Direct3D feature level used: 10.0");
			break;
		case D3D_FEATURE_LEVEL_10_1:
			Msg("*     Direct3D feature level used: 10.1");
			break;
		case D3D_FEATURE_LEVEL_11_0:
			Msg("*     Direct3D feature level used: 11.0");
			break;
		}

#ifndef _EDITOR
	updateWindowProps							(m_hWnd);
	fill_vid_mode_list							(this);
#endif

}

void CHW::DestroyDevice()
{
	//	Destroy state managers
	StateManager.Reset();
	RSManager.ClearStateArray();
	DSSManager.ClearStateArray();
	BSManager.ClearStateArray();
	SSManager.ClearStateArray();

	_SHOW_REF				("refCount:pBaseZB",pBaseZB);
	_RELEASE				(pBaseZB);

	_SHOW_REF				("refCount:pBaseRT",pBaseRT);
	_RELEASE				(pBaseRT);
//#ifdef DEBUG
//	_SHOW_REF				("refCount:dwDebugSB",dwDebugSB);
//	_RELEASE				(dwDebugSB);
//#endif

	//	Must switch to windowed mode to release swap chain
	if (!m_ChainDesc.Windowed) m_pSwapChain->SetFullscreenState( FALSE, NULL);
	_SHOW_REF				("refCount:m_pSwapChain",m_pSwapChain);
	_RELEASE				(m_pSwapChain);
	_RELEASE				(pContext);
	_SHOW_REF				("DeviceREF:",HW.pDevice);
	_RELEASE				(HW.pDevice);


	DestroyD3D				();

#ifndef _EDITOR
	free_vid_mode_list		();
#endif
}

//////////////////////////////////////////////////////////////////////
// Resetting device
//////////////////////////////////////////////////////////////////////
void CHW::Reset (HWND hwnd)
{
	DXGI_SWAP_CHAIN_DESC &cd = m_ChainDesc;
	BOOL bWindowed = psDeviceFlags.is(rsFullscreen);
	cd.Windowed = !bWindowed;

	DXGI_MODE_DESC	&desc = m_ChainDesc.BufferDesc;
	selectResolution(desc.Width, desc.Height, !bWindowed);
	desc.RefreshRate = selectRefresh(desc.Width, desc.Height, desc.Format);


#ifdef DEBUG
	//	_RELEASE			(dwDebugSB);
#endif
	_SHOW_REF				("refCount:pBaseZB",pBaseZB);
	_SHOW_REF				("refCount:pBaseRT",pBaseRT);

	_RELEASE(pBaseZB);
	_RELEASE(pBaseRT);

	m_pSwapChain->SetFullscreenState(bWindowed, NULL);
	CHK_DX(m_pSwapChain->ResizeTarget(&desc));
	CHK_DX(m_pSwapChain->ResizeBuffers(
		cd.BufferCount,
		desc.Width,
		desc.Height,
		desc.Format,
		DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));

	UpdateViews();

	updateWindowProps	(hwnd);

}

void CHW::safeClearState()
{
	if (S_OK == lastPresentStatus)
		pContext->ClearState();
}

bool CHW::isSupportingColorFormat(DXGI_FORMAT format, D3D10_FORMAT_SUPPORT support)
{
	UINT values = 0;
	if (!pDevice)
	  return false;
	if (FAILED(pDevice->CheckFormatSupport(format, &values)))
		return false;

	if (values && support)
		return true;

	return false;
}

DXGI_FORMAT CHW::selectFormat(bool isForOutput)
{
	DXGI_FORMAT format = DXGI_FORMAT_UNKNOWN;
	if (isForOutput)
		format = DXGI_FORMAT_R8G8B8A8_UNORM;
	else
		format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	return format;
}

DXGI_FORMAT CHW::selectDepthStencil	(DXGI_FORMAT fTarget)
{
	// this is UNUSED!
	return DXGI_FORMAT_D24_UNORM_S8_UINT;  // D3DFMT_D24S8
}

void CHW::selectResolution( u32 &dwWidth, u32 &dwHeight, BOOL bWindowed )
{
	fill_vid_mode_list			(this);

	if(bWindowed)
	{
		dwWidth		= psCurrentVidMode[0];
		dwHeight	= psCurrentVidMode[1];
	}
	else //check
	{
		string64					buff;
		xr_sprintf					(buff,sizeof(buff),"%dx%d",psCurrentVidMode[0],psCurrentVidMode[1]);

		if(_ParseItem(buff,vid_mode_token)==u32(-1)) //not found
		{ //select safe
			xr_sprintf				(buff,sizeof(buff),"vid_mode %s",vid_mode_token[0].name);
			Console->Execute		(buff);
		}

		dwWidth						= psCurrentVidMode[0];
		dwHeight					= psCurrentVidMode[1];
	}
}

DXGI_RATIONAL CHW::selectRefresh(u32 dwWidth, u32 dwHeight, DXGI_FORMAT fmt)
{
	// utak3r: when resizing target, let DXGI calculate the refresh rate for itself.
	// This is very important for performance, that this value is correct.
	DXGI_RATIONAL refresh;
	refresh.Numerator = 0;
	refresh.Denominator = 1;
	return refresh;
}

void CHW::OnAppActivate()
{
	if ( m_pSwapChain && !m_ChainDesc.Windowed )
	{
		ShowWindow( m_ChainDesc.OutputWindow, SW_RESTORE );
		m_pSwapChain->SetFullscreenState( TRUE, NULL );
		Device.m_pRender->updateGamma();
	}
}

void CHW::OnAppDeactivate()
{
	if ( m_pSwapChain && !m_ChainDesc.Windowed )
	{
		m_pSwapChain->SetFullscreenState( FALSE, NULL );
		ShowWindow( m_ChainDesc.OutputWindow, SW_MINIMIZE );
	}
}


BOOL CHW::support( D3DFORMAT fmt, DWORD type, DWORD usage)
{
	//	TODO: DX10: implement stub for this code.
	VERIFY(!"Implement CHW::support");
	/*
	HRESULT hr		= pD3D->CheckDeviceFormat(DevAdapter,DevT,Caps.fTarget,usage,(D3DRESOURCETYPE)type,fmt);
	if (FAILED(hr))	return FALSE;
	else			return TRUE;
	*/
	return TRUE;
}

void CHW::updateWindowProps(HWND m_hWnd)
{
	// utak3r: with DX10 we no longer want to play with window's styles and flags,
	// DXGI will do this for its own for us, if we're in a fullscreen mode.
	// Thus, we will check if the current mode is a windowed mode
	// and only then perform our actions.

	BOOL	bWindowed				= !psDeviceFlags.is	(rsFullscreen);

	if (bWindowed)		{
		if (m_move_window) {
      LONG dwWindowStyle = 0;
      
			if (strstr(Core.Params,"-no_dialog_header"))
				SetWindowLong	( m_hWnd, GWL_STYLE, dwWindowStyle=(WS_BORDER|WS_VISIBLE) );
			else
				SetWindowLong	( m_hWnd, GWL_STYLE, dwWindowStyle=(WS_BORDER|WS_DLGFRAME|WS_VISIBLE|WS_SYSMENU|WS_MINIMIZEBOX ) );

			RECT			m_rcWindowBounds;
			BOOL			bCenter = TRUE;
			if (strstr(Core.Params, "-no_center_screen"))	bCenter = FALSE;

			if (bCenter) {
				RECT				DesktopRect;

				GetClientRect		(GetDesktopWindow(), &DesktopRect);

				SetRect(			&m_rcWindowBounds, 
					(DesktopRect.right-m_ChainDesc.BufferDesc.Width)/2, 
					(DesktopRect.bottom-m_ChainDesc.BufferDesc.Height)/2, 
					(DesktopRect.right+m_ChainDesc.BufferDesc.Width)/2, 
					(DesktopRect.bottom+m_ChainDesc.BufferDesc.Height)/2);
			}else{
				SetRect(			&m_rcWindowBounds,
					0, 
					0, 
					m_ChainDesc.BufferDesc.Width, 
					m_ChainDesc.BufferDesc.Height);
			};

			AdjustWindowRect(&m_rcWindowBounds, dwWindowStyle, FALSE);

			SetWindowPos(m_hWnd, 
				HWND_NOTOPMOST,	
				m_rcWindowBounds.left, 
				m_rcWindowBounds.top,
				( m_rcWindowBounds.right - m_rcWindowBounds.left ),
				( m_rcWindowBounds.bottom - m_rcWindowBounds.top ),
				SWP_SHOWWINDOW|SWP_NOCOPYBITS|SWP_DRAWFRAME );
		}
	}

	ShowCursor	(FALSE);
	SetForegroundWindow( m_hWnd );
}


struct _uniq_mode
{
	_uniq_mode(LPCSTR v):_val(v){}
	LPCSTR _val;
	bool operator() (LPCSTR _other) {return !stricmp(_val,_other);}
};

#ifndef _EDITOR

void free_vid_mode_list()
{
	for( int i=0; vid_mode_token[i].name; i++ )
	{
		xr_free					(vid_mode_token[i].name);
	}
	xr_free						(vid_mode_token);
	vid_mode_token				= NULL;
}

void fill_vid_mode_list(CHW* _hw)
{
	if(vid_mode_token != NULL)		return;
	xr_vector<LPCSTR>	_tmp;
	xr_vector<DXGI_MODE_DESC>	modes;

	IDXGIOutput *pOutput;
	//_hw->m_pSwapChain->GetContainingOutput(&pOutput);
	_hw->m_pAdapter->EnumOutputs(0, &pOutput);
	VERIFY(pOutput);

	UINT num = 0;
	DXGI_FORMAT format = _hw->selectFormat(true);
	UINT flags         = 0;

	// Get the number of display modes available
	pOutput->GetDisplayModeList( format, flags, &num, 0);

	// Get the list of display modes
	modes.resize(num);
	pOutput->GetDisplayModeList( format, flags, &num, &modes.front());

	_RELEASE(pOutput);

	for (u32 i=0; i<num; ++i)
	{
		DXGI_MODE_DESC &desc = modes[i];
		string32		str;

		if(desc.Width < 800)
			continue;

		xr_sprintf(str, sizeof(str), "%dx%d", desc.Width, desc.Height);

		if(_tmp.end() != std::find_if(_tmp.begin(), _tmp.end(), _uniq_mode(str)))
			continue;

		_tmp.push_back				(NULL);
		_tmp.back()					= xr_strdup(str);
	}
	


//	_tmp.push_back				(NULL);
//	_tmp.back()					= xr_strdup("1024x768");

	u32 _cnt						= _tmp.size()+1;

	vid_mode_token					= xr_alloc<xr_token>(_cnt);

	vid_mode_token[_cnt-1].id			= -1;
	vid_mode_token[_cnt-1].name		= NULL;

#ifdef DEBUG
	Msg("Available video modes[%d]:",_tmp.size());
#endif // DEBUG
	for( u32 i=0; i<_tmp.size(); ++i )
	{
		vid_mode_token[i].id		= i;
		vid_mode_token[i].name		= _tmp[i];
#ifdef DEBUG
		Msg							("[%s]",_tmp[i]);
#endif // DEBUG
	}
}

void CHW::UpdateViews()
{
	DXGI_SWAP_CHAIN_DESC& sd = m_ChainDesc;
	HRESULT R;

	/*
	BOOL currentFullscreen;
	m_pSwapChain->GetFullscreenState(&currentFullscreen, NULL);
	DXGI_MODE_DESC &desc = m_ChainDesc.BufferDesc;
	if (currentFullscreen == psDeviceFlags.is(rsFullscreen))
		{
		m_pSwapChain->SetFullscreenState(!psDeviceFlags.is(rsFullscreen), NULL);
		m_pSwapChain->ResizeTarget(&desc);
		}
	*/

	// Create a render target view
	ID3DTexture2D* pBuffer;
	R = m_pSwapChain->GetBuffer(0, __uuidof(ID3DTexture2D), reinterpret_cast<void**>(&pBuffer));
	R_CHK(R);

	R = pDevice->CreateRenderTargetView(pBuffer, NULL, &pBaseRT);
	pBuffer->Release();
	R_CHK(R);

	//  Create Depth/stencil buffer
	ID3DTexture2D* pDepthStencil = NULL;
	D3D_TEXTURE2D_DESC descDepth;
	descDepth.Width = sd.BufferDesc.Width;
	descDepth.Height = sd.BufferDesc.Height;
	descDepth.MipLevels = 1;
	descDepth.ArraySize = 1;
	descDepth.Format = selectFormat(false);
	descDepth.SampleDesc.Count = 1;
	descDepth.SampleDesc.Quality = 0;
	descDepth.Usage = D3D_USAGE_DEFAULT;
	descDepth.BindFlags = D3D_BIND_DEPTH_STENCIL;
	descDepth.CPUAccessFlags = 0;
	descDepth.MiscFlags = 0;
	R = pDevice->CreateTexture2D(&descDepth, NULL, &pDepthStencil);
	R_CHK(R);

	R = pDevice->CreateDepthStencilView(pDepthStencil, NULL, &pBaseZB);
	R_CHK(R);

	pDepthStencil->Release();
}
#endif
