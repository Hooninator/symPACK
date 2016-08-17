/// @file utility.hpp
/// @brief Various utility subroutines.
/// @author Lin Lin & Mathias Jacquelin
/// @date 2012-09-27
#ifndef _UTILITY_HPP_ 
#define _UTILITY_HPP_

#include  <mpi.h>
#include  <stdlib.h>
#include  "sympack/Environment.hpp"
//#include  "sympack/NumVec.hpp"
//#include  "sympack/NumMat.hpp"
#include  "sympack/DistSparseMatrix.hpp"
#include  "sympack/ETree.hpp"

namespace SYMPACK{

//exact cost of a mxn panel
#define CHOLESKY_COST(m,n)  ((n)*pow((m),2.0) + 2*(n)*(m)-pow((n),3.0)/3.0 - 3.0*pow((n),2.0)/2.0 - (n)/6.0 -1.0)


template <typename F> void SetValue( SYMPACK::vector<F>& vec, F val ){
  fill(vec.begin(),vec.end(),val);
}

void SetValue( SYMPACK::vector<char>& vec, bool val );


// *********************************************************************
// Define constants
// *********************************************************************

// Write format control parameters 
const int LENGTH_VAR_NAME = 8;
const int LENGTH_DBL_DATA = 16;
const int LENGTH_INT_DATA = 5;
const int LENGTH_VAR_UNIT = 6;
const int LENGTH_DBL_PREC = 8;
const int LENGTH_VAR_DATA = 16;



// *********************************************************************
// Formatted output stream
// *********************************************************************

// Bool is NOT defined due to ambiguity with Int

inline Int PrintBlock(std::ostream &os, const std::string name){

	os << std::endl<< "*********************************************************************" << std::endl;
	os << name << std::endl;
	os << "*********************************************************************" << std::endl << std::endl;
	return 0;
}

// String
inline Int Print(std::ostream &os, const std::string name) {
  os << std::setiosflags(std::ios::left) << name << std::endl;
  return 0;
};

inline Int Print(std::ostream &os, const char* name) {
  os << std::setiosflags(std::ios::left) << std::string(name) << std::endl;
  return 0;
};

inline Int Print(std::ostream &os, const std::string name, std::string val) {
  os << std::setiosflags(std::ios::left) 
     << std::setw(LENGTH_VAR_NAME) << name
     << std::setw(LENGTH_VAR_DATA) << val
     << std::endl;
  return 0;
};

inline Int Print(std::ostream &os, const std::string name, const char* val) {
  os << std::setiosflags(std::ios::left) 
     << std::setw(LENGTH_VAR_NAME) << name
     << std::setw(LENGTH_VAR_DATA) << std::string(val)
     << std::endl;
  return 0;
};


// Real

// one real number

inline Int Print(std::ostream &os, const std::string name, Real val) {
  os << std::setiosflags(std::ios::left) 
     << std::setw(LENGTH_VAR_NAME) << name
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_DBL_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::endl;
  return 0;
};

inline Int Print(std::ostream &os, const char* name, Real val) {
  os << std::setiosflags(std::ios::left) 
     << std::setw(LENGTH_VAR_NAME) << std::string(name)
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_DBL_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::endl;
  return 0;
};


inline Int Print(std::ostream &os, const std::string name, Real val, const std::string unit) {
  os << std::setiosflags(std::ios::left) 
     << std::setw(LENGTH_VAR_NAME) << name
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_DBL_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_UNIT) << unit 
     << std::endl;
  return 0;
};

inline Int Print(std::ostream &os, const char *name, Real val, const char *unit) {
  os << std::setiosflags(std::ios::left) 
     << std::setw(LENGTH_VAR_NAME) << std::string(name)
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_DBL_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_UNIT) << std::string(unit) 
     << std::endl;
  return 0;
};

// Two real numbers
inline Int Print(std::ostream &os, const std::string name1, Real val1, const std::string unit1,
  const std::string name2, Real val2, const std::string unit2) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << name1
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val1
     << std::setw(LENGTH_VAR_UNIT) << unit1 
     << std::setw(LENGTH_VAR_NAME) << name2
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val2
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_UNIT) << unit2 
     << std::endl;
  return 0;
};

inline Int Print(std::ostream &os, const char *name1, Real val1, const char *unit1,
  char *name2, Real val2, char *unit2) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << std::string(name1)
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val1
     << std::setw(LENGTH_VAR_UNIT) << std::string(unit1) 
     << std::setw(LENGTH_VAR_NAME) << std::string(name2)
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val2
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_UNIT) << std::string(unit2) 
     << std::endl;
  return 0;
};

// Int and Real
inline Int Print(std::ostream &os, const std::string name1, Int val1, const std::string unit1,
  const std::string name2, Real val2, const std::string unit2) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << name1
     << std::setw(LENGTH_INT_DATA) << val1
     << std::setw(LENGTH_VAR_UNIT) << unit1 
     << std::setw(LENGTH_VAR_NAME) << name2
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val2
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_UNIT) << unit2 
     << std::endl;
  return 0;
};

inline Int Print(std::ostream &os, const char *name1, Int val1, const char *unit1,
  char *name2, Real val2, char *unit2) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << std::string(name1)
     << std::setw(LENGTH_INT_DATA) << val1
     << std::setw(LENGTH_VAR_UNIT) << std::string(unit1) 
     << std::setw(LENGTH_VAR_NAME) << std::string(name2)
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val2
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_UNIT) << std::string(unit2) 
     << std::endl;
  return 0;
};

