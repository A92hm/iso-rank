//
//  DenseMatrix.h
//  Sparse_Matrix
//
//  Created by Ali Hajimirza on 6/11/13.
//  Copyright (c) 2013 Ali Hajimirza. All rights reserved.
//

#ifndef _DenseMatrix_h
#define _DenseMatrix_h

#define DEFAULT_MATRIX_SIZE 1

#include <cstdlib>
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include <cmath>
#include <fstream>
#include <sstream>
#include <iterator>
#include <iomanip>
#include <cstring>
#include <stdexcept>
#include <limits>
#include <math.h>
#include "MatrixExceptions.h"
#include "SparseElement.h"

#ifdef EIGEN
#include <Eigen/Dense>
#include <Eigen/Sparse>
#endif

#ifdef ARPACK
#include "dsmatrxa.h"
#include "ardsmat.h"
#include "ardssym.h"
#include "lsymsol.h"
#endif

#ifdef USE_MPI
#include "mpi.h"
#endif


template <typename T>
class DenseMatrix;

template <typename T>
std::ostream& operator<< (std::ostream&, const DenseMatrix<T>&);

template <typename T>
class DenseMatrix
{
private:
    void _copy(const DenseMatrix<T>&);
    void _initializeMatrix(bool);
    
protected:
    unsigned int _rows;
    unsigned int _cols;
    T** _edges;
    //std::vector<SparseElement<T> >sparse_form;
    
public:
    /**************
     *Constructors*
     **************/
    DenseMatrix();
    DenseMatrix(const std::string &file_path);
    DenseMatrix(int rows, int cols);
    DenseMatrix(const DenseMatrix<T>&);

    #ifdef USE_MPI
    DenseMatrix(int,int,MPI_Status&);
    //DenseMatrix(int,int,MPI_Status&, int);
    #endif

    /************
     *Destructor*
     ************/
    virtual ~DenseMatrix();

    /***********
     *ACCESSORS*
     ***********/
    bool isSquare() const;
    bool isSymmetric() const;
    T getFrobNorm() const;
    int getNumberOfRows() const;
    int getNumberOfColumns() const;
    // int getSparseFormSize() const;
    T* getTopEigenVector();
    std::vector<int> getNeighbors(int vertex) const;
    std::vector<SparseElement<T> >getSparseForm() const;
    DenseMatrix<T> getScatteredSelection(const std::vector<int>& vec_A, const std::vector<int>& vec_B) const;
  
    /**********
    *OPERATIONS*
    **********/
    DenseMatrix<T> transpose() const;
    std::vector<T> getSumOfRows() const;
    DenseMatrix<T> kron(const DenseMatrix<T>& matrix) const;
    DenseMatrix<T> diagonalVectorTimesMatrix(const std::vector<T>&) const;
    DenseMatrix<T> matrixTimesDiagonalVector(const std::vector<T>&) const;

    /**********
     *OPERATORS*
     **********/
    T* operator[](int);
    friend std::ostream& operator<< <> (std::ostream& stream, const DenseMatrix<T>& matrix);
    void operator= (const DenseMatrix<T>&);
    DenseMatrix<T> operator-(const DenseMatrix<T>& other_matrix) const;
    DenseMatrix<T> operator*(const DenseMatrix<T>& other_matrix) const;
};

//==========================================================CONSTRUCTORS============================================================

template <typename T>
DenseMatrix<T>::DenseMatrix()
{
    this->_rows = DEFAULT_MATRIX_SIZE;
    this->_cols = DEFAULT_MATRIX_SIZE;
    _initializeMatrix(true);
}

template<typename T>
DenseMatrix<T>::DenseMatrix(const std::string &file_path)
{
    int tmp_x;
    int tmp_y;
    std::ifstream file_reader;
    file_reader.open(file_path.c_str());
    
    if(file_reader.fail())
    {
        file_reader.close();
        throw FileDoesNotExistException(file_path.c_str());
    }
    
    file_reader >> this->_rows;
    file_reader >> this->_cols;
    file_reader >> tmp_x;      //skip the number of lines entry
    _initializeMatrix(true);
  
    
    while (!file_reader.eof())
    {
        file_reader >> tmp_x;
        file_reader >> tmp_y;
        this->_edges[tmp_x - 1][tmp_y - 1] = 1;
    }
    
    file_reader.close();
    
}

