#include "StdAfx.h"
#include "LogBridge.h"

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "Wwise" );

void WwiseLogServerMessageHandler( ErrorCode in_eErrorCode, const AkOSChar* in_pszError, ErrorLevel in_eErrorLevel, AkPlayingID in_playingID, AkGameObjectID in_gameObjID )
{
	switch( in_eErrorLevel )
	{
		case ErrorLevel_Error :
			switch (in_eErrorCode)
			{
			case ErrorCode_UnknownGameObject:
				CCP_LOGERR_CH(s_ch, "A Wwise API call was made on game object %d which does not exist in Wwise. Make sure game objects are registered before calling Wwise "
					"methods on them.", in_gameObjID);
				break;
			default:
				CCP_LOGERR_CH(s_ch, "%S", in_pszError);
				break;
			}
		case ErrorLevel_Message :
			CCP_LOGWARN_CH( s_ch, "%S", in_pszError );
			break;
		default:
			break;
	}
}

void WwiseLogServerBridgeInit( ErrorLevel errorLevel )
{
	AKRESULT result = SetLocalOutput( errorLevel, &WwiseLogServerMessageHandler);
	if ( result != AK_NotCompatible )
	{
		CCP_LOG_CH( s_ch, "Wwise LogBridge initialized." );
	}
}