inline Int Print(std::ostream &os, 
  const char *name1, Int val1, 
  const char *name2, Real val2, 
  char *name3, Real val3) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << std::string(name1)
     << std::setw(LENGTH_INT_DATA) << val1 
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_NAME) << std::string(name2)
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val2
     << std::setw(LENGTH_VAR_NAME) << std::string(name3) 
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val3
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::endl;
  return 0;
};


inline Int Print(std::ostream &os, 
  const char *name1, Int val1, 
  const char *name2, Real val2, 
  const char *name3, Real val3,
  const char *name4, Real val4 ) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << std::string(name1)
     << std::setw(LENGTH_INT_DATA) << val1 
     << std::setiosflags(std::ios::scientific)
     << std::setiosflags(std::ios::showpos)
     << std::setw(LENGTH_VAR_NAME) << std::string(name2)
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val2
     << std::setw(LENGTH_VAR_NAME) << std::string(name3) 
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val3
     << std::setw(LENGTH_VAR_NAME) << std::string(name4) 
     << std::setw(LENGTH_VAR_DATA) << std::setprecision(LENGTH_DBL_PREC)<< val4
     << std::resetiosflags(std::ios::scientific)
     << std::resetiosflags(std::ios::showpos)
     << std::endl;
  return 0;
};

// Int

// one integer number
inline Int Print(std::ostream &os, std::string name, Int val) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << name
     << std::setw(LENGTH_VAR_DATA) << val
     << std::endl;
  return 0;
};


inline Int Print(std::ostream &os, const char *name, Int val) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << std::string(name)
     << std::setw(LENGTH_VAR_DATA) << val
     << std::endl;
  return 0;
};


inline Int Print(std::ostream &os, const std::string name, Int val, const std::string unit) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << name
     << std::setw(LENGTH_VAR_DATA) << val
     << std::setw(LENGTH_VAR_UNIT) << unit 
     << std::endl;
  return 0;
};


inline Int Print(std::ostream &os, const char* name, Int val, const std::string unit) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << std::string(name)
     << std::setw(LENGTH_VAR_DATA) << val
     << std::setw(LENGTH_VAR_UNIT) << unit 
     << std::endl;
  return 0;
};



// two integer numbers
inline Int Print(std::ostream &os, const std::string name1, Int val1, const std::string unit1,
  const std::string name2, Int val2, const std::string unit2) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << name1
     << std::setw(LENGTH_VAR_DATA) << val1
     << std::setw(LENGTH_VAR_UNIT) << unit1 
     << std::setw(LENGTH_VAR_NAME) << name2
     << std::setw(LENGTH_VAR_DATA) << val2
     << std::setw(LENGTH_VAR_UNIT) << unit2 
     << std::endl;
  return 0;
};

inline Int Print(std::ostream &os, const char *name1, Int val1, const char *unit1,
  char *name2, Int val2, char *unit2) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << std::string(name1)
     << std::setw(LENGTH_VAR_DATA) << val1
     << std::setw(LENGTH_VAR_UNIT) << std::string(unit1) 
     << std::setw(LENGTH_VAR_NAME) << std::string(name2)
     << std::setw(LENGTH_VAR_DATA) << val2
     << std::setw(LENGTH_VAR_UNIT) << std::string(unit2) 
     << std::endl;
  return 0;
};

// Bool

// one boolean number
inline Int Print(std::ostream &os, const std::string name, bool val) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << name;
	if( val == true )
     os << std::setw(LENGTH_VAR_NAME) << "true" << std::endl;
	else
     os << std::setw(LENGTH_VAR_NAME) << "false" << std::endl;
  return 0;
};


inline Int Print(std::ostream &os, const char* name, bool val) {
  os << std::setiosflags(std::ios::left)
     << std::setw(LENGTH_VAR_NAME) << std::string(name);
	if( val == true )
     os << std::setw(LENGTH_VAR_NAME) << "true" << std::endl;
	else
     os << std::setw(LENGTH_VAR_NAME) << "false" << std::endl;
  return 0;
};

// *********************************************************************
// Overload << and >> operators for basic data types
// *********************************************************************



// std::set
template <class F> inline std::ostream& operator<<( std::ostream& os, const std::set<F>& s)
{
	os<<s.size()<<std::endl;
	os.setf(std::ios_base::scientific, std::ios_base::floatfield);
  for(typename std::set<F>::iterator it = s.begin();it!=s.end();it++){
		os<<" "<<*it;
  }
	os<<std::endl;
	return os;
}







// SYMPACK::vector
template <class F> inline std::ostream& operator<<( std::ostream& os, const SYMPACK::vector<F>& vec)
{
	os<<vec.size()<<std::endl;
	os.setf(std::ios_base::scientific, std::ios_base::floatfield);
	for(Int i=0; i<vec.size(); i++)	 
		os<<" "<<vec[i];
	os<<std::endl;
	return os;
}

