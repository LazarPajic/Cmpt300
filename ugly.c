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
int GLOBAL_CHECKPOINT = 0;
int LOCK_CONFIG = 0;
int P = 0;
int THREAD_NUMBER = 0;


typedef struct metadata_struct {
	char file_path[MAX_LINE];
	float alpha;
	float beta;
	float alpha_calcs[256];
	float beta_calcs[256];
	int array_size;
	int buffer_size;
}metadata_struct;


void* file_calculations(void* arg){
	metadata_struct *data_file_head = (metadata_struct*) arg;
	bool single_thread_reading_complete =false;

	// arraies for storing values for each file to achieve local checkpointing
	char chS[P];
	int indexS[P];
	char ansS[P][data_file_head->buffer_size];
	float fvalueS[P];
	float new_sample_valueS[P];
	FILE *fileS[P];
	bool kBytescomplete[P];

	// initialization offf arraies
	for(int i = 0; i < P; i++){
		metadata_struct *arg_struct2 = &data_file_head[THREAD_NUMBER * i];
		char ans[arg_struct2->buffer_size];
		// array deep copy
		for(int j = 0; j < arg_struct2->buffer_size; j++){
			ans[j] = ansS[i][j];
		}
		memset(ans, '\0', sizeof(ans));
		indexS[i] = 0;
		fvalueS[i] = 0.0;
		new_sample_valueS[i] = 0.0;
		fileS[i] = fopen(arg_struct2->file_path, "r");
		complete[i] = false;
	}


	// outter most while loop for enforcing local checkpointing
	// the inner for loop will go through remaining file after reaching k bytes
	
	while(!single_thread_reading_complete){

		FILE *fp1;

		// going through files inside a thread
        // the for loop act as local checkpointing
		for(int a = 0; a < P; a++){
			// skipping files that's already done

			if(complete[a] == true){
				continue;
			}

			metadata_struct *arg_struct2 = &data_file_head[THREAD_NUMBER * a];
			// restore previous value
			fp1 = fileS[a];
			double fvalue = fvalueS[a];
			char ch = chS[a]; 			// might need initialization for array
			float new_sample_value = new_sample_valueS[a];
			int index = indexS[a];
			char ans[arg_struct2->buffer_size];
			// array deep copy
			for(int j = 0; j < arg_struct2->buffer_size; j++){
				ans[j] = ansS[a][j];
			}
			int buffer_counter = 0;
			int pos = 0;

			// single file reading loop, reading K bytes then stop
			// or reaches the end
			while(ch != EOF && buffer_counter != arg_struct2->buffer_size){
				
				ch = fgetc(fp1);
				// converts from char to int??
				printf("ch is: %c\n", ch);
				
				//add a valid character to save as a value
				if(ch != '\n' && ch != '\r' && ch != EOF){
					if(buffer_counter < arg_struct2->buffer_size){
						//strcat(ans, ch);
						ans[pos] = ch;
						printf("***\n");
						printf("ans is: %s\n", ans);
						printf("***\n");
						buffer_counter++;
						pos++;
					}
				}
				//treat \n as a byte for buffer_size 
				else if(ch == '\n'){
					buffer_counter++;
				}	
				
				//when length of value is equal to buffer_size or when current character equals End Of Final and there is valid value
				//do the alpha and beta calculations
				if(buffer_counter == arg_struct2->buffer_size || (ch == EOF && pos > 0)){
					
					printf("counter is %d\n", buffer_counter);
					//change to float value
					fvalue = atof(ans);
					printf("value is: %f\n", fvalue);

					if(GLOBAL_CHECKPOINT == 1){
						// wait for other threads
					}
					
					// puasing reading if buffer is reached
					if(buffer_counter == arg_struct2->buffer_size){
						
					}
					
					//reset values for next line
					buffer_counter = 0;
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
						// check complete condition, 
				index++;
				}
			}
			// end of reading loop

			// finish reading k bytes
			// store current progress
			fvalueS[a] = fvalue;
			chS[a] = ch; 			// might need initialization for array
			new_sample_valueS[a] = new_sample_value;
			indexS[a] = index;
			fileS[a] = fp1;
			// array deep copy
			for(int j = 0; j < arg_struct2->buffer_size; j++){
				ansS[a][j] = ans[j];
			}
			
		// end of file for loop
		}

		printf("one thread done\n");

		// global checkpoint
        if(GLOBAL_CHECKPOINT == 1){
            // lock implementation
        }
	}


	// closing file pointer
	for(int i = 0; i < P; i++){
		fclose(fileS[i]);
	}



	// a for loop to go through p files
	// if it's thread i(0), then it will go through 0, 0 + thread * i until reaches P
	// for(int i = 0; i < P; i++){
	// 	metadata_struct *arg_struct2 = &data_file_head[THREAD_NUMBER * i];

	// 	//define variables
	// 	float fvalue = 0;
	// 	printf("%s\n", arg_struct2->file_path);
	// 	char ch;
	// 	char ans[arg_struct2->buffer_size];
	// 	memset(ans, '\0', sizeof(ans));
	// 	int buff_counter = 0;
	// 	int pos = 0;
	// 	int index = 0;
	// 	float new_sample_value = 0;
	// 	//bool output_complete = false;
		
	// 	//Declare file pointer and open file in reading mode
	// 	FILE *fp1;
	// 	fp1 = fopen(arg_struct2->file_path, "r"); 
		
	// 	//Go through the current file character by character
	// 	do {
	// 		ch = fgetc(fp1);
	// 		printf("ch is: %c\n", ch);
			
	// 		//add a valid character to save as a value
	// 		if(ch != '\n' && ch != '\r' && ch != EOF){
	// 			if(buff_counter < arg_struct2->buffer_size){
	// 				//strcat(ans, ch);
	// 				ans[pos] = ch;
	// 				printf("***\n");
	// 				printf("ans is: %s\n", ans);
	// 				printf("***\n");
	// 				buff_counter++;
	// 				pos++;
	// 			}
	// 		}
	// 		//treat \n as a byte for buffer_size 
	// 		else if(ch == '\n'){
	// 			buff_counter++;
	// 		}	
			
	// 		//when length of value is equal to buffer_size or when current character equals End Of Final and there is valid value
	// 		//do the alpha and beta calculations
	// 		if(buff_counter == arg_struct2->buffer_size || (ch == EOF && pos > 0)){
	// 			printf("counter is %d\n", buff_counter);
	// 			printf("buffer is %d\n", arg_struct2->buffer_size);
				
	// 			//change to float value
	// 			fvalue = atof(ans);
	// 			printf("value is: %f\n", fvalue);

	// 			if(GLOBAL_CHECKPOINT == 1){
	// 				// wait for other threads
	// 			}
				
	// 			//reset values for next line
	// 			buff_counter = 0;
	// 			pos = 0;
				
	// 			//save the first value to array in position zero without calculations
	// 			if(index == 0){
	// 				arg_struct2->alpha_calcs[0] = fvalue;
	// 				printf("alpha: %f\n", arg_struct2->alpha_calcs[0]);
					
	// 				arg_struct2->beta_calcs[0] = arg_struct2->beta * fvalue;
	// 				printf("beta: %f is %f\n", arg_struct2->beta, arg_struct2->beta_calcs[0]);
	// 			}
	// 			//do calculations and save to array
	// 			else{
	// 				new_sample_value = arg_struct2->alpha * fvalue + (1 - arg_struct2->alpha) * arg_struct2->alpha_calcs[index - 1];	
					
	// 				arg_struct2->alpha_calcs[index] = new_sample_value;
	// 				printf("alpha: %f\n", arg_struct2->alpha_calcs[index]);
					
	// 				arg_struct2->beta_calcs[index] = arg_struct2->beta * new_sample_value;
	// 				printf("beta: %f for this %f\n", arg_struct2->beta, arg_struct2->beta_calcs[index]);
	// 			}
				
	// 			//fill ans with terminating characters to reuse
	// 			for(int i = 0; i <= arg_struct2->buffer_size; i++){
	// 				ans[i] = '\0';
	// 			}
				
	// 			//increase position in array
	// 			index++;
	// 		}
			
	// 	} while(ch != EOF);

	// 	// store the number of values from the file into the file's struct
	// 	// arg_struct2->array_size = index;
	// 	// printf("index is: %d\n", index);
	// }

	pthread_exit(0);
}


