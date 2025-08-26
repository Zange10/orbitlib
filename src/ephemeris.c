#include "orbitlib_ephemeris.h"
#include "orbitlib_fileio.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>


void print_ephem(struct Ephem ephem) {
	printf("Date: %f  (", ephem.epoch);
	print_date(convert_JD_date(ephem.epoch, DATE_ISO), 0);
	printf(")\nx: %g m,   y: %g m,   z: %g m\n"
		   "vx: %g m/s,   vy: %g m/s,   vz: %g m/s\n\n",
		   ephem.r.x, ephem.r.y, ephem.r.z, ephem.v.x, ephem.v.y, ephem.v.z);
}

void get_ephem_data_filepath(int id, char *filepath, const char *ephem_directory) {
	sprintf(filepath, "%s/%d.ephem", ephem_directory, id);
}

int is_ephem_available(int body_code, const char *ephem_directory) {
	char filepath[50];
	get_ephem_data_filepath(body_code, filepath, ephem_directory);
	FILE *file = fopen(filepath, "r");  // Try to open file in read mode
	if (file) {
		fclose(file);  // Close file if it was opened
		return 1;      // File exists
	}
	return 0;          // File does not exist
}

void get_body_ephems(Body *body, Datetime min_date, Datetime max_date, Datetime time_step, const char *ephem_directory) {
	if(body->orbit.cb == NULL) return;
	if(!directory_exists(ephem_directory)) create_directory(ephem_directory);
	
	char filepath[50];
	get_ephem_data_filepath(body->id, filepath, ephem_directory);
	
	if(!is_ephem_available(body->id, ephem_directory)) {
		char d0_s[32];
		char d1_s[32];
		date_to_string(min_date, d0_s, 1);
		date_to_string(max_date, d1_s, 1);
		// Construct the URL with your API key and parameters
		
		char timestep_s[8];
		if(time_step.y > 0) {
			sprintf(timestep_s, "%d y", time_step.y);
		} else if(time_step.m > 0) {
			sprintf(timestep_s, "%d mo", time_step.m);
		} else if(time_step.d > 0) {
			sprintf(timestep_s, "%d d", time_step.d);
		} else return;
		
		char url[256];
		sprintf(url, "https://ssd.jpl.nasa.gov/api/horizons.api?"
					 "format=text&"
					 "COMMAND='%d'&"
					 "OBJ_DATA='NO'&"
					 "MAKE_EPHEM='YES'&"
					 "EPHEM_TYPE='VECTORS'&"
					 "CENTER='500@%d'&"
					 "START_TIME='%s'&"
					 "STOP_TIME='%s'&"
					 "STEP_SIZE='%s'&"
					 "VEC_TABLE='2'", body->id, body->orbit.cb->id, d0_s, d1_s, timestep_s);
		
		download_file(url, filepath);
	}
	
	FILE *file;
	char line[256];  // Assuming lines are no longer than 255 characters
	
	file = fopen(filepath, "r");
	
	if(file == NULL) {
		perror("Unable to open file");
		return;
	}
	
	// Read lines from the file until the end is reached
	while(fgets(line, sizeof(line), file) != NULL) {
		line[strcspn(line, "\n")] = '\0';
		if(strcmp(line, "$$SOE") == 0) {
			break; // Exit the loop when "$$SOE" is encountered
		}
	}
	
	int max_num_ephems = 12;
	
	if(body->ephem != NULL) free(body->ephem);
	body->ephem = calloc(max_num_ephems, sizeof(Ephem));
	body->num_ephems = 0;
	
	fgets(line, sizeof(line), file);
	line[strcspn(line, "\n")] = '\0';
	
	while(strcmp(line, "$$EOE") != 0) {
		char *endptr;
		double date = strtod(line, &endptr);
		
		fgets(line, sizeof(line), file);
		double x, y, z;
		sscanf(line, " X =%lf Y =%lf Z =%lf", &x, &y, &z);
		fgets(line, sizeof(line), file);
		double vx, vy, vz;
		sscanf(line, " VX=%lf VY=%lf VZ=%lf", &vx, &vy, &vz);
		
		if(body->num_ephems == max_num_ephems) {
			max_num_ephems *= 2;
			Ephem *temp = realloc(body->ephem, max_num_ephems*sizeof(Ephem));
			if(temp != NULL) body->ephem = temp;
		}
		
		body->ephem[body->num_ephems].epoch = date;
		body->ephem[body->num_ephems].r.x = x*1e3;
		body->ephem[body->num_ephems].r.y = y*1e3;
		body->ephem[body->num_ephems].r.z = z*1e3;
		body->ephem[body->num_ephems].v.x = vx*1e3;
		body->ephem[body->num_ephems].v.y = vy*1e3;
		body->ephem[body->num_ephems].v.z = vz*1e3;
		body->num_ephems++;
		
		fgets(line, sizeof(line), file);
		line[strcspn(line, "\n")] = '\0';
	}
	fclose(file);
}

Ephem get_closest_ephem(Ephem *ephem, int num_ephems, double epoch) {
	for(int i = 0; i < num_ephems; i++) {
		if(epoch < ephem[i].epoch) {
			if(i == 0) return ephem[0];
			if(fabs(ephem[i-1].epoch - epoch) < fabs(ephem[i].epoch-epoch)) return ephem[i-1];
			else return ephem[i];
		}
	}
	return ephem[num_ephems-1];
}

OSV osv_from_ephem(Ephem *ephem_list, int num_ephems, double epoch, struct Body *cb) {
	Ephem ephem = get_closest_ephem(ephem_list, num_ephems, epoch);
	double dt = (epoch - ephem.epoch) * (24 * 60 * 60);
	return propagate_osv_time((OSV){ephem.r, ephem.v}, cb, dt);
}