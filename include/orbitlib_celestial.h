#ifndef ORBITLIB_ORBITLIB_CELESTIAL_H
#define ORBITLIB_ORBITLIB_CELESTIAL_H

#include "orbitlib_orbit.h"
#include "orbitlib_ephemeris.h"

/**
 * @brief Represents a celestial body with physical and orbital properties
 */
typedef struct Body {
	char name[32];              /**< Name of the celestial body */
	double color[3];            /**< RGB color used for visualization (orbit, body) */
	int id;                     /**< Body ID as defined by JPL's Horizon API */
	double mu;                  /**< Gravitational parameter (GM) [m³/s²] */
	double radius;              /**< Physical radius of the body [m] */
	double rotation_period;     /**< Sidereal rotation period [s] */
	double sl_atmo_p;           /**< Atmospheric pressure at sea level [Pa] */
	double scale_height;        /**< Scale height of the atmosphere [m] */
	double atmo_alt;            /**< Maximum altitude with atmosphere (KSP-specific) [m] */
	double north_pole_ra;		/**< Right Ascension of the north pole relative to helio-centric coordinate system [rad] */
	double north_pole_decl;		/**< Declination of the north pole relative to helio-centric coordinate system [rad] */
	double rot_ut0;				/**< Rotation at UT0 (Angle between xz-plane (x+) and prime meridian) [rad] */
	struct CelestSystem *system;/**< Pointer to the system this body is the central body of */
	struct Orbit orbit;         /**< Orbit of the body at reference time (UT0) */
	struct Ephem *ephem;        /**< Pointer to ephemeris data (if available) */
	int num_ephems;             /**< Number of ephemeris states stored */
} Body;


/**
 * @brief Propagation method for celestial system bodies
 */
enum CelestSystemPropMethod {
	ORB_ELEMENTS,  /**< Use orbital elements for propagation */
	EPHEMS         /**< Use ephemerides for propagation */
};

/**
 * @brief Represents a celestial system with a central body and orbiting bodies
 */
typedef struct CelestSystem {
	char name[50];                             	/**< Name of the celestial system */
	int num_bodies;                            	/**< Number of bodies orbiting the central body */
	struct Body *cb;                           	/**< Central body of the system */
	struct Body *home_body;						/**< Home body of the system (KSP-related; NULL if there is none) */
	struct Body **bodies;                      	/**< Array of pointers to orbiting bodies */
	enum CelestSystemPropMethod prop_method;   	/**< Propagation method: orbital elements or ephemerides */
	double ut0;                                	/**< Reference time (UT0) for the system */
} CelestSystem;

/*
 * ------------------------------------
 * Body & System Creation
 * ------------------------------------
 */


/**
 * @brief Allocates and initializes a new celestial body
 *
 * @return Pointer to the newly created Body
 */
struct Body * new_body();


/**
 * @brief Returns the equatorial frame of the body
 *
 * @param body The body
 * @return The equatorial frame
 */
Plane3 get_body_equatorial_plane(Body *body);


/**
 * @brief Sets the visualization color of a celestial body
 *
 * @param body Pointer to the body to modify
 * @param red Red component (0–1)
 * @param green Green component (0–1)
 * @param blue Blue component (0–1)
 */
void set_body_color(struct Body *body, double red, double green, double blue);

/**
 * @brief Allocates and initializes a new celestial system
 *
 * @return Pointer to the newly created CelestSystem
 */
CelestSystem * new_system();


/*
 * ------------------------------------
 * System Queries
 * ------------------------------------
 */

/**
 * @brief Returns the number of subsystems within a celestial system
 *
 * Subsystems are systems where a body in the current system is the central body of another system.
 *
 * @param system Pointer to the celestial system
 * @return Number of subsystems
 */
int get_number_of_subsystems(CelestSystem *system);

/**
 * @brief Returns the top-level system that contains the given system
 *
 * @param system Pointer to the current celestial system
 * @return Pointer to the top-level system
 */
CelestSystem * get_top_level_system(CelestSystem *system);

/**
 * @brief Searches for a body by name within a given celestial system
 *
 * @param name Name of the body to find
 * @param system Pointer to the system to search in
 * @return Pointer to the body if found, NULL otherwise
 */
struct Body * get_body_by_name(char *name, CelestSystem *system);

/**
 * @brief Returns the index (system-local ID) of a body within a celestial system
 *
 * @param body Pointer to the body
 * @param system Pointer to the system
 * @return Index of the body in the system's body list, or -1 if not found
 */
int get_body_system_id(struct Body *body, CelestSystem *system);

/*
 * ------------------------------------
 * System Printing
 * ------------------------------------
 */

/**
 * @brief Prints a single layer of the celestial system hierarchy
 *
 * A layer corresponds to a subsystem level within the system.
 *
 * @param system Pointer to the celestial system
 * @param layer Subsystem depth to print (0 = top level)
 */
void print_celestial_system_layer(CelestSystem *system, int layer);

/**
 * @brief Prints the entire celestial system hierarchy
 *
 * @param system Pointer to the top-level celestial system
 */
void print_celestial_system(CelestSystem *system);


/*
 * ------------------------------------
 * System Initialization & Cleanup
 * ------------------------------------
 */

/**
 * @brief Loads and initializes all available celestial systems from a directory
 *
 * Searches the specified directory for system definition files and initializes
 * all valid celestial systems found.
 *
 * @param directory Path to the directory containing system files
 * @param num_systems Output parameter for the number of systems loaded
 * @return Array of pointers to initialized celestial systems
 */
CelestSystem ** init_available_systems_from_path(const char *directory, int *num_systems);

/**
 * @brief Frees memory associated with a single celestial system
 *
 * Deallocates all heap-allocated memory used by the system and its bodies.
 *
 * @param system Pointer to the system to free
 */
void free_celestial_system(CelestSystem *system);

/**
 * @brief Frees memory associated with an array of celestial systems
 *
 * Frees each system in the array and the array itself.
 *
 * @param systems Array of pointers to celestial systems
 * @param num_systems Number of systems in the array
 */
void free_celestial_systems(CelestSystem **systems, int num_systems);


/*
 * ------------------------------------
 * Altitude / Radius Conversions
 * ------------------------------------
 */

/**
 * @brief Converts radius to altitude above a body's surface
 *
 * @param body Pointer to the celestial body
 * @param radius Distance from the center of the body [m]
 * @return Altitude above the surface [m]
 */
double radius2alt(Body *body, double radius);

/**
 * @brief Converts altitude above a body's surface to radius from center
 *
 * @param body Pointer to the celestial body
 * @param altitude Altitude above the surface [m]
 * @return Radius from the center of the body [m]
 */
double alt2radius(Body *body, double altitude);

/**
 * @brief Converts altitude above the atmosphere to radius from center
 *
 * @param body Pointer to the celestial body
 * @param altitude_above_atmosphere Altitude above the top of the atmosphere [m]
 * @return Radius from the center of the body [m]
 */
double altatmo2radius(Body *body, double altitude_above_atmosphere);

#endif //ORBITLIB_ORBITLIB_CELESTIAL_H