// Etree
inline std::ostream& operator<<( std::ostream& os, const ETree& tree) 
{
  os<<tree.n()<<std::endl;
  for(Int i = 1; i<=tree.Size(); i++){
//    os<<i<<" ["<<tree.parent_(i-1)<<"] ";
    os<<tree.Parent(i-1)<<" ";
  }
  os<<std::endl;

  if(tree.IsPostOrdered()){
    for(Int i = 1; i<=tree.Size(); i++){
//      os<<i<<" ["<<tree.PostParent(i-1)<<"] ";
      os<<tree.PostParent(i-1)<<" ";
    }
    os<<std::endl;
  }

  return os;
}

// *********************************************************************
// serialize/deserialize for basic types
// More specific serialize/deserialize will be defined in individual
// class files
// *********************************************************************

// standard case for most serialization/deserialization process.
const SYMPACK::vector<Int> NO_MASK(1);

//bool
inline Int serialize(const bool& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
  os.write((char*)&val, sizeof(bool));
  return 0;
}

inline Int deserialize(bool& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
  is.read((char*)&val, sizeof(bool));
  return 0;
}

//char
inline Int serialize(const char& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
  os.write((char*)&val, sizeof(char));
  return 0;
}

inline Int deserialize(char& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
  is.read((char*)&val, sizeof(char));
  return 0;
}

inline Int combine(char& val, char& ext)
{
	throw  std::logic_error( "Combine operation not implemented." );
}

//-------------------
//Int
inline Int serialize(const Int& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
  os.write((char*)&val, sizeof(Int));
  return 0;
}

inline Int deserialize(Int& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
  is.read((char*)&val, sizeof(Int));
  return 0;
}

inline Int combine(Int& val, Int& ext)
{
  val += ext;
  return 0;
}

//-------------------
//Real
inline Int serialize(const Real& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
  os.write((char*)&val, sizeof(Real));
  return 0;
}

inline Int deserialize(Real& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
  is.read((char*)&val, sizeof(Real));
  return 0;
}

inline Int combine(Real& val, Real& ext)
{
  val += ext;
  return 0;
}

//-------------------
//Complex
inline Int serialize(const Complex& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
  os.write((char*)&val, sizeof(Complex));
  return 0;
}

inline Int deserialize(Complex& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
  is.read((char*)&val, sizeof(Complex));
  return 0;
}

inline Int combine(Complex& val, Complex& ext)
{
  val += ext;
  return 0;
}

//-------------------
//SYMPACK::vector
template<class T>
Int serialize(const SYMPACK::vector<T>& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
  Int sz = val.size();
  os.write((char*)&sz, sizeof(Int));
  for(Int k=0; k<sz; k++)
    serialize(val[k], os, mask);
  return 0;
}

template<class T>
Int deserialize(SYMPACK::vector<T>& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
  Int sz;
  is.read((char*)&sz, sizeof(Int));
  val.resize(sz);
  for(Int k=0; k<sz; k++)
    deserialize(val[k], is, mask);
  return 0;
}

template<class T>
Int combine(SYMPACK::vector<T>& val, SYMPACK::vector<T>& ext)
{
	throw  std::logic_error( "Combine operation not implemented." );
  return 0;
}

//-------------------
//std::set
template<class T>
Int serialize(const std::set<T>& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
  Int sz = val.size();
  os.write((char*)&sz, sizeof(Int));
  for(typename std::set<T>::const_iterator mi=val.begin(); mi!=val.end(); mi++) 
	serialize((*mi), os, mask);
  return 0;
}

template<class T>
Int deserialize(std::set<T>& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
  val.clear();
  Int sz;
  is.read((char*)&sz, sizeof(Int));
  for(Int k=0; k<sz; k++) {
	T t; deserialize(t, is, mask);
	val.insert(t);
  }
  return 0;
}

template<class T>
Int combine(std::set<T>& val, std::set<T>& ext)
{
	throw  std::logic_error( "Combine operation not implemented." );
  return 0;
}

//-------------------
//std::map
template<class T, class S>
Int serialize(const std::map<T,S>& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
  Int sz = val.size();
  os.write((char*)&sz, sizeof(Int));
  for(typename std::map<T,S>::const_iterator mi=val.begin(); mi!=val.end(); mi++) {
    serialize((*mi).first, os, mask);
    serialize((*mi).second, os, mask);
  }
  return 0;
}

template<class T, class S>
Int deserialize(std::map<T,S>& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
  val.clear();
  Int sz;
  is.read((char*)&sz, sizeof(Int));
  for(Int k=0; k<sz; k++) {
    T t;	deserialize(t, is, mask);
    S s;	deserialize(s, is, mask);
    val[t] = s;
  }
  return 0;
}

template<class T, class S>
Int combine(std::map<T,S>& val, std::map<T,S>& ext)
{
	throw  std::logic_error( "Combine operation not implemented." );
  return 0;
}

//-------------------
//std::pair
template<class T, class S>
Int serialize(const std::pair<T,S>& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
  serialize(val.first, os, mask);
  serialize(val.second, os, mask);
  return 0;
}

template<class T, class S>
Int deserialize(std::pair<T,S>& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
  deserialize(val.first, is, mask);
  deserialize(val.second, is, mask);
  return 0;
}

template<class T, class S>
Int combine(std::pair<T,S>& val, std::pair<T,S>& ext)
{
	throw  std::logic_error( "Combine operation not implemented." );
  return 0;
}



