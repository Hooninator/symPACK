/*
   "symPACK" Copyright (c) 2016, The Regents of the University of California,
   through Lawrence Berkeley National Laboratory (subject to receipt of any
   required approvals from the U.S. Dept. of Energy).  All rights reserved.

   Redistribution and use in source and binary forms, with or without
   modification, are permitted provided that the following conditions are met:

   (1) Redistributions of source code must retain the above copyright notice,
   this list of conditions and the following disclaimer.

   (2) Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

   (3) Neither the name of the University of California, Lawrence Berkeley
   National Laboratory, U.S. Dept. of Energy nor the names of its contributors
   may be used to endorse or promote products derived from this software without
   specific prior written permission.

   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
   AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
   IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
   DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE
   FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
   DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
   SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
   CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
   OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
   OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
   Copyright (c) 2016 The Regents of the University of California,
   through Lawrence Berkeley National Laboratory.  

Author: Mathias Jacquelin

This file is part of symPACK. All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

(1) Redistributions of source code must retain the above copyright notice, this
list of conditions and the following disclaimer.
(2) Redistributions in binary form must reproduce the above copyright notice,
this list of conditions and the following disclaimer in the documentation
and/or other materials provided with the distribution.
(3) Neither the name of the University of California, Lawrence Berkeley
National Laboratory, U.S. Dept. of Energy nor the names of its contributors may
be used to endorse or promote products derived from this software without
specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON
ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

You are under no obligation whatsoever to provide any bug fixes, patches, or
upgrades to the features, functionality or performance of the source code
("Enhancements") to anyone; however, if you choose to make your Enhancements
available either publicly, or directly to Lawrence Berkeley National
Laboratory, without imposing a separate written license agreement for such
Enhancements, then you hereby grant the following license: a non-exclusive,
royalty-free perpetual license to install, use, modify, prepare derivative
works, incorporate into other computer software, distribute, and sublicense
such enhancements or derivative works thereof, in binary and source code form.
 */
#ifndef _SYMPACK_MATRIX2D_DECL_HPP_
#define _SYMPACK_MATRIX2D_DECL_HPP_

#include "sympack/Environment.hpp"
#include "sympack/symPACKMatrixBase.hpp"
#include "sympack/symPACKMatrix.hpp"

#include "sympack/mpi_interf.hpp"

//#define LOCK_SRC 2

#define _COMPACT_DEPENDENCIES_

//#define _MEMORY_LIMIT_
#define _PRIORITY_QUEUE_AVAIL_

#include <functional>

namespace symPACK{
  namespace scheduling {
    //cell row, cell col, src snode, op_type
    //using key_t = std::tuple<Idx,Idx,Idx,Factorization::op_type>;
    struct key_t { 
      unsigned int cell_J; 
      unsigned int cell_I;
      unsigned int src;
      Factorization::op_type type;
      key_t(unsigned int j, unsigned int i, unsigned int s, Factorization::op_type t){
        cell_J=j;
        cell_I=i;
        src=s;
        type=t;
      }
    };
  }
}

// custom specialization of std::hash can be injected in namespace std
namespace std
{
  inline bool operator==(const symPACK::scheduling::key_t& lhs, const symPACK::scheduling::key_t& rhs)
  {
    return lhs.cell_J==rhs.cell_J && lhs.cell_I == rhs.cell_I && lhs.src == rhs.src && lhs.type == rhs.type;
  }

  template<> struct hash<symPACK::scheduling::key_t>
  {
    typedef symPACK::scheduling::key_t argument_type;
    typedef std::size_t result_type;
    result_type operator()(argument_type const& s) const noexcept
    {
      //this is inspired by https://gist.github.com/nsuke/5990643
      result_type seed = 0;
      result_type const h1 ( std::hash<unsigned int>{}(s.cell_J) );
      seed ^= h1 + 0x9e3779b9 + (seed<<6) + (seed>>2);
      result_type const h2 ( std::hash<unsigned int>{}(s.cell_I) );
      seed ^= h2 + 0x9e3779b9 + (seed<<6) + (seed>>2);
      result_type const h3 ( std::hash<unsigned int>{}(s.src) );
      seed ^= h3 + 0x9e3779b9 + (seed<<6) + (seed>>2);
      result_type const h4 ( std::hash<int>{}((int)s.type) );
      seed ^= h4 + 0x9e3779b9 + (seed<<6) + (seed>>2);
      return seed; // or use boost::hash_combine (see Discussion)
    }
  };
}


namespace symPACK{


#ifdef NEW_UPCXX



  class blockCellBase_t {
    using intrank_t = upcxx::intrank_t;
    public:
        intrank_t owner;
        int i;
        int j;
      blockCellBase_t():owner(0),i(0),j(0){}
      virtual int I(){return i;}
  };

  namespace scheduling {

    template <typename ttask_t, typename tmeta_t>
      class incoming_data_t;

    //(from I, to J,Factorization::op_type, updated first col, facing row));
    using meta_t = std::tuple<Idx,Idx,Factorization::op_type,Idx,Idx>;
    using depend_t = std::tuple<Int,Int>; //(I,J)

    template
      < typename tmeta_t = meta_t,
      typename tdepend_t = depend_t >
        class task_t{
          public:
            //cell description
            using depend_task_t = tdepend_t;
            using meta_t = tmeta_t;
            using data_t = incoming_data_t<task_t,tmeta_t>;

            meta_t _meta;

            std::function< void() > execute;

            //byte storage for tasks 
#ifdef _COMPACT_DEPENDENCIES_
            std::vector< int > out_dependencies;
#else
            std::vector< depend_task_t > out_dependencies;
#endif
            //need counter here as same REMOTE cell can be input to many tasks.
            std::vector< std::shared_ptr< data_t > > input_msg;

            //in fact we only need the SIZE, the counts
            //TODO REMOVE THIS
            int in_remote_dependencies_cnt;
            int in_local_dependencies_cnt;

            //promise to sync all outgoing RPCs
            upcxx::promise<> out_prom;
            //promise to wait for all incoming_data_t to be created
            upcxx::promise<> in_avail_prom;
            //promise to sync on all incoming_data_t fetch
            upcxx::promise<> in_prom;

            bool executed;

            task_t( ):executed(false),
            in_remote_dependencies_cnt(0),
            in_local_dependencies_cnt(0){ }

            ~task_t(){
              //bassert(out_prom.get_future().ready());
              //bassert(in_prom.get_future().ready());
              //bassert(in_avail_prom.get_future().ready());
            }

            void reset(){
              //bassert(out_prom.get_future().ready());
              //bassert(in_prom.get_future().ready());
              //bassert(in_avail_prom.get_future().ready());
              out_prom = upcxx::promise<>();
              in_prom = upcxx::promise<>();
              in_avail_prom = upcxx::promise<>();
            }

        };


    template <typename ttask_t = task_t<meta_t, depend_t> , typename tmeta_t = task_t<meta_t, depend_t>::meta_t>
      class incoming_data_t {
        public:
          upcxx::promise<incoming_data_t *> on_fetch;

          using task_t = ttask_t;
          using meta_t = tmeta_t;
          //this should be made generic, not specific to sparse matrices
          std::vector< task_t *> target_tasks;

          meta_t in_meta;
          //a pointer to be used by the user if he wants to attach some data
          //void * extra_data;
          std::unique_ptr<blockCellBase_t> extra_data;
          //blockCellBase_t * extra_data;

          incoming_data_t():transfered(false),size(0),extra_data(nullptr),landing_zone(nullptr){};
          bool transfered;
          upcxx::global_ptr<char> remote_gptr;
          size_t size;
          char * landing_zone;

          void allocate(){
            if(landing_zone == nullptr) {
//	           struct rusage bous, eous;
//           getrusage(RUSAGE_SELF, &bous);
//           logfileptr->OFS()<<size<<"bytes Before "<<bous.ru_maxrss<<" KiB used"<<std::endl; 
              landing_zone = new char[size];
//           getrusage(RUSAGE_SELF, &eous);
//           logfileptr->OFS()<<size<<"bytes After "<<eous.ru_maxrss<<" KiB used"<<std::endl; 
            }
          }

          upcxx::future<incoming_data_t *> fetch(){
            if(!transfered){
              transfered=true;
              upcxx::rget(remote_gptr,landing_zone,size).then([this](){
                  on_fetch.fulfill_result(this);
                  return;});
            }
            return on_fetch.get_future();
          }

          ~incoming_data_t(){
            if (landing_zone) {
              delete [] landing_zone;
            }
            //if ( extra_data ) {
            //  delete extra_data;
            //}
          }

      };

    template <typename bin_label_t = Int, typename ttask_t = task_t<meta_t, depend_t> >
      class task_graph_t: public std::unordered_map< bin_label_t,  std::deque< ttask_t * > > {
      };

    template <typename label_t = key_t, typename ttask_t = task_t<meta_t, depend_t> >
      class task_graph_t2: public std::unordered_map< label_t,  std::unique_ptr<ttask_t> > {
      };

    template <typename ttask_t = task_t<meta_t, depend_t> , typename ttaskgraph_t = task_graph_t<Int,task_t<meta_t, depend_t> > >
      class Scheduler2D{
        public:
          int sp_handle;
          std::list<ttask_t*> ready_tasks;

#ifdef _PRIORITY_QUEUE_AVAIL_
           struct avail_comp{
             bool operator()(ttask_t *& a,ttask_t*& b){ return a->out_dependencies.size() > b->out_dependencies.size();};
           };
          std::priority_queue<ttask_t*,std::vector<ttask_t*>,avail_comp> avail_tasks;
#else
          std::list<ttask_t*> avail_tasks;
#endif
          void execute(ttaskgraph_t & graph, double & mem_budget );
      };

  }

  extern std::map<int, symPACKMatrixBase *  > g_sp_handle_to_matrix;

  template <typename colptr_t, typename rowind_t, typename T, typename int_t = int> 
    class blockCell_t: public blockCellBase_t {
      protected:
        using intrank_t = upcxx::intrank_t;

      public:
        class block_t {
          public:
            //this is just an offset from the begining of the cell;
            size_t offset;
            rowind_t first_row;
        };

        virtual int I(){return i;}
//        int_t i;
//        int_t j;
        //intrank_t owner;
        rowind_t first_col;
        std::tuple<rowind_t> _dims;
        upcxx::global_ptr< char > _gstorage;
        char * _storage;
        T* _nzval;
        //#ifndef _NDEBUG_
        size_t _cblocks; //capacity
        size_t _cnz; //capacity
        //#endif
        size_t _nnz;
        size_t _storage_size;
        rowind_t _total_rows;


        class block_container_t {
          public:
            block_t * _blocks;
            size_t _nblocks;

            block_container_t():_blocks(nullptr),_nblocks(0){};

            size_t size() const{
              return _nblocks;
            }

            block_t * data(){
              return _blocks;
            }

            class iterator
            {
              public:
                typedef iterator self_type;
                typedef block_t value_type;
                typedef block_t& reference;
                typedef block_t* pointer;
                typedef std::forward_iterator_tag iterator_category;
                typedef int difference_type;
                iterator(pointer ptr) : ptr_(ptr) { }
                self_type operator++() { self_type i = *this; ptr_++; return i; }
                self_type operator++(int junk) { ptr_++; return *this; }
                reference operator*() { return *ptr_; }
                pointer operator->() { return ptr_; }
                bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
                bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
              private:
                pointer ptr_;
            };

            class const_iterator
            {
              public:
                typedef const_iterator self_type;
                typedef block_t value_type;
                typedef block_t& reference;
                typedef block_t* pointer;
                typedef int difference_type;
                typedef std::forward_iterator_tag iterator_category;
                const_iterator(pointer ptr) : ptr_(ptr) { }
                self_type operator++() { self_type i = *this; ptr_++; return i; }
                self_type operator++(int junk) { ptr_++; return *this; }
                const reference operator*() { return *ptr_; }
                const pointer operator->() { return ptr_; }
                bool operator==(const self_type& rhs) { return ptr_ == rhs.ptr_; }
                bool operator!=(const self_type& rhs) { return ptr_ != rhs.ptr_; }
              private:
                pointer ptr_;
            };


            iterator begin()
            {
              return iterator(_blocks);
            }

            iterator end()
            {
              return iterator(_blocks+_nblocks);
            }

            const_iterator begin() const
            {
              return const_iterator(_blocks);
            }

            const_iterator end() const
            {
              return const_iterator(_blocks+_nblocks);
            }

            block_t& operator[](size_t index)
            {
              bassert(index < _nblocks);
              return _blocks[index];
            }

            const block_t& operator[](size_t index) const
            {
              bassert(index < _nblocks);
              return _blocks[index];
            }



        };

        block_container_t _block_container;




        blockCell_t(): //i(0),j(0),
        first_col(0),_dims(std::make_tuple(0)),_total_rows(0),
        _storage(nullptr),_nzval(nullptr),//_blocks(nullptr),_nblocks(0),
        _gstorage(nullptr),_nnz(0),_storage_size(0), _own_storage(true){}
        bool _own_storage;

        ~blockCell_t() {
          //gdb_lock();
          if (_own_storage) {
            if ( !_gstorage.is_null() ) {
              bassert(_storage == _gstorage.local());
              upcxx::deallocate( _gstorage );
            }
            else {
              delete [] _storage;
            }
          }
        }

        rowind_t total_rows(){
          if(this->_block_container.size()>0 && _total_rows==0){
            for( auto & block: this->_block_container){ _total_rows += block_nrows(block);}
          }
          return _total_rows;
        }

        blockCell_t ( int_t i, int_t j, rowind_t firstcol, rowind_t width, size_t nzval_cnt, size_t block_cnt, bool shared_segment = true  ): blockCell_t() {
          this->i = i;
          this->j = j;
          _dims = std::make_tuple(width);
          first_col = firstcol;
          allocate(nzval_cnt,block_cnt, shared_segment);
          initialize(nzval_cnt,block_cnt);
        }

        blockCell_t (  int_t i, int_t j,char * ext_storage, rowind_t firstcol, rowind_t width, size_t nzval_cnt, size_t block_cnt ): blockCell_t() {
          this->i = i;
          this->j = j;
          _own_storage = false;
          _gstorage = nullptr;
          _storage = ext_storage;
          _dims = std::make_tuple(width);
          first_col = firstcol;
          initialize(nzval_cnt,block_cnt);
          _nnz = nzval_cnt;
          _block_container._nblocks = block_cnt;
        }

        blockCell_t (  int_t i, int_t j,upcxx::global_ptr<char> ext_gstorage, rowind_t firstcol, rowind_t width, size_t nzval_cnt, size_t block_cnt ): blockCell_t() {
          this->i = i;
          this->j = j;
          _own_storage = false;
          _gstorage = ext_gstorage;
          _storage = _gstorage.local();
          _dims = std::make_tuple(width);
          first_col = firstcol;
          initialize(nzval_cnt,block_cnt);
          _nnz = nzval_cnt;
          _block_container._nblocks = block_cnt;
        }

        // Copy constructor.  
        blockCell_t ( const blockCell_t & other ): blockCell_t() {
          i           = other.i;
          j           = other.j;
          owner       = other.owner;
          first_col   = other.first_col;
          _dims       = other._dims;
          _total_rows = other._total_rows;
          _own_storage = other._own_storage;

          allocate( other.nz_capacity(), other.block_capacity() , !other._gstorage.is_null() );
          //now copy the data
          std::copy( other._storage, other._storage + other._storage_size, _storage );
          _block_container._nblocks = other._block_container._nblocks;
          _nnz = other._nnz;
        }

        // Move constructor.  
        blockCell_t ( const blockCell_t && other ): blockCell_t() {
          i           = other.i;
          j           = other.j;
          owner       = other.owner;
          first_col   = other.first_col;
          _dims       = other._dims;
          _total_rows = other._total_rows;
          _own_storage = other._own_storage;

          _gstorage = other._gstorage;
          _storage = other._storage;
          initialize( other._cnz , other._cblocks );

          //invalidate other
          other._gstorage = nullptr;
          other._storage = nullptr;
          other._storage_size = 0;
          other._cblocks = 0;
          other._cnz = 0;
          other._nnz = 0;
          other._nzval = nullptr;
          other._block_container._nblocks = 0;
          other._block_container._blocks = nullptr;
          other._own_storage = false;
        }




        // Copy assignment operator.  
        blockCell_t& operator=(const blockCell_t& other)  {  
          if (this != &other)  
          {  
            // Free the existing resource.  
            if ( ! _gstorage.is_null() ) {
              upcxx::deallocate( _gstorage );
            }
            else {
              delete [] _storage;
            }

            i           = other.i;
            j           = other.j;
            owner       = other.owner;
            first_col   = other.first_col;
            _dims       = other._dims;
            _total_rows = other._total_rows;
            _own_storage = other._own_storage;

            allocate( other._cnz, other._cblocks , !other._gstorage.is_null() );
            //now copy the data
            std::copy( other._storage, other._storage + other._storage_size, _storage );
            _block_container._nblocks = other._block_container._nblocks;
            _nnz = other._nnz;

          } 
          return *this;  
        }  

        // Move assignment operator.  
        blockCell_t& operator=(const blockCell_t&& other)  {  
          if (this != &other)  
          {  
            // Free the existing resource.  
            if ( !_gstorage.is_null() ) {
              upcxx::deallocate( _gstorage );
            }
            else {
              delete [] _storage;
            }

            i           = other.i;
            j           = other.j;
            owner       = other.owner;
            first_col   = other.first_col;
            _dims       = other._dims;
            _total_rows = other._total_rows;
            _own_storage = other._own_storage;

            _gstorage = other._gstorage;
            _storage = other._storage;
            initialize( other._cnz , other._cblocks );

            //invalidate other
            other._gstorage = nullptr;
            other._storage = nullptr;
            other._storage_size = 0;
            other._cblocks = 0;
            other._cnz = 0;
            other._nnz = 0;
            other._nzval = nullptr;        
            other._block_container._nblocks = 0;
            other._block_container._blocks = nullptr;
            other._own_storage = false;
          } 
          return *this;  
        }  




        inline int_t block_capacity() const { return _cblocks; }
        inline int_t nblocks() const { return _block_container.size(); }
        inline const block_container_t & blocks() const { return _block_container; }
        inline int_t nz_capacity() const { return _cnz; }
        inline int_t nnz() const { return _nnz; }
        inline int_t width() const { return std::get<0>(_dims); }


        //THIS IS VERY DANGEROUS
        inline rowind_t block_nrows(const block_t & block) const{
          auto blkidx = &block - _block_container._blocks;
          bassert(blkidx<nblocks());
          size_t end = (blkidx<nblocks()-1)?_block_container[blkidx+1].offset:_nnz;
          rowind_t nRows = (end-block.offset)/width();
          return nRows;
        }

        inline rowind_t block_nrows(const int_t blkidx) const{
          bassert(blkidx<nblocks());
          size_t end = (blkidx<nblocks()-1)?_block_container[blkidx+1].offset:_nnz;
          rowind_t nRows = (end-_block_container[blkidx].offset)/width();
          return nRows;
        }

        void add_block( rowind_t first_row, rowind_t nrows ){
          assert( this->_block_container.data()!=nullptr );
          assert( this->_block_container.size() + 1 <= block_capacity() );
          assert( nnz() + nrows*std::get<0>(_dims) <= nz_capacity() );

          bassert( this->_block_container.data()!=nullptr );
          bassert( this->_block_container.size() + 1 <= block_capacity() );
          bassert( nnz() + nrows*std::get<0>(_dims) <= nz_capacity() );
          auto & new_block = _block_container[_block_container._nblocks++];
          new_block.first_row = first_row;
          //new_block.nrows = nrows;
          new_block.offset = _nnz;
          _nnz += nrows*std::get<0>(_dims);
        }

        void initialize ( size_t nzval_cnt, size_t block_cnt ) {
          _storage_size = nzval_cnt*sizeof(T) + block_cnt*sizeof(block_t);
          //#ifndef _NDEBUG_
          _cnz = nzval_cnt;
          _cblocks = block_cnt;
          //#endif
          _nnz = 0;
          _block_container._nblocks = 0;
          _block_container._blocks = reinterpret_cast<block_t*>( _storage );
          _nzval = reinterpret_cast<T*>( _block_container._blocks + _cblocks );
        }

        void allocate ( size_t nzval_cnt, size_t block_cnt, bool shared_segment ) {
          bassert(nzval_cnt!=0 && block_cnt!=0);
          if ( shared_segment ) {

            _gstorage = upcxx::allocate<char>( nzval_cnt*sizeof(T) + block_cnt*sizeof(block_t) );
            _storage = _gstorage.local();
            if ( this->_storage==nullptr ) std::cout<<"Trying to allocate "<<nzval_cnt<<" for cell ("<<i<<","<<j<<")"<<std::endl;
            assert( this->_storage!=nullptr );
          }
          else {
            _gstorage = nullptr;
            _storage = new char[nzval_cnt*sizeof(T) + block_cnt*sizeof(block_t)];
          }
          initialize( nzval_cnt, block_cnt );
        }


