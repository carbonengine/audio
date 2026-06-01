#include "../stdafx.h"
#include "WaapiManager.h"

// Define chooser for CurveShape enum
const Be::VarChooser CurveShapeChooser[] = {
    { "Linear", static_cast<int>(CurveShape::Linear) },
    { "Log1", static_cast<int>(CurveShape::Log1) },
    { "Log2", static_cast<int>(CurveShape::Log2) },
    { "Log3", static_cast<int>(CurveShape::Log3) },
    { "Exp1", static_cast<int>(CurveShape::Exp1) },
    { "Exp2", static_cast<int>(CurveShape::Exp2) },
    { "Exp3", static_cast<int>(CurveShape::Exp3) },
    { "SCurve", static_cast<int>(CurveShape::SCurve) },
    { "InvSCurve", static_cast<int>(CurveShape::InvSCurve) },
    { nullptr, 0 }
};

BLUE_DEFINE( WaapiManager );
BLUE_REGISTER_ENUM( "CURVE_SHAPE", CurveShape, CurveShapeChooser );

const Be::ClassInfo* WaapiManager::ExposeToBlue()
{
	EXPOSURE_BEGIN( WaapiManager, "Manages connection and communication with Wwise Authoring Tool via WAAPI for development purposes." )
		MAP_INTERFACE( WaapiManager )

		MAP_METHOD_AND_WRAP
		(
			"Connect",
			Connect,
			"Connect to Wwise Authoring Tool via WAAPI."
			":param host: Host address (default: '127.0.0.1')"
			":param port: Port number (default: 8080)"
			":return: True if connection succeeded, False otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"Disconnect",
			Disconnect,
			"Disconnect from Wwise Authoring Tool."
		)
		MAP_METHOD_AND_WRAP
		(
			"IsConnected",
			IsConnected,
			"Check if currently connected to Wwise Authoring Tool."
			":return: True if connected, False otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetEventReferencedTargetIds",
			GetEventReferencedTargetIds,
			"Get IDs of all action targets referenced by a given event.\n"
			":param eventName: Name of the event to query.\n"
			":return: List of target IDs (sounds, containers, etc.), empty if unsuccessful."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetEventReferencedTargetNames",
			GetEventReferencedTargetNames,
			"Get names of all action targets referenced by a given event.\n"
			":param eventName: Name of the event to query.\n"
			":return: List of target names (sounds, containers, etc.), empty if unsuccessful."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetEventReferencedTargetPaths",
			GetEventReferencedTargetPaths,
			"Get path names of all action targets referenced by a given event.\n"
			":param eventName: Name of the event to query.\n"
			":return: List of target path names (sounds, containers, etc.), empty if unsuccessful."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetSoundReferencedAttenuationId",
			GetSoundReferencedAttenuationId,
			"Get ID of attenuation referenced by a given sound.\n"
			":param soundId: ID of the sound to query.\n"
			":return: Attenuation ID if successful, empty string otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetSoundReferencedAttenuationName",
			GetSoundReferencedAttenuationName,
			"Get name of attenuation referenced by a given sound.\n"
			":param soundId: ID of the sound to query.\n"
			":return: Attenuation name if successful, empty string otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetObjectEffectiveAttenuationId",
			GetObjectEffectiveAttenuationId,
			"Get the effective attenuation ID for a Wwise object by following positioning inheritance.\n"
			":param objectId: ID of the object to query.\n"
			":return: Attenuation ID if an override positioning source defines one, empty string otherwise."
		)
#ifndef AK_OPTIMIZED
		MAP_METHOD_AND_WRAP
		(
			"GetObjectEffectiveAttenuationConeAttenuationId",
			GetObjectEffectiveAttenuationConeAttenuationId,
			"Get the attenuation ID selected by the hierarchy cone resolver for a Wwise object.\n"
			":param objectId: ID of the object to query.\n"
			":return: Attenuation ID if cone data could be resolved, empty string otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetAttenuationConeInnerAngle",
			GetAttenuationConeInnerAngle,
			"Get the cone inner angle for a given attenuation.\n"
			":param attenuationId: ID of the attenuation to query.\n"
			":return: Cone inner angle if successful, 0.0 otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetAttenuationConeOuterAngle",
			GetAttenuationConeOuterAngle,
			"Get the cone outer angle for a given attenuation.\n"
			":param attenuationId: ID of the attenuation to query.\n"
			":return: Cone outer angle if successful, 0.0 otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetBestProfilerVoiceObjectIdForPlayingEvent",
			GetBestProfilerVoiceObjectIdForPlayingEvent,
			"Get the best currently voiced Wwise object GUID for a game object and playing ID from profiler voice data.\n"
			":param gameObjectId: Wwise game object ID.\n"
			":param playingId: Wwise playing ID.\n"
			":return: Voice object GUID if available, empty string otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"SelectDebugProfilerVoiceObjectIdForTest",
			SelectDebugProfilerVoiceObjectIdForTest,
			"Test seam for debug cone profiler voice selection.\n"
			":param gameObjectId: Target Wwise game object ID.\n"
			":param playingId: Target Wwise playing ID."
		)
#endif
		MAP_METHOD_AND_WRAP
		(
			"GetAttenuationVolumeCurveDistances",
			GetAttenuationVolumeCurveDistances,
			"Get distance values from volume attenuation curve.\n"
			":param attenuationId: ID of the attenuation to query.\n"
			":return: List of distance values for each curve point, empty if unsuccessful."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetAttenuationVolumeCurveValues",
			GetAttenuationVolumeCurveValues,
			"Get volume values from volume attenuation curve.\n"
			":param attenuationId: ID of the attenuation to query.\n"
			":return: List of volume values for each curve point (in dB), empty if unsuccessful."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetAttenuationName",
			GetAttenuationName,
			"Get the name of an attenuation object by its ID.\n"
			":param attenuationId: ID of the attenuation to query.\n"
			":return: Attenuation name if successful, empty string otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetAttenuationMaxRadius",
			GetAttenuationMaxRadius,
			"Get maximum radius for a given attenuation.\n"
			":param attenuationId: ID of the attenuation to query.\n"
			":return: Radius value if successful, 0.0 if unsuccessful."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetAttenuationMaxRadius",
			SetAttenuationMaxRadius,
			"Set maximum radius for a specific attenuation object.\n"
			":param attenuationId: ID of the attenuation to modify.\n"
			":param radius: Radius value to set.\n"
			":return: True if successful, False otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetAttenuationVolumeCurve",
			SetAttenuationVolumeCurve,
			"Set volume attenuation curve for a given attenuation object.\n"
			":param attenuationId: ID of the attenuation to modify.\n"
			":param distances: List of distance values for each curve point.\n"
			":param values: List of volume values for each curve point (in dB).\n"
			":param shapeInts: List of shape integers (0=Linear, 1=Log1, 2=Log2, 3=Log3, 4=Exp1, 5=Exp2, 6=Exp3, 7=SCurve, 8=InvSCurve).\n"
			":return: True if successful, False otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"GetAttenuationVolumeCurveShapeInts",
			GetAttenuationVolumeCurveShapeInts,
			"Get interpolation shapes as integers from volume attenuation curve.\n"
			":param attenuationId: ID of the attenuation to query.\n"
			":return: List of shape integers (0=Linear, 1=Log1, 2=Log2, 3=Log3, 4=Exp1, 5=Exp2, 6=Exp3, 7=SCurve, 8=InvSCurve) for each curve point, empty if unsuccessful."
		)
		MAP_METHOD_AND_WRAP
		(
			"SubscribeToPropertyChanges",
			SubscribeToPropertyChanges,
			"Subscribe to property changes for a specific Wwise object.\n"
			"When the property changes in Wwise, the specified callback is invoked on the main thread.\n"
			"\n"
			":param objectId: ID of the Wwise object to monitor.\n"
			":param propertyName: Name of the property to monitor.\n"
			":param callback: Python function to call when property changes. Receives (objectId, propertyName).\n"
			":return: True if subscription succeeded, False otherwise.\n"
			"\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"SubscribeToAttenuationMaxRadius",
			SubscribeToAttenuationMaxRadius,
			"Subscribe to RadiusMax property changes for a specific attenuation.\n"
			"Convenience wrapper for SubscribeToPropertyChanges.\n"
			"\n"
			":param attenuationId: ID of the attenuation to monitor.\n"
			":param callback: Python function to call when RadiusMax changes.\n"
			":return: True if subscription succeeded, False otherwise.\n"
			"\n"
		)
		MAP_METHOD_AND_WRAP
		(
			"UnsubscribeFromPropertyChanges",
			UnsubscribeFromPropertyChanges,
			"Unsubscribe from property changes for a specific object.\n"
			":param objectId: ID of the object to stop monitoring.\n"
			":return: True if unsubscription succeeded, False otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"IsSubscribedToProperty",
			IsSubscribedToProperty,
			"Check if currently subscribed to an object's property changes.\n"
			":param objectId: ID of the object to check.\n"
			":return: True if subscribed, False otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"CreateAttenuation",
			CreateAttenuation,
			"Create a new attenuation in Wwise.\n"
			":param attenuationName: Name for the new attenuation.\n"
			":param path: Path or ID of parent (e.g., '\\\\Attenuations\\\\Default Work Unit').\n"
			":return: The created attenuation ID if successful, empty string otherwise."
		)
		MAP_METHOD_AND_WRAP
		(
			"SetReference",
			SetReference,
			"Set a reference property on a Wwise object.\n"
			":param objectId: ID of the object to modify.\n"
			":param referenceName: Name of the reference property (e.g., 'Attenuation', 'OutputBus').\n"
			":param referenceId: ID of the object to reference (empty string to clear reference).\n"
			":return: True if successful, False otherwise."
		)

	EXPOSURE_END()
}
