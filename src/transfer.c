#include "orbitlib_transfer.h"
#include "geometrylib.h"
#include <math.h>
#include <stdio.h>

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

// dt --> 0
double departure_true_anomaly_at_min_dt(double r0, double r1, double delta_ta) {
	delta_ta = pi_norm(delta_ta);
	double max_arr_ta;
	if(delta_ta < M_PI) {
		double r = r1/r0;
		double r1r2 = sqrt(1 + r*r - 2 * r * cos(delta_ta));
		double beta = acos((1 + r1r2 * r1r2 - r * r) / (2 * r1r2));
		double alpha = M_PI / 2 - beta;
		max_arr_ta = 2*M_PI-alpha;
	} else {
		max_arr_ta = 2*M_PI-delta_ta/2;
	}
	return pi_norm(max_arr_ta);
}

// dt --> inf
double departure_true_anomaly_at_max_dt(double r0, double r1, double delta_ta) {
	double min_arr_ta;
	Vector2 p1 = {r0, 0};
	Vector2 p2 = {cos(delta_ta)*r1, sin(delta_ta)*r1};
	double pxr = (p2.x-p1.x)/(r1 - r0);
	double pyr = (p2.y-p1.y)/(r1 - r0);
	double p = (2*pxr)/(pxr*pxr + pyr*pyr);
	double q = (1-pyr*pyr)/(pxr*pxr + pyr*pyr);
	
	// IF NAN IN 2D TRANSFER CALC, LOOK HERE. SQRT SHOULD NOT GET NEGATIVE
	double inside_sqrt = p*p/4 - q;
	if(inside_sqrt < 0) inside_sqrt *= -1;
	
	double mx1 = -p/2 - sqrt(inside_sqrt);
	double mx2 = -p/2 + sqrt(inside_sqrt);
	
	Vector2 m_n[] = {
			{mx1, +sqrt(1-mx1*mx1)},
			{mx1, -sqrt(1-mx1*mx1)},
			{mx2, +sqrt(1-mx2*mx2)},
			{mx2, -sqrt(1-mx2*mx2)}
	};
	
	double m_ml_dot[4];
	
	// dot product of m and ml should be 0, due to imprecision not accurately 0 -> find smallest of the pairs
	// pair (0,1) and (2,3) (see above; two times two solutions of square root)
	for(int i = 0; i < 4; i++) m_ml_dot[i] = m_n[i].x*m_n[i].x+pxr*m_n[i].x+m_n[i].y*m_n[i].y+pyr*m_n[i].y;
	if(fabs(m_ml_dot[1]) < fabs(m_ml_dot[0])) m_n[0] = m_n[1];
	m_n[1] = fabs(m_ml_dot[2]) < fabs(m_ml_dot[3]) ? m_n[2] : m_n[3];
	
	for(int i = 0; i < 2; i++) {
		min_arr_ta = angle_ccw_vec2_vec2(p1, m_n[i]);
		double min_theta2 = angle_ccw_vec2_vec2(p2, m_n[i]);
		if(min_arr_ta > min_theta2) {
			break;
		}
	}
	return pi_norm(min_arr_ta);
}

