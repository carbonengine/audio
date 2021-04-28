// Send Wwise errors directly to our error logging system if running the Debug or Profile versions of the Wwise SDK.
#include "stdafx.h"
#include "LogBridge.h"

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "Wwise" );

void WwiseLogServerMessageHandler( ErrorCode in_eErrorCode, const wchar_t *in_pszError, ErrorLevel in_eErrorLevel, AkPlayingID in_playingID, AkGameObjectID in_gameObjID )
{
	switch( in_eErrorLevel )
	{
		case AK::Monitor::ErrorLevel_Error :
			CCP_LOGERR_CH( s_ch, "%S", in_pszError );
			break;
		case AK::Monitor::ErrorLevel_Message :
			CCP_LOGWARN_CH( s_ch, "%S", in_pszError );
			break;
		default:
			break;
	}
}

void WwiseLogServerBridgeInit( AK::Monitor::ErrorLevel errorLevel )
{
	AKRESULT result = AK::Monitor::SetLocalOutput( errorLevel, &WwiseLogServerMessageHandler );
	if ( result != AK_NotCompatible )
	{
		CCP_LOG_CH( s_ch, "Wwise LogBridge initialized." );
	}
}