        void print_block(const blockCell_t & block, std::string prefix) const{  
          logfileptr->OFS()<<prefix<<" ("<<block.i<<","<<block.j<<")"<<std::endl;
          logfileptr->OFS()<<prefix<<" nzval:"<<std::endl;
          for(auto & nzblock: block.blocks() ){
            logfileptr->OFS()<<nzblock.first_row<<"-|"<<block.first_col<<"---------------------"<<block.first_col+block.width()-1<<std::endl;
            for(int vrow = 0; vrow< block.block_nrows(nzblock); vrow++){
              //logfileptr->OFS()<<nzblock.first_row+vrow<<" | ";
              std::streamsize p = logfileptr->OFS().precision();
              logfileptr->OFS().precision(std::numeric_limits< T >::max_digits10);
              for(int vcol = 0; vcol< block.width() ; vcol++){
                logfileptr->OFS()<<std::scientific<<ToMatlabScalar(block._nzval[nzblock.offset+vrow*block.width()+vcol])<<" ";
              }
              logfileptr->OFS().precision(p);
              logfileptr->OFS()<<std::endl;
            }
            logfileptr->OFS()<<nzblock.first_row+block.block_nrows(nzblock)-1<<"-|"<<block.first_col<<"---------------------"<<block.first_col+block.width()-1<<std::endl;
          }
        }  




        int factorize( TempUpdateBuffers<T> & tmpBuffers){
          scope_timer(a,blockCell_t::factorize);
#if defined(_NO_COMPUTATION_)
          return 0;
#endif
          auto snode_size = std::get<0>(_dims);
          auto diag_nzval = _nzval;

          //          print_block(*this,"diag before Potrf");
          try{
            lapack::Potrf( 'U', snode_size, diag_nzval, snode_size);
          }
          catch(const std::runtime_error& e){
            std::cerr << "Runtime error: " << e.what() << '\n';
            gdb_lock();
          }

          //          print_block(*this,"diag after Potrf");
          return 0;
        }

        int trsm( const blockCell_t & diag, TempUpdateBuffers<T> & tmpBuffers){
          scope_timer(a,blockCell_t::trsm);
#if defined(_NO_COMPUTATION_)
          return 0;
#endif

          bassert(diag.nblocks()>0);
          auto diag_nzval = diag._nzval;

          auto snode_size = std::get<0>(_dims);
          auto nzblk_nzval = _nzval;
          //          print_block(diag,"diag");
          //          print_block(*this,"offdiag before Trsm");
          blas::Trsm('L','U','T','N',snode_size, total_rows(), T(1.0),  diag_nzval, snode_size, nzblk_nzval, snode_size);
          //          print_block(*this,"offdiag after Trsm");
          return 0;
        }

        int update( /*const*/ blockCell_t & pivot, /*const*/ blockCell_t & facing, TempUpdateBuffers<T> & tmpBuffers){
          scope_timer(a,blockCell_t::update);
#if defined(_NO_COMPUTATION_)
          return 0;
#endif
          //do the owner compute update first

#if 1
          {
            bassert(nblocks()>0);

            bassert(pivot.nblocks()>0);
            auto pivot_fr = pivot._block_container[0].first_row;

            bassert(facing.nblocks()>0);
            auto facing_fr = facing._block_container[0].first_row;
            auto facing_lr = facing._block_container[facing.nblocks()-1].first_row+facing.block_nrows(facing.nblocks()-1)-1;

            auto src_snode_size = std::get<0>(pivot._dims);
            auto tgt_snode_size = std::get<0>(_dims);


            //find the first row updated by src_snode
            auto tgt_fc = pivot_fr;
            auto tgt_lc = pivot._block_container[pivot._block_container._nblocks-1].first_row
              + pivot.block_nrows(pivot._block_container._nblocks-1) -1;

            int_t first_pivot_idx = 0;
            int_t last_pivot_idx = pivot.nblocks()-1;

            //determine the first column that will be updated in the target supernode
            rowind_t tgt_local_fc =  tgt_fc - this->first_col;
            rowind_t tgt_local_lc =  tgt_lc - this->first_col;

            rowind_t pivot_nrows = pivot.total_rows();
            rowind_t tgt_nrows = this->total_rows();
            rowind_t src_nrows = facing.total_rows();
            //            rowind_t src_lr = facing_fr + src_nrows-1;

            //condensed update width is the number of rows in the pivot block
            int_t tgt_width = pivot_nrows;

            T * pivot_nzval = pivot._nzval;
            T * facing_nzval = facing._nzval;
            T * tgt = this->_nzval;

            //Pointer to the output buffer of the GEMM
            T * buf = NULL;
            T beta = T(0);

            //If the target supernode has the same structure,
            //The GEMM is directly done in place
            size_t tgt_offset = 0;
            bool in_place = ( first_pivot_idx == last_pivot_idx );

            if (in_place){ 
              int tgt_first_upd_blk_idx  = 0;
              for ( ; tgt_first_upd_blk_idx < this->_block_container.size(); tgt_first_upd_blk_idx++ ){
                auto & block = this->_block_container[tgt_first_upd_blk_idx];
                if(facing_fr >= block.first_row && facing_fr <= block.first_row + this->block_nrows(block) -1)
                  break;
              }

              tgt_offset = this->_block_container[tgt_first_upd_blk_idx].offset
                + (facing_fr - this->_block_container[tgt_first_upd_blk_idx].first_row) * this->width() 
                + tgt_local_fc; 

              //find the last block updated
              int tgt_last_upd_blk_idx  = this->_block_container.size()-1;
              for ( ; tgt_last_upd_blk_idx > tgt_first_upd_blk_idx; tgt_last_upd_blk_idx-- ){
                auto & block = this->_block_container[tgt_last_upd_blk_idx];
                if(facing_lr >= block.first_row && facing_lr <= block.first_row + this->block_nrows(block) -1)
                  break;
              }
              //make sure that in between these two blocks, everything matches
              int upd_blk_cnt = tgt_last_upd_blk_idx - tgt_first_upd_blk_idx +1;
              if ( in_place && upd_blk_cnt == facing.nblocks() && upd_blk_cnt>=1) {
                //              if(this->i==86 && this->j==86 && in_place){gdb_lock();}
                for ( int blkidx = tgt_first_upd_blk_idx; blkidx <= tgt_last_upd_blk_idx; blkidx++) {
                  int facingidx = blkidx - tgt_first_upd_blk_idx;
                  int f_fr = facing._block_container[facingidx].first_row; 
                  int f_lr = facing.block_nrows(facingidx) + f_fr -1;
                  int t_fr = std::max(facing_fr, this->_block_container[blkidx].first_row); 
                  int t_lr = std::min(facing_lr, this->_block_container[blkidx].first_row+this->block_nrows(blkidx)-1); 
                  if (f_fr != t_fr || f_lr != t_lr) {
                    //                 gdb_lock();
                    in_place = false;
                    break;
                  }
                }
              }
              else {
                in_place = false;
              }
            }
            //in_place= false;

            int ldbuf = tgt_width;
            //            if(src_nrows == tgt_nrows )
            if(in_place)
            {

              buf = tgt + tgt_offset;
              beta = T(1);
              ldbuf = tgt_snode_size;
            }
            else{
              //Compute the update in a temporary buffer
#ifdef SP_THREADS
              tmpBuffers.tmpBuf.resize(tgt_width*src_nrows + src_snode_size*tgt_width);
#endif
              buf = &tmpBuffers.tmpBuf[0];
            }

            ///                print_block(pivot,"pivot");
            ///                print_block(facing,"facing");
            ///                print_block(*this,"tgt before update");

            //everything is in row-major
            SYMPACK_TIMER_SPECIAL_START(UPDATE_SNODE_GEMM);
            blas::Gemm('T','N',tgt_width, src_nrows,src_snode_size,
                T(-1.0),pivot_nzval,src_snode_size,
                facing_nzval,src_snode_size,beta,buf,ldbuf);
            SYMPACK_TIMER_SPECIAL_STOP(UPDATE_SNODE_GEMM);

            //If the GEMM wasn't done in place we need to aggregate the update
            //This is the assembly phase
            //            if(src_nrows != tgt_nrows)
            if(!in_place)
            {
              {
                SYMPACK_TIMER_SPECIAL_START(UPDATE_SNODE_INDEX_MAP);
                tmpBuffers.src_colindx.resize(tgt_width);
                tmpBuffers.src_to_tgt_offset.resize(src_nrows);
                colptr_t colidx = 0;
                colptr_t rowidx = 0;
                size_t offset = 0;

                block_t * tgt_ptr = _block_container._blocks;

                for ( auto & cur_block: facing.blocks() ) {
                  rowind_t cur_src_nrows = facing.block_nrows(cur_block);
                  rowind_t cur_src_lr = cur_block.first_row + cur_src_nrows -1;
                  rowind_t cur_src_fr = cur_block.first_row;

                  //The other one MUST reside into a single block in the target
                  rowind_t row = cur_src_fr;
                  while(row<=cur_src_lr){
                    do {
                      if (tgt_ptr->first_row <= row 
                          && row< tgt_ptr->first_row + block_nrows(*tgt_ptr) ) {
                        break;
                      }
                    } while( ++tgt_ptr<_block_container._blocks + _block_container._nblocks ); 

                    int_t lr = std::min(cur_src_lr,tgt_ptr->first_row + block_nrows(*tgt_ptr)-1);
                    int_t tgtOffset = tgt_ptr->offset + (row - tgt_ptr->first_row)*tgt_snode_size;

                    for(int_t cr = row ;cr<=lr;++cr){
                      offset+=tgt_width;
                      tmpBuffers.src_to_tgt_offset[rowidx] = tgtOffset + (cr - row)*tgt_snode_size;
                      rowidx++;
                    }
                    row += (lr-row+1);
                  }
                }

                for ( auto & cur_block: pivot.blocks() ) {
                  rowind_t cur_src_ncols = pivot.block_nrows(cur_block);
                  rowind_t cur_src_lc = std::min(cur_block.first_row + cur_src_ncols -1, this->first_col+this->width()-1);
                  rowind_t cur_src_fc = std::max(cur_block.first_row,this->first_col);

                  for (rowind_t col = cur_src_fc ;col<=cur_src_lc;++col) {
                    bassert(this->first_col <= col && col< this->first_col+this->width() );
                    tmpBuffers.src_colindx[colidx++] = col;
                  }
                }






                //Multiple cases to consider
                SYMPACK_TIMER_SPECIAL_STOP(UPDATE_SNODE_INDEX_MAP);
                SYMPACK_TIMER_SPECIAL_START(UPDATE_SNODE_ADD);
                if(first_pivot_idx==last_pivot_idx){
                  // Updating contiguous columns
                  rowind_t tgt_offset = (tgt_fc - this->first_col);
                  for (rowind_t rowidx = 0; rowidx < src_nrows; ++rowidx) {
                    T * A = &buf[rowidx*tgt_width];
                    T * B = &tgt[tmpBuffers.src_to_tgt_offset[rowidx] + tgt_offset];
#pragma unroll
                    for(rowind_t i = 0; i < tgt_width; ++i){ B[i] += A[i]; }
                  }
                }
                else{
                  // full sparse case
                  for (rowind_t rowidx = 0; rowidx < src_nrows; ++rowidx) {
                    for (colptr_t colidx = 0; colidx< tmpBuffers.src_colindx.size();++colidx) {
                      rowind_t col = tmpBuffers.src_colindx[colidx];
                      rowind_t tgt_colidx = col - this->first_col;
                      tgt[tmpBuffers.src_to_tgt_offset[rowidx] + tgt_colidx] 
                        += buf[rowidx*tgt_width+colidx]; 
                    }
                  }
                }


                SYMPACK_TIMER_SPECIAL_STOP(UPDATE_SNODE_ADD);
              }
            }

            //              print_block(*this,"tgt after update");
          }
#else


          bassert(nblocks()>0);
          bassert(pivot.nblocks()>0);
          bassert(facing.nblocks()>0);

          auto facing_fr = facing._block_container[0].first_row;
          auto facing_lr = facing._block_container[facing.nblocks()-1].first_row+facing.block_nrows(facing.nblocks()-1)-1;

          auto src_snode_size = pivot.width();
          auto tgt_snode_size = this->width();

          bool in_place = pivot.nblocks()==1;

          //find the first row updated by src_snode
          auto tgt_fc = pivot._block_container[0].first_row;
          auto tgt_lc = pivot._block_container[pivot._block_container._nblocks-1].first_row
            + pivot.block_nrows(pivot._block_container._nblocks-1]) -1;
          //determine the first column that will be updated in the target supernode
          rowind_t tgt_local_fc =  tgt_fc - this->first_col;
          rowind_t tgt_local_lc =  tgt_lc - this->first_col;

          rowind_t pivot_nrows = pivot.total_rows();
          rowind_t src_nrows = facing.total_rows();

          //condensed update width is the number of rows in the pivot block
          int_t tgt_width = pivot_nrows;

          T * pivot_nzval = pivot._nzval;
          T * facing_nzval = facing._nzval;
          T * tgt = this->_nzval;

          //Pointer to the output buffer of the GEMM
          T * buf = NULL;
          T beta = T(0);


          int tgt_first_upd_blk_idx  = 0;
          for ( ; tgt_first_upd_blk_idx < this->_block_container.size(); tgt_first_upd_blk_idx++ ){
            auto & block = this->_block_container[tgt_first_upd_blk_idx];
            if(facing_fr >= block.first_row && facing_fr <= block.first_row + block_nrows(block) -1)
              break;
          }

          //find the last block updated
          int tgt_last_upd_blk_idx  = this->_block_container.size()-1;
          for ( ; tgt_last_upd_blk_idx > tgt_first_upd_blk_idx; tgt_last_upd_blk_idx-- ){
            auto & block = this->_block_container[tgt_last_upd_blk_idx];
            if(facing_lr >= block.first_row && facing_lr <= block.first_row + block_nrows(block) -1)
              break;
          }

          //offset to the first updated element in the target nzval array
          size_t tgt_offset = this->_block_container[tgt_first_upd_blk_idx].offset
            + (facing_fr - this->_block_container[tgt_first_upd_blk_idx].first_row) * this->width() 
            + tgt_local_fc; 

          //If the target supernode has the same structure,
          //The GEMM is directly done in place
          if (in_place){ 
            //make sure that in between these two blocks, everything matches
            int upd_blk_cnt = tgt_last_upd_blk_idx - tgt_first_upd_blk_idx +1;
            in_place = upd_blk_cnt == facing.nblocks();
            if ( in_place ) {
              for ( int blkidx = tgt_first_upd_blk_idx; blkidx <= tgt_last_upd_blk_idx; blkidx++) {
                int facingidx = blkidx - tgt_first_upd_blk_idx;
                int f_fr = facing._block_container[facingidx].first_row; 
                int f_lr = facing.block_nrows(facingidx) + f_fr -1;
                int t_fr = std::max(facing_fr, this->_block_container[blkidx].first_row); 
                int t_lr = std::min(facing_lr, this->_block_container[blkidx].first_row+this->block_nrows(blkidx)-1); 
                if (f_fr != t_fr || f_lr != t_lr) {
                  in_place = false;
                  break;
                }
              }
            }
          }

          int ldbuf = tgt_width;
          if(in_place)
          {
            buf = tgt + tgt_offset;
            beta = T(1);
            ldbuf = tgt_snode_size;
          }
          else{
            //Compute the update in a temporary buffer
#ifdef SP_THREADS
            tmpBuffers.tmpBuf.resize(tgt_width*src_nrows + src_snode_size*tgt_width);
#endif
            buf = &tmpBuffers.tmpBuf[0];
          }

          //everything is in row-major
          SYMPACK_TIMER_SPECIAL_START(UPDATE_SNODE_GEMM);
          blas::Gemm('T','N',tgt_width, src_nrows,src_snode_size,
              T(-1.0),pivot_nzval,src_snode_size,
              facing_nzval,src_snode_size,beta,buf,ldbuf);
          SYMPACK_TIMER_SPECIAL_STOP(UPDATE_SNODE_GEMM);

          if (!in_place) {
            for ( int facingidx = 0; facingidx < facing.nblocks(); facingidx++){
              auto & facing_blk = facing._block_container[facingidx];

              int blkidx = 0;
              for ( blkidx = tgt_first_upd_blk_idx; blkidx<= tgt_last_upd_blk_idx; blkidx++){
                if ( this->_block_container[blkidx].first_row <= facing_blk.first_row &&
                    this->_block_container[blkidx].first_row + this->block_nrows(blkidx) >=
                    facing_blk.first_row + facing.block_nrows(facing_blk))
                  break;
              }


              //                auto blkidx = tgt_first_upd_blk_idx;
              //                while ( this->_block_container[blkidx].first_row 
              //                      + this->_block_container[blkidx].nrows -1 < facing_blk.first_row) { blkidx++;}


              auto & tgt_blk = this->_block_container[blkidx];
              bassert( tgt_blk.first_row <= facing_blk.first_row && facing_blk.first_row + facing.block_nrows(facing_blk) <= tgt_blk.first_row + block_nrows(tgt_blk));

              int f_fr = facing_blk.first_row; 
              int f_lr = facing.block_nrows(facing_blk) + f_fr -1;
              int t_fr = std::max(facing_fr, tgt_blk.first_row); 

              size_t row_offset = (f_fr - t_fr)*tgt_snode_size;

              int local_src_row = (facing_blk.offset - facing._block_container[0].offset)/src_snode_size;

              for ( auto row = 0; row < facing.block_nrows(facing_blk) ; row++ ) {

                for ( auto & pivot_blk: pivot.blocks() ) {
                  size_t col_offset = pivot_blk.first_row - this->first_col;
                  size_t t_offset = row_offset+ row*tgt_snode_size + col_offset;

                  int local_src_col = (pivot_blk.offset - pivot._block_container[0].offset)/src_snode_size;
                  size_t s_offset = (local_src_row+row)*ldbuf + local_src_col;

                  blas::Axpy( pivot.block_nrows(pivot_blk),1.0,buf+s_offset, ldbuf,tgt+t_offset,tgt_snode_size);
                }
              }
            }

          }



#endif

          return 0;
        }
    };


