#include "orbitlib_fileio.h"
#include "orbitlib_celestial.h"
#include "orbitlib_orbit.h"
#include "orbitlib_ephemeris.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <ctype.h>

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

void parse_and_sort_into_celestial_subsystems(CelestSystem *system) {
	system->cb->system = system;
	
	for(int i = 0; i < system->num_bodies; i++) {
		int num_child_bodies = 0;
		struct Body *body = system->bodies[i];
		for(int j = 0; j < system->num_bodies; j++) {
			if(system->bodies[j]->orbit.cb == body) num_child_bodies++;
		}
		if(num_child_bodies > 0) {
			CelestSystem *child_system = new_system();
			sprintf(child_system->name, "%s SYSTEM", body->name);
			child_system->num_bodies = num_child_bodies;
			child_system->bodies = malloc(num_child_bodies * sizeof(struct Body*));
			child_system->cb = body;
			child_system->prop_method = system->prop_method;
			child_system->ut0 = system->ut0;
			body->system = child_system;
		}
	}
	
	for(int i = 0; i < system->num_bodies; i++) {
		struct Body *body = system->bodies[i];
		struct Body *attractor = body->orbit.cb;
		if(attractor != system->cb) {
			for(int j = 0; j < attractor->system->num_bodies-1; j++) {
				attractor->system->bodies[j] = attractor->system->bodies[j+1];
			}
			attractor->system->bodies[attractor->system->num_bodies-1] = body;
			for(int j = i; j < system->num_bodies-1; j++) {
				system->bodies[j] = system->bodies[j+1];
			}
			system->num_bodies--;
			i--;
		}
	}
	
	struct Body **temp = realloc(system->bodies, system->num_bodies*(sizeof(struct Body*)));
	if(temp == NULL) return;
	system->bodies = temp;
}

int get_key_and_value_from_config(char *key, char *value, char *line) {
	char *equalSign = strchr(line, '=');  // Find '='
	if (equalSign) {
		size_t keyLen = equalSign - line;
		strncpy(key, line, keyLen);  // Copy key
		key[keyLen] = '\0';  // Null-terminate
		if(isspace(key[keyLen-1])) key[keyLen-1] = '\0';	// remove space
		
		strcpy(value,  equalSign + (isspace(*(equalSign + 1)) ? 2 : 1));  // Copy value and skip space
		
		// remove line breaks
		size_t len = strlen(value);
		if (len > 0 && (value[len - 1] == '\n' || value[len - 1] == '\r')) {
			value[len - 1] = '\0';  // Replace newline with null terminator
		}
		if (len > 0 && value[len - 2] == '\r') {
			value[len - 2] = '\0';  // Replace newline with null terminator
		}
		return 1;
	}
	return 0;
}

enum STORED_UNITS {UNITS_LEGACY, UNITS_M_DEG_PA};

struct Body * load_body_from_config_file(FILE *file, CelestSystem *system, enum STORED_UNITS units) {
	struct Body *body = new_body();
	double mean_anomaly = 0;
	double g_asl = 0;
	int has_mean_anomaly = 0;
	int has_g_asl = 0;
	int has_central_body_name = 0;
	char central_body_name[50];
	
	char line[256];  // Buffer for each line
	while (fgets(line, sizeof(line), file)) {
		if(strncmp(line, "[", 1) == 0) {
			sscanf(line, "[%50[^]]]", body->name);
		} else if(strcmp(line, "\n") == 0 || strcmp(line,"\r\n") == 0){
			break;
		} else {
			char key[50], value[50];
			if(get_key_and_value_from_config(key, value, line)) {
				if (strcmp(key, "color") == 0) {
					sscanf(value, "[%lf, %lf, %lf]", &body->color[0], &body->color[1], &body->color[2]);
				} else if (strcmp(key, "id") == 0) {
					sscanf(value, "%d", &body->id);
				} else if (strcmp(key, "gravitational_parameter") == 0) {
					sscanf(value, "%lg", &body->mu);
				} else if (strcmp(key, "g_asl") == 0) {
					sscanf(value, "%lg", &g_asl); has_g_asl = 1;
				} else if (strcmp(key, "radius") == 0) {
					sscanf(value, "%lf", &body->radius);
				} else if (strcmp(key, "rotational_period") == 0) {
					sscanf(value, "%lf", &body->rotation_period);
				} else if (strcmp(key, "sea_level_pressure") == 0) {
					sscanf(value, "%lf", &body->sl_atmo_p);
				} else if (strcmp(key, "scale_height") == 0) {
					sscanf(value, "%lf", &body->scale_height);
				} else if (strcmp(key, "atmosphere_altitude") == 0) {
					sscanf(value, "%lf", &body->atmo_alt);
				} else if (strcmp(key, "semi_major_axis") == 0) {
					sscanf(value, "%lf", &body->orbit.a);
				} else if (strcmp(key, "eccentricity") == 0) {
					sscanf(value, "%lf", &body->orbit.e);
				} else if (strcmp(key, "inclination") == 0) {
					sscanf(value, "%lf", &body->orbit.i);
					body->orbit.i = deg2rad(body->orbit.i);
				} else if (strcmp(key, "raan") == 0) {
					sscanf(value, "%lf", &body->orbit.raan);
					body->orbit.raan = deg2rad(body->orbit.raan);
				} else if (strcmp(key, "argument_of_periapsis") == 0) {
					sscanf(value, "%lf", &body->orbit.arg_peri);
					body->orbit.arg_peri = deg2rad(body->orbit.arg_peri);
				} else if (strcmp(key, "true_anomaly_ut0") == 0) {
					sscanf(value, "%lf", &body->orbit.ta);
					body->orbit.ta = deg2rad(body->orbit.ta);
				} else if (strcmp(key, "mean_anomaly_ut0") == 0) {
					sscanf(value, "%lf", &mean_anomaly);
					has_mean_anomaly = 1;
				} else if (strcmp(key, "parent_body") == 0) {
					sscanf(value, "%s", central_body_name);
					has_central_body_name = 1;
				}
			}
		}
	}
	if(units == UNITS_LEGACY) {
		body->radius *= 1e3;  // Convert from km to m
		body->sl_atmo_p *= 1e3;  // Convert from kpa to pa
		body->atmo_alt *= 1e3;  // Convert from km to m
		body->orbit.a *= 1e3;  // Convert from km to m
	}
	
