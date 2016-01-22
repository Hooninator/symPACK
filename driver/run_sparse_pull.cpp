/// @file run_sparse_fanboth.cpp
/// @brief Test for the interface of SuperLU_DIST and SelInv.
/// @author Mathias Jacquelin
/// @date 2014-01-23
//#define _DEBUG_


#include <mpi.h>

#include <time.h>
#include <omp.h>

#include  "sympack.hpp"
#include  "sympack/SupernodalMatrix2.hpp"

#include  "sympack/sp_blas.hpp"
#include  "sympack/CommTypes.hpp"
#include  "sympack/Ordering.hpp"

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

#include <upcxx.h>

#define SCALAR double
//#define SCALAR std::complex<double>
#define INSCALAR double


using namespace SYMPACK;


int main(int argc, char **argv) 
{
  MPI_Init(&argc,&argv);
  upcxx::init(&argc, &argv);

  NGCholOptions optionsFact;

  MPI_Comm worldcomm;
  MPI_Comm_dup(MPI_COMM_WORLD,&worldcomm);
  //  int np, iam;
  MPI_Comm_size(worldcomm,&np);
  MPI_Comm_rank(worldcomm,&iam);


#if defined(PROFILE) || defined(PMPI)
  TAU_PROFILE_INIT(argc, argv);
  //TAU_PROFILE_SET_CONTEXT(worldcomm);
#endif




  logfileptr = new LogFile(iam);
  logfileptr->OFS()<<"********* LOGFILE OF P"<<iam<<" *********"<<endl;
  logfileptr->OFS()<<"**********************************"<<endl;

#ifdef TRACK_PROGRESS
  char suffix[50];
  sprintf(suffix,"%d",iam);
  progressptr = new LogFile("progress",suffix);
  progstr = new stringstream();
#endif


  // *********************************************************************
  // Input parameter
  // *********************************************************************
  std::map<std::string,std::vector<std::string> > options;

  OptionsCreate(argc, argv, options);

  std::string filename;
  if( options.find("-in") != options.end() ){
    filename= options["-in"].front();
  }
  std::string informatstr;
  if( options.find("-inf") != options.end() ){
    informatstr= options["-inf"].front();
  }

  Int maxSnode = -1;
  if( options.find("-b") != options.end() ){
    maxSnode= atoi(options["-b"].front().c_str());
  }

  optionsFact.relax.SetMaxSize(maxSnode);//  maxSnode= atoi(options["-b"].c_str());
  if( options.find("-relax") != options.end() ){
    if(options["-relax"].size()==3){
      optionsFact.relax.SetNrelax0(atoi(options["-relax"][0].c_str()));//  maxSnode= atoi(options["-b"].c_str());
      optionsFact.relax.SetNrelax1(atoi(options["-relax"][1].c_str()));//  maxSnode= atoi(options["-b"].c_str());
      optionsFact.relax.SetNrelax2(atoi(options["-relax"][2].c_str()));//  maxSnode= atoi(options["-b"].c_str());
    }
    else{
      //disable relaxed supernodes
      optionsFact.relax.SetNrelax0(-1);
    }
  }

  Int maxIsend = 0;
  if( options.find("-is") != options.end() ){
    maxIsend= atoi(options["-is"].front().c_str());
  }

  Int doFB = 0;
  if( options.find("-fb") != options.end() ){
    doFB= atoi(options["-fb"].front().c_str());
  }


  if( options.find("-lb") != options.end() ){
      optionsFact.load_balance_str = options["-lb"].front();
//    if(options["-lb"]=="NNZ"){
//      optionsFact.load_balance = NNZ;
//    }
//    else if(options["-lb"]=="FLOPS"){
//      optionsFact.load_balance = FLOPS;
//    }
//    else if(options["-lb"]=="SUBCUBE"){
//      optionsFact.load_balance = SUBCUBE;
//    }
//    else if(options["-lb"]=="SUBCUBE_NNZ"){
//      optionsFact.load_balance = SUBCUBE_NNZ;
//    }
//    else{
//      optionsFact.load_balance = NOLB;
//    }
  }

  if( options.find("-ordering") != options.end() ){
    if(options["-ordering"].front()=="AMD"){
      optionsFact.ordering = AMD;
    }
    else if(options["-ordering"].front()=="NDBOX"){
      optionsFact.ordering = NDBOX;
    }
    else if(options["-ordering"].front()=="NDGRID"){
      optionsFact.ordering = NDGRID;
    }
#ifdef USE_SCOTCH
    else if(options["-ordering"].front()=="SCOTCH"){
      optionsFact.ordering = SCOTCH;
    }
#endif
#ifdef USE_METIS
    else if(options["-ordering"].front()=="METIS"){
      optionsFact.ordering = METIS;
    }
#endif
#ifdef USE_PARMETIS
    else if(options["-ordering"].front()=="PARMETIS"){
      optionsFact.ordering = PARMETIS;
    }
#endif
#ifdef USE_PTSCOTCH
    else if(options["-ordering"].front()=="PTSCOTCH"){
      optionsFact.ordering = PTSCOTCH;
    }
#endif
    else{
      optionsFact.ordering = MMD;
    }
  }

  if( options.find("-scheduler") != options.end() ){
    if(options["-scheduler"].front()=="MCT"){
      optionsFact.scheduler = MCT;
    }
    if(options["-scheduler"].front()=="FIFO"){
      optionsFact.scheduler = FIFO;
    }
    else if(options["-scheduler"].front()=="PR"){
      optionsFact.scheduler = PR;
    }
    else{
      optionsFact.scheduler = DL;
    }
  }



  Int maxIrecv = 0;
  if( options.find("-ir") != options.end() ){
    maxIrecv= atoi(options["-ir"].front().c_str());
  }


  Real timeSta, timeEnd;


  if( options.find("-map") != options.end() ){

    optionsFact.mappingTypeStr = options["-map"].front();

//    if(options["-map"] == "Modwrap2D"){
//      optionsFact.mappingType = MODWRAP2D;
//    }
//    else if(options["-map"] == "Modwrap2DNS"){
//      optionsFact.mappingType = MODWRAP2DNS;
//    }
//    else if(options["-map"] == "Wrap2D"){
//      optionsFact.mappingType = WRAP2D;
//    }
//    else if(options["-map"] == "Wrap2DForced"){
//      optionsFact.mappingType = WRAP2DFORCED;
//    }
//    else if(options["-map"] == "Row2D"){
//      optionsFact.mappingType = ROW2D;
//    }
//    else if(options["-map"] == "Col2D"){
//      optionsFact.mappingType = COL2D;
//    }
//    else{
//      optionsFact.mappingType = MODWRAP2D;
//    }
  }
  else{
    optionsFact.mappingTypeStr = "MODWRAP2D";
//    optionsFact.mappingType = MODWRAP2D;
  }

  Int all_np = np;
  np = optionsFact.used_procs(np);

  MPI_Comm workcomm;
  MPI_Comm_split(worldcomm,iam<np,iam,&workcomm);

  upcxx::team * workteam;
  Int new_rank = (iam<np)?iam:iam-np;
  upcxx::team_all.split(iam<np,new_rank, workteam);
  
      logfileptr->OFS()<<"ALL SPLITS ARE DONE"<<endl;


  if(iam<np){
    //  int np, iam;
    MPI_Comm_size(workcomm,&np);
    MPI_Comm_rank(workcomm,&iam);



    sparse_matrix_file_format_t informat;
    TIMER_START(READING_MATRIX);
    DistSparseMatrix<SCALAR> HMat(workcomm);
    //Read the input matrix
    if(informatstr == "CSC"){
      ParaReadDistSparseMatrix( filename.c_str(), HMat, workcomm ); 
    }
    else{
      informat = sparse_matrix_file_format_string_to_enum (informatstr.c_str());
      sparse_matrix_t* Atmp = load_sparse_matrix (informat, filename.c_str());
      sparse_matrix_convert (Atmp, CSC);
      const csc_matrix_t * cscptr = (const csc_matrix_t *) Atmp->repr;
      HMat.ConvertData(cscptr->n,cscptr->nnz,cscptr->colptr,cscptr->rowidx,(INSCALAR *)cscptr->values);
      destroy_sparse_matrix (Atmp);
    }

    TIMER_STOP(READING_MATRIX);
    if(iam==0){ cout<<"Matrix order is "<<HMat.size<<endl; }


#ifdef _CHECK_RESULT_

    Int nrhs = 1;
    NumMat<SCALAR> RHS;
    NumMat<SCALAR> XTrue;

    Int n = HMat.size;


    RHS.Resize(n,nrhs);
    XTrue.Resize(n,nrhs);

    //SetValue(XTrue,1.0);
    Int val = 1.0;
    for(Int i = 0; i<n;++i){ 
      for(Int j=0;j<nrhs;++j){
        XTrue(i,j) = val;
        val = -val;
      }
    }
    //        UniformRandom(XTrue);

    if(iam==0){
      cout<<"Starting spGEMM"<<endl;
    }

    timeSta = get_time();

    {
      SparseMatrixStructure Local = HMat.GetLocalStructure();
      SparseMatrixStructure Global;
      Local.ToGlobal(Global,workcomm);
      Global.ExpandSymmetric();

      Int numColFirst = std::max(1,n / np);

      SetValue(RHS,(SCALAR)0.0);
      for(Int j = 1; j<=n; ++j){
        Int iOwner = std::min((j-1)/numColFirst,np-1);
        if(iam == iOwner){
          Int iLocal = (j-(numColFirst)*iOwner);
          //do I own the column ?
          SCALAR t = XTrue(j-1,0);
          //do a dense mat mat mul ?
          for(Int ii = Local.colptr[iLocal-1]; ii< Local.colptr[iLocal];++ii){
            Int row = Local.rowind[ii-1];
            RHS(row-1,0) += t*HMat.nzvalLocal[ii-1];
            if(row>j){
              RHS(j-1,0) += XTrue(row-1,0)*HMat.nzvalLocal[ii-1];
            }
          }
        }
      }
      //Do a reduce of RHS
      MPI_Allreduce(MPI_IN_PLACE,&RHS(0,0),RHS.Size(),MPI_DOUBLE,MPI_SUM,workcomm);
    }

    timeEnd = get_time();
    if(iam==0){
      cout<<"spGEMM time: "<<timeEnd-timeSta<<endl;
    }

#endif


    if(iam==0){
      cout<<"Starting allocation"<<endl;
    }


    NumMat<SCALAR> XFinal;
    {
      //do the symbolic factorization and build supernodal matrix
      optionsFact.maxSnode = maxSnode;
      optionsFact.maxIsend = maxIsend;
      optionsFact.maxIrecv = maxIrecv;

        optionsFact.factorization = FANBOTH;

      optionsFact.commEnv = new CommEnvironment(workcomm);
      SupernodalMatrix2<SCALAR>*  SMat;

      try{
        timeSta = get_time();
        SMat = new SupernodalMatrix2<SCALAR>(HMat,optionsFact);
        timeEnd = get_time();
        SMat->team_ = workteam;
      }
      catch(const std::bad_alloc& e){
        std::cout << "Allocation failed: " << e.what() << '\n';
        SMat = NULL;
        abort();
      }

      if(iam==0){
        cout<<"Allocation time: "<<timeEnd-timeSta<<endl;
      }

      if(iam==0){
        cout<<"Starting Factorization"<<endl;
      }

      //SMat->Dump();

#ifndef _NO_COMPUTATION_
      timeSta = get_time();
      TIMER_START(FACTORIZATION);
      SMat->Factorize();
      TIMER_STOP(FACTORIZATION);
      timeEnd = get_time();

      if(iam==0){
        cout<<"Factorization time: "<<timeEnd-timeSta<<endl;
      }
      logfileptr->OFS()<<"Factorization time: "<<timeEnd-timeSta<<endl;

#ifdef _CHECK_RESULT_
      //sort X the same way (permute rows)
      XFinal = RHS;

      if(iam==0){
        cout<<"Starting solve"<<endl;
      }

      timeSta = get_time();
      SMat->Solve(&XFinal);
      timeEnd = get_time();

      if(iam==0){
        cout<<"Solve time: "<<timeEnd-timeSta<<endl;
      }

      SMat->GetSolution(XFinal);
      //  SMat->Dump();
#endif

#endif

      delete SMat;

    }


#ifndef _NO_COMPUTATION_
#ifdef _CHECK_RESULT_
    {
      SparseMatrixStructure Local = HMat.GetLocalStructure();

      Int numColFirst = std::max(1,n / np);

      NumMat<SCALAR> AX(n,nrhs);
      SetValue(AX,(SCALAR)0.0);

      for(Int j = 1; j<=n; ++j){
        Int iOwner = std::min((j-1)/numColFirst,np-1);
        if(iam == iOwner){
          Int iLocal = (j-(numColFirst)*iOwner);
          //do I own the column ?
          SCALAR t = XFinal(j-1,0);
          //do a dense mat mat mul ?
          for(Int ii = Local.colptr[iLocal-1]; ii< Local.colptr[iLocal];++ii){
            Int row = Local.rowind[ii-1];
            AX(row-1,0) += t*HMat.nzvalLocal[ii-1];
            if(row>j){
              AX(j-1,0) += XFinal(row-1,0)*HMat.nzvalLocal[ii-1];
            }
          }
        }
      }
      //Do a reduce of RHS
      MPI_Allreduce(MPI_IN_PLACE,&AX(0,0),AX.Size(),MPI_DOUBLE,MPI_SUM,workcomm);

      if(iam==0){
        blas::Axpy(AX.m()*AX.n(),-1.0,&RHS(0,0),1,&AX(0,0),1);
        double normAX = lapack::Lange('F',AX.m(),AX.n(),&AX(0,0),AX.m());
        double normRHS = lapack::Lange('F',RHS.m(),RHS.n(),&RHS(0,0),RHS.m());
        cout<<"Norm of residual after SPCHOL is "<<normAX/normRHS<<std::endl;
      }
    }
#endif
#endif

    delete optionsFact.commEnv;
  }
  else{
//    gdb_lock();
//    //missing barrier at the end of FanBoth()
//    upcxx::barrier();
  }

  MPI_Barrier(workcomm);
  MPI_Comm_free(&workcomm);

  MPI_Barrier(worldcomm);
  MPI_Comm_free(&worldcomm);

#ifdef TRACK_PROGRESS

  progressptr->OFS() << progstr->str();
  delete progressptr;
  delete progstr;
#endif

  delete logfileptr;

#ifdef MAP_PROFILING
  //LOCK FOR PROFILING
  string end;
  cin>>end;
  //gdb_lock();
#endif


  upcxx::finalize();
#if 0
  int finalized = 0;
  MPI_Finalized(&finalized);
  if(!finalized){ 
    MPI_Finalize();
  }
#endif
  return 0;
}