int main(int argc, char **argv){	
	//define variables 
	char line[256] = "";
	int file_num = 0;
	float value = 0;
	
	int buffer_size = atoi(argv[1]);
	THREAD_NUMBER = atoi(argv[2]);
	char *metadata_path = argv[3];
	LOCK_CONFIG = atoi(argv[4]);
	GLOBAL_CHECKPOINT = atoi(argv[5]);
	char *output_file_path = argv[6];
	
	pthread_t thid[THREAD_NUMBER];
	
	printf("%s \n", metadata_path);
	
	//Declare file pointer and open file in reading mode
	FILE *fp;
	fp = fopen(metadata_path, "r");
	if(fp != NULL){
		//get the number of files
		fgets(line, 256, fp);
		file_num = atoi(line);		
	}else{
		printf("Meta data file not found\n");
		return 1;
	}
	
	//Make a struct for each file and assign input values 
	metadata_struct channel_files[file_num];	
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
	// for (int i = 0; i < file_num; i++){
	// 	printf("%s and %f and %f \n", channel_files[i].file_path, channel_files[i].alpha, channel_files[i].beta);
	// }
	
	int p = file_num % THREAD_NUMBER;
	if(p != 0){
		printf("Pvalue is not an integer!\n");
		return 0;
	}

	// P is the number of file each thread has to process
	P = file_num / THREAD_NUMBER;

	if(LOCK_CONFIG == 1){


	}else if (LOCK_CONFIG == 2){


	}else{

	}
	

	// original implementation
	//Create threads and call file calculations
	// for(int i = 0; i < file_num/THREAD_NUMBER; i++){
		// for(int j = 0; j < THREAD_NUMBER; j++){
			// pthread_attr_t attr2;
			// pthread_attr_init(&attr2);
			// pthread_create(&thid[j], NULL, file_calculations, &channel_files[i*THREAD_NUMBER + j]);
			// pthread_join(thid[i], NULL);
		// }
	// }


	// alternative implementation
	for(int i = 0; i < THREAD_NUMBER; i++){
		// thread 1 grabs file 1 and go through p files...
		// thread 2 grabs file 2 and go through p files...
		pthread_create(&thid[i], NULL, file_calculations, &channel_files[i]);
		pthread_join(thid[i],NULL);
	}

	
	//Calculate the results from the files and find the largest number of values
	float results[256] = {0.0f};
	// printf("testingn 0000 sf results: %f \n", results[0]);
	int largest_array_size = 0;
	printf("file num: %d\n", file_num);
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