#if 1
  template <typename colptr_t, typename rowind_t, typename T, typename int_t = int> 
    class blockCellLDL_t: public blockCell_t<colptr_t,rowind_t,T,int_t> {
      protected:
        T * _diag;
        rowind_t first_row;

      public:
        using block_t = typename blockCell_t<colptr_t,rowind_t, T>::block_t;

        blockCellLDL_t():blockCell_t<colptr_t,rowind_t, T>(),_diag(nullptr) {
        }

        ~blockCellLDL_t() {
        }

        blockCellLDL_t ( rowind_t firstrow, rowind_t firstcol, rowind_t width, size_t nzval_cnt, size_t block_cnt, bool shared_segment = true  ): blockCell_t<colptr_t,rowind_t, T>() {
          this->_dims = std::make_tuple(width);
          this->first_col = firstcol;
          this->first_row = firstrow;
          this->allocate(nzval_cnt,block_cnt, shared_segment);
          this->initialize(nzval_cnt,block_cnt);
        }

        blockCellLDL_t ( char * ext_storage, rowind_t firstrow, rowind_t firstcol, rowind_t width, size_t nzval_cnt, size_t block_cnt ): blockCell_t<colptr_t,rowind_t, T>() {
          this->_gstorage = nullptr;
          this->_storage = ext_storage;
          this->_dims = std::make_tuple(width);
          this->first_col = firstcol;
          this->first_row = firstrow;
          this->initialize(nzval_cnt,block_cnt);
          this->_nnz = nzval_cnt;
          this->_block_container._nblocks = block_cnt;
        }

        blockCellLDL_t ( upcxx::global_ptr<char> ext_gstorage, rowind_t firstrow, rowind_t firstcol, rowind_t width, size_t nzval_cnt, size_t block_cnt ): blockCell_t<colptr_t,rowind_t, T>() {
          this->_gstorage = ext_gstorage;
          this->_storage = this->_gstorage.local();
          this->_dims = std::make_tuple(width);
          this->first_col = firstcol;
          this->first_row = firstrow;
          this->initialize(nzval_cnt,block_cnt);
          this->_nnz = nzval_cnt;
          this->_block_container._nblocks = block_cnt;
        }

        // Copy constructor.  
        blockCellLDL_t ( const blockCellLDL_t & other ): blockCellLDL_t() {
          this->i           = other.i;
          this->j           = other.j;
          this->owner       = other.owner;
          this->first_col   = other.first_col;
          this->_dims       = other._dims;
          this->_total_rows = other._total_rows;
          this->first_row   = other.first_row;

          allocate( other.nz_capacity(), other.block_capacity() , !other._gstorage.is_null() );
          //now copy the data
          std::copy( other._storage, other._storage + other._storage_size, this->_storage );
          this->_block_container._nblocks = other._block_container._nblocks;
          this->_nnz = other._nnz;

        }

        // Move constructor.  
        blockCellLDL_t ( const blockCellLDL_t && other ): blockCellLDL_t() {
          this->i           = other.i;
          this->j           = other.j;
          this->owner       = other.owner;
          this->first_col   = other.first_col;
          this->_dims       = other._dims;
          this->_total_rows = other._total_rows;
          this->first_row   = other.first_row;

          this->_gstorage = other._gstorage;
          this->_storage = other._storage;
          initialize( other._cnz , other._cblocks );

          //invalidate other
          other._gstorage = nullptr;
          other._storage = nullptr;
          other._storage_size = 0;
          other._cblocks = 0;
          other._cnz = 0;
          other._nnz = 0;
          other._nzval = nullptr;
          other._diag = nullptr;        
          other._block_container._nblocks = 0;
          other._block_container._blocks = nullptr;
        }

        // Copy assignment operator.  
        blockCellLDL_t& operator=(const blockCellLDL_t& other)  {  
          if (this != &other)  
          {  
            // Free the existing resource.  
            if ( ! this->_gstorage.is_null() ) {
              upcxx::deallocate( this->_gstorage );
            }
            else {
              delete [] this->_storage;
            }

            this->i           = other.i;
            this->j           = other.j;
            this->owner       = other.owner;
            this->first_col   = other.first_col;
            this->first_row   = other.first_row;
            this->_dims       = other._dims;
            this->_total_rows = other._total_rows;

            allocate( other._cnz, other._cblocks , !other._gstorage.is_null() );
            //now copy the data
            std::copy( other._storage, other._storage + other._storage_size, this->_storage );
            this->_block_container._nblocks = other._block_container._nblocks;
            this->_nnz = other._nnz;
          } 
          return *this;  
        }  

        // Move assignment operator.  
        blockCellLDL_t& operator=(const blockCellLDL_t&& other)  {  
          if (this != &other)  
          {  
            // Free the existing resource.  
            if ( !this->_gstorage.is_null() ) {
              upcxx::deallocate( this->_gstorage );
            }
            else {
              delete [] this->_storage;
            }

            this->i           = other.i;
            this->j           = other.j;
            this->owner       = other.owner;
            this->first_col   = other.first_col;
            this->first_row   = other.first_row;
            this->_dims       = other._dims;
            this->_total_rows = other._total_rows;

            this->_gstorage = other._gstorage;
            this->_storage = other._storage;
            initialize( other._cnz , other._cblocks );

            //invalidate other
            other._gstorage = nullptr;
            other._storage = nullptr;
            other._storage_size = 0;
            other._cblocks = 0;
            other._cnz = 0;
            other._nnz = 0;
            other._nzval = nullptr;        
            other._diag = nullptr;        
            other._block_container._nblocks = 0;
            other._block_container._blocks = nullptr;

          } 
          return *this;  
        }  

        void initialize ( size_t nzval_cnt, size_t block_cnt ) {
          ((blockCell_t<colptr_t,rowind_t, T>*)this)->initialize(nzval_cnt, block_cnt);

          if ( this->first_row == this->first_col ) {
            this->_storage_size += this->width()*sizeof(T);
          }

          this->_diag = reinterpret_cast<T*>( this->_nzval + nzval_cnt );
        }

        void allocate ( size_t nzval_cnt, size_t block_cnt, bool shared_segment ) {
          bassert(nzval_cnt!=0 && block_cnt!=0);
          if ( shared_segment ) {
            if ( first_row == this->first_col ) {
              this->_gstorage = upcxx::allocate<char>( nzval_cnt*sizeof(T) + block_cnt*sizeof(block_t) + this->width()*sizeof(T) );
            }
            else{
              this->_gstorage = upcxx::allocate<char>( nzval_cnt*sizeof(T) + block_cnt*sizeof(block_t) );
            }
            this->_storage = this->_gstorage.local();
          }
          else {
            this->_gstorage = nullptr;
            if ( first_row == this->first_col ) {
              this->_storage = new char[nzval_cnt*sizeof(T) + block_cnt*sizeof(block_t) + this->width()*sizeof(T)];
            }
            else {
              this->_storage = new char[nzval_cnt*sizeof(T) + block_cnt*sizeof(block_t)];
            }
          }
          initialize( nzval_cnt, block_cnt );
        }


        void print_block(const blockCellLDL_t & block, std::string prefix) const{  
          logfileptr->OFS()<<prefix<<" ("<<block.i<<","<<block.j<<")"<<std::endl;
          logfileptr->OFS()<<prefix<<" nzval:"<<std::endl;
          for(auto & nzblock: block.blocks() ){
            logfileptr->OFS()<<nzblock.first_row<<"-|"<<block.first_col<<"---------------------"<<block.first_col+block.width()-1<<std::endl;
            for(int vrow = 0; vrow< block.block_nrows(nzblock); vrow++){
              //logfileptr->OFS()<<nzblock.first_row+vrow<<" | ";
              std::streamsize p = logfileptr->OFS().precision();
              logfileptr->OFS().precision(std::numeric_limits< T >::max_digits10);
              for(int vcol = 0; vcol< block.width() ; vcol++){
                logfileptr->OFS()<<std::scientific<<ToMatlabScalar(block._nzval[nzblock.offset+vrow*block.width()+vcol])<<" ";
              }
              logfileptr->OFS().precision(p);
              logfileptr->OFS()<<std::endl;
            }
            logfileptr->OFS()<<nzblock.first_row+block.block_nrows(nzblock)-1<<"-|"<<block.first_col<<"---------------------"<<block.first_col+block.width()-1<<std::endl;
          }
        }  




        int factorize( TempUpdateBuffers<T> & tmpBuffers){
          scope_timer(a,blockCellLDL_t::factorize);
#if defined(_NO_COMPUTATION_)
          return 0;
#endif
          auto snode_size = std::get<0>(this->_dims);
          auto diag_nzval = this->_nzval;

          //          print_block(*this,"diag before Potrf");
          int_t INFO = 0;
          try{
            lapack::Potrf_LDL( "U", snode_size, diag_nzval, snode_size, tmpBuffers, INFO);
            //copy the diagonal entries into the diag portion of the supernode
#pragma unroll
            for(int_t col = 0; col< snode_size; col++){ this->_diag[col] = diag_nzval[col+ (col)*snode_size]; }
          }
          catch(const std::runtime_error& e){
            std::cerr << "Runtime error: " << e.what() << '\n';
            gdb_lock();
          }
          //          print_block(*this,"diag after Potrf");
          return 0;
        }

        int trsm( const blockCellLDL_t & diag, TempUpdateBuffers<T> & tmpBuffers){
          scope_timer(a,blockCellLDL_t::trsm);
#if defined(_NO_COMPUTATION_)
          return 0;
#endif

          bassert(diag.nblocks()>0);
          auto diag_nzval = diag._nzval;

          auto snode_size = std::get<0>(this->_dims);
          auto nzblk_nzval = this->_nzval;
          //          print_block(diag,"diag");
          //          print_block(*this,"offdiag before Trsm");
          blas::Trsm('L','U','T','U',snode_size, this->total_rows(), T(1.0),  diag_nzval, snode_size, nzblk_nzval, snode_size);

          //scale column I
          for ( int_t I = 1; I<=snode_size; I++) {
            blas::Scal( this->total_rows(), T(1.0)/this->_diag[I-1], &nzblk_nzval[I-1], snode_size );
          }
          //          print_block(*this,"offdiag after Trsm");
          return 0;
        }

        int update( T* diag, /*const*/ blockCellLDL_t & pivot, /*const*/ blockCellLDL_t & facing, TempUpdateBuffers<T> & tmpBuffers){
          scope_timer(a,blockCellLDL_t::update);
#if defined(_NO_COMPUTATION_)
          return 0;
#endif
          //do the owner compute update first

          {
            bassert(this->nblocks()>0);

            bassert(pivot.nblocks()>0);
            auto pivot_fr = pivot._block_container[0].first_row;

            bassert(facing.nblocks()>0);
            auto facing_fr = facing._block_container[0].first_row;
            auto facing_lr = facing._block_container[facing.nblocks()-1].first_row+facing.block_nrows(facing.nblocks()-1)-1;

            auto src_snode_size = std::get<0>(pivot._dims);
            auto tgt_snode_size = std::get<0>(this->_dims);


            //find the first row updated by src_snode
            auto tgt_fc = pivot_fr;
            auto tgt_lc = pivot._block_container[pivot._block_container._nblocks-1].first_row
              + pivot.block_nrows(pivot._block_container._nblocks-1) -1;

            int_t first_pivot_idx = 0;
            int_t last_pivot_idx = pivot.nblocks()-1;

            //determine the first column that will be updated in the target supernode
            rowind_t tgt_local_fc =  tgt_fc - this->first_col;
            rowind_t tgt_local_lc =  tgt_lc - this->first_col;

            rowind_t pivot_nrows = pivot.total_rows();
            rowind_t tgt_nrows = this->total_rows();
            rowind_t src_nrows = facing.total_rows();
            //            rowind_t src_lr = facing_fr + src_nrows-1;

            //condensed update width is the number of rows in the pivot block
            int_t tgt_width = pivot_nrows;

            T * pivot_nzval = pivot._nzval;
            T * facing_nzval = facing._nzval;
            T * tgt = this->_nzval;

            //Pointer to the output buffer of the GEMM
            T * buf = NULL;
            T beta = T(0);
            T * bufLDL = NULL;
            //If the target supernode has the same structure,
            //The GEMM is directly done in place
            size_t tgt_offset = 0;
            bool in_place = ( first_pivot_idx == last_pivot_idx );

            if (in_place){ 
              int tgt_first_upd_blk_idx  = 0;
              for ( ; tgt_first_upd_blk_idx < this->_block_container.size(); tgt_first_upd_blk_idx++ ){
                auto & block = this->_block_container[tgt_first_upd_blk_idx];
                if(facing_fr >= block.first_row && facing_fr <= block.first_row + this->block_nrows(block) -1)
                  break;
              }

              tgt_offset = this->_block_container[tgt_first_upd_blk_idx].offset
                + (facing_fr - this->_block_container[tgt_first_upd_blk_idx].first_row) * this->width() 
                + tgt_local_fc; 

              //find the last block updated
              int tgt_last_upd_blk_idx  = this->_block_container.size()-1;
              for ( ; tgt_last_upd_blk_idx > tgt_first_upd_blk_idx; tgt_last_upd_blk_idx-- ){
                auto & block = this->_block_container[tgt_last_upd_blk_idx];
                if(facing_lr >= block.first_row && facing_lr <= block.first_row + this->block_nrows(block) -1)
                  break;
              }
              //make sure that in between these two blocks, everything matches
              int upd_blk_cnt = tgt_last_upd_blk_idx - tgt_first_upd_blk_idx +1;
              if ( in_place && upd_blk_cnt == facing.nblocks() && upd_blk_cnt>=1) {
                //              if(this->i==86 && this->j==86 && in_place){gdb_lock();}
                for ( int blkidx = tgt_first_upd_blk_idx; blkidx <= tgt_last_upd_blk_idx; blkidx++) {
                  int facingidx = blkidx - tgt_first_upd_blk_idx;
                  int f_fr = facing._block_container[facingidx].first_row; 
                  int f_lr = facing.block_nrows(facingidx) + f_fr -1;
                  int t_fr = std::max(facing_fr, this->_block_container[blkidx].first_row); 
                  int t_lr = std::min(facing_lr, this->_block_container[blkidx].first_row+this->block_nrows(blkidx)-1); 
                  if (f_fr != t_fr || f_lr != t_lr) {
                    //                 gdb_lock();
                    in_place = false;
                    break;
                  }
                }
              }
              else {
                in_place = false;
              }
            }
            //in_place= false;

            int ldbuf = tgt_width;
            //            if(src_nrows == tgt_nrows )
            if(in_place)
            {
              //TODO
              buf = tgt + tgt_offset;
              beta = T(1);
              ldbuf = tgt_snode_size;
#ifdef SP_THREADS
              tmpBuffers.tmpBuf.resize(src_snode_size*tgt_width);
#endif
              bufLDL = &tmpBuffers.tmpBuf[0];
            }
            else{
              //Compute the update in a temporary buffer
#ifdef SP_THREADS
              tmpBuffers.tmpBuf.resize(tgt_width*src_nrows + src_snode_size*tgt_width);
#endif
              bufLDL = &tmpBuffers.tmpBuf[0];
              buf = bufLDL + src_snode_size*tgt_width;
            }

            ///                print_block(pivot,"pivot");
            ///                print_block(facing,"facing");
            ///                print_block(*this,"tgt before update");

            //everything is in row-major
            SYMPACK_TIMER_SPECIAL_START(UPDATE_SNODE_GEMM);
            blas::Gemm('T','N',tgt_width, src_nrows,src_snode_size,
                T(-1.0),pivot_nzval,src_snode_size,
                facing_nzval,src_snode_size,beta,buf,ldbuf);

            //First do W = DLT 
            //T * diag = src_snode->_diag;//static_cast<SuperNodeInd<T,Allocator> * >(src_snode)->GetDiag();
            for(int_t row = 0; row<src_snode_size; row++){
              for(int_t col = 0; col<tgt_width; col++){
                bufLDL[col+row*tgt_width] = diag[row]*(pivot[row+col*src_snode_size]);
              }
            }

            //Then do -L*W (gemm)
            blas::Gemm('N','N',tgt_width,src_nrows,src_snode_size,
              T(-1.0),bufLDL,tgt_width,facing_nzval,src_snode_size,beta,buf,ldbuf);


            SYMPACK_TIMER_SPECIAL_STOP(UPDATE_SNODE_GEMM);

            //If the GEMM wasn't done in place we need to aggregate the update
            //This is the assembly phase
            //            if(src_nrows != tgt_nrows)
            if(!in_place)
            {
              {
                SYMPACK_TIMER_SPECIAL_START(UPDATE_SNODE_INDEX_MAP);
                tmpBuffers.src_colindx.resize(tgt_width);
                tmpBuffers.src_to_tgt_offset.resize(src_nrows);
                colptr_t colidx = 0;
                colptr_t rowidx = 0;
                size_t offset = 0;

                block_t * tgt_ptr = this->_block_container._blocks;

                for ( auto & cur_block: facing.blocks() ) {
                  rowind_t cur_src_nrows = facing.block_nrows(cur_block);
                  rowind_t cur_src_lr = cur_block.first_row + cur_src_nrows -1;
                  rowind_t cur_src_fr = cur_block.first_row;

                  //The other one MUST reside into a single block in the target
                  rowind_t row = cur_src_fr;
                  while(row<=cur_src_lr){
                    do {
                      if (tgt_ptr->first_row <= row 
                          && row< tgt_ptr->first_row + block_nrows(*tgt_ptr) ) {
                        break;
                      }
                    } while( ++tgt_ptr<this->_block_container._blocks + this->_block_container._nblocks ); 

                    int_t lr = std::min(cur_src_lr,tgt_ptr->first_row + block_nrows(*tgt_ptr)-1);
                    int_t tgtOffset = tgt_ptr->offset + (row - tgt_ptr->first_row)*tgt_snode_size;

                    for(int_t cr = row ;cr<=lr;++cr){
                      offset+=tgt_width;
                      tmpBuffers.src_to_tgt_offset[rowidx] = tgtOffset + (cr - row)*tgt_snode_size;
                      rowidx++;
                    }
                    row += (lr-row+1);
                  }
                }

                for ( auto & cur_block: pivot.blocks() ) {
                  rowind_t cur_src_ncols = pivot.block_nrows(cur_block);
                  rowind_t cur_src_lc = std::min(cur_block.first_row + cur_src_ncols -1, this->first_col+this->width()-1);
                  rowind_t cur_src_fc = std::max(cur_block.first_row,this->first_col);

                  for (rowind_t col = cur_src_fc ;col<=cur_src_lc;++col) {
                    bassert(this->first_col <= col && col< this->first_col+this->width() );
                    tmpBuffers.src_colindx[colidx++] = col;
                  }
                }

                //Multiple cases to consider
                SYMPACK_TIMER_SPECIAL_STOP(UPDATE_SNODE_INDEX_MAP);
                SYMPACK_TIMER_SPECIAL_START(UPDATE_SNODE_ADD);
                if(first_pivot_idx==last_pivot_idx){
                  // Updating contiguous columns
                  rowind_t tgt_offset = (tgt_fc - this->first_col);
                  for (rowind_t rowidx = 0; rowidx < src_nrows; ++rowidx) {
                    T * A = &buf[rowidx*tgt_width];
                    T * B = &tgt[tmpBuffers.src_to_tgt_offset[rowidx] + tgt_offset];
#pragma unroll
                    for(rowind_t i = 0; i < tgt_width; ++i){ B[i] += A[i]; }
                  }
                }
                else{
                  // full sparse case
                  for (rowind_t rowidx = 0; rowidx < src_nrows; ++rowidx) {
                    for (colptr_t colidx = 0; colidx< tmpBuffers.src_colindx.size();++colidx) {
                      rowind_t col = tmpBuffers.src_colindx[colidx];
                      rowind_t tgt_colidx = col - this->first_col;
                      tgt[tmpBuffers.src_to_tgt_offset[rowidx] + tgt_colidx] 
                        += buf[rowidx*tgt_width+colidx]; 
                    }
                  }
                }

                SYMPACK_TIMER_SPECIAL_STOP(UPDATE_SNODE_ADD);
              }
            }

            //              print_block(*this,"tgt after update");
          }
          return 0;
        }
    };
