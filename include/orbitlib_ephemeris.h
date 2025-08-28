#ifndef ORBITLIB_ORBITLIB_EPHEMERIS_H
#define ORBITLIB_ORBITLIB_EPHEMERIS_H

#include "geometrylib.h"
#include "orbitlib_celestial.h"
#include "orbitlib_datetime.h"

/**
 * @brief Represents the ephemeral data consisting of epoch, position, and velocity components
 */
typedef struct Ephem {
	double epoch;	/**< Epoch associated with the ephemeral data (Julian Date) */
	Vector3 r;		/**< Position Vector */
	Vector3 v;		/**< Velocity Vector */
} Ephem;

/**
 * @brief Prints the date, position and velocity vector of the given ephemeris
 *
 * @param ephem The given ephemeris
 */
void print_ephem(struct Ephem ephem);


/**
 * @brief Retrieves the ephemeral data of requested body for requested time (from JPL's Horizon API or from file)
 *
 * @param body The body for which the ephemerides should be stored
 * @param central_body The central body of the body's system
 */
void get_body_ephems(Body *body, Datetime min_date, Datetime max_date, Datetime time_step, const char *ephem_directory);


OSV osv_from_ephem(Ephem *ephem_list, int num_ephems, double epoch, Body *cb);

#endif //ORBITLIB_ORBITLIB_EPHEMERIS_H
