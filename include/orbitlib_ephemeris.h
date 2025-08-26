#ifndef ORBITLIB_ORBITLIB_EPHEMERIS_H
#define ORBITLIB_ORBITLIB_EPHEMERIS_H

#include "geometrylib.h"

/**
 * @brief Represents the ephemeral data consisting of epoch, position, and velocity components
 */
typedef struct Ephem {
	double epoch;	/**< Epoch associated with the ephemeral data (Julian Date) */
	Vector3 r;		/**< Position Vector */
	Vector3 v;		/**< Velocity Vector */
} Ephem;

#endif //ORBITLIB_ORBITLIB_EPHEMERIS_H