#endif

  template <typename T> class symPACKMatrix;

  template <typename colptr_t, typename rowind_t, typename T> 
    class symPACKMatrix2D: public symPACKMatrixMeta<T>{

  
      friend class symPACKMatrix<T>;

      using snodeBlock_t = blockCell_t<colptr_t,rowind_t, T>;
      using SparseTask2D = scheduling::task_t<scheduling::meta_t, scheduling::depend_t>;

      public:
      std::vector< std::shared_ptr<snodeBlock_t> > localBlocks_;
      int nsuper;
      double mem_budget;

      //using cell_key_t = std::size_t;
      //cell_key_t coord2supidx(uint64_t i, uint64_t j){ 
      //  cell_key_t val =  (i-j) + j*(nsuper+1) - (j+1)*(j)/2;  
      //  bassert(val>=0);
      //  return val;
      //};

      using cell_key_t = std::pair<uint64_t,uint64_t>;;
      cell_key_t coord2supidx(uint64_t i, uint64_t j){ 
        cell_key_t val = std::make_pair(i,j);
        return val;
      };
      std::map<cell_key_t, std::shared_ptr<blockCellBase_t> > cells_;

      inline std::shared_ptr<blockCellBase_t> pQueryCELL (int a, int b)  { 
        auto idx = coord2supidx((a),(b)); 
        auto it = this->cells_.find(idx);
        if (it != this->cells_.end() ) { 
//          auto ptr = std::dynamic_pointer_cast<snodeBlock_t>(it->second);
//          if ( ptr == nullptr ) { gdb_lock(); }
          return it->second;
        }
        else
          return nullptr;
        }

      inline std::shared_ptr<snodeBlock_t> pQueryCELL2 (int a, int b)  { 
        auto idx = coord2supidx((a),(b)); 
        auto it = this->cells_.find(idx);
        std::shared_ptr<snodeBlock_t> ptr = nullptr;
        if (it != this->cells_.end() ){ 
          ptr = std::dynamic_pointer_cast<snodeBlock_t>(it->second);
          if ( ptr == nullptr ) { gdb_lock(); }
        }
          return ptr;
        }
      inline std::shared_ptr<snodeBlock_t> pCELL (int a, int b) { return std::static_pointer_cast<snodeBlock_t>(this->cells_[coord2supidx((a),(b))]); }
      inline snodeBlock_t & CELL (int a, int b) { auto ptr = pQueryCELL2(a,b); assert(ptr!=nullptr && (ptr->i-1==a && ptr->j-1==b)); return *ptr;  }
//      inline snodeBlock_t & CELL (int a, int b) { auto ptr = pQueryCELL(a,b); assert(ptr && (ptr->i-1==a && ptr->j-1==b)); return *std::static_pointer_cast<snodeBlock_t>(ptr);  }

      //TODO implement this
      //auto idx2coord = [nsuper](Int idx){ return 0; };

      protected:


      //TODO ideally, this should be DistSparseMatrixGraph<colptr_t,rowind_t>
#ifdef SP_THREADS
      std::map<std::thread::id,TempUpdateBuffers<T> > tmpBufs_th;
#else
      TempUpdateBuffers<T> tmpBufs;
#endif


#ifndef NO_MPI
#endif
      public:
      using TaskGraph2D = scheduling::task_graph_t2<scheduling::key_t,SparseTask2D >; 
      TaskGraph2D task_graph;
      scheduling::Scheduler2D<SparseTask2D,TaskGraph2D> scheduler;

      symPACKMatrix2D();
      ~symPACKMatrix2D();


      void DumpMatlab();

      void Init(symPACKOptions & options );
      void Init(DistSparseMatrix<T> & pMat, symPACKOptions & options );

      void SymbolicFactorization(DistSparseMatrix<T> & pMat);
      void DistributeMatrix(DistSparseMatrix<T> & pMat);


      void Factorize();



    };

  template <typename colptr_t, typename rowind_t, typename T>
    symPACKMatrix2D<colptr_t,rowind_t,T>::symPACKMatrix2D():
      symPACKMatrixMeta<T>()
  {
#ifndef NO_MPI
#endif
    g_sp_handle_to_matrix[this->sp_handle] = this;
  }

  template <typename colptr_t, typename rowind_t, typename T>
    symPACKMatrix2D<colptr_t,rowind_t,T>::~symPACKMatrix2D(){
     
    }

  template <typename colptr_t, typename rowind_t, typename T>
    void symPACKMatrix2D<colptr_t,rowind_t,T>::Init(symPACKOptions & options ){
      scope_timer(a,symPACKMatrix2D::Init);
      this->options_ = options;
      logfileptr->verbose = this->options_.verbose>0;

      this->mem_budget = this->options_.memory_limit;

#ifndef NO_MPI
      this->all_np = 0;
      MPI_Comm_size(this->options_.MPIcomm,&this->all_np);
      MPI_Comm_rank(this->options_.MPIcomm,&this->iam);
#endif

#ifdef NEW_UPCXX
      this->iam = upcxx::rank_me();
      this->all_np = upcxx::rank_n();
#endif

      this->np = this->options_.used_procs(this->all_np);


#ifndef NO_MPI
      MPI_Comm_split(this->options_.MPIcomm,this->iam<this->np,this->iam,&this->workcomm_);

      //do another split to contain P0 and all the non working processors
      if(this->all_np!=this->np){
        this->non_workcomm_ = MPI_COMM_NULL;
        MPI_Comm_split(this->options_.MPIcomm,this->iam==0||this->iam>=this->np,this->iam==0?0:this->iam-this->np+1,&this->non_workcomm_);
      }
#else
      //    upcxx::team_all.split(this->iam<this->np,new_rank, this->team_);
#endif

    } 

  template <typename colptr_t, typename rowind_t, typename T>
    void symPACKMatrix2D<colptr_t,rowind_t,T>::Init(DistSparseMatrix<T> & pMat,symPACKOptions & options ){
    } 

  template <typename colptr_t, typename rowind_t, typename T>
    void symPACKMatrix2D<colptr_t,rowind_t,T>::SymbolicFactorization(DistSparseMatrix<T> & pMat ){
      scope_timer(a,symPACKMatrix2D::SymbolicFactorization);
      std::vector<Int> cc;
      {
#ifdef _MEM_PROFILER_
        utility::scope_memprofiler m("symPACK2D_symbolic_before_mapping");
#endif
      //This has to be declared here to be able to debug ...
      std::vector<int, Mallocator<int> > xadj;
      std::vector<int, Mallocator<int> > adj;
      Idx row;
      int fc,lc,colbeg,colend,col;
      Ptr supbeg,supend,rowidx;
      Int I;

#ifndef NO_MPI
      if(this->fullcomm_!=MPI_COMM_NULL){
        MPI_Comm_free(&this->fullcomm_);
      }
      MPI_Comm_dup(pMat.comm,&this->fullcomm_);
#endif

      this->iSize_ = pMat.size;

      this->graph_ = pMat.GetLocalGraph();
      this->graph_.SetBaseval(1);
      this->graph_.SetSorted(1);
      this->graph_.ExpandSymmetric();
      logfileptr->OFS()<<"Matrix structure expanded"<<std::endl;

      SparseMatrixGraph * sgraph = NULL;
      {
        {
          double timeSta = get_time();
          scope_timer(c,symPACKMatrix2D::ordering);

          if(this->options_.orderingStr=="MMD"){
            this->options_.ordering = symPACK::MMD;
          }
          else if(this->options_.orderingStr=="RCM"){
            this->options_.ordering = symPACK::RCM;
          }
          else if(this->options_.orderingStr=="AMD"){
            this->options_.ordering = symPACK::AMD;
          }
#ifdef USE_METIS
          else if(this->options_.orderingStr=="METIS"){
            this->options_.ordering = symPACK::METIS;
          }
#endif
#ifdef USE_SCOTCH
          else if(this->options_.orderingStr=="SCOTCH"){
            this->options_.ordering = symPACK::SCOTCH;
          }
#endif
#ifdef USE_PARMETIS
          else if(this->options_.orderingStr=="PARMETIS"){
            this->options_.ordering = symPACK::PARMETIS;
          }
#endif
#ifdef USE_PTSCOTCH
          else if(this->options_.orderingStr=="PTSCOTCH"){
            this->options_.ordering = symPACK::PTSCOTCH;
          }
#endif
          else if(this->options_.orderingStr=="NATURAL"){
            this->options_.ordering = symPACK::NATURAL;
          }
          else if(this->options_.orderingStr=="USER"){
            if(this->options_.perm ==NULL){
              throw std::logic_error( "When using USER, symPACKOptions.perm must be provided.\n" );
            }
            else{
              this->options_.ordering = symPACK::USER;
            }
          }
          else{
            std::stringstream sstr;
            sstr<<"This ordering method is not supported by symPACK. Valid options are:";
            sstr<<"NATURAL MMD AMD RCM NDBOX NDGRID USER ";
#ifdef USE_SCOTCH
            sstr<<"SCOTCH ";
#endif
#ifdef USE_PTSCOTCH
            sstr<<"PTSCOTCH ";
#endif
#ifdef USE_METIS
            sstr<<"METIS ";
#endif
#ifdef USE_PARMETIS
            sstr<<"PARMETIS ";
#endif
            sstr<<std::endl;
            throw std::logic_error( sstr.str()  );

          }

          this->Order_.NpOrdering = this->options_.NpOrdering;
          switch(this->options_.ordering){
            case MMD:
              {
                sgraph = new SparseMatrixGraph();
                this->graph_.GatherStructure(*sgraph,0);
                sgraph->SetBaseval(1);
                sgraph->SetKeepDiag(0);
                this->Order_.MMD(*sgraph, this->graph_.comm);
              }
              break;

            case RCM:
              {
                sgraph = new SparseMatrixGraph();
                this->graph_.GatherStructure(*sgraph,0);
                sgraph->SetBaseval(1);
                sgraph->SetKeepDiag(0);
                this->Order_.RCM(*sgraph, this->graph_.comm);
              }
              break;

            case AMD:
              {
                sgraph = new SparseMatrixGraph();
                this->graph_.GatherStructure(*sgraph,0);
                sgraph->SetBaseval(1);
                sgraph->SetKeepDiag(0);
                this->Order_.AMD(*sgraph, this->graph_.comm);
              }
              break;

            case USER:
              {
                this->Order_.perm.resize(this->iSize_);
                //find baseval
                auto baseval = std::min_element(&this->options_.perm[0],&this->options_.perm[0]+this->iSize_);
                //now compute everything in 1 based 
                for(int i=0;i<this->Order_.perm.size();i++){this->Order_.perm[i] = this->options_.perm[i] - *baseval + 1; /*1 based*/}
                this->Order_.invp.resize(this->iSize_);
                for(int i=0;i<this->Order_.perm.size();i++){this->Order_.invp[this->Order_.perm[i]-1] = i;} 
              }
              break;

            case NDBOX:
              this->Order_.NDBOX(this->Size(), this->graph_.comm);
              break;

            case NDGRID:
              this->Order_.NDGRID(this->Size(), this->graph_.comm);
              break;

#ifdef USE_SCOTCH
            case SCOTCH:
              {

                sgraph = new SparseMatrixGraph();
                this->graph_.GatherStructure(*sgraph,0);
                sgraph->SetKeepDiag(0);
                this->Order_.SCOTCH(*sgraph, this->graph_.comm);
              }
              break;
#endif
#ifdef USE_METIS
            case METIS:
              {
                sgraph = new SparseMatrixGraph();
                this->graph_.GatherStructure(*sgraph,0);
                sgraph->SetKeepDiag(0);
                this->Order_.METIS(*sgraph, this->graph_.comm);
              }
              break;
#endif
#ifdef USE_PARMETIS
            case PARMETIS:
              {
                this->graph_.SetKeepDiag(0);
                this->Order_.PARMETIS(this->graph_);
              }
              break;
#endif
#ifdef USE_PTSCOTCH
            case PTSCOTCH:
              {
                this->graph_.SetKeepDiag(0);
                this->Order_.PTSCOTCH(this->graph_);
              }
              break;
#endif
            case NATURAL:
              this->Order_.perm.resize(this->iSize_);
              for(int i=0;i<this->Order_.perm.size();i++){this->Order_.perm[i]=i+1;} 
              this->Order_.invp = this->Order_.perm;
              break;
            default:
              {
                std::stringstream sstr;
                sstr<<"This ordering method is not supported by symPACK. Valid options are:";
                sstr<<"MMD AMD RCM NDBOX NDGRID USER ";
#ifdef USE_SCOTCH
                sstr<<"SCOTCH ";
#endif
#ifdef USE_PTSCOTCH
                sstr<<"PTSCOTCH ";
#endif
#ifdef USE_METIS
                sstr<<"METIS ";
#endif
#ifdef USE_PARMETIS
                sstr<<"PARMETIS ";
#endif
                sstr<<std::endl;
                throw std::logic_error( sstr.str()  );
                //do nothing: either natural or user provided ordering
              }
              break;
          }

          //The ordering is available on every processor of the full communicator
          double timeStop = get_time();
          if(this->iam==0 && this->options_.verbose){
            std::cout<<"Ordering time: "<<timeStop - timeSta<<std::endl;
          }
          logfileptr->OFS()<<"Ordering done"<<std::endl;
        }
      }

      if(this->options_.dumpPerm>0){
        logfileptr->OFS()<<"perm = [";
        for (auto i : this->Order_.perm){ 
          logfileptr->OFS()<<i<<" ";
        }
        logfileptr->OFS()<<"]"<<std::endl;
      }

      std::vector<Int> rc;


      if(sgraph==NULL){ 
        logfileptr->OFS()<<"copying graph"<<std::endl;
        auto graph = this->graph_;
        double timeSta = get_time();
        graph.SetKeepDiag(1);
        graph.SetBaseval(1);
        logfileptr->OFS()<<"graph copied"<<std::endl;
        //Expand to unsymmetric storage
        graph.ExpandSymmetric();
        logfileptr->OFS()<<"graph expanded"<<std::endl;

        auto backinvp = this->Order_.invp;
        graph.Permute(this->Order_.invp.data());
        logfileptr->OFS()<<"graph permuted 1/2"<<std::endl;
        this->ETree_.ConstructETree(graph,this->Order_);
        this->ETree_.PostOrderTree(this->Order_);
        logfileptr->OFS()<<"ETree computed and postordered"<<std::endl;

        std::vector<Int> relinvp;
        this->Order_.GetRelativeInvp(backinvp,relinvp);
        graph.Permute(relinvp.data());
        logfileptr->OFS()<<"graph permuted 2/2"<<std::endl;

        double timeSta_cc = get_time();
        this->getLColRowCount(graph,cc,rc);
        double timeStop_cc = get_time();
        if(this->iam==0 && this->options_.verbose){
          std::cout<<"Column count (distributed) construction time: "<<timeStop_cc - timeSta_cc<<std::endl;
        }

        if(this->options_.ordering != NATURAL){
          this->ETree_.SortChildren(cc,this->Order_);
          if(this->options_.dumpPerm>0){
            logfileptr->OFS()<<"perm = "<<this->Order_.perm<<std::endl;
          }
        }

        double timeStop = get_time();
        if(this->iam==0 && this->options_.verbose){
          std::cout<<"Elimination tree construction time: "<<timeStop - timeSta<<std::endl;
        }
      }
      else
      {
        //gather the graph if necessary to build the elimination tree
        if(sgraph==NULL){ 
          sgraph = new SparseMatrixGraph();
          this->graph_.GatherStructure(*sgraph,0);
        }
        this->graph_.SetKeepDiag(1);
        sgraph->SetBaseval(1);
        sgraph->SetKeepDiag(1);

        double timeSta = get_time();
        this->ETree_.ConstructETree(*sgraph,this->Order_,this->fullcomm_);
        this->ETree_.PostOrderTree(this->Order_);

        double timeSta_cc = get_time();
        this->getLColRowCount(*sgraph,cc,rc);
        double timeStop_cc = get_time();
        if(this->iam==0 && this->options_.verbose){
          std::cout<<"Column count (gather + serial + bcast) construction time: "<<timeStop_cc - timeSta_cc<<std::endl;
        }

        if(this->options_.ordering != NATURAL){
          this->ETree_.SortChildren(cc,this->Order_);
          if(this->options_.dumpPerm>0){
            logfileptr->OFS()<<"perm = "<<this->Order_.perm<<std::endl;
          }
        }

        double timeStop = get_time();
        if(this->iam==0 && this->options_.verbose){
          std::cout<<"Elimination tree construction time: "<<timeStop - timeSta<<std::endl;
        }
      }

      //compute some statistics
      if(this->iam==0 && this->options_.verbose){
        double flops = 0.0;
        int64_t NNZ = 0;
        for(Int i = 0; i<cc.size();++i){
          flops+= (double)pow((double)cc[i],2.0);
          NNZ+=cc[i];
        }
        std::cout<<"Flops: "<<flops<<std::endl;
        std::cout<<"NNZ in L factor: "<<NNZ<<std::endl;
      }

      //get rid of the sequential graph
      if(sgraph!=NULL && this->options_.order_refinement_str.substr(0,4) != "TSPB"){delete sgraph;}

      { 
        double timeSta = get_time();
        this->findSupernodes(this->ETree_,this->Order_,cc,this->SupMembership_,this->Xsuper_,this->options_.relax.maxSize);
        logfileptr->OFS()<<"Supernodes found"<<std::endl;

        if(this->options_.relax.nrelax0>0)
        {
          this->relaxSupernodes(this->ETree_, cc,this->SupMembership_, this->Xsuper_, this->options_.relax );
          logfileptr->OFS()<<"Relaxation done"<<std::endl;
        }

        //modify this->np since it cannot be greater than the number of supernodes
        this->np = std::min(this->np,this->options_.used_procs(this->Xsuper_.size()-1)); 

        if(this->workcomm_!=MPI_COMM_NULL){
          MPI_Comm_free(&this->workcomm_);
        }
        MPI_Comm_split(this->options_.MPIcomm,this->iam<this->np,this->iam,&this->workcomm_);
        this->group_.reset( new RankGroup( this->workcomm_ ) ); 

        //do another split to contain P0 and all the non working processors
        if(this->all_np!=this->np){
          if(this->non_workcomm_!=MPI_COMM_NULL){
            MPI_Comm_free(&this->non_workcomm_);
          }
          this->non_workcomm_ = MPI_COMM_NULL;
          MPI_Comm_split(this->options_.MPIcomm,this->iam==0||this->iam>=this->np,this->iam==0?0:this->iam-this->np+1,&this->non_workcomm_);
        }

        //Compute this->XsuperDist_
        std::vector<Idx> newVertexDist;
        {
          Idx supPerProc = std::max((size_t)1,(this->Xsuper_.size()-1) / this->np);
          this->XsuperDist_.resize(this->all_np+1,0);
          this->XsuperDist_[this->np] = this->Xsuper_.size();
          for(int p = 0; p<this->np; p++){
            this->XsuperDist_[p]= std::min(this->XsuperDist_[this->np],(Int)(p*supPerProc+1));
          }
          for(int p = this->np+1; p<this->all_np; p++){
            this->XsuperDist_[p]= this->XsuperDist_[p-1];
          }
          this->XsuperDist_[this->all_np] = this->Xsuper_.size();


          newVertexDist.resize(this->all_np+1,0);
          newVertexDist[this->all_np] = this->iSize_+1;
          for(int p = 0; p < this->all_np; p++){
            Int S = this->XsuperDist_[p];
            newVertexDist[p] = this->Xsuper_[S-1];
          }
        }

        double timeStaSymb = get_time();
        this->symbolicFactorizationRelaxedDist(cc);

        double timeStopSymb = get_time();
        if(this->iam==0 && this->options_.verbose){
          std::cout<<"Symbolic factorization time: "<<timeStopSymb - timeStaSymb<<std::endl;
        }
        logfileptr->OFS()<<"Symbfact done"<<std::endl;


        if(this->options_.order_refinement_str != "NO") {
          double timeSta = get_time();
          if(this->options_.order_refinement_str == "SET"){ 
            this->refineSupernodes(3,1,&pMat);
          }
          else if(this->options_.order_refinement_str == "SET10"){ 
            this->refineSupernodes(1,0,&pMat);
          }
          else if(this->options_.order_refinement_str == "SET11"){ 
            this->refineSupernodes(1,1,&pMat);
          }
          else if(this->options_.order_refinement_str == "SET20"){ 
            this->refineSupernodes(2,0,&pMat);
          }
          else if(this->options_.order_refinement_str == "SET21"){ 
            this->refineSupernodes(2,1,&pMat);
          }
          else if(this->options_.order_refinement_str == "SET30"){ 
            this->refineSupernodes(3,0,&pMat);
          }
          else if(this->options_.order_refinement_str == "SET31"){ 
            this->refineSupernodes(3,1,&pMat);
          }
          else if(this->options_.order_refinement_str == "TSP"){
            auto SupETree = this->ETree_.ToSupernodalETree(this->Xsuper_,this->SupMembership_,this->Order_);

            std::vector<Ptr> xlindx;
            std::vector<Idx> lindx;
            this->gatherLStructure(xlindx, lindx);

            if(this->iam==0){
              TSP::SymbolMatrix * symbmtx = TSP::GetPastixSymbolMatrix(this->Xsuper_,this->SupMembership_, xlindx, lindx);
              TSP::Order * psorder = TSP::GetPastixOrder(symbmtx,this->Xsuper_, SupETree, &this->Order_.perm[0], &this->Order_.invp[0]);

              double timeSta = get_time();
              TSP::symbolReordering( symbmtx, psorder, 0, std::numeric_limits<int>::max(), 0 );
              double timeStop = get_time();
              if(this->iam==0 && this->options_.verbose){
                std::cout<<"TSP reordering done in "<<timeStop-timeSta<<std::endl;
              }

              //overwrite order
              for(int i = 0; i < this->Order_.perm.size(); ++i){
                this->Order_.perm[i] = psorder->peritab[i]+1;
                this->Order_.invp[i] = psorder->permtab[i]+1;
              }
            }

            MPI_Bcast(this->Order_.perm.data(),this->Order_.perm.size()*sizeof(Int),MPI_BYTE,0,this->fullcomm_);
            MPI_Bcast(this->Order_.invp.data(),this->Order_.invp.size()*sizeof(Int),MPI_BYTE,0,this->fullcomm_);
          } 
          else if(this->options_.order_refinement_str.substr(0,4) == "TSPB"){

            if(sgraph==NULL){ 
              sgraph = new SparseMatrixGraph();
              this->graph_.GatherStructure(*sgraph,0);
            }

            ETree& tree = this->ETree_;
            Ordering & aOrder = this->Order_;
            std::vector<Int> & supMembership = this->SupMembership_; 
            std::vector<Int> & xsuper = this->Xsuper_; 

            std::vector<int> ixlindx;
            std::vector<int> ilindx;

            std::vector<int>  new_invp;

            //Gather this->locXlindx_ and this->locLindx_
            {
              std::vector<Ptr> xlindx;
              std::vector<Idx> lindx;
              this->gatherLStructure(xlindx, lindx);

              if(this->iam==0){
                ixlindx.resize(xlindx.size());
                for(int i = 0;i<xlindx.size();i++){
                  ixlindx[i] = xlindx[i];
                }
                ilindx.resize(lindx.size());
                for(int i = 0;i<lindx.size();i++){
                  ilindx[i] = lindx[i];
                }
              }
            }

            if(this->iam==0){
              bassert(sgraph!=nullptr);
              int neqns = this->iSize_;

              int nofsub =ilindx.size();
              nsuper = xsuper.size()-1;


              new_invp.assign(neqns,0);


              std::vector<int>  new_perm(neqns,0);

              for(size_t i =0; i<new_perm.size(); i++){ new_invp[i] = this->Order_.invp[i];}
              for(size_t i =0; i<new_perm.size(); i++){ new_perm[i] = this->Order_.perm[i];}

              int supsiz = 0;
              for(I = 1; I <= nsuper; I++){
                Int fc = this->Xsuper_[I-1];
                Int lc = this->Xsuper_[I]-1;
                supsiz = std::max(supsiz,lc-fc+1);
              }

              sgraph->SetKeepDiag(0);
              xadj.resize(sgraph->colptr.size());
              adj.resize(sgraph->rowind.size());
              int nadj = adj.size();
              for(size_t i = 0; i< sgraph->colptr.size(); i++){ xadj[i] = int(sgraph->colptr[i]); }
              for(size_t i = 0; i< sgraph->rowind.size(); i++){ adj[i] = int(sgraph->rowind[i]); }

              if(sgraph!=NULL){delete sgraph;}

              std::vector<int, Mallocator<int> > etpar(neqns);
              for(int i = 0; i<neqns; i++){ etpar[i] = tree.PostParent(i); }


              std::vector<int, Mallocator<int> > xskadj(neqns+1);
              std::vector<int, Mallocator<int> > sklenf(neqns);
              std::vector<int, Mallocator<int> > sklenb(neqns);
              std::vector<int, Mallocator<int> > skadj(nadj);
              std::vector<int, Mallocator<int> > invp2(neqns);
              std::vector<int, Mallocator<int> > link(neqns);
              std::vector<int, Mallocator<int> > fstloc(nsuper);
              std::vector<int, Mallocator<int> > sperm(nsuper);
              std::vector<int, Mallocator<int> > fstloc2(neqns);
              std::vector<int, Mallocator<int> > dist1((supsiz+1)*(supsiz+1));
              std::vector<int, Mallocator<int> > suppar(nsuper);
              int iwsiz = 8*neqns+3;
              std::vector<int, Mallocator<int> > iwork(iwsiz);

              int iflag = 0;
              double timeSta = get_time();
              if(this->options_.order_refinement_str == "TSPB"){
                std::vector<int, Mallocator<int> > rep(neqns);
                FORTRAN(ordsup_ind_tsp_paths2)
                  ( 
                   &nadj, &neqns , &nofsub, &nsuper, &supsiz, 
                   xsuper.data(), ixlindx.data(), ilindx.data(), supMembership.data(), 
                   xadj.data(), adj.data(), 
                   etpar.data(),
                   new_perm.data(), new_invp.data(), 
                   &iflag , 
                   xskadj.data(), sklenf.data(), sklenb.data(), 
                   skadj.data() , invp2.data() , link.data()  , 
                   fstloc.data(), sperm.data() , fstloc2.data(), dist1.data(), 
                   suppar.data(), &iwsiz , iwork.data() , rep.data()             );
              }
              else{
                FORTRAN(ordsup_ind_tsp_paths)
                  ( 
                   &nadj, &neqns , &nofsub, &nsuper, &supsiz, 
                   xsuper.data(), ixlindx.data(), ilindx.data(), supMembership.data(), 
                   xadj.data(), adj.data(), 
                   etpar.data(),
                   new_perm.data(), new_invp.data(), 
                   &iflag , 
                   xskadj.data(), sklenf.data(), sklenb.data(), 
                   skadj.data() , invp2.data() , link.data()  , 
                   fstloc.data(), sperm.data() , fstloc2.data(), dist1.data(), 
                   suppar.data(), &iwsiz , iwork.data() );
              }

              double timeStop = get_time();
              if(this->iam==0 && this->options_.verbose){
                std::cout<<"TSPB reordering done in "<<timeStop-timeSta<<std::endl;
              }


              //this->Order_.Compose(new_invp);
              for(size_t i =0; i<new_perm.size(); i++){ this->Order_.invp[i] = new_invp[i];}
              for(size_t i =0; i<new_perm.size(); i++){ this->Order_.perm[i] = new_perm[i];}
            }

            // broadcast invp
            Int N = aOrder.invp.size();
            MPI_Bcast(&aOrder.invp[0],N*sizeof(Int),MPI_BYTE,0,this->fullcomm_);
            MPI_Bcast(&aOrder.perm[0],N*sizeof(Int),MPI_BYTE,0,this->fullcomm_);

          }

          double timeStop = get_time();

          if(this->iam==0 && this->options_.verbose){
            std::cout<<"Supernode reordering done in "<<timeStop-timeSta<<std::endl;
          }

          {
            double timeSta = get_time();
            this->symbolicFactorizationRelaxedDist(cc);
            double timeStop = get_time();
            if(this->iam==0 && this->options_.verbose){
              std::cout<<"Symbolic factorization time: "<<timeStop - timeSta<<std::endl;
            }
            logfileptr->OFS()<<"Symbfact done"<<std::endl;
          }
        }

        double timeStop = get_time();
        if(this->iam==0 && this->options_.verbose){
          std::cout<<"Total symbolic factorization time: "<<timeStop - timeSta<<std::endl;
        }

        //Print statistics
        if(this->options_.print_stats){
          OrderStats stats;
          stats.get(this->Xsuper_, this->XsuperDist_, this->locXlindx_, this->locLindx_, this->fullcomm_);
          if (this->iam==0){
            stats.print();
          }
        }
      }

#ifdef _DEBUG_
      logfileptr->OFS()<<"Membership list is "<<this->SupMembership_<<std::endl;
      logfileptr->OFS()<<"xsuper "<<this->Xsuper_<<std::endl;
#endif


#ifdef _OUTPUT_ETREE_
      logfileptr->OFS()<<"ETree is "<<this->ETree_<<std::endl;
      {
        auto supETree = this->ETree_.ToSupernodalETree(this->Xsuper_,this->SupMembership_,this->Order_);
        logfileptr->OFS()<<"Supernodal ETree is "<<supETree<<std::endl;
        logfileptr->OFS()<<"Xsuper is "<<this->Xsuper_<<std::endl;
      } 
#endif
      }

      {
#ifdef _MEM_PROFILER_
        utility::scope_memprofiler m("symPACK2D_symbolic_mapping");
#endif
        //compute a mapping
        // Do a 2D cell-cyclic distribution for now.
        nsuper = this->Xsuper_.size()-1;
        auto iam = this->iam;
        auto np = this->np;
        Int npcol = std::floor(std::sqrt(np));
        Int nprow = std::floor(np / npcol);

        {
          using cell_tuple_t = std::tuple<Idx,Idx,Int,Int>;
          std::list< cell_tuple_t> cellsToSend;

          Int numLocSnode = this->XsuperDist_[this->iam+1]-this->XsuperDist_[this->iam];
          Int firstSnode = this->XsuperDist_[this->iam];

          //now create cell structures
          for(Int locsupno = 1; locsupno<this->locXlindx_.size(); ++locsupno){
            Idx I = locsupno + firstSnode-1;
            Int first_col = this->Xsuper_[I-1];
            Int last_col = this->Xsuper_[I]-1;

            Ptr lfi = this->locXlindx_[locsupno-1];
            Ptr lli = this->locXlindx_[locsupno]-1;

            Int iWidth = last_col - first_col + 1;

            Int * pNrows = nullptr;
            Int * pNblock = nullptr;

            Idx iStartRow = this->locLindx_[lfi-1];
            Idx iPrevRow = iStartRow;
            Int iContiguousRows = 0;


            Int K = -1;
            Int K_prevSnode = -1;
            for(Ptr K_sidx = lfi; K_sidx<=lli;K_sidx++){
              Idx K_row = this->locLindx_[K_sidx-1];
              K = this->SupMembership_[K_row-1];

              //Split at boundary or after diagonal block
              if(K!=K_prevSnode){
                if(K>=I){
                  //add last block from previous cell
                  if (pNblock) {
                    bassert(K>I);
                    bassert(iContiguousRows>0);
                    //now we are at the end of a block
                    (*pNblock)++;
                    //add a fake cell to describe the block
                    cellsToSend.push_back(std::make_tuple(-1,-1,iStartRow,iContiguousRows));
                    iContiguousRows = 0;
                  }

                  iContiguousRows = 0;
                  iStartRow = K_row;
                  iPrevRow = iStartRow;
                  cellsToSend.push_back(std::make_tuple(K,I,0,0));
                  pNblock = &std::get<2>(cellsToSend.back());
                  pNrows = &std::get<3>(cellsToSend.back());
                }
              }
              K_prevSnode = K;


              //counting contiguous rows
              if(K_row==iPrevRow+1 || K_row==iStartRow ){
                ++iContiguousRows;
              }
              //add previous block from current cell
              else{
                bassert(iContiguousRows>0);
                //now we are at the end of a block
                (*pNblock)++;
                //add a fake cell to describe the block
                cellsToSend.push_back(std::make_tuple(-1,-1,iStartRow,iContiguousRows));
                iContiguousRows = 1;
                iStartRow = K_row;
              }
              iPrevRow=K_row;
              (*pNrows)++;
            }

            if (pNblock) {
              //now we are at the end of a block
              (*pNblock)++;
              //add a fake cell to describe the block
              bassert(iContiguousRows>0);
              cellsToSend.push_back(std::make_tuple(-1,-1,iStartRow,iContiguousRows));
              iContiguousRows = 0;
            }

          }

          MPI_Datatype type;
          MPI_Type_contiguous( sizeof(cell_tuple_t), MPI_BYTE, &type );
          MPI_Type_commit(&type);

          //then do an allgatherv
          //compute send sizes
          int ssize = cellsToSend.size();

          //Build the contiguous array
          vector<cell_tuple_t> sendbuf;
          sendbuf.reserve(ssize);
          sendbuf.insert(sendbuf.end(),cellsToSend.begin(),cellsToSend.end());

          //allgather receive sizes
          vector<int> rsizes(this->np,0);
          MPI_Allgather(&ssize,sizeof(int),MPI_BYTE,&rsizes[0],sizeof(int),MPI_BYTE,this->workcomm_);

          //compute receive displacements
          vector<int> rdispls(this->np+1,0);
          rdispls[0] = 0;
          std::partial_sum(rsizes.begin(),rsizes.end(),&rdispls[1]);


          //Now do the allgatherv
          vector<cell_tuple_t> recvbuf(rdispls.back());
          MPI_Allgatherv(&sendbuf[0],ssize,type,&recvbuf[0],&rsizes[0],&rdispls[0],type,this->workcomm_);
          MPI_Type_free(&type);


#define _SUBCUBE2D_
#ifdef _SUBCUBE2D_
          std::map<std::tuple<int,int>, int> mapping;
          using ProcGroup = TreeLoadBalancer::ProcGroup;
          std::vector< ProcGroup > levelGroups_;
          std::vector< Int > groupIdx_;
          std::vector<double> NodeLoad;
          {

            auto factor_cost = [](Int m, Int n)->double{
              return CHOLESKY_COST(m,n);
            };

            auto update_cost = [](Int m, Int n, Int k)->double{
              return 2.0*m*n*k;
            };


            auto supETree = this->ETree_.ToSupernodalETree(this->Xsuper_,this->SupMembership_,this->Order_);
            Int numLevels = 1;
            NodeLoad.assign(supETree.Size()+1,0.0);
            std::vector<double> SubTreeLoad(supETree.Size()+1,0.0);
            std::vector<int> children(supETree.Size()+1,0);
            Int numLocSnode = this->XsuperDist_[iam+1]-this->XsuperDist_[iam];
            Int firstSnode = this->XsuperDist_[iam];

            for(Int locsupno = 1; locsupno<this->locXlindx_.size(); ++locsupno){
              Idx I = locsupno + firstSnode-1;

              Idx fc = this->Xsuper_[I-1];
              Idx lc = this->Xsuper_[I]-1;

              Int width = lc - fc + 1;
              Int height = cc[fc-1];

              //cost of factoring curent panel
              double local_load = factor_cost(height,width);
              SubTreeLoad[I]+=local_load;
              NodeLoad[I]+=local_load;

              //cost of updating ancestors
              {
                Ptr lfi = this->locXlindx_[locsupno-1];
                Ptr lli = this->locXlindx_[locsupno]-1;

                //parse rows
                Int tgt_snode_id = I;
                Idx tgt_fr = fc;
                Idx tgt_lr = fc;
                Ptr tgt_fr_ptr = 0;
                Ptr tgt_lr_ptr = 0;
                for(Ptr rowptr = lfi; rowptr<=lli;rowptr++){
                  Idx row = this->locLindx_[rowptr-1]; 
                  if(this->SupMembership_[row-1]==tgt_snode_id){ continue;}

                  //we have a new tgt_snode_id
                  tgt_lr_ptr = rowptr-1;
                  tgt_lr = this->locLindx_[rowptr-1 -1];
                  if(tgt_snode_id!=I){
                    Int m = lli-tgt_fr_ptr+1;
                    Int n = width;
                    Int k = tgt_lr_ptr - tgt_fr_ptr+1; 
                    double cost = update_cost(m,n,k);
                    SubTreeLoad[tgt_snode_id]+=cost;
                    NodeLoad[tgt_snode_id]+=cost;
                  }
                  tgt_fr = row;
                  tgt_fr_ptr = rowptr;
                  tgt_snode_id = this->SupMembership_[row-1];
                }

                //do the last update supernode
                //we have a new tgt_snode_id
                tgt_lr_ptr = lli;
                tgt_lr = this->locLindx_[lli -1];
                if(tgt_snode_id!=I){
                  Int m = lli-tgt_fr_ptr+1;
                  Int n = width;
                  Int k = tgt_lr_ptr - tgt_fr_ptr+1; 
                  double cost = update_cost(m,n,k);
                  SubTreeLoad[tgt_snode_id]+=cost;
                  NodeLoad[tgt_snode_id]+=cost;
                }

              }
            }

            //Allreduce SubTreeLoad and NodeLoad
            MPI_Allreduce(MPI_IN_PLACE,&NodeLoad[0],NodeLoad.size(),MPI_DOUBLE,MPI_SUM,this->workcomm_);
            MPI_Allreduce(MPI_IN_PLACE,&SubTreeLoad[0],SubTreeLoad.size(),MPI_DOUBLE,MPI_SUM,this->workcomm_);

            for(Int I=1;I<=supETree.Size();I++){
              Int parent = supETree.Parent(I-1);
              ++children[parent];
              SubTreeLoad[parent]+=SubTreeLoad[I];
            }

            int n_ = nsuper;//supETree.Size();
            std::vector<Int> levels(supETree.Size()+1,0);
            for(Int I=n_; I>= 1;I--){ 
              Int parent = supETree.Parent(I-1);
              if(parent==0){levels[I]=0;}
              else{ levels[I] = levels[parent]+1; numLevels = std::max(numLevels,levels[I]);}
            }
            numLevels++;

            std::vector< ProcGroup > procGroups_;
            groupIdx_.resize(n_+1,0);
            procGroups_.resize(n_+1);
            procGroups_[0].Ranks().reserve(np);
            for(Int p = 0;p<np;++p){ procGroups_[0].Ranks().push_back(p);}

            std::vector<Int> pstart(n_+1,0);
            levelGroups_.reserve(numLevels);
            levelGroups_.push_back(ProcGroup());//reserve(numLevels);
            levelGroups_[0].Ranks().reserve(np);
            for(Int p = 0;p<np;++p){levelGroups_[0].Ranks().push_back(p);}

            std::vector<double> load(n_+1,0.0);
            for(Int I=n_; I>= 1;I--){ 
              Int parent = supETree.Parent(I-1);

              //split parent's proc list
              double parent_load = 0.0;

              if(parent!=0){
                Int fc = this->Xsuper_[parent-1];
                Int width = this->Xsuper_[parent] - this->Xsuper_[parent-1];
                Int height = cc[fc-1];
                parent_load = NodeLoad[parent];// factor_cost(height,width);
                load[parent] = parent_load;
              }

              double proportion = std::min(1.0,SubTreeLoad[I]/(SubTreeLoad[parent]-parent_load));
              Int npParent = levelGroups_[groupIdx_[parent]].Ranks().size();
              Int pFirstIdx = std::min(pstart[parent],npParent-1);
              Int npIdeal =(Int)std::round(npParent*proportion);
              Int numProcs = std::max((Int)1,std::min(npParent-pFirstIdx,npIdeal));
              Int pFirst = levelGroups_[groupIdx_[parent]].Ranks().at(pFirstIdx);
              pstart[parent]+= numProcs;

              if(npParent!=numProcs){
                levelGroups_.push_back(ProcGroup());//reserve(numLevels);
                levelGroups_.back().Ranks().reserve(numProcs);

                std::vector<Int> & parentRanks = levelGroups_[groupIdx_[parent]].Ranks();
                levelGroups_.back().Ranks().insert(levelGroups_.back().Ranks().begin(),parentRanks.begin()+pFirstIdx,parentRanks.begin()+pFirstIdx+numProcs);

                groupIdx_[I] = levelGroups_.size()-1;
              }
              else{
                groupIdx_[I] = groupIdx_[parent];
              }
            }
          }

          std::vector<std::vector<double>> group_load(nsuper);
          auto find_min_proc = [this,&levelGroups_,&groupIdx_,&group_load,&NodeLoad](int j){
            Int minLoadP= -1;
            double minLoad = -1;
            ProcGroup & group = levelGroups_[groupIdx_[j-1]];
            auto & load = group_load[j-1];
            if ( load.size() < group.Ranks().size() ){ load.assign(group.Ranks().size(),0.0); }

            for(Int i = 0; i<group.Ranks().size();++i){
              if(load[i]<minLoad || minLoad==-1){
                minLoad = load[i];
                minLoadP = i;
              }
            }
            double local_load = NodeLoad[j-1];
            load[minLoadP]+=local_load;
            return group.Ranks()[minLoadP];
          };


          std::shared_ptr<blockCellBase_t> pLast_cell = nullptr;

          for(auto it = recvbuf.begin();it!=recvbuf.end();it++){
            auto & cur_cell = (*it);
            Int i = std::get<0>(cur_cell);
            Int j = std::get<1>(cur_cell);

            if (i==-1 && j==-1) {
              bassert(pLast_cell!=nullptr);
              if ( pLast_cell->owner == iam ) {
                auto ptr = std::static_pointer_cast<snodeBlock_t>(pLast_cell);
                Int first_row = std::get<2>(cur_cell);
                Int nrows = std::get<3>(cur_cell);
                ptr->add_block(first_row,nrows);
              }
            }
            else{
              Int nBlock = std::get<2>(cur_cell);
              Int nRows = std::get<3>(cur_cell);
              auto idx = coord2supidx(i-1,j-1);

              //j is supernode index
              auto p = find_min_proc(j);
              Idx fc = this->Xsuper_[j-1];
              Idx lc = this->Xsuper_[j]-1;
              Int iWidth = lc-fc+1;
              size_t block_cnt = nBlock;
              size_t nnz = nRows * iWidth;

              std::shared_ptr<blockCellBase_t> sptr(nullptr);
              if ( p == iam ) {
                sptr = std::static_pointer_cast<blockCellBase_t>(std::make_shared<snodeBlock_t>(i,j,fc,iWidth,nnz,block_cnt));
                
                this->localBlocks_.push_back(std::static_pointer_cast<snodeBlock_t>(sptr));

//logfileptr->OFS()<<" orig2 : S"<<sptr->j <<" -> "<<((snodeBlock_t*)sptr.get())->nnz()<<" vs. "<<nnz<<std::endl;
              }
              else {
                sptr = std::make_shared<blockCellBase_t>();
              }
              cells_[idx] = sptr;
              sptr->i = i;
              sptr->j = j;
              sptr->owner = p;
              pLast_cell = sptr;

              if ( p ==iam){
                auto & test = CELL(i-1,j-1);
              }
              else { 
                auto test = pQueryCELL(i-1,j-1);
                assert(test);
              }
            }
          }


          std::sort(this->localBlocks_.begin(),this->localBlocks_.end(),[](std::shared_ptr<snodeBlock_t> & a, std::shared_ptr<snodeBlock_t> & b){
              return a->j < b->j || (a->i < b->i &&  a->j == b->j ) ; 
              });

#else

#define _BALANCE_NNZ_
#ifdef _BALANCE_NNZ_
          using load_t = std::tuple<int,size_t>;
          auto compare_load = [](load_t & a, load_t & b){ return std::get<1>(a)>std::get<1>(b);};
          std::priority_queue< load_t, std::vector<load_t>, decltype( compare_load ) > load(compare_load);
          for ( int p = 0; p < nprow*npcol ; p++) {
            load.push( std::make_tuple(p, (size_t)0));
          }
#endif
          std::shared_ptr<blockCellBase_t> pLast_cell = nullptr;
          for(auto it = recvbuf.begin();it!=recvbuf.end();it++){
            auto & cur_cell = (*it);
            Int i = std::get<0>(cur_cell);
            Int j = std::get<1>(cur_cell);

            if (i==-1 && j==-1) {
              bassert(pLast_cell!=nullptr);
              if ( pLast_cell->owner == iam ) {
                auto ptr = std::static_pointer_cast<snodeBlock_t>(pLast_cell);
                Int first_row = std::get<2>(cur_cell);
                Int nrows = std::get<3>(cur_cell);
                ptr->add_block(first_row,nrows);
              }
            }
            else{
              Int nBlock = std::get<2>(cur_cell);
              Int nRows = std::get<3>(cur_cell);
              //auto idx = coord2supidx(i,j);
              auto idx = coord2supidx(i-1,j-1);

#ifdef _BALANCE_NNZ_
              auto proc = load.top();
              load.pop();
              auto p = std::get<0>(proc);
#else
              auto prow = i % nprow;
              auto pcol = j % npcol;
              auto p = pcol*nprow+prow;
#endif      

              Idx fc = this->Xsuper_[j-1];
              Idx lc = this->Xsuper_[j]-1;
              Int iWidth = lc-fc+1;
              size_t block_cnt = nBlock;
              size_t nnz = nRows * iWidth;
#ifdef _BALANCE_NNZ_
              auto & lp = std::get<1>(proc);
              lp += nnz*iWidth;
              load.push(proc);
#endif

              std::shared_ptr<blockCellBase_t> sptr(nullptr);
              if ( p == iam ) {
                sptr = std::static_pointer_cast<blockCellBase_t>(std::make_shared<snodeBlock_t>(i,j,fc,iWidth,nnz,block_cnt));
                this->localBlocks_.push_back(std::static_pointer_cast<snodeBlock_t>(sptr));
              }
              else {
                sptr = std::make_shared<blockCellBase_t>();
              }

              cells_[idx] = sptr;
              sptr->i = i;
              sptr->j = j;
              sptr->owner = p;
              pLast_cell = sptr;
            }
          }

          std::sort(this->localBlocks_.begin(),this->localBlocks_.end(),[](std::shared_ptr<snodeBlock_t> & a, std::shared_ptr<snodeBlock_t> & b){
              return a->j < b->j || (a->i < b->i &&  a->j == b->j ) ; 
              });
#endif
        }
      }

      logfileptr->OFS()<<"#supernodes = "<<nsuper<<" #cells = "<<cells_.size()<< " which is "<<cells_.size()*sizeof(snodeBlock_t)<<" bytes"<<std::endl;


      if(this->iam==0){
        std::cout<<"#supernodes = "<<nsuper<<" #cells = "<<cells_.size()<< " which is "<<cells_.size()*sizeof(snodeBlock_t)<<" bytes"<<std::endl;
      }

      //generate task graph for the factorization
      {
#ifdef _MEM_PROFILER_
        utility::scope_memprofiler m("symPACK2D_symbolic_task_graph");
#endif

        MPI_Datatype type;
        MPI_Type_contiguous( sizeof(SparseTask2D::meta_t), MPI_BYTE, &type );
        MPI_Type_commit(&type);
        vector<int> ssizes(this->np,0);
        vector<int> sdispls(this->np+1,0);
        vector<SparseTask2D::meta_t> sendbuf;

        auto supETree = this->ETree_.ToSupernodalETree(this->Xsuper_,this->SupMembership_,this->Order_);

        {  
          //we will need to communicate if only partial xlindx_, lindx_
          //idea: build tasklist per processor and then exchange
          //tuple is: src_snode,tgt_snode,op_type,lt_first_row = updated fc, facing_first_row = updated fr,
          std::map<Idx, std::list< SparseTask2D::meta_t> > Updates;
          std::vector<int> marker(this->np,0);
          std::vector< std::map< Idx, std::pair<Idx,std::set<Idx> > > > updCnt(this->Xsuper_.size() -1 );

          Int numLocSnode = this->XsuperDist_[this->iam+1]-this->XsuperDist_[this->iam];
          Int firstSnode = this->XsuperDist_[this->iam];
          for(Int locsupno = 1; locsupno<this->locXlindx_.size(); ++locsupno){
            Idx I = locsupno + firstSnode-1;
            Int first_col = this->Xsuper_[I-1];
            Int last_col = this->Xsuper_[I]-1;

            bassert( cells_.find(coord2supidx(I-1,I-1)) != cells_.end() );

            auto ptr_fcell = pQueryCELL(I-1,I-1);
            Int iOwner = ptr_fcell->owner;

            //Create the factor task on the owner
            Updates[iOwner].push_back(std::make_tuple(I,I,Factorization::op_type::FACTOR,first_col,first_col));

            Ptr lfi = this->locXlindx_[locsupno-1];
            Ptr lli = this->locXlindx_[locsupno]-1;
            std::list<Int> ancestor_rows;

            Int K = -1; 
            Idx K_prevSnode = -1;
            for(Ptr K_sidx = lfi; K_sidx<=lli;K_sidx++){
              Idx K_row = this->locLindx_[K_sidx-1];
              K = this->SupMembership_[K_row-1];

              //Split at boundary or after diagonal block
              if(K!=K_prevSnode){
                if(K>I){
                  ancestor_rows.push_back(K_row);
                }
              }
              K_prevSnode = K;
            }

            for(auto J_row : ancestor_rows){
              Int J = this->SupMembership_[J_row-1];

              auto ptr_fodcell = pQueryCELL(J-1,I-1);
              Int iFODOwner = ptr_fodcell->owner;

              Updates[iOwner].push_back(std::make_tuple(I,I,Factorization::op_type::TRSM_SEND,J_row,J_row));
              Updates[iFODOwner].push_back(std::make_tuple(I,I,Factorization::op_type::TRSM_RECV,J_row,J_row));
              Updates[iFODOwner].push_back(std::make_tuple(I,I,Factorization::op_type::TRSM,J_row,J_row));

              //add the UPDATE_SEND from FODOwner tasks
              for(auto K_row : ancestor_rows){
                K = this->SupMembership_[K_row-1];

                if(K>=J){
                  auto ptr_tgtupdcell = pQueryCELL(K-1,J-1);
                  Int iTgtOwner = ptr_tgtupdcell->owner;
                  //TODO update this for non fan-out mapping
                  Int iUpdOwner = ptr_tgtupdcell->owner;

                  //cell(J,I) to cell(K,J)
                  Updates[iFODOwner].push_back(std::make_tuple(I,J,Factorization::op_type::UPDATE2D_SEND_OD,K_row,J_row));
                  Updates[iUpdOwner].push_back(std::make_tuple(I,J,Factorization::op_type::UPDATE2D_RECV_OD,K_row,J_row));
                }

                if(K<=J){
                  auto ptr_tgtupdcell = pQueryCELL(J-1,K-1);
                  Int iTgtOwner = ptr_tgtupdcell->owner;
                  //TODO update this for non fan-out mapping
                  Int iUpdOwner = ptr_tgtupdcell->owner;

                  auto ptr_facingcell = pQueryCELL(J-1,I-1);
                  Int iFacingOwner = ptr_facingcell->owner;

                  //sender point of view
                  //cell(J,I) to cell(J,K)
                  Updates[iFacingOwner].push_back(std::make_tuple(I,K,Factorization::op_type::UPDATE2D_SEND,K_row,J_row));
                  Updates[iUpdOwner].push_back(std::make_tuple(I,K,Factorization::op_type::UPDATE2D_RECV,K_row,J_row));
                }
              }

              for(auto K_row : ancestor_rows){
                K = this->SupMembership_[K_row-1];
                if(K>=J){
                  auto ptr_tgtupdcell = pQueryCELL(K-1,J-1);
                  Int iTgtOwner = ptr_tgtupdcell->owner;
                  //TODO update this for non fan-out mapping
                  Int iUpdOwner = ptr_tgtupdcell->owner;

                  //update on cell(K,J)
                  Updates[iUpdOwner].push_back(std::make_tuple(I,J,Factorization::op_type::UPDATE2D_COMP,J_row,K_row));
                  //cell(K,J) to cell (K,J)
                  Updates[iUpdOwner].push_back(std::make_tuple(I,J,Factorization::op_type::AGGREGATE2D_SEND,J_row,K_row));
                  Updates[iTgtOwner].push_back(std::make_tuple(I,J,Factorization::op_type::AGGREGATE2D_RECV,J_row,K_row));
                }
              }
            }
          }

          //then do an alltoallv
          //compute send sizes
          for(auto itp = Updates.begin();itp!=Updates.end();itp++){
            ssizes[itp->first] = itp->second.size();
          }

          //compute send displacements
          sdispls[0] = 0;
          std::partial_sum(ssizes.begin(),ssizes.end(),&sdispls[1]);

          //Build the contiguous array of pairs
          sendbuf.reserve(sdispls.back());

          for(auto itp = Updates.begin();itp!=Updates.end();itp++){
            sendbuf.insert(sendbuf.end(),itp->second.begin(),itp->second.end());
          }
        }

        //gather receive sizes
        vector<int> rsizes(this->np,0);
        MPI_Alltoall(&ssizes[0],sizeof(int),MPI_BYTE,&rsizes[0],sizeof(int),MPI_BYTE,this->workcomm_);

        //compute receive displacements
        vector<int> rdispls(this->np+1,0);
        rdispls[0] = 0;
        std::partial_sum(rsizes.begin(),rsizes.end(),&rdispls[1]);

        //Now do the alltoallv
        vector<SparseTask2D::meta_t> recvbuf(rdispls.back());
        MPI_Alltoallv(&sendbuf[0],&ssizes[0],&sdispls[0],type,&recvbuf[0],&rsizes[0],&rdispls[0],type,this->workcomm_);
        MPI_Type_free(&type);

        //clear the send buffer
        {
          vector<SparseTask2D::meta_t> tmp;
          sendbuf.swap( tmp );
        }

        //do a top down traversal of my local task list
        for(auto it = recvbuf.begin();it!=recvbuf.end();it++){
          auto & cur_op = (*it);
          auto & src_snode = std::get<0>(cur_op);
          auto & tgt_snode = std::get<1>(cur_op);
          auto & type = std::get<2>(cur_op);
          auto & lt_first_row = std::get<3>(cur_op);
          auto & facing_first_row = std::get<4>(cur_op);

          SparseTask2D * ptask = nullptr;
          switch(type){
            case Factorization::op_type::TRSM:
            case Factorization::op_type::FACTOR:
            case Factorization::op_type::UPDATE2D_COMP:
              {
                ptask = new SparseTask2D;
                size_t tuple_size = sizeof(SparseTask2D::meta_t);
                auto meta = &ptask->_meta;
                meta[0] = *it;
              }
              break;
            default:
              break;
          }

          auto I = src_snode;
          auto J = tgt_snode;

          if(ptask!=nullptr){
            scheduling::key_t key(this->SupMembership_[std::get<4>(cur_op)-1], std::get<1>(cur_op), std::get<0>(cur_op), std::get<2>(cur_op));

            auto it3 = this->task_graph.find(key);
            if(it3!=this->task_graph.end()){
              auto key2=it3->first;
              SparseTask2D * ptask2 = it3->second.get();
              if(ptask2==nullptr){gdb_lock();}
              //do a top down traversal of my local task list
              for(auto it2 = recvbuf.begin();it2!=recvbuf.end();it2++){
                auto & cur_op = (*it2);

                auto & src_snode = std::get<0>(cur_op);
                auto & tgt_snode = std::get<1>(cur_op);
                auto & type = std::get<2>(cur_op);
                auto & lt_first_row = std::get<3>(cur_op);
                auto & facing_first_row = std::get<4>(cur_op);

                if(ptask2!=nullptr){
                  if(ptask2->_meta == cur_op){
                    gdb_lock();
                  }
                }
              }
            }

            bassert(this->task_graph.find(key)==this->task_graph.end());

            this->task_graph[key] = std::unique_ptr<SparseTask2D>(ptask);
          }
#ifdef _VERBOSE_
          switch(type){
            case Factorization::op_type::TRSM:
              {
                auto J = this->SupMembership_[facing_first_row-1];
                logfileptr->OFS()<<"TRSM"<<" from "<<I<<" to ("<<I<<") cell ("<<J<<","<<I<<")"<<std::endl;
              }
              break;
            case Factorization::op_type::FACTOR:
              {
                logfileptr->OFS()<<"FACTOR"<<" cell ("<<J<<","<<I<<")"<<std::endl;
              }
              break;
            case Factorization::op_type::UPDATE2D_COMP:
              {
                auto K = this->SupMembership_[facing_first_row-1];
                logfileptr->OFS()<<"UPDATE"<<" from "<<I<<" to "<<J<<" facing cell ("<<K<<","<<I<<") and cell ("<<J<<","<<I<<") to cell ("<<K<<","<<J<<")"<<std::endl;
              }
              break;
            default:
              break;
          }
#endif
        }

        //return;

        //process dependency tasks
        for(auto it = recvbuf.begin();it!=recvbuf.end();it++){
          auto & cur_op = (*it);
          auto & src_snode = std::get<0>(cur_op);
          auto & tgt_snode = std::get<1>(cur_op);
          auto & type = std::get<2>(cur_op);
          auto & lt_first_row = std::get<3>(cur_op);
          auto & facing_first_row = std::get<4>(cur_op);


          auto I = src_snode;
          auto J = tgt_snode;

          switch(type){
            case Factorization::op_type::TRSM_SEND:
              {
                auto J = this->SupMembership_[facing_first_row-1];
                auto taskptr = task_graph[scheduling::key_t(I,I,I,Factorization::op_type::FACTOR)].get();
                bassert(taskptr!=nullptr); 
#ifdef _VERBOSE_
                logfileptr->OFS()<<"TRSM_SEND"<<" from "<<I<<" to "<<I<<" cell ("<<J<<","<<I<<")"<<std::endl;
#endif
#ifdef _COMPACT_DEPENDENCIES_
                taskptr->out_dependencies.push_back( J );
#else
                taskptr->out_dependencies.push_back( std::make_tuple(J,I) );
#endif
#ifdef _VERBOSE_
                logfileptr->OFS()<<"        out dep added to FACTOR"<<" from "<<I<<" to "<<I<<std::endl;
#endif
              }
              break;
            case Factorization::op_type::TRSM_RECV:
              {
                auto J = this->SupMembership_[facing_first_row-1];

                //find the TRSM and add cell I,I as incoming dependency
                auto taskptr = task_graph[scheduling::key_t(J,I,I,Factorization::op_type::TRSM)].get();
                bassert(taskptr!=nullptr); 
#ifdef _VERBOSE_
                logfileptr->OFS()<<"TRSM_RECV"<<" from "<<I<<" to "<<I<<" cell ("<<J<<","<<I<<")"<<std::endl;
#endif
                if (  pQueryCELL(J-1,I-1)->owner != pQueryCELL(I-1,I-1)->owner )
                  taskptr->in_remote_dependencies_cnt++;
                else
                  taskptr->in_local_dependencies_cnt++;
#ifdef _VERBOSE_
                logfileptr->OFS()<<"        in dep added to TRSM"<<" from "<<I<<" to "<<I<<std::endl;
#endif
              }
              break;
            case Factorization::op_type::AGGREGATE2D_RECV:
              {
                auto K = this->SupMembership_[facing_first_row-1];

#ifdef _VERBOSE_
                logfileptr->OFS()<<"AGGREGATE2D_RECV"<<" from "<<I<<" to "<<J<<" cell ("<<K<<","<<J<<") to cell("<<K<<","<<J<<")"<<std::endl;
#endif
                //find the FACTOR or TRSM and add cell K,J as incoming dependency
                if(K==J){
                  auto taskptr = task_graph[scheduling::key_t(J,J,J,Factorization::op_type::FACTOR)].get();
                  bassert(taskptr!=nullptr); 

                  taskptr->in_local_dependencies_cnt++;
#ifdef _VERBOSE_
                  logfileptr->OFS()<<"        in dep added to FACTOR"<<" from "<<J<<" to "<<J<<std::endl;
#endif
                }
                else{
                  auto taskptr = task_graph[scheduling::key_t(K,J,J,Factorization::op_type::TRSM)].get();
                  bassert(taskptr!=nullptr); 
                  taskptr->in_local_dependencies_cnt++;
#ifdef _VERBOSE_
                  logfileptr->OFS()<<"        in dep added to TRSM"<<" from "<<J<<" to "<<J<<std::endl;
#endif
                }
              }
              break;
            case Factorization::op_type::AGGREGATE2D_SEND:
              {
                //TODO this might be useless
                auto K = this->SupMembership_[facing_first_row-1];

#ifdef _VERBOSE_
                logfileptr->OFS()<<"AGGREGATE2D_SEND"<<" from "<<I<<" to "<<J<<" cell ("<<K<<","<<J<<") to cell("<<K<<","<<J<<")"<<std::endl;
#endif
                //find the UPDATE2D_COMP and add cell K,J as outgoing dependency
                auto taskptr = task_graph[scheduling::key_t(K,J,I,Factorization::op_type::UPDATE2D_COMP)].get();
                bassert(taskptr!=nullptr); 


#ifdef _COMPACT_DEPENDENCIES_
                taskptr->out_dependencies.push_back( J );
#else
                taskptr->out_dependencies.push_back( std::make_tuple(K,J) );
#endif

#ifdef _VERBOSE_
                logfileptr->OFS()<<"        out dep added to UPDATE_COMP"<<" from "<<I<<" to "<<J<<std::endl;
#endif
              }
              break;

            case Factorization::op_type::UPDATE2D_RECV:
              {
                //TODO this might be useless
                Idx K = this->SupMembership_[facing_first_row-1];
                std::swap(K,J);
#ifdef _VERBOSE_
                logfileptr->OFS()<<"UPDATE_RECV"<<" from "<<I<<" to "<<K<<" cell ("<<J<<","<<I<<") to cell ("<<J<<","<<K<<")"<<std::endl;
#endif
                //find the UPDATE2D_COMP and add cell J,I as incoming dependency
                auto taskptr = task_graph[scheduling::key_t(J,K,I,Factorization::op_type::UPDATE2D_COMP)].get();
                bassert(taskptr!=nullptr);

                if (  pQueryCELL(J-1,I-1)->owner != pQueryCELL(J-1,K-1)->owner )
                  taskptr->in_remote_dependencies_cnt++;
                else
                  taskptr->in_local_dependencies_cnt++;

#ifdef _VERBOSE_
                logfileptr->OFS()<<"        in dep added to UPDATE_COMP"<<" from "<<I<<" to "<<K<<std::endl;
#endif
              }
              break;
            case Factorization::op_type::UPDATE2D_SEND:
              {
                Idx K = this->SupMembership_[facing_first_row-1];
                std::swap(K,J);
#ifdef _VERBOSE_
                logfileptr->OFS()<<"UPDATE_SEND"<<" from "<<I<<" to "<<K<<" cell ("<<J<<","<<I<<") to cell ("<<J<<","<<K<<")"<<std::endl;
#endif
                //find the TRSM and add cell J,K as outgoing dependency
                auto taskptr = task_graph[scheduling::key_t(J,I,I,Factorization::op_type::TRSM)].get();
                bassert(taskptr!=nullptr); 
#ifdef _COMPACT_DEPENDENCIES_
                taskptr->out_dependencies.push_back( K );
#else
                taskptr->out_dependencies.push_back( std::make_tuple(J,K) );
#endif
#ifdef _VERBOSE_
                logfileptr->OFS()<<"        out dep added to TRSM"<<" from "<<I<<" to "<<I<<std::endl;
#endif
              }
              break;
            case Factorization::op_type::UPDATE2D_SEND_OD:
              {
                auto K = this->SupMembership_[lt_first_row-1];
#ifdef _VERBOSE_
                logfileptr->OFS()<<"UPDATE_SEND OD"<<" from "<<I<<" to "<<J<<" cell ("<<J<<","<<I<<") to cell ("<<K<<","<<J<<")"<<std::endl;
#endif

                //find the TRSM and add cell K,J as outgoing dependency
                auto taskptr = task_graph[scheduling::key_t(J,I,I,Factorization::op_type::TRSM)].get();

                bassert(taskptr!=nullptr); 
#ifdef _COMPACT_DEPENDENCIES_
                taskptr->out_dependencies.push_back( -K );
#else
                taskptr->out_dependencies.push_back( std::make_tuple(K,J) );
#endif
#ifdef _VERBOSE_
                logfileptr->OFS()<<"        out dep DOWN added to TRSM"<<" from "<<I<<" to "<<I<<std::endl;
#endif
              }
              break;
            case Factorization::op_type::UPDATE2D_RECV_OD:
              {
                auto K = this->SupMembership_[lt_first_row-1];
#ifdef _VERBOSE_
                logfileptr->OFS()<<"UPDATE_RECV OD"<<" from "<<I<<" to "<<J<<" cell ("<<J<<","<<I<<") to cell ("<<K<<","<<J<<")"<<std::endl;
#endif

                //find the UPDATE2D_COMP and add cell J,I as incoming dependency
                auto taskptr = task_graph[scheduling::key_t(K,J,I,Factorization::op_type::UPDATE2D_COMP)].get();

                bassert(taskptr!=nullptr);
                if (  pQueryCELL(J-1,I-1)->owner != pQueryCELL(K-1,J-1)->owner )
                  taskptr->in_remote_dependencies_cnt++;
                else
                  taskptr->in_local_dependencies_cnt++;

#ifdef _VERBOSE_
                logfileptr->OFS()<<"        in dep DOWN added to UPDATE2D_COMP"<<" from "<<I<<" to "<<J<<std::endl;
#endif
              }
              break;
            default:
              break;
          }
        }

        scheduler.sp_handle = this->sp_handle;
        //Now we have our local part of the task graph
        for(auto it = this->task_graph.begin(); it != this->task_graph.end(); it++){
          auto & key = it->first;
          auto & ptask = it->second;
          auto meta = &ptask->_meta;

          auto & src_snode = std::get<0>(meta[0]);
          auto & tgt_snode = std::get<1>(meta[0]);
          auto & lt_first_row = std::get<3>(meta[0]);
          auto & facing_row = std::get<4>(meta[0]);
          auto & type = std::get<2>(meta[0]);

          auto I = src_snode;
          auto J = tgt_snode;
          auto K = this->SupMembership_[facing_row-1];

          auto remote_deps = ptask->in_remote_dependencies_cnt;
          auto local_deps = ptask->in_local_dependencies_cnt;

          ptask->in_prom.require_anonymous(local_deps + remote_deps);
          ptask->in_avail_prom.require_anonymous(remote_deps);

          auto ptr = ptask.get();
          auto fut_comm = ptask->in_avail_prom.finalize();
          if (remote_deps >0 ) {
            fut_comm.then([this,ptr](){
#ifdef _PRIORITY_QUEUE_AVAIL_
                this->scheduler.avail_tasks.push(ptr);
#else
                this->scheduler.avail_tasks.push_back(ptr);
#endif
                });
          }
          //fulfill the promise by one to "unlock" that promise
          //ptask->in_avail_prom->fulfill_anonymous(1);

          //do not fulfill it to lock the tasks
          //auto fut = ptask->in_prom->get_future();
          auto fut = ptask->in_prom.finalize();
          fut.then([this,ptr](){
              this->scheduler.ready_tasks.push_back(ptr);
              });

          switch(type){
            case Factorization::op_type::FACTOR:
              {

                ptask->execute = [this,src_snode,ptr,I,J,K] () {
                  scope_timer(b,FB_FACTOR_DIAG_TASK);
//            utility::scope_memprofiler2 m("symPACKMatrix_factorization",1);

                  auto ptask = ptr;
                  //logfileptr->OFS()<<"Exec FACTOR"<<" cell ("<<I<<","<<I<<")"<<std::endl;

                  auto ptr_diagcell = pQueryCELL2(I-1,I-1);
                  assert(ptr_diagcell);

                  auto & diagcell = CELL(I-1,I-1);
                  bassert( diagcell.owner == this->iam);

#ifdef SP_THREADS
                  std::thread::id tid = std::this_thread::get_id();
                  auto & tmpBuf = tmpBufs_th[tid];
#else
                  auto & tmpBuf = tmpBufs;
#endif
                  diagcell.factorize(tmpBuf);

                  std::unordered_map<Int,std::list<SparseTask2D::depend_task_t> > data_to_send;
#ifdef _COMPACT_DEPENDENCIES_
                  for (auto &tpl : ptask->out_dependencies) {
                    auto tgt_j = tpl;
                    auto ptr_tgt_cell = pQueryCELL(tgt_j-1,I-1);
                    //add this cell to the list of cells depending on diagcell for the TRSM task(s)
                    data_to_send[ptr_tgt_cell->owner].push_back(std::make_tuple(tgt_j,I));
                  }
#else
                  for (auto &tpl : ptask->out_dependencies) {
                    auto tgt_i = std::get<0>(tpl);
                    auto tgt_j = std::get<1>(tpl);
                    auto ptr_tgt_cell = pQueryCELL(tgt_i-1,tgt_j-1);
                    //add this cell to the list of cells depending on diagcell for the TRSM task(s)
                    data_to_send[ptr_tgt_cell->owner].push_back(tpl);
                  }
#endif

                  //send factor and update local tasks
                  //ptask->out_prom->require_anonymous(data_to_send.size());
                  for (auto it = data_to_send.begin(); it!=data_to_send.end(); it++) {
                    auto pdest = it->first;
                    auto & tgt_cells = it->second;
                    //serialize data once, and list of meta data
                    //factor is output data so it will not be deleted
                    if ( pdest != this->iam ) {
                      auto cxs = upcxx::source_cx::as_buffered() | upcxx::source_cx::as_promise(ptask->out_prom);
#if 1
                      upcxx::rpc_ff( pdest, /*cxs,*/ 
                          [ ] (int sp_handle, upcxx::global_ptr<char> gptr, size_t storage_size, size_t nnz, size_t nblocks, rowind_t width, SparseTask2D::meta_t meta, upcxx::view<SparseTask2D::depend_task_t> target_cells ) { 
                          //gdb_lock();
                          //store pointer & associated metadata somewhere
                          auto data = std::make_shared<SparseTask2D::data_t >();
                          data->in_meta = meta;
                          data->size = storage_size;
                          data->remote_gptr = gptr;

                          //there is a map between sp_handle and task_graphs
                          auto matptr = (symPACKMatrix2D<colptr_t,rowind_t,T> *) g_sp_handle_to_matrix[sp_handle];

                          for ( auto & tgt_cell: target_cells) {
                          auto tgt_i = std::get<0>(tgt_cell);
                          auto tgt_j = std::get<1>(tgt_cell);
                          auto taskptr = matptr->task_graph[scheduling::key_t(tgt_i,tgt_j,tgt_j,Factorization::op_type::TRSM)].get();
                          taskptr->input_msg.push_back(data);
                          data->target_tasks.push_back(taskptr);
                          taskptr->in_avail_prom.fulfill_anonymous(1);
                          }

                          auto I = std::get<1>(meta);
                          rowind_t fc = matptr->Xsuper_[I-1];

                          data->on_fetch.get_future().then(
                              [fc,width,nnz,nblocks,I](SparseTask2D::data_t * pdata){
                              //create snodeBlock_t and store it in the extra_data
                              pdata->extra_data = std::unique_ptr<blockCellBase_t>( (blockCellBase_t*)new snodeBlock_t(I,I,pdata->landing_zone,fc,width,nnz,nblocks) );
                              //pdata->extra_data = ( (blockCellBase_t*)new snodeBlock_t(I,I,pdata->landing_zone,fc,width,nnz,nblocks) );
                              });
                          //TODO check this
                          data->allocate();
                          data->fetch();

                          }, this->sp_handle, diagcell._gstorage, diagcell._storage_size, diagcell.nnz(), diagcell.nblocks(), std::get<0>(diagcell._dims) ,ptask->_meta, upcxx::make_view(tgt_cells.begin(),tgt_cells.end())); 
#endif
                    }
                    else {
                      for ( auto & tgt_cell: tgt_cells ) {
                        auto tgt_i = std::get<0>(tgt_cell);
                        auto tgt_j = std::get<1>(tgt_cell);
                        bassert(I==tgt_j);
                        auto taskptr = task_graph[scheduling::key_t(tgt_i,tgt_j,tgt_j,Factorization::op_type::TRSM)].get();
                        bassert(taskptr!=nullptr); 
                        //mark the dependency as satisfied
                        taskptr->in_prom.fulfill_anonymous(1);
                      }
                    }
                  }

                  //the task pointed by ptask can be deleted when all outgoing communications have been performed.
                  auto fut = ptask->out_prom.finalize();
                  fut.wait();
                  //fut.then( [ptask]() { delete ptask; } );
                  //
                  //
                  //TODO what do do with the task OR return a future
                  ptask->executed = true;
                };
              }
              break;
            case Factorization::op_type::TRSM:
              {

                ptask->execute = [this,src_snode,tgt_snode,ptr,I,J,K] () {
                    static std::map< std::tuple<int,int>, bool > block_used_update; 
                  scope_timer(b,FB_TRSM_TASK);
//            utility::scope_memprofiler2 m("symPACKMatrix_trsm",1);
                  auto ptask = ptr;

                  //logfileptr->OFS()<<"Exec TRSM"<<" from "<<I<<" to ("<<I<<") cell ("<<K<<","<<I<<")"<<std::endl;

                  auto ptr_od_cell = pQueryCELL2(K-1,I-1);
                  assert(ptr_od_cell);

                  auto & od_cell = CELL(K-1,I-1);//*std::static_pointer_cast<snodeBlock_t>(cells_[coord2supidx(tgt_snode-1,src_snode-1)]);
                  //auto & od_cell = cells_[coord2supidx(tgt_snode-1,src_snode-1)];
                  bassert( od_cell.owner == this->iam);

#ifdef SP_THREADS
                  std::thread::id tid = std::this_thread::get_id();
                  auto & tmpBuf = tmpBufs_th[tid];
#else
                  auto & tmpBuf = tmpBufs;
#endif


                  //input data is one or 0 (local diagonal block)
                  bassert(ptask->input_msg.size()<=1);
                  auto ptr_diagCell = pQueryCELL(I-1,I-1).get(); 
                  if ( ptr_diagCell->owner != this->iam ){
                    ptr_diagCell = (snodeBlock_t*)(ptask->input_msg.begin()->get()->extra_data.get());
                    //ptr_diagCell = (snodeBlock_t*)(ptask->input_msg.begin()->get()->extra_data);

//                      auto it = block_used_update.find( std::make_tuple(I,I) ) ;
//                      if ( it == block_used_update.end() ) {
//                        block_used_update[std::make_tuple(I,I)] = true;
//                        logfileptr->OFS()<<ptask->input_msg.begin()->get()->size<<" vs "<<ptr_diagCell->_storage_size<<std::endl;

//                        m.pstate->overhead_mb += ptask->input_msg.begin()->get()->size / 1024. /m.pstate->divider; 
//                      }


                  }
                  bassert(ptr_diagCell!=NULL);


                  od_cell.trsm(*dynamic_cast<snodeBlock_t*>(ptr_diagCell),tmpBuf);

#ifdef _MEMORY_LIMIT_
              if ( this->mem_budget != -1.0 ) {
                size_t mem_cost = 0;
                for(auto & msg : ptask->input_msg){
                  mem_cost += msg->size;
                }
                this->mem_budget += mem_cost; 
//                  logfileptr->OFS()<<"mem budget is back to "<<mem_budget<<std::endl;
              }
#endif
                  //get rid of the shared_ptr
                  ptask->input_msg.clear();

                  //send this to the all facing blocks (tgt_snode-1,*) and off diagonal blocks (*,tgt_snode-1)
                  std::unordered_map<Int,std::list<SparseTask2D::depend_task_t> > data_to_send;
#ifdef _COMPACT_DEPENDENCIES_
                  for (auto &tpl : ptask->out_dependencies) {
                    auto tgt_j = tpl;
                    if ( tgt_j > 0 ) {
                      auto ptr_tgt_cell = pQueryCELL(K-1,tgt_j-1);
                      data_to_send[ptr_tgt_cell->owner].push_back(std::make_tuple(K,tgt_j));
                    }
                    else {
                      auto ptr_tgt_cell = pQueryCELL(-tgt_j-1,K-1);
                      data_to_send[ptr_tgt_cell->owner].push_back(std::make_tuple(-tgt_j,K));
                    }
                  }
#else
                  for (auto &tpl : ptask->out_dependencies) {
                    auto tgt_i = std::get<0>(tpl);
                    auto tgt_j = std::get<1>(tpl);
                    auto ptr_tgt_cell = pQueryCELL(tgt_i-1,tgt_j-1);
                    data_to_send[ptr_tgt_cell->owner].push_back(tpl);
                  }
#endif


                  //send factor and update local tasks
                  for (auto it = data_to_send.begin(); it!=data_to_send.end(); it++) {
                    auto pdest = it->first;
                    auto & tgt_cells = it->second;
                    //serialize data once, and list of meta data
                    //factor is output data so it will not be deleted
                    if ( pdest != this->iam ) {
                      auto cxs = upcxx::source_cx::as_buffered() | upcxx::source_cx::as_promise(ptask->out_prom);
#if 1
                      upcxx::rpc_ff( pdest, //cxs, 
                          [K,I] (int sp_handle, upcxx::global_ptr<char> gptr, size_t storage_size, size_t nnz, size_t nblocks, rowind_t width, SparseTask2D::meta_t meta, upcxx::view<SparseTask2D::depend_task_t> target_cells ) { 
                          //                            gdb_lock();
                          //store pointer & associated metadata somewhere
                          auto data = std::make_shared<SparseTask2D::data_t >();
                          data->in_meta = meta;
                          data->size = storage_size;
                          data->remote_gptr = gptr;
                          //there is a map between sp_handle and task_graphs
                          auto matptr = (symPACKMatrix2D<colptr_t,rowind_t,T> *) g_sp_handle_to_matrix[sp_handle];
                          for ( auto & tgt_cell: target_cells) {
                          auto tgt_i = std::get<0>(tgt_cell);
                          auto tgt_j = std::get<1>(tgt_cell);
                          auto taskptr = matptr->task_graph[scheduling::key_t(tgt_i,tgt_j,I,Factorization::op_type::UPDATE2D_COMP)].get();
                          taskptr->input_msg.push_back(data);
                          data->target_tasks.push_back(taskptr);
                          taskptr->in_avail_prom.fulfill_anonymous(1);

                          }

                          //auto I = std::get<1>(meta);
                          rowind_t fc = matptr->Xsuper_[I-1];
                          data->on_fetch.get_future().then(
                              [fc,width,nnz,nblocks,K,I](SparseTask2D::data_t * pdata){
                              //create snodeBlock_t and store it in the extra_data
                              pdata->extra_data = std::unique_ptr<blockCellBase_t>( (blockCellBase_t*)new snodeBlock_t(K,I,pdata->landing_zone,fc,width,nnz,nblocks) );
                              //pdata->extra_data = ( (blockCellBase_t*)new snodeBlock_t(K,I,pdata->landing_zone,fc,width,nnz,nblocks) );
                              }
                              );
                          //TODO check this
                          data->allocate();
                          data->fetch();

                          }, this->sp_handle, od_cell._gstorage,od_cell._storage_size, od_cell.nnz(),od_cell.nblocks(), std::get<0>(od_cell._dims),ptask->_meta, upcxx::make_view(tgt_cells.begin(),tgt_cells.end())); 
#endif
                    }
                    else {
                      for ( auto & tgt_cell: tgt_cells ) {
                        auto tgt_i = std::get<0>(tgt_cell);
                        auto tgt_j = std::get<1>(tgt_cell);

                        bassert(K==tgt_i || K==tgt_j); // K is either facing or pivot row
                        auto taskptr = task_graph[scheduling::key_t(tgt_i,tgt_j,I,Factorization::op_type::UPDATE2D_COMP)].get();
                        bassert(taskptr!=nullptr); 
                        //mark the dependency as satisfied
                        taskptr->in_prom.fulfill_anonymous(1);
                      }
                    }
                  }

                  //the task pointed by ptask can be deleted when all outgoing communications have been performed.
                  auto fut = ptask->out_prom.finalize();
                  fut.wait();

                  ptask->executed = true;

                };
              }
              break;
            case Factorization::op_type::UPDATE2D_COMP:
              {

                ptask->execute = [this,src_snode,ptr,I,J,K] () {
                    static std::map< std::tuple<int,int>, bool > block_used_update; 
                  scope_timer(b,FB_UPDATE2D_TASK);
//            utility::scope_memprofiler2 m("symPACKMatrix_update",1);
                  auto ptask = ptr;

                  auto ptr_upd_cell = pQueryCELL2(K-1,J-1);
                  assert(ptr_upd_cell);

                  auto & upd_cell = CELL(K-1,J-1);
                  //TODO false for fan-both
                  bassert(upd_cell.owner==this->iam);

#ifdef SP_THREADS
                  std::thread::id tid = std::this_thread::get_id();
                  auto & tmpBuf = tmpBufs_th[tid];
#else
                  auto & tmpBuf = tmpBufs;
#endif

                  //TODO do something
                  //input data should be at most two

                  bassert(ptask->input_msg.size()<=2);

                  auto ptr_odCell = pQueryCELL(J-1,I-1).get(); 
                  if ( ptr_odCell->owner != this->iam ){
                    ptr_odCell = (snodeBlock_t*)(ptask->input_msg.begin()->get()->extra_data.get());
//                    ptr_odCell = (snodeBlock_t*)(ptask->input_msg.begin()->get()->extra_data);
//                      auto it = block_used_update.find( std::make_tuple(J,I) ) ;
//                      if ( it == block_used_update.end() ) {
//                        block_used_update[std::make_tuple(J,I)] = true;
//                        m.pstate->overhead_mb += ptask->input_msg.begin()->get()->size  / 1024./ m.pstate->divider; 
//                      }

                  }
                  bassert(ptr_odCell!=NULL);

                  auto ptr_facingCell = pQueryCELL(K-1,I-1).get(); 
                  if ( ptr_facingCell->owner != this->iam ){
                    ptr_facingCell = (snodeBlock_t*)(ptask->input_msg.rbegin()->get()->extra_data.get());
//                    ptr_facingCell = (snodeBlock_t*)(ptask->input_msg.rbegin()->get()->extra_data);

//                      auto it = block_used_update.find( std::make_tuple(K,I) ) ;
//                      if ( it == block_used_update.end() ) {
//                        block_used_update[std::make_tuple(K,I)] = true;
//                        m.pstate->overhead_mb += ptask->input_msg.rbegin()->get()->size  / 1024./ m.pstate->divider; 
//                      }
                  }
                  bassert(ptr_facingCell!=NULL);

                  //check that they don't need to be swapped
                  if ( ptr_facingCell->i < ptr_odCell->i ) {
                    std::swap(ptr_facingCell, ptr_odCell);
                  }

                  upd_cell.update(*dynamic_cast<snodeBlock_t*>(ptr_odCell),*dynamic_cast<snodeBlock_t*>(ptr_facingCell),tmpBuf);

#ifdef _MEMORY_LIMIT_
              if ( this->mem_budget != -1.0 ) {
                size_t mem_cost = 0;
                for(auto & msg : ptask->input_msg){
                  mem_cost += msg->size;
                }
                this->mem_budget += mem_cost; 
//                  logfileptr->OFS()<<"mem budget is back to "<<mem_budget<<std::endl;
              }
#endif
                  //get rid of the shared_ptr
                  ptask->input_msg.clear();

                  //send this to all facing blocks (tgt_snode-1,*) and off diagonal blocks (*,tgt_snode-1)
                  std::unordered_map<Int,std::list<SparseTask2D::depend_task_t> > data_to_send;
#ifdef _COMPACT_DEPENDENCIES_
                  for (auto &tpl : ptask->out_dependencies) {
                    auto tgt_j = tpl;
                    auto ptr_tgt_cell = pQueryCELL(K-1,tgt_j-1);
                    data_to_send[ptr_tgt_cell->owner].push_back(std::make_tuple(K,tgt_j));
                  }
#else
                  for (auto &tpl : ptask->out_dependencies) {
                    auto tgt_i = std::get<0>(tpl);
                    auto tgt_j = std::get<1>(tpl);
                    auto ptr_tgt_cell = pQueryCELL(tgt_i-1,tgt_j-1);
                    data_to_send[ptr_tgt_cell->owner].push_back(tpl);
                  }
#endif
                  //send factor and update local tasks
                  //                    ptask->out_prom->require_anonymous(data_to_send.size());
                  for (auto it = data_to_send.begin(); it!=data_to_send.end(); it++) {
                    auto pdest = it->first;
                    auto & tgt_cells = it->second;
                    //serialize data once, and list of meta data
                    //factor is output data so it will not be deleted
                    if ( pdest != this->iam ) {
                      auto cxs = upcxx::source_cx::as_buffered() | upcxx::source_cx::as_promise(ptask->out_prom);
#if 1
                      upcxx::rpc_ff( pdest, //cxs, 
                          [K,J] (int sp_handle, upcxx::global_ptr<char> gptr, size_t storage_size, size_t nnz, size_t nblocks, rowind_t width, SparseTask2D::meta_t meta, upcxx::view<SparseTask2D::depend_task_t> target_cells ) { 
                          //                            gdb_lock();
                          //store pointer & associated metadata somewhere
                          auto data = std::make_shared<SparseTask2D::data_t >();
                          //data->target_cells.insert(data->target_cells.begin(),
                          //    target_cells.begin(),target_cells.end());
                          data->in_meta = meta;
                          data->size = storage_size;
                          data->remote_gptr = gptr;

                          //there is a map between sp_handle and task_graphs
                          auto matptr = (symPACKMatrix2D<colptr_t,rowind_t,T> *) g_sp_handle_to_matrix[sp_handle];
                          for ( auto & tgt_cell: target_cells) {
                          auto tgt_i = std::get<0>(tgt_cell);
                          auto tgt_j = std::get<1>(tgt_cell);
                          auto K = tgt_i;
                          auto J = tgt_j;
                          auto taskptr = matptr->task_graph[scheduling::key_t(tgt_i,tgt_j,J,(tgt_i==tgt_j?Factorization::op_type::FACTOR:Factorization::op_type::TRSM))].get();

                          taskptr->input_msg.push_back(data);
                          data->target_tasks.push_back(taskptr);
                          taskptr->in_avail_prom.fulfill_anonymous(1);
                          }


                          auto I = std::get<1>(meta);
                          rowind_t fc = matptr->Xsuper_[I-1];
                          data->on_fetch.get_future().then(
                              [fc,width,nnz,nblocks,K,J](SparseTask2D::data_t * pdata){
                              //create snodeBlock_t and store it in the extra_data
                              pdata->extra_data = std::unique_ptr<blockCellBase_t>( (blockCellBase_t*)new snodeBlock_t(K,J,pdata->landing_zone,fc,width,nnz,nblocks) );
                              }
                              );
                          //TODO check this
                          data->allocate();
                          data->fetch();

                          }, this->sp_handle, upd_cell._gstorage, upd_cell._storage_size,upd_cell.nnz(),upd_cell.nblocks(), std::get<0>(upd_cell._dims),ptask->_meta, upcxx::make_view(tgt_cells.begin(),tgt_cells.end())); 
#endif
                    }
                    else {
                      for ( auto & tgt_cell: tgt_cells ) {
                        auto tgt_i = std::get<0>(tgt_cell);
                        auto tgt_j = std::get<1>(tgt_cell);
                        bassert(K==tgt_i && J==tgt_j);
                        auto taskptr = task_graph[scheduling::key_t(tgt_i,tgt_j,J,
                            (tgt_i==tgt_j?Factorization::op_type::FACTOR:Factorization::op_type::TRSM))].get();
                        bassert(taskptr!=nullptr); 
                        //mark the dependency as satisfied
                        taskptr->in_prom.fulfill_anonymous(1);
                      }
                    }
                  }

                  //the task pointed by ptask can be deleted when all outgoing communications have been performed.
                  auto fut = ptask->out_prom.finalize();
                  fut.wait();

                  ptask->executed = true;
                };

              }
              break;
            default:
              {
                ptask.reset(nullptr);
                //                delete ptask;
              }
              break;
          }
        }


        logfileptr->OFS()<<"Task graph created"<<std::endl;
      }

      //generate task graph for the solution phase
    } 

  template <typename colptr_t, typename rowind_t, typename T>
    void symPACKMatrix2D<colptr_t,rowind_t,T>::DistributeMatrix(DistSparseMatrix<T> & pMat ){

      std::map<Int,size_t > send_map;

      typedef std::conditional< sizeof(Idx) < sizeof(Ptr), Idx, Ptr>::type minTypeIdxPtr;
      using minType = typename std::conditional< sizeof(minTypeIdxPtr) < sizeof(T), minTypeIdxPtr, T >::type;

      size_t minSize = sizeof(minType);
      size_t IdxToMin = sizeof(Idx) / minSize;
      size_t PtrToMin = sizeof(Ptr) / minSize;
      size_t TToMin = sizeof(T) / minSize;

      Int baseval = pMat.Localg_.GetBaseval();
      Idx FirstLocalCol = pMat.Localg_.vertexDist[this->iam] + (1 - baseval); //1-based
      Idx LastLocalCol = pMat.Localg_.vertexDist[this->iam+1] + (1 - baseval); //1-based

      {
#ifdef _MEM_PROFILER_
        utility::scope_memprofiler m("symPACKMatrix2D::DistributeMatrix::Counting");
#endif
        scope_timer(a,symPACKMatrix2D::DistributeMatrix::Counting);
        for(Int I=1;I<this->Xsuper_.size();I++){
          Idx fc = this->Xsuper_[I-1];
          Idx lc = this->Xsuper_[I]-1;
          Int iWidth = lc-fc+1;

          //post all the recv and sends
          for(Idx col = fc;col<=lc;col++){
            //corresponding column in the unsorted matrix A
            Idx orig_col = this->Order_.perm[col-1];
            if(orig_col>= FirstLocalCol && orig_col < LastLocalCol){
              Ptr nrows = 0;
              Idx local_col = orig_col - FirstLocalCol+1;//1 based
              for(Ptr rowidx = pMat.Localg_.colptr[local_col-1] + (1-baseval); rowidx<pMat.Localg_.colptr[local_col]+(1-baseval); ++rowidx){
                Idx orig_row = pMat.Localg_.rowind[rowidx-1]+(1-baseval);//1-based
                Idx row = this->Order_.invp[orig_row-1];

                Int J = this->SupMembership_[row-1];

                if(row>=col){
                  //this has to go in cell(J,I)
                  auto ptr_tgt_cell = pQueryCELL(J-1,I-1);
                  Int iDest = ptr_tgt_cell->owner;
                  send_map[iDest] += IdxToMin+PtrToMin+ (IdxToMin + TToMin);
                }
                else{
                  //this has to go in cell(I,J)
                  auto ptr_tgt_cell = pQueryCELL(I-1,J-1);
                  Int iDestJ = ptr_tgt_cell->owner;
                  //add the pair (col,row) to processor owning column row
                  send_map[iDestJ] += IdxToMin+PtrToMin+ (IdxToMin + TToMin);
                }
              }
            }
          }
        }
      }



      {
        //allocate one buffer for every remote processor
        //compute the send structures
        size_t total_send_size = 0;
        std::vector<size_t> stotcounts(this->all_np,0);
        for(auto it = send_map.begin(); it!=send_map.end();it++){
          Int iCurDest = it->first;
          size_t & send_bytes = it->second;
          stotcounts[iCurDest] = send_bytes;
        }
        //compute send displacements
        std::vector<size_t> spositions(this->all_np+1,0);
        spositions[0] = 0;
        std::partial_sum(stotcounts.begin(),stotcounts.end(),&spositions[1]);
        total_send_size = spositions.back();

        std::vector<minType, Mallocator<minType> > sendBuffer(total_send_size);

        {
#ifdef _MEM_PROFILER_
          utility::scope_memprofiler m("symPACKMatrix2D::DistributeMatrix::Serializing");
#endif
          scope_timer(a,symPACKMatrix2D::DistributeMatrix::Serializing);

          for(Int I=1;I<this->Xsuper_.size();I++){
            Idx fc = this->Xsuper_[I-1];
            Idx lc = this->Xsuper_[I]-1;
            Int iWidth = lc-fc+1;

            //Serialize
            for(Idx col = fc;col<=lc;col++){
              Idx orig_col = this->Order_.perm[col-1];

              if(orig_col>= FirstLocalCol && orig_col < LastLocalCol){
                Ptr nrows = 0;
                Idx local_col = orig_col - FirstLocalCol+1;//1 based

                for(Ptr rowidx = pMat.Localg_.colptr[local_col-1]+(1-baseval); rowidx<pMat.Localg_.colptr[local_col]+(1-baseval); ++rowidx){
                  Idx orig_row = pMat.Localg_.rowind[rowidx-1]+(1-baseval);
                  Idx row = this->Order_.invp[orig_row-1];

                  Int J = this->SupMembership_[row-1];

                  if(row>=col){
                    //this has to go in cell(J,I)
                    auto ptr_tgt_cell = pQueryCELL(J-1,I-1);
                    bassert(ptr_tgt_cell->i==J && ptr_tgt_cell->j==I);
                    Int iDest = ptr_tgt_cell->owner;

                    T val = pMat.nzvalLocal[rowidx-1];

                    *((Idx*)&sendBuffer[spositions[iDest]]) = col;
                    spositions[iDest]+=IdxToMin;
                    *((Ptr*)&sendBuffer[spositions[iDest]]) = 1;
                    spositions[iDest]+=PtrToMin;
                    *((Idx*)&sendBuffer[spositions[iDest]]) = row;
                    spositions[iDest]+=IdxToMin;
                    *((T*)&sendBuffer[spositions[iDest]]) = val;
                    spositions[iDest]+=TToMin;
                  }
                  else{
                    //this has to go in cell(I,J)
                    auto ptr_tgt_cell = pQueryCELL(I-1,J-1);
                    bassert(ptr_tgt_cell->i==I && ptr_tgt_cell->j==J);
                    Int iDestJ = ptr_tgt_cell->owner;
                    //add the pair (col,row) to processor owning column row

                    T val = pMat.nzvalLocal[rowidx-1];

                    *((Idx*)&sendBuffer[spositions[iDestJ]]) = row;
                    spositions[iDestJ]+=IdxToMin;
                    *((Ptr*)&sendBuffer[spositions[iDestJ]]) = 1;
                    spositions[iDestJ]+=PtrToMin;
                    *((Idx*)&sendBuffer[spositions[iDestJ]]) = col;
                    spositions[iDestJ]+=IdxToMin;
                    *((T*)&sendBuffer[spositions[iDestJ]]) = val;
                    spositions[iDestJ]+=TToMin;
                  }
                }
              }
            }
          }
        }

        spositions[0] = 0;
        std::partial_sum(stotcounts.begin(),stotcounts.end(),&spositions[1]);


        //size_t myGCD = 0;
        //myGCD = findGCD(stotcounts.data(),stotcounts.size());
        ////now we need to to an allgather of these
        //std::vector<size_t> gcds(this->np,0);
        //MPI_Allgather(&myGCD,sizeof(size_t),MPI_BYTE,gcds.data(),sizeof(size_t),MPI_BYTE,this->workcomm_);
        //myGCD = findGCD(gcds.data(),gcds.size());


        //MPI_Datatype fused_type;
        //MPI_Type_contiguous( myGCD*sizeof(minType), MPI_BYTE, &fused_type );
        //MPI_Type_commit(&fused_type);

        //for ( int i = 0; i< stotcounts.size(); i++){ stotcounts[i]/=myGCD; }
        //spositions[0] = 0;
        //std::partial_sum(stotcounts.begin(),stotcounts.end(),&spositions[1]);

        size_t total_recv_size = 0;
        std::vector<minType, Mallocator<minType> > recvBuffer;
        std::function<void(std::vector<minType, Mallocator<minType> >&,size_t)> resize_lambda =
        [](std::vector<minType, Mallocator<minType> >& container, size_t sz){
          container.resize(sz);
        };

        MPI_Datatype type;
        MPI_Type_contiguous( sizeof(minType), MPI_BYTE, &type );
        MPI_Type_commit(&type);


        mpi::Alltoallv(sendBuffer, &stotcounts[0], &spositions[0], type ,
            recvBuffer,this->fullcomm_, resize_lambda);

        total_recv_size = recvBuffer.size();

        MPI_Type_free(&type);
        //MPI_Type_free(&fused_type);
        //Need to parse the structure sent from the processor owning the first column of the supernode


        {
#ifdef _MEM_PROFILER_
          utility::scope_memprofiler m("symPACKMatrix2D::DistributeMatrix::Deserializing");
#endif
          scope_timer(a,symPACKMatrix2D::DistributeMatrix::Deserializing);
          size_t head = 0;

          while(head < total_recv_size)
          { 
            //Deserialize
            Idx col = *((Idx*)&recvBuffer[head]);
            head+=IdxToMin;
            //nrows of column col sent by processor p
            Ptr nrows = *((Ptr*)&recvBuffer[head]);
            head+=PtrToMin;
            Idx * rowind = (Idx*)(&recvBuffer[head]);
            head+=nrows*IdxToMin;
            T * nzvalA = (T*)(&recvBuffer[head]);
            head+=nrows*TToMin;

            Int I = this->SupMembership_[col-1];

            Int fc = this->Xsuper_[I-1];
            Int lc = this->Xsuper_[I]-1;
            Int iWidth = lc-fc+1;
            //              SuperNode<T> * snode = snodeLocal(I);

            //Here, do a linear search instead for the blkidx
            Ptr colbeg = 1;
            Ptr colend = nrows;
            if(colbeg<=colend){
              //sort rowind and nzvals
              std::vector<size_t> lperm = sort_permutation(&rowind[colbeg-1],&rowind[colend-1]+1,std::less<Idx>());
              apply_permutation(&rowind[colbeg-1],&rowind[colend-1]+1,lperm);
              apply_permutation(&nzvalA[colbeg-1],&nzvalA[colend-1]+1,lperm);
              Idx firstRow = rowind[colbeg-1];

              for(Ptr rowidx = colbeg; rowidx<=colend; ++rowidx){
                Idx row = rowind[rowidx-1];

                Int J = this->SupMembership_[row-1];

                  auto ptr_tgt_cell = pQueryCELL2(J-1,I-1);
                  assert(ptr_tgt_cell);

                auto & tgt_cell =CELL(J-1,I-1);
                bassert(this->iam == tgt_cell.owner);
                bassert(tgt_cell.i==J && tgt_cell.j==I);

                bool found = false;
                for(auto & block: tgt_cell.blocks()){
                  if ( block.first_row <= row && row < block.first_row + tgt_cell.block_nrows(block) ){
                    auto offset = block.offset + (row - block.first_row)*tgt_cell.width() + (col-fc);
                    tgt_cell._nzval[offset] = nzvalA[rowidx-1];
                    found = true;
                    break;

                  }  
                } 

                bassert(found);

              }
            }
          }

        }

      }







    } 

  template <typename colptr_t, typename rowind_t, typename T>
    void symPACKMatrix2D<colptr_t,rowind_t,T>::Factorize( ){
      this->scheduler.execute(this->task_graph,this->mem_budget);
    } 




