#include "Matrix.h"
#include <cstdio>	
#include <cstdlib>

#define MAX_MATRIX_RANDOM 4

template <typename T>
Matrix<T>::Matrix(int row1, int col1, char representation){
		rep = representation;	
		rows = row1;
		columns = col1;
		if(representation == 'R'){
			major = new T*[col1];
			for(int i = 0; i < row1;i++){
				major[i] = new T[row1 + 100];
				//We're adding a zero buffer on the end so when we read over, we don't cause issues
			}	
		}else if (representation == 'C'){
			major = new T*[row1];
			for(int i = 0; i < row1;i++){
				major[i] = new T[col1 + 100];
				//We're adding a zero buffer on the end so when we read over, we don't cause issues
			}	
		}else{
			printf("ISSUE-NOT A VALID MATRIX REPRESENTATION");
		}
}

template <typename T>
T Matrix<T>::get(int row, int col){
	if(rep == 'C'){
		return major[row][col];
	}else if(rep == 'R'){
		return major[col][row];
	}
}


template <typename T>
void Matrix<T>::set(int row, int col, T value){
	if(rep == 'C'){
		major[row][col] = value;
	}else if (rep == 'R'){
		major[col][row] = value;
	}
}

template <>
void Matrix<int>::randomize(){
	for(int r = 0 ; r < rows ; r++){
	for(int c = 0 ; c < columns; c++){
		set(r,c,(int)(rand() % MAX_MATRIX_RANDOM));
	}
	}
}

template <>
void Matrix<float>::randomize(){
	for(int r = 0 ; r < rows ; r++){
	for(int c = 0 ; c < columns; c++){
		set(r,c,(float)(rand() % MAX_MATRIX_RANDOM));
	}
	}
}

template <typename T>
T * Matrix<T>::getRowPointer(int r, int c){
	if(rep == 'R'){
		return (major[c] + r);
	}else{
		printf("NOT IMPLEMENTED 1");
		return NULL;
	}
};


template <typename T>
T * Matrix<T>::getColumnPointer(int r, int c, int * alloc){
	if(rep == 'C'){
		return (major[r] + c);	
		if(alloc != NULL){
			*alloc = 0;
		}
	}else{
		T* buffer = new T[8];
		for(int i = 0; i < 8 && i < columns;i++){
			buffer[i] = major[r + i][c];
		}
		if(alloc != NULL){
			*alloc = 1;
		}
		return buffer;
	}
};

template <>
void Matrix<int>::print(){
	for(int r = 0; r < rows;r++){
	for(int c = 0; c < columns;c++){
		if(rep == 'R'){
			printf("%2i ", major[c][r]);
		}
		if(rep == 'C'){
			printf("%2i ", major[r][c]);
		}
	}
	printf("\n");
	}
}

template <>
void Matrix<float>::print(){
	for(int r = 0; r < rows;r++){
	for(int c = 0; c < columns;c++){
		if(rep == 'R'){
			printf("%6.1f ", major[c][r]);
		}
		if(rep == 'C'){
			printf("%6.1f ", major[r][c]);
		}
	}
	printf("\n");
	}
}

template class Matrix<float>;
template class Matrix<int>;
