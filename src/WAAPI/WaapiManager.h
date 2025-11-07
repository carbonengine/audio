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

namespace AK { namespace WwiseAuthoringAPI { class Client; } }

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

    // Radius Management

    /**
     * @brief Set maximum radius for a specific attenuation object
     * @param attenuationId ID of the attenuation to modify
     * @param radius Radius value to set
     * @return True if successful, false otherwise
     */
    bool SetAttenuationMaxRadius(const std::string& attenuationId, double radius);

    /**
     * @brief Create a new attenuation in Wwise
     * @param attenuationName Name for the new attenuation
     * @param path Path or ID of parent (e.g., '\\Attenuations\\Default Work Unit')
     * @return The created attenuation ID if successful, empty string otherwise
     */
    std::string CreateAttenuation(const std::string& attenuationName,
                                   const std::string& path);

    bool SetReference(const std::string& objectId,
                    const std::string& referenceName,
                    const std::string& referenceId);

    private:
    // Internal helper methods
    bool GetFirstPlatformId(std::string& outPlatformId);

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

    // Helper methods for enum conversion
    static std::string CurveShapeToString(CurveShape shape);
    static CurveShape StringToCurveShape(const std::string& shapeStr);
    static int CurveShapeToInt(CurveShape shape);
    static CurveShape IntToCurveShape(int shapeInt);

    // Internal WAAPI implementation methods
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
};

TYPEDEF_BLUECLASS( WaapiManager );

