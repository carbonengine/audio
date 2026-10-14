// Copyright © 2026 CCP ehf.

#include "Platform.h"

#include <cstdio>
#include <cstdlib>

#if defined( _WIN32 )
	#include <windows.h>
	#include <shellapi.h>
#elif defined( __APPLE__ )
	#include <CoreFoundation/CoreFoundation.h>
	#include <mach-o/dyld.h>
	#include <unistd.h>
#endif

namespace AudioLab
{
#if defined( _WIN32 )
	std::wstring ToWide( const std::string& utf8 )
	{
		if( utf8.empty() )
		{
			return {};
		}
		const int length = MultiByteToWideChar( CP_UTF8, 0, utf8.data(), static_cast<int>( utf8.size() ), nullptr, 0 );
		std::wstring wide( length, L'\0' );
		MultiByteToWideChar( CP_UTF8, 0, utf8.data(), static_cast<int>( utf8.size() ), wide.data(), length );
		return wide;
	}

	std::string ToUtf8( const std::wstring& wide )
	{
		if( wide.empty() )
		{
			return {};
		}
		const int length = WideCharToMultiByte( CP_UTF8, 0, wide.data(), static_cast<int>( wide.size() ), nullptr, 0, nullptr, nullptr );
		std::string utf8( length, '\0' );
		WideCharToMultiByte( CP_UTF8, 0, wide.data(), static_cast<int>( wide.size() ), utf8.data(), length, nullptr, nullptr );
		return utf8;
	}

	std::wstring ToWidePath( const std::filesystem::path& path )
	{
		return path.wstring();
	}

	std::filesystem::path FromUtf8Path( const std::string& utf8 )
	{
		return std::filesystem::path( ToWide( utf8 ) );
	}

	std::string ToUtf8Path( const std::filesystem::path& path )
	{
		return ToUtf8( path.wstring() );
	}
#else
	std::wstring ToWide( const std::string& utf8 )
	{
		std::wstring wide;
		wide.reserve( utf8.size() );
		for( size_t i = 0; i < utf8.size(); )
		{
			const unsigned char lead = static_cast<unsigned char>( utf8[i] );
			const int extra = lead < 0x80 ? 0 : lead < 0xE0 ? 1 : lead < 0xF0 ? 2 : 3;
			char32_t code = extra == 0 ? lead : extra == 1 ? lead & 0x1F : extra == 2 ? lead & 0x0F : lead & 0x07;
			for( int k = 1; k <= extra && i + k < utf8.size(); ++k )
			{
				code = ( code << 6 ) | ( static_cast<unsigned char>( utf8[i + k] ) & 0x3F );
			}
			wide.push_back( static_cast<wchar_t>( code ) );
			i += extra + 1;
		}
		return wide;
	}

	std::string ToUtf8( const std::wstring& wide )
	{
		std::string utf8;
		utf8.reserve( wide.size() );
		for( const wchar_t character : wide )
		{
			const char32_t code = static_cast<char32_t>( character );
			if( code < 0x80 )
			{
				utf8.push_back( static_cast<char>( code ) );
			}
			else if( code < 0x800 )
			{
				utf8.push_back( static_cast<char>( 0xC0 | ( code >> 6 ) ) );
				utf8.push_back( static_cast<char>( 0x80 | ( code & 0x3F ) ) );
			}
			else if( code < 0x10000 )
			{
				utf8.push_back( static_cast<char>( 0xE0 | ( code >> 12 ) ) );
				utf8.push_back( static_cast<char>( 0x80 | ( ( code >> 6 ) & 0x3F ) ) );
				utf8.push_back( static_cast<char>( 0x80 | ( code & 0x3F ) ) );
			}
			else
			{
				utf8.push_back( static_cast<char>( 0xF0 | ( code >> 18 ) ) );
				utf8.push_back( static_cast<char>( 0x80 | ( ( code >> 12 ) & 0x3F ) ) );
				utf8.push_back( static_cast<char>( 0x80 | ( ( code >> 6 ) & 0x3F ) ) );
				utf8.push_back( static_cast<char>( 0x80 | ( code & 0x3F ) ) );
			}
		}
		return utf8;
	}

