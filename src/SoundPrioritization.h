// Copyright © 2025 CCP ehf.

#pragma once

#include <chrono>
#include <vector>
#include <limits>
#include "IPrioritizedObject.h"
#include <CcpMutex.h>
#include "Audio2.h" 

/**
 * @class SoundPrioritization
 * @brief Core system for managing and prioritizing audio objects
 * 
 * The SoundPrioritization class manages a collection of audio objects and determines
 * which ones should be active based on factors such as distance from listener,
 * visibility, and sound importance. It implements culling logic to optimize audio
 * processing by limiting the number of simultaneously active objects.
 */
class SoundPrioritization {
public:

    /**
     * @struct CullingSettings
     * @brief Configuration settings for the audio culling system
     * 
     * Contains all configurable parameters that control how the prioritization
     * system behaves and calculates object weights.
     */
    struct CullingSettings {
        int maxAwakeGameObjects;      /**< Maximum number of simultaneously active audio objects */
        long long oneShotWindow;      /**< Time window in milliseconds for one-shot sound opportunities */
        float weightMultiplier;       /**< Global multiplier applied to all weight calculations */
        float playingVitalSoundWeight;/**< Priority weight for objects playing vital sounds */
        float playing2DWeight;        /**< Priority weight for objects playing 2D sounds */
        float rangeWeight;            /**< Priority weight for objects within listener range */
        float activeSoundsWeight;     /**< Priority weight for objects with active sounds */
        float waitingOneShotWeight;   /**< Priority weight for objects with pending one-shot sounds */
        float visibleWeight;          /**< Priority weight for visible objects */
        float usedEmitterWeight;      /**< Priority weight for objects that have been used */
    };

    /** @brief Default constructor */
    SoundPrioritization();
    
    /** @brief Destructor */
    ~SoundPrioritization();

    /**
     * @brief Register a new object for prioritization
     * @param object Pointer to the object implementing IPrioritizedObject
     */
    void RegisterGameObject(IPrioritizedObject* object);

    /**
     * @brief Unregister an object from the prioritization system
     * @param objectID The unique identifier of the object to unregister
     */
    void UnregisterGameObject(AkGameObjectID objectID);

    /**
     * @brief Execute the main culling algorithm
     * 
     * Evaluates all registered objects and determines which should be active
     * based on their calculated weights and the maximum allowed active objects.
     */
    void CullAudio();

    /**
     * @brief Enable the audio culling system
     */
    void EnableAudioCulling();

    /**
     * @brief Disable the audio culling system
     */
    void DisableAudioCulling();

    /**
     * @brief Check if audio culling is currently enabled
     * @return True if culling is enabled, false otherwise
     */
    bool GetAudioCullingEnabled() const;

    /**
     * @brief Reset culling settings to default values
     */
    void ResetCullingSettings();

    /**
     * @brief Get the maximum number of audio objects that can be active simultaneously
     * @return int The current maximum number of active objects
     */
    int GetMaxAwakeGameObjects() const;

    /**
     * @brief Set the maximum number of audio objects that can be active simultaneously
     * @param value The new maximum number of active objects
     */
    void SetMaxAwakeGameObjects(int value);

    /**
     * @brief Get the time window for one-shot sound opportunities
     * @return long long The current one-shot window in milliseconds
     */
    long long GetOneShotWindow() const;

    /**
     * @brief Set the time window for one-shot sound opportunities
     * @param numMilliseconds The new time window in milliseconds
     */
    void SetOneShotWindow(long long numMilliseconds);

    /**
     * @brief Get the priority weight for 2D sounds
     * @return float The current 2D sound weight multiplied by the global weight multiplier
     */
    float GetPlaying2DWeight() const;

    /**
     * @brief Set the priority weight for 2D sounds
     * @param weight The new base weight for 2D sounds (will be multiplied by weight multiplier)
     */
    void SetPlaying2DWeight(float weight);

    /**
     * @brief Get the priority weight for objects with active sounds
     * @return float The current active sounds weight multiplied by the global weight multiplier
     */
    float GetPlayingEventsWeight() const;

    /**
     * @brief Set the priority weight for objects with active sounds
     * @param weight The new base weight for active sounds (will be multiplied by weight multiplier)
     */
    void SetPlayingEventsWeight(float weight);

