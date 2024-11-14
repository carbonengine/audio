#include "StdAfx.h"
#include "LogBridge.h"
#include "AudManager.h"

static CcpLogChannel_t s_ch = CCP_LOG_DEFINE_CHANNEL( "Wwise" );

void WwiseLogServerMessageHandler( ErrorCode in_eErrorCode, const AkOSChar* in_pszError, ErrorLevel in_eErrorLevel, AkPlayingID in_playingID, AkGameObjectID in_gameObjID )
{
#ifndef AK_OPTIMIZED
	switch( in_eErrorLevel )
	{
		case ErrorLevel_Error :
			switch (in_eErrorCode)
			{
			case ErrorCode_SelectedNodeNotAvailable: case ErrorCode_SelectedMediaNotAvailable:
				if (in_playingID > 0 && in_gameObjID > 0)
				{
					std::wstring eventName = g_audioManager->GetEventName(in_gameObjID, in_playingID);
					if(!eventName.empty())
					{
						CCP_LOGERR_CH(
							s_ch, 
							"A sound behind Wwise event %S is either not included in any currently loaded SoundBanks or doesn't exist anymore. "
							"Remotely connect with the Wwise authoring program to be able to determine what sound is responsible for this error message.", 
							eventName.c_str()
						);
					}
					else
					{
						CCP_LOGERR_CH( 
							s_ch, 
							"An unknown Wwise event threw the following error: %S. "
							"Remotely connect with the Wwise authoring program to be able to determine what sound is responsible for this error message.", 
							in_pszError 
					);
					}
				}
				break;
			case ErrorCode_UnknownGameObject:
				CCP_LOGERR_CH(s_ch, "A Wwise API call was made on game object %d which does not exist in Wwise. Make sure game objects are registered before calling Wwise "
					"methods on them.", in_gameObjID);
				break;
			case ErrorCode_3DObjectLimitExceeded:
				// We just want to ignore this error in loglite as it is a bit too spammy. It will still be visible when remotely connected with Wwise however.
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
#endif
}

void WwiseLogServerBridgeInit( ErrorLevel errorLevel )
{
	AKRESULT result = SetLocalOutput( errorLevel, &WwiseLogServerMessageHandler);
	if ( result != AK_NotCompatible )
	{
		CCP_LOG_CH( s_ch, "Wwise LogBridge initialized." );
	}
}