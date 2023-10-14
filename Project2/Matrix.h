#ifndef MATRIX_H
#define MATRIX_H

//In row major, the row values are next to each other
//In column major, the column values are next to each other
template <class T>
class Matrix {

	private:
		T ** major;

		char rep; // 'R' for row major order, 'C' for column row order
	public:
		int columns;
		int rows;

	T get(int row, int col);
	void set(int row, int col, T value);

	Matrix(int row1, int col1, char represenation);

	T * getRowPointer(int r, int c);
	T * getColumnPointer(int c, int r, int  * alloc);

	void randomize();

	void print();

};

#endif