    /**
     * @brief Get the priority weight for vital sounds
     * @return float The current vital sound weight multiplied by the global weight multiplier
     */
    float GetPlayingVitalSoundWeight() const;

    /**
     * @brief Set the priority weight for vital sounds
     * @param weight The new base weight for vital sounds (will be multiplied by weight multiplier)
     */
    void SetPlayingVitalSoundWeight(float weight);

    /**
     * @brief Get the priority weight for objects within listener range
     * @return float The current range weight multiplied by the global weight multiplier
     */
    float GetRangeWeight() const;

    /**
     * @brief Set the priority weight for objects within listener range
     * @param weight The new base weight for range consideration (will be multiplied by weight multiplier)
     */
    void SetRangeWeight(float weight);

    /**
     * @brief Get the priority weight for objects that have been used
     * @return float The current used emitter weight multiplied by the global weight multiplier
     */
    float GetUsedEmitterWeight() const;

    /**
     * @brief Set the priority weight for objects that have been used
     * @param weight The new base weight for used emitters (will be multiplied by weight multiplier)
     */
    void SetUsedEmitterWeight(float weight);

    /**
     * @brief Get the priority weight for visible objects
     * @return float The current visibility weight multiplied by the global weight multiplier
     */
    float GetVisibleWeight() const;

    /**
     * @brief Set the priority weight for visible objects
     * @param weight The new base weight for visibility (will be multiplied by weight multiplier)
     */
    void SetVisibleWeight(float weight);

    /**
     * @brief Get the priority weight for objects with pending one-shot sounds
     * @return float The current one-shot weight multiplied by the global weight multiplier
     */
    float GetWaitingOneShotWeight() const;

    /**
     * @brief Set the priority weight for objects with pending one-shot sounds
     * @param weight The new base weight for waiting one-shots (will be multiplied by weight multiplier)
     */
    void SetWaitingOneShotWeight(float weight);

    /**
     * @brief Get the global weight multiplier applied to all weight calculations
     * @return float The current global weight multiplier
     */
    float GetWeightMultiplier() const;

    /**
     * @brief Set the global weight multiplier applied to all weight calculations
     * @param value The new global weight multiplier
     * @note This value affects all other weights as they are multiplied by this value
     */
    void SetWeightMultiplier(float value);

    /**
     * @brief Calculate the priority weight for an audio object
     * @param distanceSq Squared distance from listener
     * @param isMuted Whether the object is muted
     * @param isInRange Whether the object is within listening range
     * @param isUsed Whether the object has been used
     * @param isVisible Whether the object is visible
     * @param isPlaying2D Whether the object is playing 2D audio
     * @param isPlayingVital Whether the object is playing vital sounds
     * @param additionalWeight Additional weight modifier
     * @param activeEventCount Number of active events
     * @param waitingOneShotWeight Weight for waiting one-shot sounds
     * @param usedEmitterWeight Weight for used emitters
     * @param rangeWeight Weight for range consideration
     * @param activeSoundsWeight Weight for active sounds
     * @param visibleWeight Weight for visibility
     * @param playing2DWeight Weight for 2D sounds
     * @param playingVitalSoundWeight Weight for vital sounds
     * @return Calculated priority weight
     */

    static float CalculateObjectWeight(
        float distanceSq,
        bool isMuted,
        bool isInRange,
        bool isUsed,
        bool isVisible,
        bool isPlaying2D,
        bool isPlayingVital,
        float additionalWeight,
        size_t activeEventCount,
        float waitingOneShotWeight,
        float usedEmitterWeight,
        float rangeWeight,
        float activeSoundsWeight,
        float visibleWeight,
        float playing2DWeight,
        float playingVitalSoundWeight
    );

    /**
     * @brief Get the current list of prioritized audio objects
     * @return Vector of pointers to prioritized objects
     */

     const std::vector<IPrioritizedObject*>& GetPrioritizedAudioObjects() const;

private:

    bool m_audioCullingEnabled;              /**< Current state of the culling system */
    CullingSettings m_settings;              /**< Current culling settings */
    CullingSettings m_defaultSettings;       /**< Default culling settings */
    IPrioritizedObject* m_listener;          /**< Pointer to the listener object */
    std::vector<IPrioritizedObject*> m_gameObjects; /**< Collection of managed objects */
    mutable CcpMutex m_objectsMutex;         /**< Mutex for thread safety */
};