//TODO redo this
  template <typename colptr_t, typename rowind_t, typename T>
    inline void symPACKMatrix2D<colptr_t,rowind_t,T>::DumpMatlab(){
      logfileptr->OFS()<<"+sparse([";
      for(Int I=1;I<this->Xsuper_.size();I++){
        Int src_first_col = this->Xsuper_[I-1];
        Int src_last_col = this->Xsuper_[I]-1;

        //       gdb_lock(); 
        for(Int J=I;J<this->Xsuper_.size();J++){
          auto idx = coord2supidx(J-1,I-1);
          if (this->cells_.find(idx)!= this->cells_.end()){

            //auto tmp = CELL(J-1,I-1);

            auto ptr_tgt_cell = pQueryCELL(J-1,I-1);
            assert(ptr_tgt_cell!=nullptr);
            

            Int iOwner = ptr_tgt_cell->owner;
            if( iOwner == this->iam ){
              auto & tgt_cell = *std::dynamic_pointer_cast<snodeBlock_t>(ptr_tgt_cell);
              for(auto & block: tgt_cell.blocks()){
                T * val = &tgt_cell._nzval[block.offset];
                Int nRows = tgt_cell.block_nrows(block);

                Int row = block.first_row;
                for(Int i = 0; i< nRows; ++i){
                  for(Int j = 0; j< tgt_cell.width(); ++j){
                    logfileptr->OFS()<<row+i<<" ";
                  }
                }
              }
            }
          }
        }
      }
      logfileptr->OFS()<<"],[";
      for(Int I=1;I<this->Xsuper_.size();I++){
        Int src_first_col = this->Xsuper_[I-1];
        Int src_last_col = this->Xsuper_[I]-1;

        for(Int J=I;J<this->Xsuper_.size();J++){
          auto idx = coord2supidx(J-1,I-1);
          if (this->cells_.find(idx)!= this->cells_.end()){
            auto ptr_tgt_cell = pQueryCELL(J-1,I-1);
            Int iOwner = ptr_tgt_cell->owner;
            if( iOwner == this->iam ){
              auto & tgt_cell = *std::dynamic_pointer_cast<snodeBlock_t>(ptr_tgt_cell);
              for(auto & block: tgt_cell.blocks()){
                T * val = &tgt_cell._nzval[block.offset];
                Int nRows = tgt_cell.block_nrows(block);

                Int row = block.first_row;
                for(Int i = 0; i< nRows; ++i){
                  for(Int j = 0; j< tgt_cell.width(); ++j){
                    logfileptr->OFS()<<src_first_col+j<<" ";
                  }
                }
              }
            }
          }
        }
      }
      logfileptr->OFS()<<"],[";
      logfileptr->OFS().precision(std::numeric_limits< T >::max_digits10);
      for(Int I=1;I<this->Xsuper_.size();I++){
        Int src_first_col = this->Xsuper_[I-1];
        Int src_last_col = this->Xsuper_[I]-1;

        for(Int J=I;J<this->Xsuper_.size();J++){
          auto idx = coord2supidx(J-1,I-1);
          auto it = this->cells_.end();
          if ( (it = this->cells_.find(idx)) != this->cells_.end()){
            auto ptr_tgt_cell = pQueryCELL(J-1,I-1);
            Int iOwner = ptr_tgt_cell->owner;
            if( iOwner == this->iam ){
              auto & tgt_cell = *std::dynamic_pointer_cast<snodeBlock_t>(ptr_tgt_cell);
              for(auto & block: tgt_cell.blocks()){
                T * val = &tgt_cell._nzval[block.offset];
                auto nRows = tgt_cell.block_nrows(block);

                auto row = block.first_row;
                for(auto i = 0; i< nRows; ++i){
                  for(auto j = 0; j< tgt_cell.width(); ++j){
                    logfileptr->OFS()<<std::scientific<<ToMatlabScalar(val[i*tgt_cell.width()+j])<<" ";
                  }
                }
              }
            }
          }
        }
      }
      logfileptr->OFS()<<"],"<<this->iSize_<<","<<this->iSize_<<")"<<std::endl;
    }













  namespace scheduling {

    template <typename ttask_t , typename ttaskgraph_t >
      inline void Scheduler2D<ttask_t,ttaskgraph_t>::execute(ttaskgraph_t & task_graph, double & mem_budget )
      {
        try{
int64_t local_task_cnt;
          {
#ifdef _MEM_PROFILER_
          utility::scope_memprofiler m("symPACKMatrix_execute_1");
#endif
          local_task_cnt = task_graph.size();
          }

          {
#ifdef _MEM_PROFILER_
            utility::scope_memprofiler m("symPACKMatrix_execute_2");
#endif
            bool progress = false;
            while(local_task_cnt>0){
              if (!ready_tasks.empty()) {
              //while (!ready_tasks.empty()) {
                upcxx::progress(upcxx::progress_level::internal);
                auto ptask = ready_tasks.front();
                ready_tasks.pop_front();
                ptask->execute(); 
                local_task_cnt--;
              }

              upcxx::progress();

              //handle communications
              //while (!avail_tasks.empty()) {
              if (!avail_tasks.empty()) {
                //get all comms from a task
//                avail_tasks.sort([](ttask_t* & a, ttask_t* &b){ return a->out_dependencies.size() > b->out_dependencies.size();});
#ifdef _MEMORY_LIMIT_
              if ( mem_budget != -1.0 ) {
                ttask_t* ptask = nullptr;
                auto task_cnt = avail_tasks.size();
                size_t mem_cost = 0;
                size_t cnt = 0;
#ifdef _PRIORITY_QUEUE_AVAIL_
                std::list<ttask_t*> temp_list;
#endif
                do {
#ifdef _PRIORITY_QUEUE_AVAIL_
                  ptask = avail_tasks.top();
                  avail_tasks.pop();
#else
                  ptask = avail_tasks.front();
                  avail_tasks.pop_front();
#endif

                  mem_cost = 0;
                  for(auto & msg : ptask->input_msg){
                    mem_cost += msg->size;
                  }
                  if (mem_cost > mem_budget && ready_tasks.size()>0) {
#ifdef _PRIORITY_QUEUE_AVAIL_
                    temp_list.push_back(ptask);
#else
                    avail_tasks.push_back(ptask);
#endif

                    cnt++;
                    ptask = nullptr;
                    continue;
                  }
                  else{
                    break;
                  }
                } while ( cnt < task_cnt );

#ifdef _PRIORITY_QUEUE_AVAIL_
                for (auto ptask : temp_list) {
                  avail_tasks.push(ptask);
                }
#endif
                if ( ptask != nullptr ) {
                  mem_budget -= mem_cost;
//                  logfileptr->OFS()<<"mem budget is "<<mem_budget<<std::endl;
                  for(auto & msg : ptask->input_msg){
                    msg->allocate();
                    msg->fetch().then([ptask](incoming_data_t<ttask_t,meta_t> * pmsg){
                        //fulfill promise by one, when this reaches 0, ptask is moved to scheduler.ready_tasks
                        ptask->in_prom.fulfill_anonymous(1);
                        });
                  } 
                }
              }
              else{
#ifdef _PRIORITY_QUEUE_AVAIL_
                auto ptask = avail_tasks.top();
                avail_tasks.pop();
#else
                auto ptask = avail_tasks.front();
                avail_tasks.pop_front();
#endif

                for(auto & msg : ptask->input_msg){
                  msg->allocate();
                  msg->fetch().then([ptask](incoming_data_t<ttask_t,meta_t> * pmsg){
                      //fulfill promise by one, when this reaches 0, ptask is moved to scheduler.ready_tasks
                      ptask->in_prom.fulfill_anonymous(1);
                      });
                } 
              }

#else
#ifdef _PRIORITY_QUEUE_AVAIL_
                auto ptask = avail_tasks.top();
                avail_tasks.pop();
#else
                auto ptask = avail_tasks.front();
                avail_tasks.pop_front();
#endif
                for(auto & msg : ptask->input_msg){
                  msg->allocate();
                  //if (!msg->transfered)
                      msg->fetch().then([ptask](incoming_data_t<ttask_t,meta_t> * pmsg){
                        //fulfill promise by one, when this reaches 0, ptask is moved to scheduler.ready_tasks
                        ptask->in_prom.fulfill_anonymous(1);
                        });
                } 
#endif
              
              }
            }
//            utility::scope_memprofiler2::print_stats(1024.);
          }
        }
        catch(const std::runtime_error& e){
          std::cerr << "Runtime error: " << e.what() << '\n';
        }
        upcxx::barrier();
      }
  }
#endif

}

#include <sympack/impl/symPACKMatrix2D_impl.hpp>

#endif //_SYMPACK_MATRIX2D_DECL_HPP_