	std::wstring ToWidePath( const std::filesystem::path& path )
	{
		return ToWide( path.string() );
	}

	std::filesystem::path FromUtf8Path( const std::string& utf8 )
	{
		return std::filesystem::path( utf8 );
	}

	std::string ToUtf8Path( const std::filesystem::path& path )
	{
		return path.string();
	}
#endif

	namespace Platform
	{
#if defined( _WIN32 )
		std::vector<std::string> GetArguments( int, char** )
		{
			std::vector<std::string> arguments;
			int count = 0;
			if( LPWSTR* wide = CommandLineToArgvW( GetCommandLineW(), &count ) )
			{
				for( int i = 1; i < count; ++i )
				{
					arguments.push_back( ToUtf8( wide[i] ) );
				}
				LocalFree( wide );
			}
			return arguments;
		}

		std::filesystem::path GetExecutablePath()
		{
			std::wstring buffer( MAX_PATH, L'\0' );
			for( ;; )
			{
				const DWORD length = GetModuleFileNameW( nullptr, buffer.data(), static_cast<DWORD>( buffer.size() ) );
				if( length < buffer.size() )
				{
					buffer.resize( length );
					return std::filesystem::path( buffer );
				}
				buffer.resize( buffer.size() * 2 );
			}
		}

		std::filesystem::path GetUserConfigFolder()
		{
			wchar_t* localAppData = nullptr;
			size_t length = 0;
			std::filesystem::path folder = _wdupenv_s( &localAppData, &length, L"LOCALAPPDATA" ) == 0 && localAppData != nullptr ? std::filesystem::path( localAppData ) : GetExecutablePath().parent_path();
			free( localAppData );
			folder /= L"CarbonAudio";
			folder /= L"AudioLab";
			std::error_code error;
			std::filesystem::create_directories( folder, error );
			return folder;
		}

		void ShowError( const std::string& title, const std::string& message )
		{
			MessageBoxW( nullptr, ToWide( message ).c_str(), ToWide( title ).c_str(), MB_ICONERROR );
		}

		void ExitProcessNow( int exitCode )
		{
			ExitProcess( static_cast<UINT>( exitCode ) );
		}

		int GetProcessId()
		{
			return static_cast<int>( GetCurrentProcessId() );
		}
#elif defined( __APPLE__ )
		std::vector<std::string> GetArguments( int argc, char** argv )
		{
			return std::vector<std::string>( argv + 1, argv + argc );
		}

		std::filesystem::path GetExecutablePath()
		{
			uint32_t size = 0;
			_NSGetExecutablePath( nullptr, &size );
			std::string buffer( size, '\0' );
			_NSGetExecutablePath( buffer.data(), &size );
			std::error_code error;
			const std::filesystem::path path = std::filesystem::canonical( buffer.c_str(), error );
			return error ? std::filesystem::path( buffer.c_str() ) : path;
		}

		std::filesystem::path GetUserConfigFolder()
		{
			const char* home = std::getenv( "HOME" );
			std::filesystem::path folder = home != nullptr ? std::filesystem::path( home ) / "Library/Application Support" : GetExecutablePath().parent_path();
			folder /= "CarbonAudio";
			folder /= "AudioLab";
			std::error_code error;
			std::filesystem::create_directories( folder, error );
			return folder;
		}

		void ShowError( const std::string& title, const std::string& message )
		{
			CFStringRef cfTitle = CFStringCreateWithCString( nullptr, title.c_str(), kCFStringEncodingUTF8 );
			CFStringRef cfMessage = CFStringCreateWithCString( nullptr, message.c_str(), kCFStringEncodingUTF8 );
			CFUserNotificationDisplayAlert( 0, kCFUserNotificationStopAlertLevel, nullptr, nullptr, nullptr, cfTitle, cfMessage, nullptr, nullptr, nullptr, nullptr );
			CFRelease( cfTitle );
			CFRelease( cfMessage );
		}

		void ExitProcessNow( int exitCode )
		{
			_exit( exitCode );
		}

		int GetProcessId()
		{
			return static_cast<int>( getpid() );
		}
#endif
	}
}
