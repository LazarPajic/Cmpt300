#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>

#define MAX_LINE 256

struct metadata_struct {
	char file_path[MAX_LINE];
	float alpha;
	float beta;
	float alpha_calcs[256];
	float beta_calcs[256];
	int array_size;
};

void* file_calculations(void* arg){
	struct metadata_struct *arg_struct2 = (struct metadata_struct*) arg;
	
	FILE *fp1;
	//char line[256] = "";
	int value = 0;
	printf("%s\n", arg_struct2->file_path);
	fp1 = fopen(arg_struct2->file_path, "r"); 
	//fgets(line, 256, fp1);
	
	//store first value since no calculations need to be done
	fscanf(fp1, "%d", &value);
	arg_struct2->alpha_calcs[0] = value;
	printf("alpha: %f\n", arg_struct2->alpha_calcs[0]);
	arg_struct2->beta_calcs[0] = arg_struct2->beta * value;
	printf("beta: %f\n", arg_struct2->beta_calcs[0]);
	
	//do calculations for the rest of the values
	int index = 1;
	float new_sample_value = 0;
	while(fscanf(fp1, "%d", &value) == 1){	
		new_sample_value = arg_struct2->alpha * value + (1 - arg_struct2->alpha) * arg_struct2->alpha_calcs[index - 1];	
		
		arg_struct2->alpha_calcs[index] = new_sample_value;
		printf("alpha: %f\n", arg_struct2->alpha_calcs[index]);
		
		arg_struct2->beta_calcs[index] = arg_struct2->beta * new_sample_value;
		printf("beta: %f\n", arg_struct2->beta_calcs[index]);
		
		index++;		
	}
	arg_struct2->array_size = index;
	
	pthread_exit(0);
}

int main(int argc, char **argv){
	//int num_args = argc - 1;
	
	int num_threads = atoi(argv[2]);
	char *metadata_path = argv[3];
	
	pthread_t thid[num_threads];
	
	FILE *fp;
	char line[256] = "";
	int file_num = 0;
	float value = 0;
	printf("%s \n", metadata_path);
	fp = fopen(metadata_path, "r"); 
	
	
	if(fp != NULL){
		fgets(line, 256, fp);
		file_num = atoi(line);		
	}
	else{
		printf("something went wrong\n");
	}
	
	struct metadata_struct channel_files[file_num];	
	for(int i = 0; i < file_num; i++){
		fgets(line, 256, fp);
		int len = strcspn(line, "\n");
		line[len] = '\0';
		strcpy(channel_files[i].file_path, line);
		
		fgets(line, 256, fp);
		value = atof(line);
		channel_files[i].alpha = value;
		
		fgets(line, 256, fp);
		value = atof(line);
		channel_files[i].beta = value;
	}
		
	for (int i = 0; i < file_num; i++){
		printf("%s and %f and %f \n", channel_files[i].file_path, channel_files[i].alpha, channel_files[i].beta);
	}
	
	for (int i = 0; i < num_threads; i++){
		pthread_attr_t attr2;
		pthread_attr_init(&attr2);
		pthread_create(&thid[i], &attr2, file_calculations, &channel_files[i]);
	}
	for (int i = 0; i < num_threads; i++){
		pthread_join(thid[i], NULL);
	}
	
	int results[256];
	
	for(int k = 0; k < file_num; k++){
		for(int i = 0; i < channel_files[k].array_size; i++){
			results[i] = round(channel_files[k].beta_calcs[i]) + results[i];
		}
	}
	
	for (int i = 0; i < channel_files[0].array_size; i++){
		printf("results: %d \n", results[i]);
	}
}
