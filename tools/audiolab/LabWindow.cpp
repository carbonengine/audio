// Copyright © 2026 CCP ehf.

#include "LabWindow.h"

#include "Platform.h"

#include <imgui.h>

namespace AudioLab
{
	void SetUpImGui( std::string& iniPath )
	{
		IMGUI_CHECKVERSION();
		ImGui::CreateContext();
		ImGuiIO& io = ImGui::GetIO();
		io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
		iniPath = ToUtf8Path( Platform::GetUserConfigFolder() / "imgui.ini" );
		io.IniFilename = iniPath.c_str();
	}

	void ApplyImGuiScale( float sizeScale, float fontScale )
	{
		ImGuiStyle& style = ImGui::GetStyle();
		style = ImGuiStyle();
		ImGui::StyleColorsDark();
		style.ScaleAllSizes( sizeScale );

		ImGuiIO& io = ImGui::GetIO();
		io.Fonts->Clear();
		ImFontConfig font;
		font.SizePixels = 13.0f * fontScale;
		io.Fonts->AddFontDefault( &font );
		io.FontGlobalScale = sizeScale / fontScale;
	}
}

#if defined( _WIN32 )

#include <imgui_impl_dx11.h>
#include <imgui_impl_win32.h>

#include <d3d11.h>
#include <windows.h>

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

namespace AudioLab
{
	struct LabWindow::Impl
	{
		HWND window = nullptr;
		HINSTANCE instance = nullptr;
		ID3D11Device* device = nullptr;
		ID3D11DeviceContext* context = nullptr;
		IDXGISwapChain* swapChain = nullptr;
		ID3D11RenderTargetView* renderTarget = nullptr;
		UINT resizeWidth = 0;
		UINT resizeHeight = 0;
		bool quit = false;
		bool occluded = false;
		bool dpiChanged = false;
		float dpiScale = 1.0f;
		std::string iniPath;

		void CreateRenderTarget()
		{
			ID3D11Texture2D* backBuffer = nullptr;
			swapChain->GetBuffer( 0, IID_PPV_ARGS( &backBuffer ) );
			device->CreateRenderTargetView( backBuffer, nullptr, &renderTarget );
			backBuffer->Release();
		}

		void ReleaseRenderTarget()
		{
			if( renderTarget != nullptr )
			{
				renderTarget->Release();
				renderTarget = nullptr;
			}
		}