Lambert2 calc_lambert2(double r0, double r1, double delta_ta, double target_dt, Body *cb) {
	// 0°, 180° and 360° are extreme edge cases with funky stuff happening with floating point imprecision -> adjust delta in true anomaly
	if(fabs(delta_ta) < 0.001 ||
	   fabs(delta_ta-M_PI) < 0.001) delta_ta += 0.001;
	if(fabs(delta_ta-2*M_PI) < 0.001) delta_ta -= 0.001;
	
	double min_ta0 = r1/r0 > 1 ? departure_true_anomaly_at_min_dt(r0, r1, delta_ta) : departure_true_anomaly_at_max_dt(r0, r1, delta_ta);
	double max_ta0 = r1/r0 > 1 ? departure_true_anomaly_at_max_dt(r0, r1, delta_ta) : departure_true_anomaly_at_min_dt(r0, r1, delta_ta);
	
	if(min_ta0 > max_ta0) min_ta0 -= 2*M_PI;
	
	DataArray2 *data = data_array2_create();
	
	data_array2_insert_new(data, min_ta0, r1/r0 > 1 ? -target_dt : 1e100);
	data_array2_insert_new(data, max_ta0, r1/r0 > 1 ? 1e100 : -target_dt);
	
	// true anomaly of r0, theta1 not normed to pi and true anomaly of r1
	double ta0, ta0_pun, ta1, last_ta0_pun;
	double mu = cb->mu;
	
	double dt;
	double a, e;
	
	enum LAMBERT_SOLVER_SUCCESS success = LAMBERT_MAX_ITERATIONS;
	
	for(int i = 0; i < 100; i++) {
		ta0_pun = root_finder_monot_func_next_x(data);
		if(i > 3 && last_ta0_pun == ta0_pun) { success = LAMBERT_IMPRECISION; break;}	// increments are 0 (due to imprecision)
		
		ta0 = pi_norm(ta0_pun);
		ta1 = pi_norm(ta0 + delta_ta);
		e = (r1 - r0) / (r0 * cos(ta0) - r1*cos(ta1));
		
		if(e < 0){  // not possible
			printf("\n\n!!!!! PANIC e < 0 !!!!!!!!\n");
			printf("ta0: %.10f°; ta1: %f°; target dt: %fd\nr0: %fe6m; r1: %fe6m; delta_ta: %f°\n", rad2deg(ta0), rad2deg(ta1), target_dt/86400, r0*1e-9, r1*1e-9, rad2deg(delta_ta));
			printf("min ta0: %f°; max ta0: %f°\ne: %f; a: %f\n", rad2deg(min_ta0), rad2deg(max_ta0), e, a);
			print_data_array2(data, "ta0", "dt");
			printf("-------------\n\n");
			success = LAMBERT_FAIL_ECC;
			break;
		} else if(e==1) e += 1e-10;	// no calculations for parabola -> make it a hyperbola
		
		double rp = r0*(1 + e * cos(ta0))/(1 + e);
		if(rp <= 0) rp = 1e-10;
		a = rp/(1-e);
		double n = sqrt(mu / pow(fabs(a),3));
		
		double t1,t2;
		double T = 2*M_PI/n;
		if(e < 1) {
			double E1 = acos((e + cos(ta0))/(1 + e*cos(ta0)));
			t1 = (E1 - e * sin(E1)) / n;
			if(ta0 > M_PI) t1 = T - t1;
			double E2 = acos((e + cos(ta1))/(1 + e*cos(ta1)));
			t2 = (E2 - e * sin(E2)) / n;
			if(ta1 > M_PI) t2 = T - t2;
			dt = ta0 < ta1 ? t2 - t1 : T - t1 + t2;
		} else {
			double one_plus_ecos = (1 + e*cos(ta0));
			// imprecision in extreme cases can lead to (1 + e*cos(ta0)) = 0, which is adjusted so F0 doesn't get infinity
			if(one_plus_ecos == 0) one_plus_ecos = 1e-10;
			double F0 = acosh((e + cos(ta0))/one_plus_ecos);
			t1 = (e * sinh(F0) - F0) / n;
			one_plus_ecos = (1 + e*cos(ta1));
			// imprecision in extreme cases can lead to (1 + e*cos(ta1)) = 0, which is adjusted so F1 doesn't get infinity
			if(one_plus_ecos == 0) one_plus_ecos = 1e-10;
			double F1 = acosh((e + cos(ta1))/one_plus_ecos);
			t2 = (e * sinh(F1) - F1) / n;
			// different quadrant
			if((ta0 < M_PI) != (ta1 < M_PI)) dt = t1 + t2;
				// past periapsis
			else if(ta0 < M_PI) dt = t2 - t1;
				// before periapsis
			else dt = t1-t2;
		}
		
		if(isnan(dt)){  // at this ta0 orbit not solvable
			printf("---!!!!   NAN   !!!!---\n");
			printf("ta0: %.10f°; ta1: %f°; target dt: %fd\nr0: %fe6m; r1: %fe6m; delta_ta: %f°\n", rad2deg(ta0), rad2deg(ta1), target_dt/86400, r0*1e-9, r1*1e-9, rad2deg(delta_ta));
			printf("min ta0: %f°; max ta0: %f°\nt1: %fd; t2: %fd; T: %fd; T/2: %fd\ne: %f; a: %f\n", rad2deg(min_ta0), rad2deg(max_ta0), t1/86400, t2/86400, T/86400, T/2/86400, e, a);
			print_data_array2(data, "ta0", "dt");
			printf("-------------\n\n");
			success = LAMBERT_FAIL_NAN;
			break;
		}
		data_array2_insert_new(data, ta0_pun, dt - target_dt);
		last_ta0_pun = ta0_pun;
		
		if(fabs(target_dt-dt) < 1) {
			success = LAMBERT_SUCCESS;
			break;
		}
	}
	
	data_array2_free(data);
	Lambert2 solution = {constr_orbit_from_elements(a, e, 0, 0, 0, 0, cb), ta0, ta1, success};
	return solution;
}

