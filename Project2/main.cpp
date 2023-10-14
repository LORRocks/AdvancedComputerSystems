#include "Matrix.h"
#include <immintrin.h>
#include <pthread.h>
#include <cstdio>

//A wrapper class used to pass arguments to our threads
template<typename T>
class args{
	public:
	Matrix<T> * A;
	Matrix<T> * B;
	Matrix<T> * C;
	int row;
};

//shared variable between the threads to track currently working threads
int currentWorkingThreads = 0;
int maxThreads = 1;

int usingSIMDInstructions = 0; //0 is regular instructions, 1 is SIMD

//These are masks shared between all the SIMD functions
__m256i zeros, ones;

//Multiply using SIMD instructions for a single point (floating-point)
void sp_simd(Matrix<float> * A, Matrix<float> * B, Matrix<float> * C, int row, int col){
	__m256 acc;
	acc = _mm256_setzero_ps();
	//Go through row/column 8 at a time and do our acc
	for(int i = 0; i < A->rows ; i += 8){
		__m256 a = _mm256_loadu_ps(A->getRowPointer(row, i));
		int alloc = 0;
		float * column = B->getColumnPointer(i,col,&alloc);
		__m256 b = _mm256_loadu_ps(column);
		if(alloc){
			free(column);
		}
		a  = _mm256_mul_ps(a,b);
		acc = _mm256_add_ps(a,acc);
	}

	//Once through, we extract the values in acc, add them and set the matrix value
	float * buffer = (float*)aligned_alloc(32, sizeof(float)*8);
	_mm256_store_ps(buffer,acc);
	float result = 0;
	for(int i = 0; i < 8;i++){
		result += buffer[i];	
	}
	C->set(row,col,result);
	free(buffer);

	return;
	return;
}
//Multiply using SIMD instructions for a single point (fixed-point)
void sp_simd(Matrix<int> * A, Matrix<int> * B, Matrix<int> * C, int row, int col){
	__m256i acc;
	acc = _mm256_setzero_si256();
	//Go through row/column 8 at a time and do our acc
	for(int i = 0; i < A->rows ; i += 8){
		__m256i a = _mm256_maskload_epi32(A->getRowPointer(row, i), ones);
		int alloc = 0;
		int * column = B->getColumnPointer(i, col, &alloc);
		__m256i b = _mm256_maskload_epi32(column,ones);
		if(alloc){
			free(column);	
		}
		
		a  = _mm256_mullo_epi32(a,b);
		acc = _mm256_add_epi32(a,acc);
	}

	//Once through, we extract the values in acc, add them and set the matrix value
	int buffer[8];
	_mm256_maskstore_epi32(buffer,ones,acc);
	int result = 0;
	for(int i = 0; i < 8;i++){
		result += buffer[i];	
	}
	C->set(row,col,result);

	return;
}


//Multiply using standard C operations for a single point (fixed-point)
/*
void sp_c(Matrix<short> * A, Matrix<short> * B, Matrix<short> * C, int row, int col){
	//Multiply the row in A with corresponding value in B and accumulate the value
	short acc = 0;
	for(int i = 0; i < A->rows;i++){
		acc += A->get(row, i) * A->get(i, col);
	}
	C->set(row, col, acc);
	return;
}
*/

template <typename T>
//Multiply using standard C operations for a single point (floating-point)
void sp_c(Matrix<T> * A, Matrix<T> * B, Matrix<T> * C, int row, int col){
	//Multiply the row in A with corresponding value in B and accumulate the value
	T acc = 0;
	for(int i = 0; i < A->rows;i++){
		acc += A->get(row, i) * A->get(i, col);
	}
	C->set(row, col, acc);
	return;
}

template void sp_c(Matrix<float> * A, Matrix<float> * B, Matrix<float> * C, int row, int col);
template void sp_c(Matrix<int> * A, Matrix<int> * B, Matrix<int> * C, int row, int col);

//Multiply an entire row
//This is caculating the Row in output C
//We divide up multiplying the rows rather than a single loop so we can easily thread our execution
template <typename T>
void * multiply_row(void * argument){
	//Fetch the matrix arguments
	args<T> * tArguments = (args<T> *)argument;
	Matrix<T> * A = tArguments->A;
	Matrix<T> * B = tArguments->B;
	Matrix<T> * C = tArguments->C;
	int row = tArguments->row;

	for(int c = 0; c < C->columns;c++){
		if(usingSIMDInstructions){
			sp_simd(A, B, C, row, c);	
		}else{
			sp_c(A, B, C, row, c);	
		}
	}
		

	currentWorkingThreads -= 1;
}

