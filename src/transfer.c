#include "orbitlib_transfer.h"
#include "geometrylib.h"
#include <math.h>

double calc_apsis_maneuver_dv(double static_apsis, double initial_apsis, double new_apsis, struct Body *cb) {
	struct Orbit initial_orbit;
	struct Orbit new_orbit;
	
	initial_orbit = constr_orbit_from_apsides(static_apsis, initial_apsis, 0, cb);
	new_orbit = constr_orbit_from_apsides(static_apsis, new_apsis, 0, cb);
	
	double v0 = calc_orbital_speed(initial_orbit, static_apsis);
	double v1 = calc_orbital_speed(new_orbit, static_apsis);
	
	return fabs(v1-v0);
}

Hohmann calc_hohmann_transfer(double r0, double r1, struct Body *cb) {
	double sma_pow_3 = pow(((r0 + r1) / 2),3);
	double dur = M_PI * sqrt(sma_pow_3 / cb->mu);
	double dv_dep = calc_apsis_maneuver_dv(r0, r0, r1, cb);
	double dv_arr = calc_apsis_maneuver_dv(r1, r0, r1, cb);
	return (Hohmann) {dur, dv_dep, dv_arr};
}


Lambert2 calc_lambert2(double r0, double r1, double delta_ta, double target_dt, Body *cb) {

}


Lambert3 calc_lambert3(Vector3 r0, Vector3 r1, double target_dt, Body *cb) {

}


double dv_circ(struct Body *body, double periapsis_altitude, double vinf) {
	periapsis_altitude += body->radius;
	return sqrt(2 * body->mu / periapsis_altitude + vinf * vinf) - sqrt(body->mu / periapsis_altitude);
}

double dv_capture(struct Body *body, double periapsis_altitude, double vinf) {
	periapsis_altitude += body->radius;
	return sqrt(2 * body->mu / periapsis_altitude + vinf * vinf) - sqrt(2 * body->mu / periapsis_altitude);
}