Lambert3 calc_lambert3(Vector3 r0, Vector3 r1, double target_dt, Body *cb) {
	double r0_mag = mag_vec3(r0);
	double r1_mag = mag_vec3(r1);
	double delta_ta = angle_vec3_vec3(r0, r1);
	if (cross_vec3(r0, r1).z < 0) delta_ta = 2 * M_PI - delta_ta;
	Lambert2 solution2d = calc_lambert2(r0_mag, r1_mag, delta_ta, target_dt, cb);
	
	if(solution2d.success == LAMBERT_FAIL_ECC) {
		return (Lambert3) {.success = solution2d.success};
	}
	
	Vector3 origin = {0, 0, 0};
	Plane3 p_0 = constr_plane3(origin, vec3(1,0,0), vec3(0,1,0));
	Plane3 p_T = constr_plane3(origin, r0, r1);
	Orbit orbit2d = solution2d.orbit;
	double e = orbit2d.e;
	double ta0 = solution2d.true_anomaly0;
	double ta1 = solution2d.true_anomaly1;
	
	double fpa0 = calc_orbit_flight_path_angle(e, ta0);
	double fpa1 = calc_orbit_flight_path_angle(e, ta1);
	
	double v0_mag = calc_orbital_speed(orbit2d, r0_mag);
	double v1_mag = calc_orbital_speed(orbit2d, r1_mag);
	
	Vector2 v0_2d = calc_vel_vec2(r0_mag, v0_mag, ta0, fpa0);
	Vector2 v1_2d = calc_vel_vec2(r1_mag, v1_mag, ta1, fpa1);
	
	// calculate RAAN, inclination and argument of periapsis
	Vector3 inters_line = calc_intersecting_line_dir_plane3(p_0, p_T);
	if(inters_line.y < 0) inters_line = scale_vec3(inters_line, -1); // for rotation of raan in clock-wise direction
	Vector3 in_plane_up = cross_vec3(inters_line, norm_vector_plane3(p_T));    // 90° to intersecting line and norm vector of plane
	if(in_plane_up.z < 0) in_plane_up = scale_vec3(in_plane_up, -1);   // this vector is always 90° before raan for prograde orbits
	double raan = in_plane_up.x <= 0 ? angle_vec3_vec3(vec3(1,0,0), inters_line) : angle_vec3_vec3(vec3(1,0,0), inters_line) + M_PI;   // raan 90° behind in_plane_up
	
	//double i = angle_plane_plane(p_T, p_0);   // can create angles greater than 90°
	double i = angle_plane3_vec3(p_0, in_plane_up);   // also possible to get angle between p_0 and in_plane_up
	
	double arg_peri = 2*M_PI - ta0;
	if(raan < M_PI) {
		if(r0.z >= 0) arg_peri += angle_vec3_vec3(inters_line, r0);
		else arg_peri += 2*M_PI - angle_vec3_vec3(inters_line, r0);
	} else {
		if(r0.z <= 0) arg_peri += angle_vec3_vec3(inters_line, r0)+M_PI;
		else arg_peri += M_PI - angle_vec3_vec3(inters_line, r0);
	}
	
	Vector3 v0 = heliocentric_rot(v0_2d, raan, arg_peri, i);
	Vector3 v1 = heliocentric_rot(v1_2d, raan, arg_peri, i);
	
	return (Lambert3) {r0, v0, r1, v1, solution2d.success};
}


