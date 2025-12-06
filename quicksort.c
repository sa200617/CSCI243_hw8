#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>
#include <errno.h>


typedef struct{
	size_t size;
	int *data;
}qs_args_t;

int thread_count = 0;
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

void partition_array(int pivot, const int *data, size_t size,
			int **less, size_t *lessS, int **equal,
			size_t *equalS, int **greater, size_t *greaterS){

	*lessS = 0;
	*equalS = 0;
	*greaterS = 0;

	for (size_t num = 0; num < size; num++){
		if (data[num] < pivot){(*lessS)++;}
		else if (data[num] > pivot){(*greaterS)++;}
		else{(*equalS)++;}
	}
	*less = malloc(*lessS * sizeof(int));
	*equal = malloc(*equalS *sizeof(int));
	*greater = malloc(*greaterS *sizeof(int));


	size_t lessL = 0;
	size_t equalL =  0;
	size_t greaterL = 0;

	for (size_t num = 0; num < size; num++){
		if(data[num] < pivot){
			(*less)[lessL++] = data[num];
		}
		else if (data[num] > pivot){
			(*greater)[greaterL++] = data[num];
		}
		else{
			(*equal)[equalL++] = data[num];
		}
	}

}


int *merge(int *less, size_t lessS, int *equal, size_t equalS,
		int *greater, size_t greaterS){

		size_t sizeL = lessS + equalS + greaterS;
		int *list = malloc(sizeL * sizeof(int));

		size_t pos = 0;

		for (size_t num = 0; num < lessS; num++){
			list[pos++] = less[num];
		}
		for (size_t num = 0; num < equalS; num++){
			list[pos++] = equal[num];
		}
		for (size_t num = 0; num < greaterS; num++){
			list[pos++] = greater[num];
		}


		return list;
}

int *quicksort(size_t size, const int *data){

	if (size == 0){
		return malloc(0);
	}

	int pivot = data[0];

	int *less, *equal, *greater;
	size_t lessS, equalS, greaterS;
	partition_array(pivot, data, size,&less, &lessS,
			&equal, &equalS, &greater, &greaterS);

	int *sortedLess = quicksort(lessS, less);
	int *sortedGreater = quicksort(greaterS, greater);

	int *res = malloc((lessS + greaterS + equalS) * sizeof(int));

	size_t pos = 0;
	for (size_t num = 0; num < lessS; num++){
		res[pos++] = sortedLess[num];
	}
	for (size_t num = 0; num < equalS; num++){
		res[pos++] = equal[num];
	}
	for (size_t num = 0; num < greaterS; num++){
		res[pos++] = sortedGreater[num];
	}

	free(less);
	free(equal);
	free(greater);
	free(sortedLess);
	free(sortedGreater);

	return res;
}

void *quicksort_threaded(void *args){

	qs_args_t *arg = args;

	if (arg->size == 0){
		int *empty = malloc(0);
		free(args);
		return empty;
	}

	int pivot = arg->data[0];

	int *less;
	int *equal;
	int *greater;

	size_t lessS;
	size_t equalS;
	size_t greaterS;

	partition_array(pivot, arg->data, arg->size,
                    &less, &lessS,
                    &equal, &equalS,
                    &greater, &greaterS);

	pthread_t thread1, thread2;

	qs_args_t *arg1 = NULL;
	qs_args_t *arg2 = NULL;
	int *sortedLess = NULL;
	int *sortedGreater = NULL;

	if (lessS > 0){
		pthread_mutex_lock(&lock);
		thread_count ++;
		pthread_mutex_unlock(&lock);

		arg1 = malloc(sizeof(qs_args_t));
		arg1->size = lessS;
		arg1-> data = less;

		pthread_create(&thread1,NULL, quicksort_threaded, arg1);

	}

	 if (greaterS > 0){
                pthread_mutex_lock(&lock);
                thread_count ++;
                pthread_mutex_unlock(&lock);

                arg2 = malloc(sizeof(qs_args_t));
                arg2->size = greaterS;
                arg2-> data = greater;

                pthread_create(&thread2,NULL, quicksort_threaded, arg2);

        }

	if (lessS > 0){
		pthread_join(thread1, (void**)&sortedLess);
	}else{
		sortedLess = malloc(0);
		free(less);
	}

	if (greaterS > 0){
                pthread_join(thread2, (void**)&sortedGreater);
        }else{
                sortedGreater = malloc(0);
                free(greater);
        }

	int *res = merge(sortedLess,lessS, equal, equalS,sortedGreater, greaterS);


	free(arg);
	return res;
}


void print(const int *arr, size_t num){
	for (size_t i = 0; i < num; i++){
		if (i > 0){
			printf(", ");
		}
		printf("%d", arr[i]);
	}
	printf("\n");
}

int main(int argc, char *argv[]) {

	int printDo = 0;
	char *filename = NULL;

	if (argc == 2){
		filename = argv[1];
	}
	else if(argc == 3  && strcmp(argv[1], "-p") == 0){
		printDo = 1;
		filename = argv[2];
	}
	else{
		fprintf(stderr, "usage: ./quicksort [-p] file_of_ints");
		return 1;
	}

	FILE *file = fopen(filename, "r");
	if (!file){
		perror("Error opening the file");
		return 1;
	}

	int *data = NULL;
	size_t count = 0;
	int val;

	while (fscanf(file, "%d", &val)== 1){
		data = realloc(data, (count+1) * sizeof(int));
		data[count++] = val;
	}
	fclose(file);

	if (printDo){
		printf("Unsorted list before non-threaded quicksort:  ");
        	print(data, count);
    	}

	clock_t starting = clock();
	int *sorted1 = quicksort(count,data);
	clock_t end = clock();
	double elapsedT = (double)(end - starting) / CLOCKS_PER_SEC;
	printf("Non-threaded time:  %0.6f\n", elapsedT);

	if (printDo){
		printf("Resulting list:  ");
		print(sorted1, count);
	}
	if (printDo){
		printf("Unsorted list before threaded quicksort:  ");
        	print(data, count);
    	}


	qs_args_t *arg = malloc(sizeof(qs_args_t));
	arg->size = count;
	arg->data = malloc(count*sizeof(int));
	for (size_t i = 0; i <count; i++){
		arg->data[i] = data[i];
	}


	starting = clock();
	int *sorted2 = quicksort_threaded(arg);
	end = clock();
	elapsedT = (double)(end - starting) / CLOCKS_PER_SEC;
	printf("Threaded time:      %0.6f\n", elapsedT);
	printf("Threads spawned:    %d\n", thread_count);
	if (printDo){
                printf("Resulting list:  ");
                print(sorted2, count);
        }

	free(data);
	free(sorted1);
	free(sorted2);

	return 0;
}
