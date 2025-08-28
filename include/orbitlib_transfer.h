#ifndef ORBITLIB_ORBITLIB_TRANSFER_H
#define ORBITLIB_ORBITLIB_TRANSFER_H

#include "orbitlib_celestial.h"
#include "orbitlib_orbit.h"


typedef struct Hohmann {
	double dur, dv_dep, dv_arr;
} Hohmann;

enum LAMBERT_SOLVER_SUCCESS {
	LAMBERT_SUCCESS,
	LAMBERT_IMPRECISION,
	LAMBERT_MAX_ITERATIONS,
	LAMBERT_FAIL_NAN,
	LAMBERT_FAIL_ECC
};

typedef struct Lambert2 {
	Orbit orbit;
	double true_anomaly0, true_anomaly1;
	enum LAMBERT_SOLVER_SUCCESS success;
} Lambert2;

typedef struct Lambert3 {
	Vector3 r0, v0, r1, v1;
	enum LAMBERT_SOLVER_SUCCESS success;
} Lambert3;

enum HyperbolaType {
	HYP_DEPARTURE,
	HYP_ARRIVAL,
	HYP_FLYBY
};

typedef struct HyperbolaLegParams {
	double decl;
	double bplane_angle;
	double bvazi;
} HyperbolaLegParams;

typedef struct HyperbolaParams {
	enum HyperbolaType type;
	double rp;
	double c3_energy;
	struct HyperbolaLegParams incoming; // to be ignored if type == HYP_DEPARTURE
	struct HyperbolaLegParams outgoing; // to be ignored if type == HYP_ARRIVAL
} HyperbolaParams;

double calc_apsis_maneuver_dv(double static_apsis, double initial_apsis, double new_apsis, struct Body *cb);

Hohmann calc_hohmann_transfer(double r0, double r1, Body *cb);

Lambert2 calc_lambert2(double r0, double r1, double delta_ta, double target_dt, Body *cb);

Lambert3 calc_lambert3(Vector3 r0, Vector3 r1, double target_dt, Body *cb);

double dv_circ(struct Body *body, double periapsis_altitude, double vinf);

double dv_capture(struct Body *body, double periapsis_altitude, double vinf);

double get_flyby_periapsis(Vector3 v_arr, Vector3 v_dep, Vector3 v_body, Body *body);

double get_flyby_inclination(Vector3 v_arr, Vector3 v_dep, Vector3 v_body);

// calculate and return fly-by hyperbola parameters
// (v_arr irrelevant for departure hyperbola, v_dep irrelevant for arrival hyperbola, h_pe irrelevant for fly-by hyperbola)
HyperbolaParams get_hyperbola_params(Vector3 v_arr, Vector3 v_dep, Vector3 v_body, struct Body *body, double h_pe, enum HyperbolaType type);


#endif //ORBITLIB_ORBITLIB_TRANSFER_H
