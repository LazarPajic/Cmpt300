#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>

#define MAX_LINE 256
//volatile atomic_flag *FLAG = ATOMIC_FLAG_INIT;
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

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
	//define current files struct 
	pthread_mutex_lock(&mutex);
	struct metadata_struct *arg_struct2 = (struct metadata_struct*) arg;
	
	//define variables
	float fvalue = 0;
	printf("%s\n", arg_struct2->file_path);
	char ch;
	char ans[arg_struct2->buffer_size];
	memset(ans, '\0', sizeof(ans));
	int buff_counter = 0;
	int pos = 0;
	int index = 0;
	float new_sample_value = 0;
	//bool output_complete = false;
	
	//Declare file pointer and open file in reading mode
	FILE *fp1;
	fp1 = fopen(arg_struct2->file_path, "r"); 
	
	//Go through the current file character by character
	do {
		ch = fgetc(fp1);
		printf("ch is: %c\n", ch);
		
		//add a valid character to save as a value
		if(ch != '\n' && ch != '\r' && ch != EOF){
			if(buff_counter < arg_struct2->buffer_size){
				//strcat(ans, ch);
				ans[pos] = ch;
				printf("***\n");
				printf("ans is: %s\n", ans);
				printf("***\n");
				buff_counter++;
				pos++;
			}
		}
		//treat \n as a byte for buffer_size 
		else if(ch == '\n'){
			buff_counter++;
		}	
		
		//when length of value is equal to buffer_size or when current character equals End Of Final and there is valid value
		//do the alpha and beta calculations
		if(buff_counter == arg_struct2->buffer_size || (ch == EOF && pos > 0)){
			printf("counter is %d\n", buff_counter);
			printf("buffer is %d\n", arg_struct2->buffer_size);
			
			//change to float value
			fvalue = atof(ans);
			printf("value is: %f\n", fvalue);
			
			//reset values for next line
			buff_counter = 0;
			pos = 0;
			
			//save the first value to array in position zero without calculations
			if(index == 0){
				arg_struct2->alpha_calcs[0] = fvalue;
				printf("alpha: %f\n", arg_struct2->alpha_calcs[0]);
				
				arg_struct2->beta_calcs[0] = arg_struct2->beta * fvalue;
				printf("beta: %f is %f\n", arg_struct2->beta, arg_struct2->beta_calcs[0]);
			}
			//do calculations and save to array
			else{
				new_sample_value = arg_struct2->alpha * fvalue + (1 - arg_struct2->alpha) * arg_struct2->alpha_calcs[index - 1];	
				
				arg_struct2->alpha_calcs[index] = new_sample_value;
				printf("alpha: %f\n", arg_struct2->alpha_calcs[index]);
				
				arg_struct2->beta_calcs[index] = arg_struct2->beta * new_sample_value;
				printf("beta: %f for this %f\n", arg_struct2->beta, arg_struct2->beta_calcs[index]);
			}
			
			//fill ans with terminating characters to reuse
			for(int i = 0; i <= arg_struct2->buffer_size; i++){
				ans[i] = '\0';
			}
			
			//increase position in array
			index++;
		}
		
	} while(ch != EOF);
	
	//store the number of values from the file into the file's struct
	arg_struct2->array_size = index;
	printf("index is: %d\n", index);
	/*
	//Edward's comment....
	while(!output_complete){
		while(!atomic_flag_test_and_set(FLAG)){
			printf("Entering cs...\n");
			// output writing...
			atomic_flag_clear(FLAG);
			output_complete = true;
		}
		printf("Exiting cs...\n");
	}*/
	
	pthread_mutex_unlock(&mutex);
	pthread_exit(0);
}


int main(int argc, char **argv){	
	//define variables 
	char line[256] = "";
	int file_num = 0;
	float value = 0;
	
	int buffer_size = atoi(argv[1]);
	int num_threads = atoi(argv[2]);
	char *metadata_path = argv[3];
	
	char *output_file_path = argv[6];
	
	pthread_t thid[num_threads];
	
	printf("%s \n", metadata_path);
	
	//Declare file pointer and open file in reading mode
	FILE *fp;
	fp = fopen(metadata_path, "r");
	if(fp != NULL){
		//get the number of files
		fgets(line, 256, fp);
		file_num = atoi(line);		
	}
	else{
		printf("something went wrong\n");
		return 1;
	}
	
	//Make a struct for each file and assign input values 
	struct metadata_struct channel_files[file_num];	
	for(int i = 0; i < file_num; i++){
		//get the filepath and add the terminating character at the end
		fgets(line, 256, fp);
		int len = strcspn(line, "\n");
		line[len] = '\0';
		strcpy(channel_files[i].file_path, line);
		
		//save the alpha value
		fgets(line, 256, fp);
		value = atof(line);
		channel_files[i].alpha = value;
		
		//save the beta value
		fgets(line, 256, fp);
		value = atof(line);
		channel_files[i].beta = value;
		
		//save the buffer_size value
		channel_files[i].buffer_size = buffer_size;
	}
	
	//Close metadata file
	fclose(fp);
	
	//printing for testing purposes
	for (int i = 0; i < file_num; i++){
		printf("%s and %f and %f \n", channel_files[i].file_path, channel_files[i].alpha, channel_files[i].beta);
	}
	
	int p = file_num % num_threads;
	
	if(p != 0){
		printf("p value is not an integer!\n");
		return 0;
	}
	
	//Create threads and call file calculations
	for(int i = 0; i < file_num/num_threads; i++){
		for(int j = 0; j < num_threads; j++){
			pthread_attr_t attr2;
			pthread_attr_init(&attr2);
			pthread_create(&thid[j], &attr2, file_calculations, &channel_files[i*num_threads + j]);
		}
		//Wait for background threads to finish
		for (int i = 0; i < num_threads; i++){
			pthread_join(thid[i], NULL);
		}
	}
	
	//Calculate the results from the files and find the largest number of values
	float results[256] = {0.0f};
	printf("testingn 0000 sf results: %f \n", results[0]);
	int largest_array_size = 0;
	for(int k = 0; k < file_num; k++){
		for(int i = 0; i < channel_files[k].array_size; i++){			
			//results[i] = ceil(channel_files[k].beta_calcs[i]) + results[i];
			results[i] = channel_files[k].beta_calcs[i] + results[i];
			printf("testingn igngsn sf results: %f \n", results[i]);
		}
		
		if(largest_array_size < channel_files[k].array_size){
			largest_array_size = channel_files[k].array_size;
		}
	}
	
	//round each result up 
	int final_results[256];
	for(int i = 0; i < largest_array_size; i++){
		final_results[i] = (int)ceil(results[i]);
	}
	
	//Declare file pointer and open file in writing mode
	FILE *fp2;
	printf("%s \n", output_file_path);
	fp2 = fopen(output_file_path, "w"); 
	if(fp2 == NULL){
		printf("Failed to open output file \n");
		return 1;
	}
	
	//Write results into the output_file
	for (int i = 0; i < largest_array_size; i++){
		fprintf(fp2, "%d\n", final_results[i]);
		printf("results: %d \n", final_results[i]);
	}
	
	//Close output_file
	fclose(fp2);
	
	return 0;
}
