#ifndef ORBITLIB_ORBITLIB_ORBIT_H
#define ORBITLIB_ORBITLIB_ORBIT_H

typedef struct Orbit {
	struct Body * cb;	// central body
	double e;           // eccentricity
	double a;           // semi-major axis
	double incl; 		// inclination
	double raan;        // right ascension of the ascending node
	double arg_peri; 	// argument of periapsis
	double ta;   		// true anomaly
	double t;			// time since periapsis
	double period;      // orbital period
	double apoapsis;    // highest point in orbit
	double periapsis;   // lowest point in orbit
} Orbit;

typedef struct OSV {
	// tbd
} OSV;

#endif //ORBITLIB_ORBITLIB_ORBIT_H