#ifdef USE_MPI
template <typename T>
DenseMatrix<T>::DenseMatrix(int source, int tag, MPI_Status& stat)
{
    MPI_Recv(&this->_rows, 1, MPI_INT, source, tag + 1, MPI_COMM_WORLD, &stat);
    this->_cols = _rows;
    _initializeMatrix(false);
    int recv_edges_size = (int)(0.5 * this->_rows * (this->_rows + 1));
    T* recv_edges = new T[recv_edges_size];
    MPI_Recv(recv_edges, recv_edges_size*sizeof(T), MPI_BYTE, source, tag + 2, MPI_COMM_WORLD, &stat);

    int counter = 0;
    for (int i = 0; i < this->_rows; i++)
    {
        for (int j = 0; j <= i; j++)
        {
           this->_edges[j][i] = this->_edges[i][j] = recv_edges[counter++];  
        }
    }
    delete [] recv_edges;
}

// template <typename T>
// DenseMatrix<T>::DenseMatrix(int source, MPI_Status& stat)
// {
//     MPI_Bcast(&this->_size, 1, MPI_INT, source, MPI_COMM_WORLD);
//     int recv_edges_size = (int)(0.5 * this->_size * (this->_size + 1));
//     T* recv_edges = new T[recv_edges_size];
//     MPI_Bcast(recv_edges, recv_edges_size*sizeof(T), MPI_BYTE, source, MPI_COMM_WORLD);
// 
//     int counter = 0;
//     for (int i = 0; i < this->_size; i++)
//     {
//         for (int j = 0; j <= i; j++)
//         {
//             this->insert(i, j,recv_edges[counter++]);
//         }
//     }
//     delete [] recv_edges;
// }
#endif

template <typename T>
DenseMatrix<T>::DenseMatrix(int rows, int cols)
{
    this->_rows = rows;
    this->_cols = cols;
    _initializeMatrix(true);
}

template <typename T>
DenseMatrix<T>::DenseMatrix(const DenseMatrix<T>& matrix)
{
    _copy(matrix);
}

//==========================================================DESTRUCTOR==============================================================
template <typename T>
DenseMatrix<T>::~DenseMatrix()
{
    if (_edges != NULL)
    {
        for(int i=0; i < this->_rows; i++)
        {
            delete [] _edges[i];
        }
        delete [] _edges;
    }
}

//===========================================================ACCESSORS===============================================================
template <typename T>
int DenseMatrix<T>::getNumberOfRows() const
{
    return _rows;
}

template <typename T>
int DenseMatrix<T>::getNumberOfColumns() const
{
    return _cols;
}

template <typename T>
bool DenseMatrix<T>::isSquare() const
{
    return (this->_rows == this->_cols);
}

template <typename T>
bool DenseMatrix<T>::isSymmetric() const
{
    for(int i=0; i < this->_rows; i++)
    {
        for(int j=0; j<this->_cols;j++)
        {
            if (this->_edges[i][j] != this->_edges[j][i])
            {
                return false;
            }
        }
    }
    
    return true;
}
template <typename T>
T DenseMatrix<T>::getFrobNorm() const
{
    T ret_val=0;

    for(int i=0; i < this->_rows; i++)
    {
        for(int j=0; j < this->_cols; j++)
        {
            ret_val+= this->_edges[i][j] * this->_edges[i][j];
        }
    }
    return ret_val;
}

template <typename T>
std::vector<SparseElement<T> > DenseMatrix<T>::getSparseForm() const
{    
    std::vector<SparseElement<T> > sparse_form;
    for(int i = 0; i < _rows; i++)
    {
        for(int j = 0; j<_cols; j++)
        {
            if(this->_edges[i][j] != 0)
            {
                sparse_form.push_back(SparseElement<T>(i,j, this->_edges[i][j]));
            }
        }
    }
    return sparse_form;
}

template <typename T>
DenseMatrix<T> DenseMatrix<T>::kron(const DenseMatrix<T>& matrix) const
{
    // also checking for matrices to be square
    if (!this->isSquare() || !matrix.isSquare())
    {
        throw NotASquareMatrixException();
    }
    
    //Initializing and allocating the product matrix
    int prod_size = this->_rows * matrix._rows;
    DenseMatrix<T> prod_matrix(prod_size, prod_size);
    
    /*
     *  Calculating the kronecker product:
     *  The indices of the product matrix is calculated by:
     *      i = (i_outer*size) + i_inner
     *      j = (j_outer*size) + j_inner
     */
    for (int i_outer = 0; i_outer < this->_rows; i_outer++)
    {
        for (int j_outer=0; j_outer < this->_cols; j_outer++)
        {
            for(int i_inner=0; i_inner < matrix._rows; i_inner++)
            {
                for(int j_inner=0; j_inner < matrix._cols; j_inner++)
                {
                    prod_matrix._edges[(i_outer*matrix._rows) + i_inner][(j_outer*matrix._cols)+j_inner] =
                    this->_edges[i_outer][j_outer] * matrix._edges[i_inner][j_inner];
                }
            }
        }
    }
    
    return prod_matrix;
}

