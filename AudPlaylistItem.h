/* 
	*************************************************************************************

	AudPlaylistItem.h

	Author:    Andri Mar
	Created:   June 2009
	OS:        Win32
	Project:   Audio2

	Description:   

		Structure for storing ID3 tags and other metadata and expose them to python


	Dependencies:

		Blue

	(c) CCP 2009

	*************************************************************************************
*/

#pragma once
#ifndef _AUDPLAYLISTITEM_H_
#define _AUDPLAYLISTITEM_H_

#include "Audio2.h"

BLUE_DECLARE( AudMp3Tag );
BLUE_DECLARE( AudMp3Info );

#include <string>

BLUE_CLASS( AudPlaylistItem ) : public IRoot
{
public:
	AudPlaylistItem( IRoot* lockobj = NULL );

	~AudPlaylistItem();

	EXPOSE_TO_BLUE();

    int m_Duration;
    std::wstring m_Title;
    std::wstring m_Artist;
};
TYPEDEF_BLUECLASS( AudPlaylistItem );

#endif