//-------------------
//DistSparseMatrix
template<class T>
Int inline serialize(const DistSparseMatrix<T>& val, std::ostream& os, const SYMPACK::vector<Int>& mask)
{
	serialize( val.size,        os, mask );
	serialize( val.nnz,         os, mask );
	serialize( val.Local_.nnz,    os, mask );
	serialize( val.Local_.colptr, os, mask );
	serialize( val.Local_.rowind, os, mask );
	serialize( val.nzvalLocal,  os, mask );
	// No need to serialize the communicator
	return 0;
}

template<class T>
Int inline deserialize(DistSparseMatrix<T>& val, std::istream& is, const SYMPACK::vector<Int>& mask)
{
	deserialize( val.size,        is, mask );
	deserialize( val.nnz,         is, mask );
	deserialize( val.Local_.nnz,    is, mask );
	deserialize( val.Local_.colptr, is, mask );
	deserialize( val.Local_.rowind, is, mask );
	deserialize( val.nzvalLocal,  is, mask );
	// No need to deserialize the communicator
  return 0;
}

template<class T>
Int inline combine(DistSparseMatrix<T>& val, DistSparseMatrix<T>& ext)
{
	throw  std::logic_error( "Combine operation not implemented." );
  return 0;
}


// *********************************************************************
// Parallel IO functions
// *********************************************************************

Int SeparateRead(std::string name, std::istringstream& is);

Int SeparateWrite(std::string name, std::ostringstream& os);

Int SeparateWriteAscii(std::string name, std::ostringstream& os);

Int SharedRead(std::string name, std::istringstream& is);

Int SharedWrite(std::string name, std::ostringstream& os);


// *********************************************************************
// Comparator
// *********************************************************************

// Real
inline bool PairLtComparator( const std::pair<Real, Int>& l, 
		const std::pair<Real, Int>& r ){
	return l.first < r.first;
}

inline bool PairGtComparator( const std::pair<Real, Int>& l, 
		const std::pair<Real, Int>& r ){
	return l.first > r.first;
}

// For sorting with indices
// Example usage:
//   std::sort(val.begin(), val.end(), IndexComp<SYMPACK::vector<int>&>(indices));
template<class T> 
struct IndexComp {
private: 
	const T indices_;
public:
	IndexComp (const T indices) : indices_(indices) {}
	bool operator()(const size_t a, const size_t b) const
		{ return indices_[a] < indices_[b]; }
};




// *********************************************************************
// Sparse Matrix
// *********************************************************************

// TODO Complex format

void ReadDistSparseMatrix( const char* filename, DistSparseMatrix<Real>& pspmat, MPI_Comm comm );

void ParaReadDistSparseMatrix ( const char* filename, DistSparseMatrix<Real>& pspmat, MPI_Comm comm );

void ParaReadDistSparseMatrix ( const char* filename, DistSparseMatrix<Complex>& pspmat, MPI_Comm comm );

void ParaWriteDistSparseMatrix ( const char* filename, DistSparseMatrix<Real>& pspmat, MPI_Comm comm );

void ReadDistSparseMatrixFormatted( const char* filename, DistSparseMatrix<Real>& pspmat, MPI_Comm comm );

// Functions for DistSparseMatrix

template <class F1, class F2> 
void
CopyPattern	( const DistSparseMatrix<F1>& A, DistSparseMatrix<F2>& B )
{
	B.size        = A.size;
	B.nnz         = A.nnz;
	B.Local_.nnz    = A.Local_.nnz;
	B.Local_.colptr = A.Local_.colptr;
	B.Local_.rowind = A.Local_.rowind;
	B.nzvalLocal.Resize( A.Local_.nnz );
	B.comm        = A.comm;
	return ;
}		// -----  end of template function CopyPattern  ----- 

// *********************************************************************
// Other numerical routines
// *********************************************************************

void
LinearInterpolation ( 
		const SYMPACK::vector<Real>& x, 
		const SYMPACK::vector<Real>& y,
		const SYMPACK::vector<Real>& xx,
		SYMPACK::vector<Real>& yy );

} // namespace SYMPACK


#if 1

#ifdef WITH_BEBOP_UTIL
extern "C" {
#include "bebop/util/config.h"
#include "bebop/smc/sparse_matrix.h"
#include "bebop/smc/csr_matrix.h"
#include "bebop/smc/csc_matrix.h"
#include "bebop/smc/sparse_matrix_ops.h"

#include "bebop/util/get_options.h"
#include "bebop/util/init.h"
#include "bebop/util/malloc.h"
#include "bebop/util/timer.h"
#include "bebop/util/util.h"
}
#endif