template <typename T>
DenseMatrix<T> DenseMatrix<T>::getScatteredSelection(const std::vector<int>& vec_A, const std::vector<int>& vec_B) const
{
    int num_in_A = 0;
    for (int i=0; i< vec_A.size(); i++)
    {
        if (vec_A[i] == 1)
        {
            num_in_A++;
        }
    }
    int num_in_B = 0;
    for (int i=0; i < vec_B.size(); i++)
    {
        if( vec_B[i] == 1)
        {
            num_in_B++;
        }
    }
    //Initializing and allocating the product matrix
    DenseMatrix<T> res_matrix(num_in_A, num_in_B);
    int counter = 0;
    for (int i=0; i< vec_A.size(); i++)
    {
        for(int j=0; j< vec_B.size(); j++)
        {
            if ( vec_A[i] == 1 && vec_B[j] ==1)
            {
                res_matrix[counter/num_in_B][counter%num_in_B] = this->_edges[i][j];
                counter++;
            }
        }
    }
    return res_matrix;
}


template <typename T>
std::vector<int> DenseMatrix<T>::getNeighbors(int vertex) const
{
  std::vector<int> neighbors;
  for(int i = 0; i < this->_rows; i++)
  {
    if(this->_edges[i][vertex] == 1)
    {
      neighbors.push_back(i);
    }
  }
  return neighbors;
} 

template <typename T>
T* DenseMatrix<T>::getTopEigenVector()
{
#ifdef ARPACK
    int arr_size = (this->_rows*(this->_rows+1))/2;
    T nzval[arr_size];
    int counter=0;
    for(int i=0; i<this->_rows; i++)
    {
        for(int j=i;j < this->_rows ;j++)
        {
            nzval[counter++]= this->_edges[i][j];
        }
    }
    
    ARdsSymMatrix<T> ARMatrix(this->_rows,nzval,'L');
    ARluSymStdEig<T> eigProb(1, ARMatrix, "LM",10);
    eigProb.FindEigenvectors();
    T* eigen_vector = new T[this->_rows];
     
    for (int i=0; i < this->_rows ; i++)
    {
        eigen_vector[i] = eigProb.Eigenvector(0,i);
    }
    return eigen_vector;
#endif

#ifdef EIGEN
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> A_eigen = Eigen::MatrixXd::Zero(this->_rows, this->_cols);
		for (long i=0; i< this->_rows; i++) 
		{
			for(int j = 0; j < this->_cols; j++)
			{
				A_eigen(i,j) = this->_edges[i][j];
			}
		}
		Eigen::SelfAdjointEigenSolver<Eigen::MatrixXd> es;
		es.compute(A_eigen);
		
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> evals_eigen = es.eigenvalues();
		Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::ColMajor> evecs_eigen = es.eigenvectors();
		T* eigen_vector = new T[this->_rows];

		for ( int i=0; i < this->_cols; i++)
		{	
			if (evals_eigen(i) == evals_eigen.maxCoeff())
			{
				for (int j= 0; j < this->_rows ; j++)
				{
					eigen_vector[j] = evecs_eigen(j,i);
				}
				break;
			}
		}
	return eigen_vector;
#endif
}

// template <typename T>
// int DenseMatrix<T>:: getSparseFormSize() const
// {
//     return sparse_form_size;
// }

//===========================================================MUTATORS================================================================

template <typename T>
std::vector<T> DenseMatrix<T>::getSumOfRows() const
{
    std::vector<T> sum_vec(this->_rows);
    for(int i=0; i < _rows; i++)
    {
        for (int j = 0; j < _cols; j++)
        {
            sum_vec[i] += this->_edges[i][j];
        }
    }
    return sum_vec;
}

