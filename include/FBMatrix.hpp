/// @file FBMatrix.hpp
/// @brief Matrix structure and methods for the FAN-BOTH method.
/// @author Mathias Jacquelin
/// @date 2014-01-21
#ifndef _FBMATRIX_HPP_ 
#define _FBMATRIX_HPP_

#include "Environment.hpp"
#include "NumVec.hpp"
#include "NumMat.hpp"
#include "DistSparseMatrix.hpp"

#include <vector>

#ifndef MAP
#define MAP modwrap2D
#endif

using namespace std;

namespace LIBCHOLESKY{



  class FBMatrix{
    public:

      //Parameters
      Int blksize;

      std::vector<DblNumMat *> AchunkLower;
      std::vector<DblNumMat *> WLower;
  
      //lock should be initialized with the number of contribution a block column is receiving
      IntNumVec AggLock;

      Int n;
      Int pcol, prow;
      Int np, iam;

      virtual FBMatrix();
      virtual ~FBMatrix();


      inline Int row2D(Int i, Int j) {return (i/blksize)%np;}
      inline Int col2D(Int i, Int j) {return (j/blksize)%np;}

      inline Int chevron2D(Int i, Int j) {return (min(i,j)/blksize)%np;}
      inline Int antichevron2D(Int i, Int j) {return (max(i,j)/blksize)%np;}

      inline Int diag2D(Int i, Int j) {return (abs(i-j)/blksize)%np;}
      inline Int antidiag2D(Int i, Int j) {return ((i+j)/blksize)%np;}


      inline Int modwrap2D(Int i, Int j) {return min(i/blksize,j/blksize)%prow + prow*floor((double)(max(i/blksize,j/blksize)%np)/(double)prow);}
      inline Int modwrap2Dns(Int i, Int j) {return(i/blksize)%prow + prow*floor((double)((j/blksize)%np)/(double)prow);}

      inline Int global_col_to_local(Int j){ return ((j)/(pcol*blksize))*blksize; }

      virtual void Allocate(Int np, Int pn, Int pblksize);

      virtual void Distribute( DblNumMat & Aorig) = 0;
//      virtual void DistributeSparse( DistSparseMatrix<double> & Aorig) = 0;

      virtual void Gather( DblNumMat & Adest){};

      virtual void WaitFactorization();

      virtual void NumericalFactorization() = 0;



      void Aggregate(Int j, DblNumMat &DistW);

      void Factor(Int j);
      void Factor_ref(Int j);

      void Update(Int j, Int i, DblNumMat & Factor);



  };




}






////namespace LIBCHOLESKY{
////
////
////  class FBMatrix;
////
////#ifdef UPCXX
////  void Gathercopy(upcxx::global_ptr<FBMatrix> Objptr, global_ptr<double> Adest, Int j);
////
////  void Update_Async(upcxx::global_ptr<FBMatrix> Aptr, Int j, upcxx::global_ptr<double> remoteFactorPtr);
////  void Aggregate_Async(upcxx::global_ptr<FBMatrix> Aptr, Int j, global_ptr<double> remoteAggregatePtr);
////
////  void Factor_Async(upcxx::global_ptr<FBMatrix> Aptr, Int j);
////#else
////
////  class MPIGrid{
////    public:
////    // Data
////    MPI_Comm    comm;
////    MPI_Comm    rowComm;
////    MPI_Comm    colComm;
////    Int         mpirank;
////    Int         mpisize; 
////    Int         numProcRow;
////    Int         numProcCol;
////
////    // Member function
////    MPIGrid( MPI_Comm Bcomm, int nprow, int npcol );
////    ~MPIGrid();
////  };
////
////#endif
////
////
////  class FBMatrix{
////    public:
////
////      //Parameters
////      Int blksize;
////      Int outstdAggreg=0;
////      Int outstdUpdate=0;
////      Int prefetch;
////
////      std::vector<DblNumMat> AchunkLower;
////      std::vector<DblNumMat> WLower;
////  
////      //lock should be initialized with the number of contribution a block column is receiving
////      IntNumVec AggLock;
////
////      Int n;
////      Int pcol, prow;
////      Int np, iam;
////
////#ifdef UPCXX
////      std::vector< upcxx::global_ptr<FBMatrix> > RemoteObjPtrs;
////#else
////      MPIGrid * mpigrid;
////#endif
////
////      FBMatrix();
////#ifdef UPCXX
////      void Initialize(upcxx::shared_array<upcxx::global_ptr<FBMatrix> > * RemoteObjPtr);
////#else
////      void Initialize(MPIGrid & grid);
////#endif
////      ~FBMatrix();
////
////
////      inline Int row2D(Int i, Int j) {return (i/blksize)%np;}
////      inline Int col2D(Int i, Int j) {return (j/blksize)%np;}
////
////      inline Int chevron2D(Int i, Int j) {return (min(i,j)/blksize)%np;}
////      inline Int antichevron2D(Int i, Int j) {return (max(i,j)/blksize)%np;}
////
////      inline Int diag2D(Int i, Int j) {return (abs(i-j)/blksize)%np;}
////      inline Int antidiag2D(Int i, Int j) {return ((i+j)/blksize)%np;}
////
////
////      inline Int modwrap2D(Int i, Int j) {return min(i/blksize,j/blksize)%prow + prow*floor((double)(max(i/blksize,j/blksize)%np)/(double)prow);}
////      inline Int modwrap2Dns(Int i, Int j) {return(i/blksize)%prow + prow*floor((double)((j/blksize)%np)/(double)prow);}
////
////      inline Int global_col_to_local(Int j){ return ((j)/(pcol*blksize))*blksize; }
////
////      void Allocate(Int pn, Int pblksize);
////
////      void Distribute( DblNumMat & Aorig);
////
////      void Gather( DblNumMat & Adest);
////
////      void Aggregate(Int j, DblNumMat &DistW);
////
////      void Factor(Int j);
////
////      void Update(Int j, Int i, DblNumMat & Factor);
////
////      void WaitFactorization();
////
////      void NumericalFactorization();
////
////  };
////
////
////
////
////}




#endif