namespace SYMPACK{



template <typename SCALAR, typename INSCALAR >
  int ReadHB_PARA_MPIIO(std::string & filename, DistSparseMatrix<SCALAR> & HMat){

    MPI_Comm & workcomm = HMat.comm;

    int mpirank;
    MPI_Comm_rank(workcomm,&mpirank);

    int mpisize;
    MPI_Comm_size(workcomm,&mpisize);

    auto n = HMat.size;
    auto nnz = HMat.nnz;
    size_t headerOffset = 0;
    int colptrWidth = 0;
    int rowindWidth = 0;
    int nzvalWidth = 0;
    int colptrCntPerRow = 0;
    int rowindCntPerRow = 0;
    int nzvalCntPerRow = 0;
      int colptrCnt = 0;
      int rowindCnt = 0;
      int nzvalCnt = 0;

    if(mpirank==0){
      ifstream infile;
      infile.open(filename.c_str());

      string line;
      stringstream iss;
      //skip 1st line
      if(getline(infile, line)){}
      if(getline(infile, line)){
        iss.str("");
        iss.clear();
        iss<<line;
        int dummy;
        iss>>dummy;
        iss>>colptrCnt>>rowindCnt>>nzvalCnt;
      }
      //read from third line

      int m;
      if(getline(infile, line))
      {
        iss.str("");
        iss.clear();
        iss<<line;
        string type;
        iss>>type;
        iss>>m>>n>>nnz;
      }


      //read from 4th line
      if(getline(infile, line))
      {
        iss.str("");
        iss.clear();
        iss<<line;
        string format;
        iss>>format;
        int dummy;
        sscanf(format.c_str(),"(%dI%d)",&colptrCntPerRow,&colptrWidth);
        iss>>format;
        sscanf(format.c_str(),"(%dI%d)",&rowindCntPerRow,&rowindWidth);
        iss>>format;
        sscanf(format.c_str(),"(%dE%d.%d)",&nzvalCntPerRow,&nzvalWidth,&dummy);
      }

      headerOffset = infile.tellg();
      infile.close();
    }


    MPI_Status mpistat;
    MPI_Datatype type;
    int lens[12];
    MPI_Aint disps[12];
    MPI_Datatype types[12];

    /* define a struct that describes all our data */
    lens[0] = sizeof(n);
    lens[1] = sizeof(nnz);
    lens[2] = sizeof(headerOffset   );
    lens[3] = sizeof(colptrWidth    );
    lens[4] = sizeof(rowindWidth    );
    lens[5] = sizeof(nzvalWidth     );
    lens[6] = sizeof(colptrCntPerRow);
    lens[7] = sizeof(rowindCntPerRow);
    lens[8] = sizeof(nzvalCntPerRow );
    lens[9] =  sizeof(colptrCnt);
    lens[10] = sizeof(rowindCnt);
    lens[11] = sizeof(nzvalCnt );
    MPI_Address(&n, &disps[0]);
    MPI_Address(&nnz, &disps[1]);
    MPI_Address(&headerOffset   , &disps[2]);
    MPI_Address(&colptrWidth    , &disps[3]);
    MPI_Address(&rowindWidth    , &disps[4]);
    MPI_Address(&nzvalWidth     , &disps[5]);
    MPI_Address(&colptrCntPerRow, &disps[6]);
    MPI_Address(&rowindCntPerRow, &disps[7]);
    MPI_Address(&nzvalCntPerRow , &disps[8]);
    MPI_Address(&colptrCnt , &disps[9]);
    MPI_Address(&rowindCnt , &disps[10]);
    MPI_Address(&nzvalCnt  , &disps[11]);
    types[0] = MPI_BYTE;
    types[1] = MPI_BYTE;
    types[2] = MPI_BYTE;
    types[3] = MPI_BYTE;
    types[4] = MPI_BYTE;
    types[5] = MPI_BYTE;
    types[6] = MPI_BYTE;
    types[7] = MPI_BYTE;
    types[8] = MPI_BYTE;
    types[9] = MPI_BYTE;
    types[10] = MPI_BYTE;
    types[11] = MPI_BYTE;
    MPI_Type_struct(12, lens, disps, types, &type);
    MPI_Type_commit(&type);


    /* broadcast the header data to everyone */
    MPI_Bcast(MPI_BOTTOM, 1, type, 0, workcomm);
    MPI_Type_free(&type);

    HMat.size = n;
    HMat.nnz = nnz;


    Int err = 0;
    int filemode = MPI_MODE_RDONLY | MPI_MODE_UNIQUE_OPEN;

    MPI_File fin;
    MPI_Status status;

    err = MPI_File_open(workcomm,(char*) filename.c_str(), filemode, MPI_INFO_NULL,  &fin);

    if (err != MPI_SUCCESS) {
      throw std::logic_error( "File cannot be opened!" );
    }


    //compute local number of columns
    int nlocal = (mpirank<mpisize-1)?n/mpisize:n-mpirank*(int)(n/mpisize);
    int firstNode = mpirank*(int)(n/mpisize) + 1;
    //initialize local arrays
    HMat.Local_.colptr.resize(nlocal+1);


    //colptr
    MPI_Offset myColptrOffset = 0;
    {

      int lineLastNode = std::ceil(( double(firstNode+nlocal) / double(colptrCntPerRow)));
      int lineFirstNode = std::ceil(( double(firstNode) / double(colptrCntPerRow)));
      int skip = (firstNode - 1)*colptrWidth + (lineFirstNode - 1);
      int readBytes = (nlocal+1)*colptrWidth + (lineLastNode - lineFirstNode);
      int skipAfter = (n+1 - (firstNode+nlocal))*colptrWidth + (colptrCnt - lineLastNode +1) ;

      myColptrOffset = headerOffset + skip;


      {
        std::string rdStr;
        rdStr.resize(readBytes);

        err= MPI_File_read_at_all(fin, myColptrOffset, &rdStr[0], readBytes, MPI_BYTE, &status);
        if (err != MPI_SUCCESS) {
          throw std::logic_error( "error reading colptr" );
        }

        istringstream iss(rdStr);
        Ptr j;
        Int locPos = 0;
        while(iss>> j){
          HMat.Local_.colptr[locPos++]=j;
        }
      }
      myColptrOffset += skipAfter;
    }

    //convert to local colptr and compute local nnz
    Ptr first_idx = HMat.Local_.colptr.front();
    Ptr last_idx = HMat.Local_.colptr.back();
    for(int i=nlocal;i>=0;i--){
      HMat.Local_.colptr[i] = HMat.Local_.colptr[i] - HMat.Local_.colptr[0] + 1;
    }
    Ptr nnzLocal = HMat.Local_.colptr.back()-1;

    HMat.Local_.rowind.resize(nnzLocal);

    //rowind
    MPI_Offset myRowindOffset = 0;
    {

      int lineLastEdge = std::ceil(( double(last_idx-1) / double(rowindCntPerRow)));
      int lineFirstEdge = std::ceil(( double(first_idx) / double(rowindCntPerRow)));
      int skip = (first_idx - 1)*rowindWidth + (lineFirstEdge - 1);
      int readBytes = (last_idx - first_idx)*rowindWidth + (lineLastEdge - lineFirstEdge);
      int skipAfter = (nnz+1 - last_idx)*rowindWidth + (rowindCnt - lineLastEdge +1) ;



      myRowindOffset = myColptrOffset + skip;


      {
        std::string rdStr;
        rdStr.resize(readBytes);

        err= MPI_File_read_at_all(fin, myRowindOffset, &rdStr[0], readBytes, MPI_BYTE, &status);
        if (err != MPI_SUCCESS) {
          throw std::logic_error( "error reading colptr" );
        }

        istringstream iss(rdStr);
        Idx j;
        Int locPos = 0;
        while(iss>> j){
          HMat.Local_.rowind[locPos++]=j;
        }
      }

      myRowindOffset+=skipAfter;
    }


    HMat.nzvalLocal.resize(nnzLocal);

    //nzval
    MPI_Offset myNzvalOffset = 0;
    {
      int lineLastEdge = std::ceil(( double(last_idx-1) / double(nzvalCntPerRow)));
      int lineFirstEdge = std::ceil(( double(first_idx) / double(nzvalCntPerRow)));
      int skip = (first_idx - 1)*nzvalWidth + (lineFirstEdge - 1);
      int readBytes = (last_idx - first_idx)*nzvalWidth + (lineLastEdge - lineFirstEdge);
      int skipAfter = (nnz+1 - last_idx)*nzvalWidth + (nzvalCnt - lineLastEdge +1) ;

      myNzvalOffset = myRowindOffset + skip;
      {
        std::string rdStr;
        rdStr.resize(readBytes);


        err= MPI_File_read_at_all(fin, myNzvalOffset, &rdStr[0], readBytes, MPI_BYTE, &status);
        if (err != MPI_SUCCESS) {
          throw std::logic_error( "error reading colptr" );
        }      istringstream iss(rdStr);

        INSCALAR j;
        Int locPos = 0;
        while(iss>> j){
          HMat.nzvalLocal[locPos++]=(SCALAR)j;
        }
      }

      myNzvalOffset+=skipAfter;
    }


    MPI_Barrier( workcomm );
    MPI_File_close(&fin);

    HMat.globalAllocated = false;
    HMat.Local_.size = HMat.size;
    HMat.Local_.nnz = nnzLocal;

    return 0;

  }








template <typename SCALAR, typename INSCALAR >
int ReadHB_PARA(std::string & filename, DistSparseMatrix<SCALAR> & HMat);

template <typename SCALAR, typename INSCALAR >
int ReadHB_PARA(std::string & filename, DistSparseMatrix<SCALAR> & HMat){
  MPI_Comm & workcomm = HMat.comm;

    int mpirank;
    MPI_Comm_rank(workcomm,&mpirank);
  
    int mpisize;
    MPI_Comm_size(workcomm,&mpisize);

  ifstream infile;
  infile.open(filename.c_str());

  string line;
  //read xadj on the first line of the input file
  stringstream iss;
  //skip 1st line
  if(getline(infile, line)){}
  Idx colptrCnt;
  Ptr rowindCnt;
  Ptr nzvalCnt;
  if(getline(infile, line)){
    iss.str("");
    iss.clear();
    iss<<line;
    Ptr dummy;
    iss>>dummy;
    iss>>colptrCnt>>rowindCnt>>nzvalCnt;
  }
  //read from third line

  auto & n = HMat.size;
  auto & nnz = HMat.nnz;


  auto m = n;
  if(getline(infile, line))
  {
    iss.str("");
    iss.clear();
    iss<<line;
    string type;
    iss>>type;
    iss>>m>>n>>nnz;
  }

  //compute local number of columns
  Idx nlocal = (mpirank<mpisize-1)?n/mpisize:n-mpirank*(Idx)(n/mpisize);
  Idx firstNode = mpirank*(int)(n/mpisize) + 1;
  //initialize local arrays
  HMat.Local_.colptr.resize(nlocal+1);

 
  //read from 4th line
  int colptrWidth = 0;
  int rowindWidth = 0;
  int nzvalWidth = 0;
  int colptrCntPerRow = 0;
  int rowindCntPerRow = 0;
  int nzvalCntPerRow = 0;
  if(getline(infile, line))
  {
    iss.str("");
    iss.clear();
    iss<<line;
    string format;
    iss>>format;
    int dummy;
    sscanf(format.c_str(),"(%dI%d)",&colptrCntPerRow,&colptrWidth);
    iss>>format;
    sscanf(format.c_str(),"(%dI%d)",&rowindCntPerRow,&rowindWidth);
    iss>>format;
    sscanf(format.c_str(),"(%dE%d.%d)",&nzvalCntPerRow,&nzvalWidth,&dummy);
  }

  //now compute the actual number of rows
  //colptr
  {
    size_t curPos = infile.tellg();
    size_t lineLastNode = std::ceil(( double(firstNode+nlocal) / double(colptrCntPerRow)));
    size_t lineFirstNode = std::ceil(( double(firstNode) / double(colptrCntPerRow)));
    size_t skip = (firstNode - 1)*colptrWidth + (lineFirstNode - 1);
    size_t readBytes = (nlocal+1)*colptrWidth + (lineLastNode - lineFirstNode);
    size_t skipAfter = (n+1 - (firstNode+nlocal))*colptrWidth + (colptrCnt - lineLastNode +1) ;

    infile.seekg(skip,ios_base::cur);

    {
      std::string rdStr;
      rdStr.resize(readBytes);

      infile.read(&rdStr[0], readBytes);

      istringstream iss(rdStr);
      Ptr j;
      Idx locPos = 0;
      while(iss>> j){
        HMat.Local_.colptr[locPos++]=j;
      }
    }

    infile.seekg(skipAfter,ios_base::cur);
    size_t curEnd = infile.tellg();
  }

  //convert to local colptr and compute local nnz
  Ptr first_idx = HMat.Local_.colptr.front();
  Ptr last_idx = HMat.Local_.colptr.back();
  for(Idx i=nlocal+1;i>=1;i--){
    HMat.Local_.colptr[i-1] = HMat.Local_.colptr[i-1] - HMat.Local_.colptr[0] + 1;
  }
  Ptr nnzLocal = HMat.Local_.colptr.back()-1;
  
  HMat.Local_.rowind.resize(nnzLocal);

  Ptr elem_idx;
  //rowind
  {
    size_t curPos = infile.tellg();
    size_t lineLastEdge = std::ceil(( double(last_idx-1) / double(rowindCntPerRow)));
    size_t lineFirstEdge = std::ceil(( double(first_idx) / double(rowindCntPerRow)));
    size_t skip = (first_idx - 1)*rowindWidth + (lineFirstEdge - 1);
    size_t readBytes = (last_idx - first_idx)*rowindWidth + (lineLastEdge - lineFirstEdge);
    size_t skipAfter = (nnz+1 - last_idx)*rowindWidth + (rowindCnt - lineLastEdge +1) ;

    infile.seekg(skip,ios_base::cur);

    {
      std::string rdStr;
      rdStr.resize(readBytes);

      infile.read(&rdStr[0], readBytes);
      istringstream iss(rdStr);
      Idx j;
      Ptr locPos = 0;
      while(iss>> j){
        HMat.Local_.rowind[locPos++]=j;
      }
    }

    infile.seekg(skipAfter,ios_base::cur);
    size_t curEnd = infile.tellg();
  }


  HMat.nzvalLocal.resize(nnzLocal);

  //nzval
  {
    size_t curPos = infile.tellg();
    size_t lineLastEdge = std::ceil(( double(last_idx-1) / double(nzvalCntPerRow)));
    size_t lineFirstEdge = std::ceil(( double(first_idx) / double(nzvalCntPerRow)));
    size_t skip = (first_idx - 1)*nzvalWidth + (lineFirstEdge - 1);
    size_t readBytes = (last_idx - first_idx)*nzvalWidth + (lineLastEdge - lineFirstEdge);
    size_t skipAfter = (nnz+1 - last_idx)*nzvalWidth + (nzvalCnt - lineLastEdge +1) ;

    infile.seekg(skip,ios_base::cur);

    {
      std::string rdStr;
      rdStr.resize(readBytes);

      infile.read(&rdStr[0], readBytes);
//      logfileptr->OFS()<<"nzval read string is"<<endl<<rdStr<<endl;

      istringstream iss(rdStr);
      INSCALAR j;
      Ptr locPos = 0;
      while(iss>> j){
        HMat.nzvalLocal[locPos++]=(SCALAR)j;
      }
    }

    infile.seekg(skipAfter,ios_base::cur);
    size_t curEnd = infile.tellg();
  }


  infile.close();


  HMat.globalAllocated = false;
  HMat.Local_.size = HMat.size;
  HMat.Local_.nnz = nnzLocal;

  return 0;
}


//template <>
//int ReadHB_PARA<double,double>(std::string & filename, DistSparseMatrix<double> & HMat);


template <typename SCALAR, typename INSCALAR >
void ReadMatrix(std::string & filename, std::string & informatstr,  DistSparseMatrix<SCALAR> & HMat){
  MPI_Comm & workcomm = HMat.comm;
  if(iam==0){ cout<<"Start reading the matrix"<<endl; }
  TIMER_START(READING_MATRIX);
  double tstart = get_time();
  //Read the input matrix
  if(informatstr == "CSC"){
    ParaReadDistSparseMatrix( filename.c_str(), HMat, workcomm ); 
  }
  else{
#if 0
#if 1
    sparse_matrix_file_format_t informat;
    int n,nnz;
    int * colptr, * rowind;
    INSCALAR * values;
    if(iam==0){

      informat = sparse_matrix_file_format_string_to_enum (informatstr.c_str());
      sparse_matrix_t* Atmp = load_sparse_matrix (informat, filename.c_str());
      sparse_matrix_convert (Atmp, CSC);
      const csc_matrix_t * cscptr = (const csc_matrix_t *) Atmp->repr;

      n = cscptr->n;
      nnz = cscptr->nnz;
      colptr = cscptr->colptr;
      rowind = cscptr->rowidx;
      values = (INSCALAR *)cscptr->values;

      MPI_Bcast(&n,sizeof(n),MPI_BYTE,0,workcomm);
      MPI_Bcast(&nnz,sizeof(n),MPI_BYTE,0,workcomm);
      MPI_Bcast(colptr,sizeof(int)*(n+1),MPI_BYTE,0,workcomm);
      MPI_Bcast(rowind,sizeof(int)*(nnz),MPI_BYTE,0,workcomm);
      MPI_Bcast(values,sizeof(INSCALAR)*(nnz),MPI_BYTE,0,workcomm);

      HMat.ConvertData(n,nnz,colptr,rowind,values);
      destroy_sparse_matrix (Atmp);
    }
    else{
      MPI_Bcast(&n,sizeof(n),MPI_BYTE,0,workcomm);
      MPI_Bcast(&nnz,sizeof(nnz),MPI_BYTE,0,workcomm);

      //allocate 
      colptr = new int[n+1];
      rowind = new int[nnz];
      values = new INSCALAR[nnz];

      MPI_Bcast(colptr,sizeof(int)*(n+1),MPI_BYTE,0,workcomm);
      MPI_Bcast(rowind,sizeof(int)*(nnz),MPI_BYTE,0,workcomm);
      MPI_Bcast(values,sizeof(INSCALAR)*(nnz),MPI_BYTE,0,workcomm);

      HMat.ConvertData(n,nnz,colptr,rowind,values);

      delete [] values;
      delete [] rowind;
      delete [] colptr;
    }
#endif
#else
    ReadHB_PARA<SCALAR,INSCALAR>(filename, HMat);
    //ReadHB_PARA_MPIIO<SCALAR,INSCALAR>(filename, HMat);
#endif
  }
  double tstop = get_time();
  TIMER_STOP(READING_MATRIX);
  if(iam==0){ cout<<"Matrix read time: "<<tstop - tstart<<endl; }
  if(iam==0){ cout<<"Matrix order is "<<HMat.size<<endl; }
}


 



} // namespace SYMPACK

