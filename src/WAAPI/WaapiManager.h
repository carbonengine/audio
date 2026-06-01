////////////////////////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Created: September 2025
// Copyright (c) 2025, CCP Games
//

#pragma once

#include <string>
#include <vector>
#include <cstdint>
#include <memory>
#include <functional>
#include <queue>
#include <atomic>
#include <mutex>
#include <unordered_map>

namespace AK { namespace WwiseAuthoringAPI {
    class Client;
} }

/**
 * @brief Curve interpolation shape types
 */
enum class CurveShape
{
    Linear = 0,
    Log1,
    Log2,
    Log3,
    Exp1,
    Exp2,
    Exp3,
    SCurve,
    InvSCurve
};

struct WaapiObjectPositioningData
{
    std::string id;
    std::string name;
    std::string type;
    std::string path;
    std::string parentId;
    bool overridePositioning = false;
    bool enableAttenuation = false;
    std::string attenuationId;
    std::string attenuationName;
};

#ifndef AK_OPTIMIZED
struct WaapiAttenuationConeData
{
    bool coneEnabled = false;
    double innerAngle = 0.0;
    double outerAngle = 0.0;
    double coneAttenuation = 0.0;
    double maxRadius = 0.0;
    std::string attenuationId;
    std::string attenuationName;
    std::string sourceObjectId;
    std::string sourceObjectName;
    std::string sourceObjectPath;
};

struct WaapiProfilerVoiceData
{
    uint32_t pipelineId = 0;
    uint32_t playingId = 0;
    uint32_t soundId = 0;
    uint64_t gameObjectId = 0;
    std::string gameObjectName;
    std::string objectGuid;
    std::string objectName;
    std::string playTargetGuid;
    std::string playTargetName;
    double baseVolume = -1000.0;
    bool isStarted = false;
    bool isVirtual = false;
    bool isForcedVirtual = false;
};
#endif

BLUE_DECLARE( WaapiManager );

/**
 * @brief Manages connection and communication with Wwise Authoring Tool via WAAPI
 *
 * WaapiManager provides an interface for connecting to Wwise Authoring Tool
 * and performing various audio content queries and modifications through the WAAPI
 * (Wwise Authoring API). It handles connection management and provides methods for
 * working with events, sounds, volume curve e.c.t attenuations using the AkJson API.
 * This is still in development and more features will be added in the future.
 */
