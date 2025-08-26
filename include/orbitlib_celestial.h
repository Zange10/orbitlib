#ifndef ORBITLIB_ORBITLIB_CELESTIAL_H
#define ORBITLIB_ORBITLIB_CELESTIAL_H

#include "orbitlib_orbit.h"
#include "orbitlib_ephemeris.h"

struct Body {
	char name[32];
	double color[3];		// color used for orbit and body visualization
	int id;                 // ID given by JPL's Horizon API
	double mu;              // gravitational parameter of body [m³/s²]
	double radius;          // radius of body [m]
	double rotation_period; // the time period, in which the body rotates around its axis [s]
	double sl_atmo_p;       // atmospheric pressure at sea level [Pa]
	double scale_height;    // the height at which the atmospheric pressure decreases by the factor e [m]
	double atmo_alt;        // highest altitude with atmosphere (ksp-specific) [m]
	struct System *system;	// the system the body is the central body of
	struct Orbit orbit;     // orbit of body at ut0
	struct Ephem *ephem;	// Ephemeris of body (if available)
};



enum SystemCalcMethod {ORB_ELEMENTS, EPHEMS};

struct System {
	char name[50];
	int num_bodies;
	struct Body *cb;					// central body of system
	struct Body **bodies;				// bodies orbiting the central body
	enum SystemCalcMethod calc_method;	// Propagation using orbital elements or ephemerides
	double ut0;							// time of t0 for system
};

#endif //ORBITLIB_ORBITLIB_CELESTIAL_H