//Multiply two matrixes
//Holds our threading function if we do that
//1 thread = no multi-threading
template <typename T>
Matrix<T> * multiply(Matrix<T> * A, Matrix<T> * B){
	Matrix<T> * C = new Matrix<T>(A->rows,B->columns,'R');
	pthread_t threads[A->rows];
	for(int r = 0; r < A->rows ; r++){
		//Wait for a thread to be free
		while(currentWorkingThreads >= maxThreads){
		}
		currentWorkingThreads += 1;
		args<T> * threadArguments = new args<T>;
		threadArguments->A = A;
		threadArguments->B = B;
		threadArguments->C = C;
		threadArguments->row = r;
		int result_code = pthread_create(&threads[r],NULL, multiply_row<T>, threadArguments);
	}
	//Once we're here, all the rows have been spawed, and we need to wait return the resulting matrix
	for(int r = 0; r < A->rows ; r++){
		pthread_join(threads[r],NULL);
	}
	return C;
}

//Helper's for the main method
int detect_int(char ptr[]) {
    char* end_ptr;
    strtol(ptr, &end_ptr, 10);
    if (*end_ptr == '\0' && atoi(ptr) >= 0) {
        return 1; // Valid integer
    } else {
        return 0; // Invalid integer
    }
}

//Program usage
// ./a.out <args> seed n
// ./a.out -xcs 1000 1024
// -x/f fixed vs floating
// -c cache--optimized
// -s simd instructions
// -t multi-threaded
// -p print matrixs

int main(int argc, char ** argv){
	//Start by allocating some masks we need
	zeros = _mm256_setzero_si256();
	ones = _mm256_set1_epi64x(-1);

	//Flag's to be extracted from the arguments
	char dataType = 'x';
	int cache_optimized = 0;
	char instruction_type = 'c';
	int multi_threaded = 0;
	int print_matrix = 0;
	int matrix_size = 0;
	
	//Let's parse our argumnets
	if(argc != 4){
		printf("Incorrect number of inputs\n");
		return 1;
	}	
	//Parse arg1
	int i = 0;
	while(argv[1][i] != 0){
		char c = argv[1][i];
		if(c == '-'){
		}
		if(c == 'x'){
			dataType = 'x';
		}
		if(c == 'f'){
			dataType = 'f';
		}
		if(c == 'c'){
			cache_optimized = 1;
		}
		if(c == 's'){
			instruction_type = 's';
		}
		if(c == 't'){
			multi_threaded = 1;
		}
		if(c == 'p'){
			print_matrix = 1;
		}
		i++;
	}
	//Parse and set our srand seed
	if(detect_int(argv[2])){
		srand(atoi(argv[2]));
	}else{
		printf("Invalid usage, not valid integer for random seed\n");
	}
	//Set our matrix size
	if(detect_int(argv[3])){
		matrix_size = atoi(argv[3]);
	}else{
		printf("Invalid usage, not valid integer for matrix size\n");
	}

	//Set some program wide flags based on the args we just received
	if(multi_threaded){
		printf("Multi-threaded \n");
		maxThreads = 8;
	}
	if(instruction_type == 's'){
		usingSIMDInstructions = 1; //0 is regular instructions, 1 is SIMD
		printf("Using SIMD Instructions\n");
	}

	printf("Multiplying two random matrix of size %ix%i\n",matrix_size,matrix_size);
	//Peform our work based on the input's we just received
	if(dataType == 'x'){
		Matrix<int> * A = new Matrix<int>(matrix_size, matrix_size, 'R');
		Matrix<int> * B ;
		if(cache_optimized){
			B = new Matrix<int>(matrix_size,matrix_size,'C');
			printf("Cache-optimized\n");
		}else{
			B = new Matrix<int>(matrix_size,matrix_size,'R');
		}
		A->randomize();
		B->randomize();
		if(print_matrix){
			A->print();
			B->print();
		}
		Matrix<int> * C = multiply(A,B);
		if(print_matrix){
			C->print();
		}
		free(B);
		free(A);
		free(C);
	}else{
		Matrix<float> * A = new Matrix<float>(matrix_size, matrix_size, 'R');
		Matrix<float> * B ;
		if(cache_optimized){
			printf("Cache-optimized\n");
			B = new Matrix<float>(matrix_size,matrix_size,'C');
		}else{
			B = new Matrix<float>(matrix_size,matrix_size,'R');
		}
		A->randomize();
		B->randomize();
		if(print_matrix){
			A->print();
			B->print();
		}
		Matrix<float> * C = multiply(A,B);
		if(print_matrix){
			C->print();
		}
		free(B);
		free(A);
		free(C);

	}	
	printf("Finished multiplying\n");
	return 0;
}


