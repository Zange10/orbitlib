#ifndef ORBITLIB_ORBITLIB_ORBIT_H
#define ORBITLIB_ORBITLIB_ORBIT_H

#include "geometrylib.h"

typedef struct Body Body;

typedef struct Orbit {
	struct Body * cb;	// central body
	double e;           // eccentricity
	double a;           // semi-major axis
	double i; 			// inclination
	double raan;        // right ascension of the ascending node
	double arg_peri; 	// argument of periapsis
	double ta;   		// true anomaly
} Orbit;

typedef struct OSV {
	Vector3 r, v;
} OSV;

// constructs orbit using orbital elements
struct Orbit constr_orbit_from_elements(double semimajor_axis, double eccentricity, double inclination, double raan, double arg_of_peri, double true_anomaly, struct Body *cb);

// constructs orbit using apsides and inclination
struct Orbit constr_orbit_from_apsides(double apsis1, double apsis2, double inclination, Body *cb);

// constructs orbit from Orbital State Vector
struct Orbit constr_orbit_from_osv(Vector3 r, Vector3 v, Body *cb);

Vector2 calc_vel_vec2(double r_mag, double v_mag, double true_anomaly, double flight_path_angle);

double calc_true_anomaly_from_mean_anomaly(struct Orbit orbit, double mean_anomaly);

double calc_orbit_flight_path_angle(double eccentricity, double true_anomaly);

Vector3 heliocentric_rot(Vector2 v, double raan, double w, double incl);

OSV osv_from_orbit(Orbit orbit);

OSV osv_from_elements(Orbit orbit, double epoch);

double calc_orbit_time_since_periapsis(Orbit orbit);

double calc_orbital_period(Orbit orbit);

Orbit propagate_orbit_time(Orbit orbit, double dt);

OSV propagate_osv_time(OSV osv, Body *cb, double dt);

OSV propagate_osv_ta(OSV osv, Body *cb, double delta_ta);

double calc_orbital_speed(struct Orbit orbit, double r);

double calc_orbit_apoapsis(struct Orbit orbit);

double calc_orbit_periapsis(struct Orbit orbit);

void print_orbit_info(struct Orbit orbit);

#endif //ORBITLIB_ORBITLIB_ORBIT_H
