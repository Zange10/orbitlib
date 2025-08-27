#ifndef ORBITLIB_ORBITLIB_TRANSFER_H
#define ORBITLIB_ORBITLIB_TRANSFER_H

#include "orbitlib_celestial.h"
#include "orbitlib_orbit.h"


typedef struct Hohmann {
	double dur, dv_dep, dv_arr;
} Hohmann;

typedef struct Lambert2 {
	Orbit orbit;
	double true_anomaly0, true_anomaly1;
} Lambert2;

typedef struct Lambert3 {
	OSV osv0, osv1;
} Lambert3;

double calc_apsis_maneuver_dv(double static_apsis, double initial_apsis, double new_apsis, struct Body *cb);

Hohmann calc_hohmann_transfer(double r0, double r1, Body *cb);

Lambert2 calc_lambert2(double r0, double r1, double delta_ta, double target_dt, Body *cb);

Lambert3 calc_lambert3(Vector3 r0, Vector3 r1, double target_dt, Body *cb);

double dv_circ(struct Body *body, double periapsis_altitude, double vinf);

double dv_capture(struct Body *body, double periapsis_altitude, double vinf);


#endif //ORBITLIB_ORBITLIB_TRANSFER_H
