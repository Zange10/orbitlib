#ifndef ORBITLIB_ORBITLIB_FILEIO_H
#define ORBITLIB_ORBITLIB_FILEIO_H

int directory_exists(const char *path);
int create_directory(const char *path);
void download_file(const char *url, const char *filepath);
char **list_files_with_extension(const char *path, const char *extension, int *count);

#endif //ORBITLIB_ORBITLIB_FILEIO_H
