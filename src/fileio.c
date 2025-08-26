#include "orbitlib_fileio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <urlmon.h>
#pragma comment(lib, "urlmon.lib")  // Link against urlmon.dll
#include <direct.h>    // _mkdir
#define MKDIR(path) _mkdir(path)
#else
#include <sys/stat.h>  // mkdir
#include <sys/types.h>
#define MKDIR(path) mkdir(path, 0755)
#include <unistd.h>
#include <dirent.h>
#endif


int directory_exists(const char *path) {
	if (!path || strlen(path) == 0)
		return 0;

#ifdef _WIN32
	DWORD attrs = GetFileAttributesA(path);
		return (attrs != INVALID_FILE_ATTRIBUTES) && (attrs & FILE_ATTRIBUTE_DIRECTORY);
#else
	struct stat st;
	return (stat(path, &st) == 0) && S_ISDIR(st.st_mode);
#endif
}

int create_directory(const char *path) {
	return MKDIR(path);
}

void download_file(const char *url, const char *filepath) {
#ifdef _WIN32
	HRESULT hr = URLDownloadToFile(NULL, url, filepath, 0, NULL);
    if (hr != S_OK) {
        fprintf(stderr, "Error downloading file: %lx\n", hr);
    }
#else
	char wget_command[512];
	snprintf(wget_command, sizeof(wget_command), "wget \"%s\" -O %s", url, filepath);
	int ret_code = system(wget_command);
	if (ret_code != 0) {
		fprintf(stderr, "Error executing wget: %d\n", ret_code);
	}
#endif
}








// Simple case-sensitive string ends-with
int ends_with(const char *filename, const char *ext) {
	size_t len = strlen(filename);
	size_t ext_len = strlen(ext);
	return len >= ext_len && strcmp(filename + len - ext_len, ext) == 0;
}

// List matching files in 'path' with given 'extension'
// Returns an array of strings and fills *count
char **list_files_with_extension(const char *path, const char *extension, int *count) {
	if (!path || !extension || !count) return NULL;
	
	char **results = NULL;
	int capacity = 10;
	int found = 0;
	results = malloc(capacity * sizeof(char*));
	if (!results) return NULL;

#ifdef _WIN32
	char search_path[MAX_PATH];
    snprintf(search_path, MAX_PATH, "%s\\*", path);

    WIN32_FIND_DATAA fd;
    HANDLE hFind = FindFirstFileA(search_path, &fd);

    if (hFind != INVALID_HANDLE_VALUE) {
        do {
            if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                if (ends_with(fd.cFileName, extension)) {
                    if (found >= capacity) {
                        capacity *= 2;
                        results = realloc(results, capacity * sizeof(char*));
                        if (!results) return NULL;
                    }
                    results[found++] = _strdup(fd.cFileName);  // Windows-specific strdup
                }
            }
        } while (FindNextFileA(hFind, &fd));
        FindClose(hFind);
    }

#else
	DIR *dir = opendir(path);
	if (!dir) {
		perror("opendir failed");
		free(results);
		return NULL;
	}
	
	struct dirent *entry;
	char full_path[1024];
	
	while ((entry = readdir(dir)) != NULL) {
		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0)
			continue;
		
		snprintf(full_path, sizeof(full_path), "%s/%s", path, entry->d_name);
		
		struct stat st;
		if (stat(full_path, &st) == 0 && S_ISREG(st.st_mode)) {
			if (ends_with(entry->d_name, extension)) {
				if (found >= capacity) {
					capacity *= 2;
					results = realloc(results, capacity * sizeof(char*));
					if (!results) return NULL;
				}
				results[found++] = strdup(entry->d_name);  // POSIX strdup
			}
		}
	}
	
	closedir(dir);
#endif
	
	*count = found;
	return results;
}