double dv_circ(struct Body *body, double periapsis_altitude, double vinf) {
	periapsis_altitude += body->radius;
	return sqrt(2 * body->mu / periapsis_altitude + vinf * vinf) - sqrt(body->mu / periapsis_altitude);
}

double dv_capture(struct Body *body, double periapsis_altitude, double vinf) {
	periapsis_altitude += body->radius;
	return sqrt(2 * body->mu / periapsis_altitude + vinf * vinf) - sqrt(2 * body->mu / periapsis_altitude);
}

double get_flyby_periapsis(Vector3 v_arr, Vector3 v_dep, Vector3 v_body, Body *body) {
	Vector3 v1 = subtract_vec3(v_arr, v_body);
	Vector3 v2 = subtract_vec3(v_dep, v_body);
	double beta = (M_PI - angle_vec3_vec3(v1, v2))/2;
	return (1 / cos(beta) - 1) * (body->mu / (pow(mag_vec3(v1), 2)));
}

HyperbolaLegParams get_dep_hyperbola_params(Vector3 v_inf) {
	Plane3 xy = constr_plane3(vec3(0,0,0), vec3(1,0,0), vec3(0,1,0));
	Plane3 xz = constr_plane3(vec3(0,0,0), vec3(1,0,0), vec3(0,0,1));
	double decl = angle_plane3_vec3(xy, v_inf);
	decl = v_inf.z < 0 ? -fabs(decl) : fabs(decl);
	double bplane_angle = -angle_plane3_vec3(xz, v_inf);
	if(cross_vec3(v_inf,vec3(0,1,0)).z < 0) bplane_angle = M_PI-bplane_angle;
	bplane_angle = pi_norm(bplane_angle);
	// bvazi only relevant with given inclination (not yet implemented here, but implemented for fly-by)
	HyperbolaLegParams hyp_params = {decl, bplane_angle, M_PI/2};
	return hyp_params;
}

HyperbolaParams get_hyperbola_params(Vector3 v_arr, Vector3 v_dep, Vector3 v_body, struct Body *body, double h_pe, enum HyperbolaType type) {
	Vector3 vinf_arr = subtract_vec3(v_arr, v_body);
	Vector3 vinf_dep = subtract_vec3(v_dep, v_body);
	HyperbolaParams hyperbola_params = {.type = type};
	
	if(type != HYP_DEPARTURE) {
		hyperbola_params.incoming = get_dep_hyperbola_params(vinf_arr);
		// invert hyperbola direction (departure to arrival hyperbola)
		hyperbola_params.incoming.decl *= -1;
		hyperbola_params.incoming.bplane_angle = pi_norm(M_PI + hyperbola_params.incoming.bplane_angle);
	}
	if(type != HYP_ARRIVAL) hyperbola_params.outgoing = get_dep_hyperbola_params(vinf_dep);
	if(type == HYP_FLYBY) {
		Vector3 N = cross_vec3(vinf_arr, vinf_dep);
		Vector3 B_arr = cross_vec3(vinf_arr, N);
		Vector3 B_dep = cross_vec3(vinf_dep, scale_vec3(N,-1));
		
		// BVAZI (Azimuth of B-vector) calculated from south
		hyperbola_params.incoming.bvazi = angle_vec3_vec3(vec3(0,0,-1),B_arr);
		hyperbola_params.outgoing.bvazi = angle_vec3_vec3(vec3(0,0,-1),B_dep);
		
		// if retrograde orbit
		if(N.z < 0) {
			hyperbola_params.incoming.bvazi *= -1;
			hyperbola_params.outgoing.bvazi *= -1;
		}
	}
	
	double rp = type == HYP_FLYBY ? get_flyby_periapsis(v_arr, v_dep, v_body, body) : h_pe+body->radius;
	double c3_energy = sq_mag_vec3(type != HYP_DEPARTURE ? vinf_arr : vinf_dep);
	
	hyperbola_params.rp = rp;
	hyperbola_params.c3_energy = c3_energy;
	
	return hyperbola_params;
}

