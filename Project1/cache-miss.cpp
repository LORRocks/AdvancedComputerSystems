#include <cstdlib>

int matrix_size = 10000;

//Cache hit Cpp

int ** generate_matrix(){
	int ** top = (int **)malloc(sizeof(int *) * matrix_size);
	for(int i = 0; i < matrix_size ;i++){
		top[i] = (int*)malloc(sizeof(int) * matrix_size);
		for(int j = 0; j < matrix_size;j++){
			top[i][j] = 1;
		}
	}	
	return top;
}

int cache_hit_addition(int ** matrix){
	int total = 0;
	for(int i = 0; i < matrix_size;i++){
		for(int j = 0; j < matrix_size;j++){
			total = total * matrix[j][i] ;
		}
	}
}

int main(int argc, char * argv[]){

	if(argc >= 2){
		matrix_size = atoi(argv[1]);
	}


	int** matrix = generate_matrix();
	cache_hit_addition(matrix);
}
