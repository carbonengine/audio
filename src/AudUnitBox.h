////////////////////////////////////////////////////////////
//
// Creator: Phevos Rinis
// Creation Date: Oct 2026
// Copyright (c) 2026 Fenris Creations
//

#pragma once

#include "Matrix.h"
#include "Vector3.h"

/**
 * @brief The unit box [-0.5, 0.5]^3 placed by a transform in right-handed world space.
 */
class AudUnitBox
{
public:
	AudUnitBox();

	/**
	 * @brief Stores the transform.
	 *
	 * @param unitBoxToWorld Maps the unit box to world space.
	 *
	 * @return False when the box has a zero-length axis.
	 */
	bool SetTransform( const Matrix& unitBoxToWorld );

	/**
	 * @brief Forgets the transform.
	 */
	void ClearTransform();

	/**
	 * @brief True when the box has a transform with no zero-length axis.
	 */
	bool IsUsable() const { return m_usable; }

	/**
	 * @brief The transform from the unit box to world space.
	 */
	const Matrix& GetUnitBoxToWorld() const { return m_unitBoxToWorld; }

	/**
	 * @brief Volume of the box.
	 */
	float GetVolume() const { return m_volume; }

	/**
	 * @brief Returns whether the point is inside the box.
	 *
	 * @param worldPosition Position in right-handed world space.
	 *
	 * @return True when the point is inside the box or on its surface.
	 */
	bool ContainsPoint( const Vector3& worldPosition ) const;

	/**
	 * @brief Returns the world position of a point in unit box space.
	 *
	 * @param unitBoxPosition Position in unit box space, where [-0.5, 0.5] on every axis is inside the box.
	 *
	 * @return The position in right-handed world space.
	 */
	Vector3 ToWorld( const Vector3& unitBoxPosition ) const;

private:
	Matrix m_unitBoxToWorld;
	float m_volume;
	bool m_usable;
};
