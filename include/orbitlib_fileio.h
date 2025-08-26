#ifndef ORBITLIB_ORBITLIB_FILEIO_H
#define ORBITLIB_ORBITLIB_FILEIO_H

#include "orbitlib_celestial.h"

int directory_exists(const char *path);
int create_directory(const char *path);
void download_file(const char *url, const char *filepath);
char **list_files_with_extension(const char *path, const char *extension, int *count);
CelestSystem * load_celestial_system_from_cfg_file(char *filename);

#endif //ORBITLIB_ORBITLIB_FILEIO_H
