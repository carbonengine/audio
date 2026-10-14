// Copyright © 2026 CCP ehf.

#include "LabWindow.h"

#include <imgui.h>
#include <imgui_impl_metal.h>
#include <imgui_impl_osx.h>

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/QuartzCore.h>

#include <chrono>
#include <thread>

/// Turns the window's close button into a request the lab's frame loop sees.
@interface AudioLabWindowDelegate : NSObject <NSWindowDelegate>
@property( nonatomic ) bool* closeRequested;
@end

@implementation AudioLabWindowDelegate
- (BOOL)windowShouldClose:(NSWindow*)sender
{
	*self.closeRequested = true;
	return NO;
}
@end

/// Turns Quit (Cmd+Q, the Dock, logging out) into the same request, so the lab stops CarbonAudio and Blue ends the
/// process; otherwise NSApplication would call exit() with Wwise's threads running. Quitting later rather than now
/// would run a modal loop inside the lab's own event loop, so a logout is cancelled and the lab quits by itself.
@interface AudioLabAppDelegate : NSObject <NSApplicationDelegate>
@property( nonatomic ) bool* closeRequested;
@end

@implementation AudioLabAppDelegate
- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication*)sender
{
	*self.closeRequested = true;
	return NSTerminateCancel;
}
@end

namespace AudioLab
{
	struct LabWindow::Impl
	{
		NSWindow* window = nil;
		NSView* view = nil;
		CAMetalLayer* layer = nil;
		id<MTLDevice> device = nil;
		id<MTLCommandQueue> queue = nil;
		MTLRenderPassDescriptor* pass = nil;
		id<CAMetalDrawable> drawable = nil;
		AudioLabWindowDelegate* delegate = nil;
		AudioLabAppDelegate* appDelegate = nil;
		bool closeRequested = false;
		float backingScale = 1.0f;
		std::string iniPath;
	};

	LabWindow::LabWindow() : m_impl( std::make_unique<Impl>() )
	{
	}

	LabWindow::~LabWindow() = default;