		bool CreateDevice()
		{
			DXGI_SWAP_CHAIN_DESC desc = {};
			desc.BufferCount = 2;
			desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
			desc.BufferDesc.RefreshRate.Numerator = 60;
			desc.BufferDesc.RefreshRate.Denominator = 1;
			desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
			desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
			desc.OutputWindow = window;
			desc.SampleDesc.Count = 1;
			desc.Windowed = TRUE;
			desc.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

			const D3D_FEATURE_LEVEL levels[] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0 };
			D3D_FEATURE_LEVEL level;
			HRESULT result = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &desc, &swapChain, &device, &level, &context );
			if( result == DXGI_ERROR_UNSUPPORTED )
			{
				result = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_WARP, nullptr, 0, levels, 2, D3D11_SDK_VERSION, &desc, &swapChain, &device, &level, &context );
			}
			if( FAILED( result ) )
			{
				return false;
			}
			CreateRenderTarget();
			return true;
		}
	};

	namespace
	{
		LabWindow::Impl* s_window = nullptr;

		LRESULT WINAPI WindowProc( HWND window, UINT message, WPARAM wParam, LPARAM lParam )
		{
			if( ImGui_ImplWin32_WndProcHandler( window, message, wParam, lParam ) )
			{
				return true;
			}
			switch( message )
			{
			case WM_SIZE:
				if( s_window != nullptr && wParam != SIZE_MINIMIZED )
				{
					s_window->resizeWidth = LOWORD( lParam );
					s_window->resizeHeight = HIWORD( lParam );
				}
				return 0;
			case WM_DPICHANGED:
				if( s_window != nullptr )
				{
					s_window->dpiScale = HIWORD( wParam ) / 96.0f;
					s_window->dpiChanged = true;
					const RECT* suggested = reinterpret_cast<const RECT*>( lParam );
					SetWindowPos( window, nullptr, suggested->left, suggested->top, suggested->right - suggested->left, suggested->bottom - suggested->top, SWP_NOZORDER | SWP_NOACTIVATE );
				}
				return 0;
			case WM_SYSCOMMAND:
				if( ( wParam & 0xfff0 ) == SC_KEYMENU )
				{
					return 0;
				}
				break;
			case WM_DESTROY:
				PostQuitMessage( 0 );
				return 0;
			}
			return DefWindowProcW( window, message, wParam, lParam );
		}
	}

	LabWindow::LabWindow() : m_impl( std::make_unique<Impl>() )
	{
	}

	LabWindow::~LabWindow() = default;

	bool LabWindow::Create( const char* title, int width, int height, std::string& error )
	{
		s_window = m_impl.get();
		ImGui_ImplWin32_EnableDpiAwareness();
		m_impl->instance = GetModuleHandleW( nullptr );
		const WNDCLASSEXW windowClass = { sizeof( windowClass ), CS_CLASSDC, WindowProc, 0L, 0L, m_impl->instance, nullptr, LoadCursorW( nullptr, IDC_ARROW ), nullptr, nullptr, L"AudioLab", nullptr };
		RegisterClassExW( &windowClass );

		const POINT origin = { 100, 100 };
		const float dpiScale = ImGui_ImplWin32_GetDpiScaleForMonitor( MonitorFromPoint( origin, MONITOR_DEFAULTTOPRIMARY ) );
		m_impl->window = CreateWindowW( windowClass.lpszClassName, ToWide( title ).c_str(), WS_OVERLAPPEDWINDOW, origin.x, origin.y,
										static_cast<int>( width * dpiScale ), static_cast<int>( height * dpiScale ), nullptr, nullptr, m_impl->instance, nullptr );
		if( m_impl->window == nullptr || !m_impl->CreateDevice() )
		{
			error = "Direct3D 11 could not be initialised.";
			return false;
		}
		ShowWindow( m_impl->window, SW_SHOWDEFAULT );
		UpdateWindow( m_impl->window );

		SetUpImGui( m_impl->iniPath );
		ImGui_ImplWin32_Init( m_impl->window );
		ImGui_ImplDX11_Init( m_impl->device, m_impl->context );
		m_impl->dpiScale = ImGui_ImplWin32_GetDpiScaleForHwnd( m_impl->window );
		ApplyImGuiScale( m_impl->dpiScale, m_impl->dpiScale );
		return true;
	}

	bool LabWindow::BeginFrame()
	{
		MSG message;
		while( PeekMessageW( &message, nullptr, 0U, 0U, PM_REMOVE ) )
		{
			TranslateMessage( &message );
			DispatchMessageW( &message );
			if( message.message == WM_QUIT )
			{
				m_impl->quit = true;
			}
		}
		if( m_impl->quit )
		{
			return false;
		}

		if( m_impl->resizeWidth != 0 && m_impl->resizeHeight != 0 )
		{
			m_impl->ReleaseRenderTarget();
			m_impl->swapChain->ResizeBuffers( 0, m_impl->resizeWidth, m_impl->resizeHeight, DXGI_FORMAT_UNKNOWN, 0 );
			m_impl->resizeWidth = m_impl->resizeHeight = 0;
			m_impl->CreateRenderTarget();
		}
		if( m_impl->dpiChanged )
		{
			m_impl->dpiChanged = false;
			ApplyImGuiScale( m_impl->dpiScale, m_impl->dpiScale );
			ImGui_ImplDX11_InvalidateDeviceObjects();
		}

		ImGui_ImplDX11_NewFrame();
		ImGui_ImplWin32_NewFrame();
		ImGui::NewFrame();
		return true;
	}

	void LabWindow::EndFrame()
	{
		ImGui::Render();
		const float clear[4] = { 0.08f, 0.08f, 0.09f, 1.0f };
		m_impl->context->OMSetRenderTargets( 1, &m_impl->renderTarget, nullptr );
		m_impl->context->ClearRenderTargetView( m_impl->renderTarget, clear );
		ImGui_ImplDX11_RenderDrawData( ImGui::GetDrawData() );
		const HRESULT result = m_impl->swapChain->Present( m_impl->occluded ? 0 : 1, m_impl->occluded ? DXGI_PRESENT_TEST : 0 );
		m_impl->occluded = result == DXGI_STATUS_OCCLUDED;
		if( m_impl->occluded )
		{
			Sleep( 16 );
		}
	}

	void LabWindow::Destroy()
	{
		if( m_impl->window == nullptr )
		{
			return;
		}
		ImGui::SaveIniSettingsToDisk( m_impl->iniPath.c_str() );
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
		m_impl->ReleaseRenderTarget();
		m_impl->swapChain->Release();
		m_impl->context->Release();
		m_impl->device->Release();
		DestroyWindow( m_impl->window );
		UnregisterClassW( L"AudioLab", m_impl->instance );
		m_impl->window = nullptr;
		s_window = nullptr;
	}

	float LabWindow::GetUiScale() const
	{
		return m_impl->dpiScale;
	}
}

#endif