template <typename T>
DenseMatrix<T> DenseMatrix<T>:: diagonalVectorTimesMatrix(const std::vector<T>& vec) const
{    
    if(_cols != vec.size())
    {
        throw DimensionMismatchException();
    }
    
    DenseMatrix<T> ret_matrix(*this);
    for(int i = 0; i < _rows; i++)
    {
        for(int j = 0; j < _cols; j++ )
        {
            ret_matrix._edges[i][j] = this->_edges[i][j] * vec[i];
        }
    }
    return ret_matrix;
}


template <typename T>
DenseMatrix<T> DenseMatrix<T>::matrixTimesDiagonalVector(const std::vector<T>& vec) const
{    
    if(_cols != vec.size())
    {
        throw DimensionMismatchException();
    }

    DenseMatrix<T> ret_matrix(*this);
    for(int i=0; i < _cols; i++)
    {
        for(int j=0; j < _rows; j++)
        {
            ret_matrix._edges[j][i] = this->_edges[j][i]*vec[i];
        }
    }
    return ret_matrix;
}


template <typename T>
DenseMatrix<T> DenseMatrix<T>::transpose() const
{
    DenseMatrix<T> ret_matrix(this->_rows,this->_cols);

    for(int i=0;i<this->_rows;i++)
    {
        for(int j=0;j<this->_cols;j++)
        {
            ret_matrix[j][i] = this->_edges[i][j];
        }
    }
    return ret_matrix;
}


//==========================================================OPERATORS================================================================
template <typename T>
std::ostream& operator<<(std::ostream& stream, const DenseMatrix<T>& matrix)
{
    stream<< "Size: " << matrix._rows << "*" << matrix._cols << '\n';
    for (int i = 0; i < matrix._rows; i++)
    {
        for( int j = 0; j< matrix._cols; j++)
        {
            stream << matrix._edges[i][j] << ' ';
        }
        stream << "\n";
    }
    stream << "\n\n\n";
    return stream;
}

template <typename T>
T* DenseMatrix<T>::operator[](int index)
{
    return this->_edges[index];
}

template <typename T>
DenseMatrix<T> DenseMatrix<T>::operator*(const DenseMatrix<T>& other_matrix) const
{
    DenseMatrix<T> ret_matrix(this->_rows,this->_cols);
    T ret_val;
    for(int i = 0; i < this->_rows;i++)
    {
        for(int j = 0; j < other_matrix._cols;j++)
        {
            ret_val = 0;
            for(int k = 0; k < this->_cols;k++)
            {
	           ret_val += this->_edges[i][k] * other_matrix._edges[k][j];
            }
            ret_matrix[i][j] = ret_val;
        }
    }
  
    return ret_matrix;
}

template <typename T>
DenseMatrix<T> DenseMatrix<T>::operator-(const DenseMatrix<T>& other_matrix) const
{
  DenseMatrix<T> ret_matrix(this->_rows,this->_cols);
  for(int i = 0; i < this->_rows ;i++)
  {   
      for(int j = 0; j < this->_cols; j++)
      {
	       ret_matrix[i][j] = this->_edges[i][j] - other_matrix._edges[i][j];
      }
  }
  return ret_matrix;
}

template <typename T>
void DenseMatrix<T>::operator=(const DenseMatrix<T>& matrix)
{
	if (_edges != NULL)
    {
        for(int i=0; i < this->_rows; i++)
        {
            delete [] _edges[i];
        }
        delete [] _edges;
    }	
    _copy(matrix);
}

//===========================================================PRIVATE=================================================================
template <typename T>
void DenseMatrix<T>::_copy(const DenseMatrix<T>& matrix)
{
    this->_rows = matrix._rows;
    this->_cols = matrix._cols;
    _initializeMatrix(false);

    for(int i=0; i < this->_rows; i++)
    {
        for(int j=0; j < this->_cols; j++)
        {
            this->_edges[i][j] = matrix._edges[i][j];
        }
    }
}

template <typename T>
void DenseMatrix<T>::_initializeMatrix(bool fill)
{
    try
    {
        this->_edges = new T*[this->_rows];
        for(int i=0; i < this->_rows; i++)
        {
            this->_edges[i] = new T[this->_cols];
            if (fill)
            {
                for(int j=0; j< this->_cols; j++)
                {
                    this->_edges[i][j] = 0;
                }
            }
        }
    }
    catch (std::bad_alloc& e)
    {
    	std::cout << "Bad alloc" << std::endl;
        //throw OutOfMemoryException();
    }
}

//===================================================================================================================================
       


#endif
    