	bool LabWindow::Create( const char* title, int width, int height, std::string& error )
	{
		@autoreleasepool
		{
			[NSApplication sharedApplication];
			[NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
			m_impl->appDelegate = [[AudioLabAppDelegate alloc] init];
			m_impl->appDelegate.closeRequested = &m_impl->closeRequested;
			NSApp.delegate = m_impl->appDelegate;

			// The application menu, for Quit and its Cmd+Q.
			NSMenu* menuBar = [[NSMenu alloc] init];
			NSMenuItem* appMenuItem = [[NSMenuItem alloc] init];
			[menuBar addItem:appMenuItem];
			NSMenu* appMenu = [[NSMenu alloc] init];
			[appMenu addItemWithTitle:@"Quit Audio Lab" action:@selector(terminate:) keyEquivalent:@"q"];
			appMenuItem.submenu = appMenu;
			NSApp.mainMenu = menuBar;

			[NSApp finishLaunching];

			m_impl->device = MTLCreateSystemDefaultDevice();
			if( m_impl->device == nil )
			{
				error = "Metal is not available.";
				return false;
			}
			m_impl->queue = [m_impl->device newCommandQueue];

			const NSWindowStyleMask style = NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable;
			m_impl->window = [[NSWindow alloc] initWithContentRect:NSMakeRect( 100, 100, width, height ) styleMask:style backing:NSBackingStoreBuffered defer:NO];
			m_impl->window.title = [NSString stringWithUTF8String:title];
			m_impl->window.releasedWhenClosed = NO;
			m_impl->delegate = [[AudioLabWindowDelegate alloc] init];
			m_impl->delegate.closeRequested = &m_impl->closeRequested;
			m_impl->window.delegate = m_impl->delegate;

			m_impl->layer = [CAMetalLayer layer];
			m_impl->layer.device = m_impl->device;
			m_impl->layer.pixelFormat = MTLPixelFormatBGRA8Unorm;
			m_impl->view = m_impl->window.contentView;
			m_impl->view.layer = m_impl->layer;
			m_impl->view.wantsLayer = YES;

			[m_impl->window makeKeyAndOrderFront:nil];
			[NSApp activateIgnoringOtherApps:YES];

			m_impl->pass = [MTLRenderPassDescriptor new];
			m_impl->pass.colorAttachments[0].loadAction = MTLLoadActionClear;
			m_impl->pass.colorAttachments[0].storeAction = MTLStoreActionStore;
			m_impl->pass.colorAttachments[0].clearColor = MTLClearColorMake( 0.08, 0.08, 0.09, 1.0 );

			SetUpImGui( m_impl->iniPath );
			ImGui_ImplMetal_Init( m_impl->device );
			ImGui_ImplOSX_Init( m_impl->view );
			m_impl->backingScale = static_cast<float>( m_impl->window.backingScaleFactor );
			ApplyImGuiScale( 1.0f, m_impl->backingScale );
		}
		return true;
	}

	bool LabWindow::BeginFrame()
	{
		@autoreleasepool
		{
			for( ;; )
			{
				NSEvent* event = [NSApp nextEventMatchingMask:NSEventMaskAny untilDate:nil inMode:NSDefaultRunLoopMode dequeue:YES];
				if( event == nil )
				{
					break;
				}
				[NSApp sendEvent:event];
			}
			if( m_impl->closeRequested )
			{
				return false;
			}

			const float scale = static_cast<float>( m_impl->window.backingScaleFactor );
			if( scale != m_impl->backingScale )
			{
				m_impl->backingScale = scale;
				ApplyImGuiScale( 1.0f, scale );
				ImGui_ImplMetal_DestroyFontsTexture();
				ImGui_ImplMetal_CreateFontsTexture( m_impl->device );
			}

			const NSSize size = m_impl->view.bounds.size;
			m_impl->layer.contentsScale = scale;
			m_impl->layer.drawableSize = CGSizeMake( size.width * scale, size.height * scale );
			const bool visible = ( m_impl->window.occlusionState & NSWindowOcclusionStateVisible ) != 0 && size.width > 0 && size.height > 0;
			m_impl->drawable = visible ? [m_impl->layer nextDrawable] : nil;
			m_impl->pass.colorAttachments[0].texture = m_impl->drawable != nil ? m_impl->drawable.texture : nil;

			ImGui_ImplMetal_NewFrame( m_impl->pass );
			ImGui_ImplOSX_NewFrame( m_impl->view );
			ImGui::NewFrame();
		}
		return true;
	}

	void LabWindow::EndFrame()
	{
		@autoreleasepool
		{
			ImGui::Render();
			if( m_impl->drawable == nil )
			{
				std::this_thread::sleep_for( std::chrono::milliseconds( 16 ) );
				return;
			}
			id<MTLCommandBuffer> commandBuffer = [m_impl->queue commandBuffer];
			id<MTLRenderCommandEncoder> encoder = [commandBuffer renderCommandEncoderWithDescriptor:m_impl->pass];
			ImGui_ImplMetal_RenderDrawData( ImGui::GetDrawData(), commandBuffer, encoder );
			[encoder endEncoding];
			[commandBuffer presentDrawable:m_impl->drawable];
			[commandBuffer commit];
			m_impl->drawable = nil;
		}
	}

	void LabWindow::Destroy()
	{
		if( m_impl->window == nil )
		{
			return;
		}
		ImGui::SaveIniSettingsToDisk( m_impl->iniPath.c_str() );
		ImGui_ImplMetal_Shutdown();
		ImGui_ImplOSX_Shutdown();
		ImGui::DestroyContext();
		[m_impl->window close];
		m_impl->window = nil;
	}

	float LabWindow::GetUiScale() const
	{
		return 1.0f;
	}
}