BLUE_CLASS( WaapiManager ) :
    public IRoot
{
    public:
    WaapiManager(IRoot* lockobj = nullptr);
    ~WaapiManager();

    EXPOSE_TO_BLUE();

    // Connection Management

    /**
     * @brief Connect to Wwise Authoring Tool via WAAPI
     * @param host Host address
     * @param port Port number
     * @return True if connection succeeded, false otherwise
     */
    bool Connect(const std::string& host = "127.0.0.1", int port = 8080);
    bool TryConnect(const std::string& host = "127.0.0.1", int port = 8080);

    /**
     * @brief Disconnect from Wwise Authoring Tool
     */
    void Disconnect();

    /**
     * @brief Check if currently connected to Wwise Authoring Tool
     * @return True if connected, false otherwise
     */
    bool IsConnected() const;

    /**
     * @brief Get the underlying WAAPI client for advanced operations
     * @return Pointer to client if connected, nullptr otherwise
     */
    AK::WwiseAuthoringAPI::Client* GetClient() const;

    // Event and Sound Queries


    /**
     * @brief Get IDs of all action targets referenced by a given event
     * @param eventName Name of the event to query
     * @return Vector of target IDs (sounds, containers, etc.), empty if unsuccessful
     */
    std::vector<std::string> GetEventReferencedTargetIds(const std::string& eventName);

    /**
     * @brief Get names of all action targets referenced by a given event
     * @param eventName Name of the event to query
     * @return Vector of target names (sounds, containers, etc.), empty if unsuccessful
     */
    std::vector<std::string> GetEventReferencedTargetNames(const std::string& eventName);

    /**
     * @brief Get path names of all action targets referenced by a given event
     * @param eventName Name of the event to query
     * @return Vector of target path names (sounds, containers, etc.), empty if unsuccessful
     */
    std::vector<std::string> GetEventReferencedTargetPaths(const std::string& eventName);

    /**
     * @brief Get ID of attenuation referenced by a given sound
     * @param soundId ID of the sound to query
     * @return Attenuation ID if successful, empty string otherwise
     */
    std::string GetSoundReferencedAttenuationId(const std::string& soundId);

    /**
     * @brief Get name of attenuation referenced by a given sound
     * @param soundId ID of the sound to query
     * @return Attenuation name if successful, empty string otherwise
     */
    std::string GetSoundReferencedAttenuationName(const std::string& soundId);

    /**
     * @brief Get the effective attenuation ID for a Wwise object by following positioning inheritance.
     * @param objectId ID of the object to query
     * @return Attenuation ID if an override positioning source defines one, empty string otherwise
     */
    std::string GetObjectEffectiveAttenuationId(const std::string& objectId);

    /**
     * @brief Get the attenuation ID selected by the hierarchy cone resolver for a Wwise object.
     * @param objectId ID of the object to query
     * @return Attenuation ID if cone data could be resolved, empty string otherwise
     */
#ifndef AK_OPTIMIZED
    std::string GetObjectEffectiveAttenuationConeAttenuationId(const std::string& objectId);

    /**
     * @brief Get the cone inner angle for a given attenuation.
     * @param attenuationId ID of the attenuation to query
     * @return Cone inner angle if successful, 0.0 otherwise
     */
    double GetAttenuationConeInnerAngle(const std::string& attenuationId);

    /**
     * @brief Get the cone outer angle for a given attenuation.
     * @param attenuationId ID of the attenuation to query
     * @return Cone outer angle if successful, 0.0 otherwise
     */
    double GetAttenuationConeOuterAngle(const std::string& attenuationId);
#endif

    // Volume Attenuation Management

    /**
     * @brief Set volume attenuation curve for a given attenuation object
     * @param attenuationId ID of the attenuation to modify
     * @param distances Vector of distance values for each curve point
     * @param values Vector of volume values for each curve point (in dB)
     * @param shapeInts Vector of shape integers
     * @return True if successful, false otherwise
     */
    bool SetAttenuationVolumeCurve(const std::string& attenuationId, const std::vector<double>& distances, const std::vector<double>& values, const std::vector<int>& shapeInts);

    /**
     * @brief Get distance values from volume attenuation curve
     * @param attenuationId ID of the attenuation to query
     * @return Vector of distance values for each curve point, empty if unsuccessful
     */
    std::vector<double> GetAttenuationVolumeCurveDistances(const std::string& attenuationId);

    /**
     * @brief Get volume values from volume attenuation curve
     * @param attenuationId ID of the attenuation to query
     * @return Vector of volume values for each curve point (in dB), empty if unsuccessful
     */
    std::vector<double> GetAttenuationVolumeCurveValues(const std::string& attenuationId);

    /**
     * @brief Get interpolation shapes as integers from volume attenuation curve
     * @param attenuationId ID of the attenuation to query
     * @return Vector of shape integers for each curve point, empty if unsuccessful
     */
    std::vector<int> GetAttenuationVolumeCurveShapeInts(const std::string& attenuationId);

    /**
     * @brief Get the name of an attenuation object by its ID
     * @param attenuationId ID of the attenuation to query
     * @return Attenuation name if successful, empty string otherwise
     */
    std::string GetAttenuationName(const std::string& attenuationId);

    /**
     * @brief Get maximum radius for a given attenuation
     * @param attenuationId ID of the attenuation to query
     * @return Radius value if successful, 0.0 if unsuccessful
     */
    double GetAttenuationMaxRadius(const std::string& attenuationId);

    /**
     * @brief Get cone attenuation settings for a given attenuation.
     * @param attenuationId ID of the attenuation to query
     * @param outData Populated cone settings if successful
     * @return True if the attenuation was found and cone data could be read
     */
#ifndef AK_OPTIMIZED
    bool GetAttenuationConeData(const std::string& attenuationId, WaapiAttenuationConeData& outData);

    /**
     * @brief Resolve effective cone attenuation data for a Wwise object by walking positioning inheritance upward.
     * @param objectId ID of a Wwise object
     * @param outData Populated cone settings if successful
     * @return True if an effective attenuation reference was found and read
     */
    bool GetObjectEffectiveAttenuationConeData(const std::string& objectId, WaapiAttenuationConeData& outData);

    /**
     * @brief Get the best currently voiced object GUID for a game object and playing ID from Wwise profiler data.
     * @param gameObjectId Wwise game object ID
     * @param playingId Wwise playing ID
     * @return Voice object GUID if a matching voice is available, empty string otherwise
     */
    std::string GetBestProfilerVoiceObjectIdForPlayingEvent(uint64_t gameObjectId, uint32_t playingId);

    /**
     * @brief Test seam for the debug cone profiler voice selection policy.
     */
    std::string SelectDebugProfilerVoiceObjectIdForTest(long long gameObjectId,
                                                        int playingId,
                                                        const std::vector<std::string>& objectGuids,
                                                        const std::vector<int>& playingIds,
                                                        const std::vector<long long>& gameObjectIds,
                                                        const std::vector<int>& isStarted,
                                                        const std::vector<int>& isVirtual,
                                                        const std::vector<double>& baseVolumes);
#endif

    // Radius Management

    /**
     * @brief Set maximum radius for a specific attenuation object
     * @param attenuationId ID of the attenuation to modify
     * @param radius Radius value to set
     * @return True if successful, false otherwise
     */
    bool SetAttenuationMaxRadius(const std::string& attenuationId, double radius);

    // Subscription and Monitoring

    /**
     * @brief Subscribe to property changes for a specified Wwise object ID
     *
     * Monitors the specified object for property changes in Wwise Authoring tool.
     * When the property changes, the provided Python callback is invoked
     * on the main thread. Use existing getter methods to retrieve the updated value.
     *
     * @param objectId ID of the Wwise object to monitor
     * @param propertyName Name of the property to monitor (e.g. "Volume")
     * @param callback Python callable to invoke when property changes (receives objectId, propertyName)
     * @return True if subscription succeeded, false otherwise
     */
    bool SubscribeToPropertyChanges(const std::string& objectId,
                                   const std::string& propertyName,
                                   const BlueScriptCallback& callback);

    /**
     * @brief Wrapper method to subscribe to attenuation RadiusMax changes
     * @param attenuationId ID of the attenuation to monitor
     * @param callback Python callable to invoke when RadiusMax changes (receives attenuationId, "RadiusMax")
     * @return True if subscription succeeded, false otherwise
     */
    bool SubscribeToAttenuationMaxRadius(const std::string& attenuationId,
                                         const BlueScriptCallback& callback);

    /**
     * @brief Unsubscribe from property changes for a specific object
     * @param objectId ID of the object to stop monitoring
     * @return True if unsubscription succeeded, false otherwise
     */
    bool UnsubscribeFromPropertyChanges(const std::string& objectId);

    /**
     * @brief Check if an object is currently being monitored
     * @param objectId ID of the object to check
     * @return True if subscribed, false otherwise
     */
    bool IsSubscribedToProperty(const std::string& objectId) const;

    /**
     * @brief Create a new attenuation in Wwise
     * @param attenuationName Name for the new attenuation
     * @param path Path or ID of parent (e.g., '\\Attenuations\\Default Work Unit')
     * @return The created attenuation ID if successful, empty string otherwise
     */
    std::string CreateAttenuation(const std::string& attenuationName,
                                   const std::string& path);

    /**
	 * @brief Set a reference from one Wwise object to another
	 * @param The object ID to set the reference on
	 * @param Reference name (e.g., "Attenuation")
	 * @return Reference ID to set
     */

    bool SetReference(const std::string& objectId,
                    const std::string& referenceName,
                    const std::string& referenceId);

    private:

    /**
     * @brief Escape special characters in a string for use in WAQL queries
     * @param str String to escape
     * @return Escaped string with backslashes and quotes properly escaped
     */
    static std::string EscapeWaqlString(const std::string& str);

    /**
     * @brief Internal method to get all action targets referenced by a given event
     * @param eventName Name of the event to query
     * @param outIds Output vector for target IDs (sounds, containers, etc.)
     * @param outNames Output vector for target names
     * @param outPathNames Output vector for target path names
     * @return True if successful, false otherwise
     */
    bool GetEventReferencedTargets(const std::string& eventName,
                                   std::vector<std::string>& outIds,
                                   std::vector<std::string>& outNames,
                                   std::vector<std::string>& outPathNames);

    /**
     * @brief Internal method to get attenuations referenced by a given sound
     * @param soundId ID of the sound to query
     * @param outAttenuationId Output for attenuation ID
     * @param outAttenuationName Output for attenuation name
     * @return True if successful, false otherwise
     */
    bool GetSoundReferencedAttenuations(const std::string& soundId,
                                        std::string& outAttenuationId,
                                        std::string& outAttenuationName);

    bool GetObjectPositioningData(const std::string& objectId, WaapiObjectPositioningData& outData);
    bool ResolveEffectiveAttenuationReference(const std::string& objectId, WaapiObjectPositioningData& outSource);
#ifndef AK_OPTIMIZED
    bool EnableProfilerVoiceData();
    bool StartProfilerCaptureForVoiceData();
    bool GetProfilerVoices(std::vector<WaapiProfilerVoiceData>& outVoices);
    static std::string SelectBestProfilerVoiceObjectId(const std::vector<WaapiProfilerVoiceData>& voices,
                                                       uint64_t gameObjectId,
                                                       uint32_t playingId);
#endif

    // Helper methods for enum conversion
    static std::string CurveShapeToString(CurveShape shape);
    static CurveShape StringToCurveShape(const std::string& shapeStr);
    static int CurveShapeToInt(CurveShape shape);
    static CurveShape IntToCurveShape(int shapeInt);

    // Internal WAAPI implementation methods
    bool ConnectInternal(const std::string& host, int port, bool logFailures);
    void MarkConnectionLost(const char* reason);
    bool GetAttenuationVolumeCurveInternal(const std::string& attenuationId,
                                          std::vector<double>& outDistances,
                                          std::vector<double>& outValues,
                                          std::vector<std::string>& outShapes);
    bool SetAttenuationVolumeCurveInternal(const std::string& attenuationId,
                                          const std::vector<double>& distances,
                                          const std::vector<double>& values,
                                          const std::vector<std::string>& shapes);
    bool GetAttenuationMaxRadiusInternal(const std::string& attenuationId, double& outRadius);


    // WAAPI connection
    std::unique_ptr<AK::WwiseAuthoringAPI::Client> m_client;
    bool m_connected;
    std::string m_host;
    int m_port;
#ifndef AK_OPTIMIZED
    bool m_profilerVoiceDataEnabled = false;
    bool m_profilerCaptureStartAttempted = false;
#endif

    // Subscription and monitoring infrastructure

    /**
     * @brief Data structure for passing callback information to main thread
     */
    struct CallbackData {
        WaapiManagerPtr manager;
        std::string objectId;
        std::string propertyName;

        CallbackData(WaapiManagerPtr mgr, const std::string& id, const std::string& prop)
            : manager(mgr), objectId(id), propertyName(prop) {}
    };

    /**
     * @brief Static callback function for mainThreadQueue
     * Invokes the appropriate Python callback on the main thread
     */
    static void PropagatePropertyChangeCallback(void* pCallbackData);

    // Subscription tracking: objectId -> WAAPI subscription ID
    std::unordered_map<std::string, uint64_t> m_Subscriptions;

    // Callback storage: objectId -> Python callback
    std::unordered_map<std::string, std::shared_ptr<BlueScriptCallback>> m_changeCallbacks;

    // Mutex to protect subscription maps (used during subscribe/unsubscribe)
    mutable std::mutex m_subscriptionMutex;
};

TYPEDEF_BLUECLASS( WaapiManager );

