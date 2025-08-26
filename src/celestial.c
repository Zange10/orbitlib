#include "orbitlib_celestial.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "orbitlib_fileio.h"


struct Body * new_body() {
	struct Body *new_body = (struct Body*) malloc(sizeof(struct Body));
	sprintf(new_body->name, "BODY");
	new_body->color[0] = 0.5;
	new_body->color[1] = 0.5;
	new_body->color[2] = 0.5;
	new_body->id = 0;
	new_body->mu = 0;
	new_body->radius = 0;
	new_body->rotation_period = 86400;
	new_body->sl_atmo_p = 0;
	new_body->scale_height = 1000;
	new_body->atmo_alt = 0;
	new_body->system = NULL;
	new_body->ephem = NULL;
	new_body->num_ephems = 0;
	
	new_body->orbit.a = 150e9;
	new_body->orbit.e = 0;
	new_body->orbit.i = 0;
	new_body->orbit.raan = 0;
	new_body->orbit.arg_peri = 0;
	new_body->orbit.ta = 0;
	new_body->orbit.cb = NULL;
	
	return new_body;
}

void set_body_color(struct Body *body, double red, double green, double blue) {
	body->color[0] = red;
	body->color[1] = green;
	body->color[2] = blue;
}

CelestSystem * new_system() {
	CelestSystem *system = malloc(sizeof(CelestSystem));
	sprintf(system->name,"CELESTIAL SYSTEM");
	system->num_bodies = 0;
	system->cb = NULL;
	system->bodies = NULL;
	system->prop_method = EPHEMS;
	system->ut0 = 0;
	return system;
}

int get_number_of_subsystems(CelestSystem *system) {
	int num_subsystems = 0;
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i]->system != NULL) {
			num_subsystems++;
			num_subsystems += get_number_of_subsystems(system->bodies[i]->system);
		}
	}
	return num_subsystems;
}

CelestSystem * get_top_level_system(CelestSystem *system) {
	if(system == NULL) return NULL;
	while(system->cb->orbit.cb != NULL) system = system->cb->orbit.cb->system;
	return system;
}

struct Body * get_body_by_name(char *name, CelestSystem *system) {
	if(strcmp(system->cb->name, name) == 0) return system->cb;
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i] == NULL) return NULL;
		if(strcmp(system->bodies[i]->name, name) == 0) return system->bodies[i];
		if(system->bodies[i]->system != NULL) {
			struct Body *body = get_body_by_name(name, system->bodies[i]->system);
			if(body != NULL) return body;
		}
	}
	return NULL;
}

void free_system(CelestSystem *system) {
	if(system == NULL) return;
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i]->ephem != NULL) free(system->bodies[i]->ephem);
		free(system->bodies[i]);
	}
	free(system->cb);
	free(system);
}

int get_body_system_id(struct Body *body, CelestSystem *system) {
	for(int i = 0; i < system->num_bodies; i++) {
		if(system->bodies[i] == body) return i;
	}
	return -1;
}

void print_celestial_system_layer(CelestSystem *system, int layer) {
	for(int i = 0; i < system->num_bodies; i++) {
		for(int j = 0; j < layer-1; j++) printf("│  ");
		if(layer != 0 && i < system->num_bodies-1) printf("├─ ");
		else if(layer != 0) printf("└─ ");
		printf("%s\n", system->bodies[i]->name);
		if(system->bodies[i]->system != NULL) print_celestial_system_layer(system->bodies[i]->system, layer+1);
	}
}

CelestSystem ** init_available_systems_from_path(const char *directory, int *num_systems) {
	if(!directory_exists(directory)) {
		create_directory(directory);
		return 0;
	}
	
	CelestSystem **p_systems = (CelestSystem **) malloc(4 * sizeof(struct System*));	// A maximum of 10 systems seems reasonable
	
	*num_systems = 0;
	char **paths = list_files_with_extension(directory, ".cfg", num_systems);
	
	char path[50];
	for(int i = 0; i < *num_systems; i++) {
		sprintf(path, "%s/%s", directory, paths[i]);
		p_systems[i] = load_celestial_system_from_cfg_file(path);
	}
	
	free(paths);
	return p_systems;
}

void print_celestial_system(CelestSystem *system) {
	printf("%s\n", system->cb->name);
	print_celestial_system_layer(system, 1);
}