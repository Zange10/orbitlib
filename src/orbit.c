#include "orbitlib_orbit.h"
#include "orbitlib_celestial.h"
#include <math.h>

struct Orbit constr_orbit_from_elements(double semimajor_axis, double eccentricity, double inclination, double raan, double arg_of_peri, double true_anomaly, struct Body *cb) {
	struct Orbit new_orbit;
	new_orbit.cb = cb;
	new_orbit.a = semimajor_axis;
	// avoid nan when converting to osv and back
	new_orbit.e = eccentricity != eccentricity ? eccentricity : eccentricity+1e-12;
	// avoid nan when converting to osv and back
	new_orbit.i = inclination != 0 ? inclination : inclination + 1e-12;
	new_orbit.raan = raan;
	new_orbit.arg_peri = arg_of_peri;
	new_orbit.ta = true_anomaly;
	
	return new_orbit;
}


struct Orbit constr_orbit_from_apsides(double apsis1, double apsis2, double inclination, struct Body *cb) {
	struct Orbit new_orbit;
	new_orbit.cb = cb;
	double ap, pe;
	if(apsis1 > apsis2) {
		ap = apsis1;
		pe = apsis2;
	} else {
		ap = apsis2;
		pe = apsis1;
	}
	new_orbit.a = (ap+pe)/2;
	new_orbit.i = inclination;
	new_orbit.e = (ap-pe)/(ap+pe);
	
	// not given, therefore zero
	new_orbit.raan = 0;
	new_orbit.arg_peri = 0;
	new_orbit.ta = 0;
	
	return new_orbit;
}

struct Orbit constr_orbit_from_osv(Vector3 r, Vector3 v, struct Body *cb) {
	double r_mag = mag_vec3(r);
	double v_mag = mag_vec3(v);
	double v_r = dot_vec3(v,r) / r_mag;
	double mu = cb->mu;
	
	double a = 1 / (2/r_mag - pow(v_mag,2)/mu);
	Vector3 h = cross_vec3(r,v);
	Vector3 e = scale_vec3(add_vec3(cross_vec3(v,h), scale_vec3(r, -mu/r_mag)), 1/mu);
	double e_mag = mag_vec3(e);
	
	Vector3 k = {0,0,1};
	Vector3 n_vec = cross_vec3(k, h);
	Vector3 n_norm = norm_vec3(n_vec);
	double RAAN, i, arg_peri;
	
	if(mag_vec3(n_vec) != 0) {
		RAAN = n_norm.y >= 0 ? acos(n_norm.x) : 2 * M_PI - acos(n_norm.x); // if n_norm.y is negative: raan > 180°
		i = acos(dot_vec3(k, norm_vec3(h)));
		double dp = dot_vec3(n_norm, e) / e_mag; if(dp > 1) dp = 1; if(dp < -1) dp = -1;	// if inside cos greater than 1 -> nan
		arg_peri = e.z >= 0 ? acos(dp) : 2 * M_PI - acos(dp);  // if r.z is positive: w > 180°
	} else {
		RAAN = 0;
		i = dot_vec3(k, norm_vec3(h)) > 0 ? 0 : M_PI;
		arg_peri = cross_vec3(r,v).z * e.y > 0 ? acos(e.x/e_mag) : 2*M_PI - acos(e.x/e_mag);
	}
	double dp = dot_vec3(e,r) / (e_mag*r_mag); if(dp > 1) dp = 1; if(dp < -1) dp = -1;	// if inside cos greater than 1 -> nan
	double ta = v_r >= 0 ? acos(dp) : 2*M_PI - acos(dp);
	
	struct Orbit new_orbit = constr_orbit_from_elements(a, e_mag, i, RAAN, arg_peri, ta, cb);
	
	return new_orbit;
}

Vector2 calc_orbital_speed_2d(double r_mag, double v_mag, double theta, double gamma) {
	Vector2 r_norm = {cos(theta), sin(theta)};
	Vector2 r = scale_vec2(r_norm, r_mag);
	Vector2 n = {-r.y, r.x};
	Vector2 v = rotate_vec2(n, gamma);
	v = scale_vec2(v, v_mag/ mag_vec2(v));
	
	return v;
}

Vector3 heliocentric_rot(Vector2 v, double RAAN, double w, double incl) {
	double Q[3][3] = {
			{-sin(RAAN)*cos(incl)*sin(w) + cos(RAAN)*cos(w), -sin(RAAN)*cos(incl)*cos(w) - cos(RAAN)*sin(w), sin(RAAN)*sin(incl)},
			{cos(RAAN)*cos(incl)*sin(w) + sin(RAAN)*cos(w),  cos(RAAN)*cos(incl)*cos(w) - sin(RAAN)*sin(w),  -cos(RAAN)*sin(incl)},
			{sin(incl)*sin(w),                               sin(incl)*cos(w),                               cos(incl)}};
	
	double v_vec[3] = {v.x, v.y, 0};
	double result[3] = {0,0,0};
	
	for(int i = 0; i < 3; i++) {
		for(int j = 0; j < 3; j++) {
			result[i] += Q[i][j]*v_vec[j];
		}
	}
	Vector3 result_v = {result[0], result[1], result[2]};
	return result_v;
}

OSV osv_from_orbit(Orbit orbit) {
	double gamma = atan(orbit.e*sin(orbit.ta)/(1+orbit.e*cos(orbit.ta)));
	double r_mag = orbit.a*(1-pow(orbit.e,2)) / (1+orbit.e*cos(orbit.ta));
	double v_mag = sqrt(orbit.cb->mu*(2/r_mag - 1/orbit.a));
	Vector2 r_2d = {cos(orbit.ta) * r_mag, sin(orbit.ta) * r_mag};
	Vector2 v_2d = calc_orbital_speed_2d(r_mag, v_mag, orbit.ta, gamma);
	
	Vector3 r = heliocentric_rot(r_2d, orbit.raan, orbit.arg_peri, orbit.i);
	Vector3 v = heliocentric_rot(v_2d, orbit.raan, orbit.arg_peri, orbit.i);
	
	OSV osv = {r, v};
	return osv;
}