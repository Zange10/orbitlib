#ifndef ORBITLIB_ORBITLIB_FILEIO_H
#define ORBITLIB_ORBITLIB_FILEIO_H

#include "orbitlib_celestial.h"

/*
 * ------------------------------------
 * File & Directory Utilities
 * ------------------------------------
 */

/**
 * @brief Checks whether a directory exists at the given path
 *
 * @param path Path to check
 * @return Non-zero if the directory exists, 0 otherwise
 */
int directory_exists(const char *path);

/**
 * @brief Creates a directory at the given path
 *
 * @param path Path where the directory should be created
 * @return 0 on success, non-zero on failure
 */
int create_directory(const char *path);

/**
 * @brief Downloads a file from a given URL to the specified file path
 *
 * @param url URL of the file to download
 * @param filepath Path where the downloaded file should be saved
 */
void download_file(const char *url, const char *filepath);

/**
 * @brief Lists all files in a directory with a specific file extension
 *
 * @param path Directory to search
 * @param extension File extension to match (e.g., ".cfg")
 * @param count Output parameter for the number of matching files
 * @return Array of matching filenames (must be freed by caller)
 */
char **list_files_with_extension(const char *path, const char *extension, int *count);


/*
 * ------------------------------------
 * Celestial System I/O
 * ------------------------------------
 */

/**
 * @brief Parses and organizes a system's bodies into their respective subsystems
 *
 * @param system Pointer to the celestial system
 */
void parse_and_sort_into_celestial_subsystems(CelestSystem *system);

/**
 * @brief Stores a celestial system in a configuration file
 *
 * @param system Pointer to the system to store
 * @param directory Path to the directory where the config file will be saved
 */
void store_system_in_config_file(CelestSystem *system, const char *directory);

/**
 * @brief Loads a celestial system from a configuration file
 *
 * @param filename Path to the configuration file
 * @return Pointer to the loaded celestial system
 */
CelestSystem * load_celestial_system_from_cfg_file(char *filename);

#endif //ORBITLIB_ORBITLIB_FILEIO_H
