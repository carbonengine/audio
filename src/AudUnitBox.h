/**
 * @file AudUnitBox.h
 * @author Phevos Rinis
 * @date Oct 2026
 * @copyright © 2026 Fenris Creations ehf.
 */

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

	/// Stores the transform. Returns false when the box has a zero-length axis.
	bool SetTransform( const Matrix& unitBoxToWorld );
	/// Forgets the transform.
	void ClearTransform();

	/// True when the box has a transform with no zero-length axis.
	bool IsUsable() const { return m_usable; }
	const Matrix& GetUnitBoxToWorld() const { return m_unitBoxToWorld; }
	/// Volume of the box.
	float GetVolume() const { return m_volume; }

	/// Returns whether the point is inside the box, in right-handed world space.
	bool ContainsPoint( const Vector3& worldPosition ) const;
	/// Returns the world position of a point in unit box space.
	Vector3 ToWorld( const Vector3& unitBoxPosition ) const;

private:
	Matrix m_unitBoxToWorld;
	float m_volume;
	bool m_usable;
};
