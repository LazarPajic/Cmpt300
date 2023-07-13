#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <semaphore.h>



#define MAX_LINE 256
// for global checkpoint 
sem_t SEMAPHORE;

// for lock_config 1 
pthread_mutex_t MUTEX = PTHREAD_MUTEX_INITIALIZER;

// for lock config 2
pthread_mutex_t ALPHA_LOCK = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t BETA_LOCK = PTHREAD_MUTEX_INITIALIZER;

// for lock config 3
volatile atomic_bool CASLOCK = false;
volatile atomic_bool ATOMIC_FALSE = false;

int GLOBAL_CHECKPOINT = 0;
int LOCK_CONFIG = 0;
int P = 0;
unsigned THREAD_NUMBER = 0;


// update on 9:30 pm 7/11/2023


typedef struct metadata_struct {
	char file_path[MAX_LINE];
	float alpha;
	float beta;
	float alpha_calcs[256];
	float beta_calcs[256];
	int array_size;
	int buffer_size;
	pthread_mutex_t* config2_locks;
}metadata_struct;


void* file_calculations1(void* arg){
	metadata_struct *data_file_head = (metadata_struct*) arg;
	bool single_thread_reading_complete = false;

	// arraies for storing values for each file to achieve local checkpointing
	char chS[P];
	int indexS[P];
	char ansS[P][data_file_head->buffer_size];
	float fvalueS[P];
	float new_sample_valueS[P];
	int posS[P];
	FILE *fileS[P];
	bool file_complete[P];


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
		posS[i] = 0;
		fileS[i] = fopen(arg_struct2->file_path, "r");
		file_complete[i] = false;
	}


	// outter most while loop for enforcing local checkpointing
	// the inner for loop will go through remaining file after reaching k bytes
	
	FILE *fp1;

	while(!single_thread_reading_complete){

		// going through files inside a thread
        // the for loop act as local checkpointing
		for(int a = 0; a < P; a++){

			// skipping files that's already done
			if(file_complete[a]){
				// printf("skipping file %d\n", a);
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
			int pos = posS[a];

			// single file reading loop, 
			// reading K bytes then stop
			// or reaches the end
			while(ch != EOF && buffer_counter < arg_struct2->buffer_size){
				
				ch = fgetc(fp1);
				// printf("ch is: %c\n", ch);
										
									
				
				//add a valid character to save as a value
				if(ch != '\n' && ch != '\r' && ch != EOF){
					if(buffer_counter < arg_struct2->buffer_size){
						//strcat(ans, ch);
						ans[pos] = ch;
						// printf("***\n");
						// printf("ans is: %s\n", ans);
						// printf("***\n");
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

					// printf("counter is %d\n", buffer_counter);
					//change to float value
					fvalue = atof(ans);
					// printf("value is: %f\n", fvalue);

					//reset values for next line
					pos = 0;
					

					// CRITICAL SECTION 1
					// global checkpoint
					// threads will have to wait before other threads complete
					if(GLOBAL_CHECKPOINT == 1){

						// since the atomic nature of the semaphore
						// no post() can be done before every thread complete wait()
						// and no wait() thread can actually go before all thread post() complete
						// the later prevents one thread going full cycle
						sem_wait(&SEMAPHORE);
						//sem_post(&SEMAPHORE);
					}
					pthread_mutex_lock(&MUTEX);
					// printf("Thread entering cs...\n");

					//save the first value to array in position zero without calculations
					if(index == 0){
						arg_struct2->alpha_calcs[0] = fvalue;
						// printf("alpha: %f\n", arg_struct2->alpha_calcs[0]);
						
						arg_struct2->beta_calcs[0] = arg_struct2->beta * fvalue;
						// printf("beta: %f is %f\n", arg_struct2->beta, arg_struct2->beta_calcs[0]);
					}
					//do calculations and save to array
					else{
						new_sample_value = arg_struct2->alpha * fvalue + (1 - arg_struct2->alpha) * arg_struct2->alpha_calcs[index - 1];	
						
						arg_struct2->alpha_calcs[index] = new_sample_value;
						// printf("alpha: %f\n", arg_struct2->alpha_calcs[index]);
						
						arg_struct2->beta_calcs[index] = arg_struct2->beta * new_sample_value;
						// printf("beta: %f for this %f\n", arg_struct2->beta, arg_struct2->beta_calcs[index]);
					}
					// printf("Thread exiting cs\n");
					pthread_mutex_unlock(&MUTEX);
					
					// global checkpoint
					// threads will have to wait before other threads complete
					if(GLOBAL_CHECKPOINT == 1){

						// since the atomic nature of the semaphore
						// no post() can be done before every thread complete wait()
						// and no wait() thread can actually go before all thread post() complete
						// the later prevents one thread going full cycle
						//sem_wait(&SEMAPHORE);
						sem_post(&SEMAPHORE);
					}
					
					// END OF CRITICAL SECTION

					//fill ans with terminating characters to reuse
					for(int i = 0; i <= arg_struct2->buffer_size; i++){
						ans[i] = '\0';
					}

					// increment index before exiting
					index++;

					// puasing reading if buffer is reached
					// break the reading while loop
					if(buffer_counter == arg_struct2->buffer_size){
						break;
					}

					//increase position in array
					// check complete condition, 
				}

				// file reaches EOF, the file is done
				// marking with true
				if(ch == EOF){
					file_complete[a] = true;
					// index++;
                	arg_struct2->array_size = index;
					// printf("recording array size: %d\n", index);
					// printf("file %d reaches end\n", a);
				}		
			}
			// end of single file reading loop

			// finish reading k bytes for one file
			// store current progress
			fvalueS[a] = fvalue;
			chS[a] = ch; 			
			new_sample_valueS[a] = new_sample_value;
			indexS[a] = index;
			fileS[a] = fp1;
			posS[a] = pos;
			// array deep copy
			for(int j = 0; j < arg_struct2->buffer_size; j++){
				ansS[a][j] = ans[j];
			}
			
			

		// end of single file reading for loop
		}

		// checking the amount of file done
		int file_EOF_count = 0;
		for(int k = 0; k < P; k++){
			if(file_complete[k]){
				file_EOF_count++;
			}
		}
		if(file_EOF_count == P){
			single_thread_reading_complete = true;
			break;
		}
		/*
		// global checkpoint
		// threads will have to wait before other threads complete
		if(GLOBAL_CHECKPOINT == 1){

			// since the atomic nature of the semaphore
			// no post() can be done before every thread complete wait()
			// and no wait() thread can actually go before all thread post() complete
			// the later prevents one thread going full cycle
			sem_wait(&SEMAPHORE);
			sem_post(&SEMAPHORE);
		}*/
    
	}



	// closing file pointer
	for(int i = 0; i < P; i++){
		fclose(fileS[i]);
	}


	pthread_exit(0);
}

void* file_calculations2(void* arg){
	metadata_struct *data_file_head = (metadata_struct*) arg;
	bool single_thread_reading_complete = false;

	// arraies for storing values for each file to achieve local checkpointing
	char chS[P];
	int indexS[P];
	char ansS[P][data_file_head->buffer_size];
	float fvalueS[P];
	float new_sample_valueS[P];
	int posS[P];
	FILE *fileS[P];
	bool file_complete[P];
	int line_counter = 0;

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
		posS[i] = 0;
		fileS[i] = fopen(arg_struct2->file_path, "r");
		file_complete[i] = false;
	}


	// outter most while loop for enforcing local checkpointing
	// the inner for loop will go through remaining file after reaching k bytes
	
	FILE *fp1;

	while(!single_thread_reading_complete){

		// going through files inside a thread
        // the for loop act as local checkpointing
		for(int a = 0; a < P; a++){

			// skipping files that's already done
			if(file_complete[a]){
				//printf("skipping file %d\n", a);
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
			int pos = posS[a];

			// single file reading loop, 
			// reading K bytes then stop
			// or reaches the end
			while(ch != EOF && buffer_counter < arg_struct2->buffer_size){
				
				ch = fgetc(fp1);
				// printf("ch is: %c\n", ch);
										
									
				
				//add a valid character to save as a value
				if(ch != '\n' && ch != '\r' && ch != EOF){
					if(buffer_counter < arg_struct2->buffer_size){
						//strcat(ans, ch);
						ans[pos] = ch;
						// printf("***\n");
						// printf("ans is: %s\n", ans);
						// printf("***\n");
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

					// printf("counter is %d\n", buffer_counter);
					//change to float value
					fvalue = atof(ans);
					// printf("value is: %f\n", fvalue);

					//reset values for next line
					pos = 0;
					

					// CRITICAL SECTION 2
					// global checkpoint
					// threads will have to wait before other threads complete
					if(GLOBAL_CHECKPOINT == 1){

						// since the atomic nature of the semaphore
						// no post() can be done before every thread complete wait()
						// and no wait() thread can actually go before all thread post() complete
						// the later prevents one thread going full cycle
						sem_wait(&SEMAPHORE);
						//sem_post(&SEMAPHORE);
					}
					pthread_mutex_lock(&arg_struct2->config2_locks[line_counter]);
					//save the first value to array in position zero without calculations
					if(index == 0){

						//if(!pthread_mutex_lock(&ALPHA_LOCK)){
							//printf("Entering alpha critical section.....\n");
							arg_struct2->alpha_calcs[0] = fvalue;
							// printf("alpha: %f\n", arg_struct2->alpha_calcs[0]);
							//printf("Exit alpha critical section\n");
							//pthread_mutex_unlock(&ALPHA_LOCK);
						//}
							
						//if(!pthread_mutex_lock(&BETA_LOCK)){
							//printf("Entering beta critical section...\n");
							arg_struct2->beta_calcs[0] = arg_struct2->beta * fvalue;
							// printf("beta: %f is %f\n", arg_struct2->beta, arg_struct2->beta_calcs[0]);
							//printf("Exit beta critical section\n");
							//pthread_mutex_unlock(&BETA_LOCK);
						//}
					}
					//do calculations and save to array
					else{
					new_sample_value = arg_struct2->alpha * fvalue + (1 - arg_struct2->alpha) * arg_struct2->alpha_calcs[index - 1];	
						
						//if(!pthread_mutex_lock(&ALPHA_LOCK)){
							//printf("Entering alpha critical section.....\n");
							arg_struct2->alpha_calcs[index] = new_sample_value;
							// printf("alpha: %f\n", arg_struct2->alpha_calcs[index]);
							//printf("Exit alpha critical section\n");
							//pthread_mutex_unlock(&ALPHA_LOCK);
						//}
						
						//if(!pthread_mutex_lock(&BETA_LOCK)){
							//printf("Entering beta critical section...\n");
							arg_struct2->beta_calcs[index] = arg_struct2->beta * new_sample_value;
							// printf("beta: %f for this %f\n", arg_struct2->beta, arg_struct2->beta_calcs[index]);
							//printf("Exit beta critical section\n");
							//pthread_mutex_unlock(&BETA_LOCK);
						//}
					}
					pthread_mutex_unlock(&arg_struct2->config2_locks[line_counter]);
					// global checkpoint
					// threads will have to wait before other threads complete
					if(GLOBAL_CHECKPOINT == 1){

						// since the atomic nature of the semaphore
						// no post() can be done before every thread complete wait()
						// and no wait() thread can actually go before all thread post() complete
						// the later prevents one thread going full cycle
						//sem_wait(&SEMAPHORE);
						sem_post(&SEMAPHORE);
					}
					// END OF CRITICAL SECTION 
					

					//fill ans with terminating characters to reuse
					for(int i = 0; i <= arg_struct2->buffer_size; i++){
						ans[i] = '\0';
					}

					// increment index before exiting
					index++;

					// puasing reading if buffer is reached
					// break the reading while loop
					if(buffer_counter == arg_struct2->buffer_size){
						break;
					}

					//increase position in array
					// check complete condition, 
				}

				// file reaches EOF, the file is done
				// marking with true
				if(ch == EOF){
					file_complete[a] = true;
					// index++;
                	arg_struct2->array_size = index;
					// printf("recording array size: %d\n", index);
					// printf("file %d reaches end\n", a);
				}		
			}
			// end of single file reading loop

			// finish reading k bytes for one file
			// store current progress
			fvalueS[a] = fvalue;
			chS[a] = ch; 			
			new_sample_valueS[a] = new_sample_value;
			indexS[a] = index;
			fileS[a] = fp1;
			posS[a] = pos;
			// array deep copy
			for(int j = 0; j < arg_struct2->buffer_size; j++){
				ansS[a][j] = ans[j];
			}
			line_counter++;
			

		// end of single file reading for loop
		}

		// checking the amount of file done
		int file_EOF_count = 0;
		for(int k = 0; k < P; k++){
			if(file_complete[k]){
				file_EOF_count++;
			}
		}
		if(file_EOF_count == P){
			single_thread_reading_complete = true;
			break;
		}
		/*
		// global checkpoint
		// threads will have to wait before other threads complete
		if(GLOBAL_CHECKPOINT == 1){

			// since the atomic nature of the semaphore
			// no post() can be done before every thread complete wait()
			// and no wait() thread can actually go before all thread post() complete
			// the later prevents one thread going full cycle
			sem_wait(&SEMAPHORE);
			sem_post(&SEMAPHORE);
		}*/
    
	}



	// closing file pointer
	for(int i = 0; i < P; i++){
		fclose(fileS[i]);
	}


	pthread_exit(0);
}


void* file_calculations3(void* arg){
	metadata_struct *data_file_head = (metadata_struct*) arg;
	bool single_thread_reading_complete = false;

	// arraies for storing values for each file to achieve local checkpointing
	char chS[P];
	int indexS[P];
	char ansS[P][data_file_head->buffer_size];
	float fvalueS[P];
	float new_sample_valueS[P];
	int posS[P];
	FILE *fileS[P];
	bool file_complete[P];


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
		posS[i] = 0;
		fileS[i] = fopen(arg_struct2->file_path, "r");
		file_complete[i] = false;
	}


	// outter most while loop for enforcing local checkpointing
	// the inner for loop will go through remaining file after reaching k bytes
	
	FILE *fp1;

	while(!single_thread_reading_complete){

		// going through files inside a thread
        // the for loop act as local checkpointing
		for(int a = 0; a < P; a++){

			// skipping files that's already done
			if(file_complete[a]){
				// printf("skipping file %d\n", a);
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
			int pos = posS[a];

			// single file reading loop, 
			// reading K bytes then stop
			// or reaches the end
			while(ch != EOF && buffer_counter < arg_struct2->buffer_size){
				
				ch = fgetc(fp1);
				// printf("ch is: %c\n", ch);
										
									
				
				//add a valid character to save as a value
				if(ch != '\n' && ch != '\r' && ch != EOF){
					if(buffer_counter < arg_struct2->buffer_size){
						//strcat(ans, ch);
						ans[pos] = ch;
						// printf("***\n");
						// printf("ans is: %s\n", ans);
						// printf("***\n");
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

					// printf("counter is %d\n", buffer_counter);
					//change to float value
					fvalue = atof(ans);
					// printf("value is: %f\n", fvalue);

					//reset values for next line
					pos = 0;
					

					// CRITICAL SECTION 3
					while(atomic_compare_exchange_strong(&CASLOCK, &ATOMIC_FALSE, true) != 0){
						// busy waiting
					}
					//printf("Thread entering cs...\n");
					//save the first value to array in position zero without calculations
					
					// global checkpoint
					// threads will have to wait before other threads complete
					if(GLOBAL_CHECKPOINT == 1){

						// since the atomic nature of the semaphore
						// no post() can be done before every thread complete wait()
						// and no wait() thread can actually go before all thread post() complete
						// the later prevents one thread going full cycle
						sem_wait(&SEMAPHORE);
						//sem_post(&SEMAPHORE);
					}
					if(index == 0){
						arg_struct2->alpha_calcs[0] = fvalue;
						// printf("alpha: %f\n", arg_struct2->alpha_calcs[0]);

						arg_struct2->beta_calcs[0] = arg_struct2->beta * fvalue;
						// printf("beta: %f is %f\n", arg_struct2->beta, arg_struct2->beta_calcs[0]);
					}
					//do calculations and save to array
					else{
						new_sample_value = arg_struct2->alpha * fvalue + (1 - arg_struct2->alpha) * arg_struct2->alpha_calcs[index - 1];	

						arg_struct2->alpha_calcs[index] = new_sample_value;
						// printf("alpha: %f\n", arg_struct2->alpha_calcs[index]);

						arg_struct2->beta_calcs[index] = arg_struct2->beta * new_sample_value;
						// printf("beta: %f for this %f\n", arg_struct2->beta, arg_struct2->beta_calcs[index]);
					}
					// global checkpoint
					// threads will have to wait before other threads complete
					if(GLOBAL_CHECKPOINT == 1){

						// since the atomic nature of the semaphore
						// no post() can be done before every thread complete wait()
						// and no wait() thread can actually go before all thread post() complete
						// the later prevents one thread going full cycle
						//sem_wait(&SEMAPHORE);
						sem_post(&SEMAPHORE);
					}
					//printf("Thread exiting cs\n");
					// END OF CRITICAL SECTION
					atomic_exchange(&CASLOCK, false);
					

					//fill ans with terminating characters to reuse
					for(int i = 0; i <= arg_struct2->buffer_size; i++){
						ans[i] = '\0';
					}

					// increment index before exiting
					index++;

					// puasing reading if buffer is reached
					// break the reading while loop
					if(buffer_counter == arg_struct2->buffer_size){
						break;
					}

					//increase position in array
					// check complete condition, 
				}

				// file reaches EOF, the file is done
				// marking with true
				if(ch == EOF){
					file_complete[a] = true;
                	arg_struct2->array_size = index;
				}		
			}
			// end of single file reading loop

			// finish reading k bytes for one file
			// store current progress
			fvalueS[a] = fvalue;
			chS[a] = ch; 			
			new_sample_valueS[a] = new_sample_value;
			indexS[a] = index;
			fileS[a] = fp1;
			posS[a] = pos;
			// array deep copy
			for(int j = 0; j < arg_struct2->buffer_size; j++){
				ansS[a][j] = ans[j];
			}
			
			

		// end of single file reading for loop
		}

		// checking the amount of file done
		int file_EOF_count = 0;
		for(int k = 0; k < P; k++){
			if(file_complete[k]){
				file_EOF_count++;
			}
		}
		if(file_EOF_count == P){
			single_thread_reading_complete = true;
			break;
		}
		/*
		// global checkpoint
		// threads will have to wait before other threads complete
		if(GLOBAL_CHECKPOINT == 1){

			// since the atomic nature of the semaphore
			// no post() can be done before every thread complete wait()
			// and no wait() thread can actually go before all thread post() complete
			// the later prevents one thread going full cycle
			sem_wait(&SEMAPHORE);
			sem_post(&SEMAPHORE);
		}*/
    
	}



	// closing file pointer
	for(int i = 0; i < P; i++){
		fclose(fileS[i]);
	}


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
	
	sem_init(&SEMAPHORE, 0, THREAD_NUMBER);
	pthread_t thid[THREAD_NUMBER];
	pthread_mutex_t Lock2Entries[256];
	
	for(int i = 0; i < 256; i++){
		pthread_mutex_init(&Lock2Entries[i], NULL);
	}

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
		
		channel_files[i].config2_locks = Lock2Entries;
	}
	
	//Close metadata file
	fclose(fp);
	
	// invalid input checking
	if(file_num % THREAD_NUMBER != 0){
		printf("Argument error: P value is not an integer\n");
		return 0;
	}
	if(THREAD_NUMBER < 1 || buffer_size < 1){
		printf("Argument error: invalid thread or buffer size number\n");
		return 0;
	}

	// P is the number of file each thread has to process
	P = file_num / THREAD_NUMBER;

	// one single global lock
	if(LOCK_CONFIG == 1){

		// alternative implementation
		for(int i = 0; i < THREAD_NUMBER; i++){
			// thread 1 grabs file 1 and go through p files...
			// thread 2 grabs file 2 and go through p files...
			pthread_create(&thid[i], NULL, file_calculations1, &channel_files[i]);
		}

	// different lock for each channel?
	}else if(LOCK_CONFIG == 2){

		// alternative implementation
		for(int i = 0; i < THREAD_NUMBER; i++){
			// thread 1 grabs file 1 and go through p files...
			// thread 2 grabs file 2 and go through p files...
			pthread_create(&thid[i], NULL, file_calculations2, &channel_files[i]);
		}

	// compare and swap
	}else{

		// alternative implementation
		for(int i = 0; i < THREAD_NUMBER; i++){
			// thread 1 grabs file 1 and go through p files...
			// thread 2 grabs file 2 and go through p files...
			pthread_create(&thid[i], NULL, file_calculations3, &channel_files[i]);
		}
	}

	// if called in the same loop as pthread_created, it causes infinite loop
	for(int i = 0; i < THREAD_NUMBER; i++){
		pthread_join(thid[i], NULL);
	}

	
	//Calculate the results from the files and find the largest number of values
	float results[256] = {0.0f};
	// printf("testingn 0000 sf results: %f \n", results[0]);
	int largest_array_size = 0;
	// printf("file num: %d\n", file_num);
	for(int k = 0; k < file_num; k++){
		for(int i = 0; i < channel_files[k].array_size; i++){			
			//results[i] = ceil(channel_files[k].beta_calcs[i]) + results[i];
			results[i] = channel_files[k].beta_calcs[i] + results[i];
			// printf("testingn igngsn sf results: %f \n", results[i]);
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
	// printf("%s \n", output_file_path);
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