#endif





////namespace SYMPACK{
////  template <typename T> void CSCToCSR(SparseMatrixStructure& sparseA, SparseMatrixStructure & sparseB, MPI_Comm & comm ){
////
////
////    Int mpirank;
////    MPI_Comm_rank(comm,&mpirank);
////  
////    Int mpisize;
////    MPI_Comm_size(comm,&mpisize);
////
////    Int numRowLocalFirst = sparseA.size / mpisize;
////    Int firstRow = mpirank * numRowLocalFirst;
////    Int numRowLocal = -1;
////    Int nnzLocal = -1;
////
////    sparseB.size = sparseA.size;
////    sparseB.nnz = sparseA.nnz;
////
////    LongInt nnz = 0;
////    SYMPACK::vector<Int> rowindGlobal;
////    SYMPACK::vector<Int> colptrGlobal;
////    //TIMER_START(ToGlobalStructure);
////    {
////      colptrGlobal.resize(sparseA.size+1);
////
////      /* Allgatherv for row indices. */ 
////      SYMPACK::vector<Int> prevnz(mpisize);
////      SYMPACK::vector<Int> rcounts(mpisize);
////      MPI_Allgather(&sparseA.nnz, sizeof(sparseA.nnz), MPI_BYTE, &rcounts[0], sizeof(sparseA.nnz), MPI_BYTE, comm);
////
////      prevnz[0] = 0;
////      for (Int i = 0; i < mpisize-1; ++i) { prevnz[i+1] = prevnz[i] + rcounts[i]; } 
////
////      nnz = 0;
////      for (Int i = 0; i < mpisize; ++i) { nnz += rcounts[i]; } 
////      rowindGlobal.resize(nnz);
////      for (Int i = 0; i < mpisize; ++i) { prevnz[i] *= sizeof(Idx); } 
////      for (Int i = 0; i < mpisize; ++i) { rcounts[i] *= sizeof(Idx); } 
////
////      MPI_Allgatherv(&sparseA.rowindLocal[0], sparseA.nnz*sizeof(Idx), MPI_BYTE, &rowindGlobal[0],&rcounts[0], &prevnz[0], MPI_BYTE, comm); 
////
////      /* Allgatherv for colptr */
////      // Compute the number of columns on each processor
////      Int numColFirst = std::max(1,sparseA.size / mpisize);
////      fill(rcounts.begin(),rcounts.end(),numColFirst);
////      rcounts[mpisize-1] = sparseA.size - numColFirst * (mpisize-1);  // Modify the last entry     
////
////      SYMPACK::vector<Int> rdispls(mpisize);
////      rdispls[0] = 0;
////      for (Int i = 0; i < mpisize-1; ++i) { rdispls[i+1] = rdispls[i] + rcounts[i]; } 
////
////      for (Int i = 0; i < mpisize; ++i) { rdispls[i] *= sizeof(Ptr); } 
////      for (Int i = 0; i < mpisize; ++i) { rcounts[i] *= sizeof(Ptr); } 
////}
////
////}
////}









#endif // _UTILITY_HPP_
