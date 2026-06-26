// Copyright © 2025 CCP ehf.

#pragma once

/**
  * @class IPrioritizedObject
  * @brief Interface defining prioritizable audio objects
  * 
  * This interface must be implemented by any object to participate
  * in the sound prioritization system. It defines the methods needed
  * for position tracking, weight calculation, and culling state management.
  */
class IPrioritizedObject
{
public:
	/** @brief Virtual destructor */
	virtual ~IPrioritizedObject() = default;

	/**
      * @brief Get the unique identifier for this object
      * @return AkGameObjectID The object's unique identifier
      */
	virtual AkGameObjectID GetID() const = 0;

	/**
      * @brief Get the current position of this object
      * @return Vector3 The object's position in 3D space
      */
	virtual Vector3 GetPosition() const = 0;

	/**
      * @brief Set the squared distance from this object to the listener
      * @param distanceSq The squared distance value
      */
	virtual void SetDistanceSqFromListener( float distanceSq ) = 0;

	/**
      * @brief Calculate this object's culling weight
      * @param now Current time point for temporal calculations
      * 
      * This method implements the logic to calculate the object's
      * priority weight based on various factors such as distance, visibility,
      * and sound importance.
      */
	virtual void CalculateCullingWeight( std::chrono::steady_clock::time_point now ) = 0;

	/**
      * @brief Get the current culling weight
      * @return float The calculated culling weight
      */
	virtual float GetCullingWeight() const = 0;

	/**
      * @brief Check if this object is currently culled
      * @return bool True if culled, false if active
      */
	virtual bool IsCulled() const = 0;

	/**
      * @brief Activate this object in the audio system
      * 
      * Implementation should handle all necessary setup to make
      * the object active in the audio system.
      */
	virtual void Wake() = 0;

	/**
      * @brief Deactivate this object in the audio system
      * 
      * Implementation should handle all necessary cleanup when
      * the object is being culled from the audio system.
      */
	virtual void Cull() = 0;
};