	if(has_g_asl) body->mu = 9.81*g_asl * body->radius*body->radius;
	
	if(system != NULL) {
		struct Body *attractor = system->cb;
		if(has_central_body_name) {
			struct Body *attr_temp = get_body_by_name(central_body_name, system);
			if(attr_temp != NULL) attractor = attr_temp;
		}
		
		body->orbit = constr_orbit_from_elements(
				body->orbit.a,
				body->orbit.e,
				body->orbit.i,
				body->orbit.raan,
				body->orbit.arg_peri,
				has_mean_anomaly ? calc_true_anomaly_from_mean_anomaly(body->orbit, mean_anomaly) : body->orbit.ta,
				attractor
		);
	}
	return body;
}


CelestSystem * load_celestial_system_from_cfg_file(char *filename) {
	enum STORED_UNITS units = UNITS_LEGACY;
	
	CelestSystem *system = new_system();
	system->num_bodies = 0;
	system->prop_method = ORB_ELEMENTS;
	char central_body_name[50];
	
	FILE *file = fopen(filename, "r");
	if (!file) {
		perror("Failed to open file");
		free(system);
		return NULL;
	}
	
	char line[256];  // Buffer for each line
	while (fgets(line, sizeof(line), file)) {
		if(strncmp(line, "[", 1) == 0) {
			sscanf(line, "[%50[^]]]", system->name);
		} else if(strcmp(line, "\n") == 0 || strcmp(line, "\r\n") == 0){
			break;
		} else {
			char key[50], value[50];
			if(get_key_and_value_from_config(key, value, line)) {
				if (strcmp(key, "propagation_method") == 0) {
					if(strcmp(value, "EPHEMERIDES") == 0) system->prop_method = EPHEMS;
				} else if (strcmp(key, "ut0") == 0) {
					sscanf(value, "%lf", &system->ut0);
				} else if (strcmp(key, "number_of_bodies") == 0) {
					sscanf(value, "%d", &system->num_bodies);
				} else if (strcmp(key, "central_body") == 0) {
					sprintf(central_body_name, "%s", value);
				} else if (strcmp(key, "units") == 0) {
					if(strcmp(value, "M_DEG_PA") == 0) units = UNITS_M_DEG_PA;
				}
			}
		}
	}
	
	struct Body *cb = load_body_from_config_file(file, NULL, units);
	
	if(cb == NULL) {printf("Couldn't load Central Body!\n"); free(system); return NULL;}
	if(strcmp(central_body_name, cb->name) != 0) {printf("Central Body not in first position!\n"); free(system); free(cb); return NULL;}
	
	system->cb = cb;
	system->bodies = (struct Body**) calloc(system->num_bodies, sizeof(struct Body*));
	for(int i = 0; i < system->num_bodies; i++) system->bodies[i] = load_body_from_config_file(file, system, units);
	
	if(system->prop_method == EPHEMS) {
		for(int i = 0; i < system->num_bodies; i++) {
			get_body_ephems(system->bodies[i], (Datetime){1950,1,1}, (Datetime){2100,1,1}, (Datetime){0,1}, "../Ephemerides");
			// Needed for orbit visualization scale
			OSV osv = osv_from_ephem(system->bodies[i]->ephem, system->bodies[i]->num_ephems, system->ut0, system->bodies[i]->orbit.cb);
			system->bodies[i]->orbit = constr_orbit_from_osv(osv.r, osv.v, system->bodies[i]->orbit.cb);
		}
	}
	
	parse_and_sort_into_celestial_subsystems(system);
	
	fclose(file);
	
	return system;
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