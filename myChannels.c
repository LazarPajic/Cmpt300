#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>

#define MAX_LINE 256
volatile atomic_flag *FLAG = ATOMIC_FLAG_INIT;

struct metadata_struct {
	char file_path[MAX_LINE];
	float alpha;
	float beta;
	float alpha_calcs[256];
	float beta_calcs[256];
	int array_size;
	int buffer_size;
};

void* file_calculations(void* arg){
	struct metadata_struct *arg_struct2 = (struct metadata_struct*) arg;
	
	FILE *fp1;
	//char line[256] = "";
	float value = 0;
	printf("%s\n", arg_struct2->file_path);
	fp1 = fopen(arg_struct2->file_path, "r"); 
	//fgets(line, 256, fp1);
	char ch;
	char ans[arg_struct2->buffer_size];
	int counter = 0;
	int pos = 0;
	int index = 0;
	float new_sample_value = 0;
	bool output_complete = false;
	
	do {
		ch = fgetc(fp1);
		printf("ch is: %c\n", ch);
		if(ch != '\n' && ch != '\r' && ch != EOF){
			if(counter < arg_struct2->buffer_size){
				//strcat(ans, ch);
				ans[pos] = ch;
				printf("***\n");
				printf("ans is: %s\n", ans);
				printf("***\n");
				counter++;
				pos++;
			}
		}
		else if(ch == '\n'){
			counter++;
		}	
		
		if(counter == arg_struct2->buffer_size || (ch == EOF && pos > 0)){
			printf("counter is %d\n", counter);
			printf("buffer is %d\n", arg_struct2->buffer_size);
			counter = 0;
			pos = 0;
			value = atof(ans);
			printf("value is: %f\n", value);
			if(index == 0){
				arg_struct2->alpha_calcs[0] = value;
				printf("alpha: %f\n", arg_struct2->alpha_calcs[0]);
				
				arg_struct2->beta_calcs[0] = arg_struct2->beta * value;
				printf("beta: %f is %f\n", arg_struct2->beta, arg_struct2->beta_calcs[0]);
			}
			else{
				new_sample_value = arg_struct2->alpha * value + (1 - arg_struct2->alpha) * arg_struct2->alpha_calcs[index - 1];	
				
				arg_struct2->alpha_calcs[index] = new_sample_value;
				printf("alpha: %f\n", arg_struct2->alpha_calcs[index]);
				
				arg_struct2->beta_calcs[index] = arg_struct2->beta * new_sample_value;
				printf("beta: %f for this %f\n", arg_struct2->beta, arg_struct2->beta_calcs[index]);
			}
			for(int i = 0; i <= arg_struct2->buffer_size; i++){
				ans[i] = '\0';
			}
			index++;
		}
		
	} while(ch != EOF);
	
	arg_struct2->array_size = index;
	printf("index is: %d\n", index);
	

	while(!output_complete){
		while(!atomic_flag_test_and_set(FLAG)){
			printf("Entering cs...\n");
			// output writing...
			atomic_flag_clear(FLAG);
			output_complete = true;
		}
		printf("Exiting cs...\n");
	}

	pthread_exit(0);
}


int main(int argc, char **argv){
	
	
	int buffer_size = atoi(argv[1]);
	int num_threads = atoi(argv[2]);
	char *metadata_path = argv[3];
	
	char *output_file_path = argv[6];
	
	pthread_t thid[num_threads];
	
	FILE *fp;
	char line[256] = "";
	int file_num = 0;
	float value = 0;
	printf("%s \n", metadata_path);
	fp = fopen(metadata_path, "r"); 
	
	
	// opening metadata file
	if(fp != NULL){
		fgets(line, 256, fp);
		file_num = atoi(line);		
	}
	else{
		printf("something went wrong\n");
		return 1;
	}

	// opening output file
	FILE *fp2;
	printf("%s \n", output_file_path);
	fp2 = fopen(output_file_path, "w"); 
	if(fp2 == NULL){
		printf("Failed to open output file \n");
		return 1;
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
		
		channel_files[i].buffer_size = buffer_size;
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
	int largest_array_size = 0;
	for(int k = 0; k < file_num; k++){
		for(int i = 0; i < channel_files[k].array_size; i++){
			results[i] = ceil(channel_files[k].beta_calcs[i]) + results[i];
		}
		
		if(largest_array_size < channel_files[k].array_size){
			largest_array_size = channel_files[k].array_size;
		}
	}

	
	for (int i = 0; i < largest_array_size; i++){
		fprintf(fp2, "%d\n", results[i]);
		//fwrite(results[i], sizeof(int), 1, fp2);
		printf("results: %d \n", results[i]);
	}
	
	fclose(fp2);
	
	return 0;
}
