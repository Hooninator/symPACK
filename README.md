# Configuring and building symPACK
--------------------------


**symPACK** is a sparse symmetric matrix direct linear solver, with optional support for CUDA devices. It can be built using the following procedure.

First, download **symPACK** as follows:


```
git clone git@github.com:symPACK/symPACK.git  /path/to/sympack

```

## External dependencies
---------------------------

### UPC++

**SymPACK** requires the [**UPC++ v1.0**](https://upcxx.lbl.gov) library to be installed. The minimum supported release version is 2019.9.0. 
If you wish the use the GPU mode of **symPACK**, the minimum supported release version is 2022.3.0. 
UPC++ can be downloaded at [upcxx.lbl.gov](https://upcxx.lbl.gov).
UPC++ contains a CMake config file which **symPACK** is using to link to the library. The install path
needs to be provided to CMake as follows:
```
-DCMAKE_PREFIX_PATH=/path/to/upcxx
``` 

### Ordering libraries
Several environment variables can be optionally set before configuring the build:

- `metis_PREFIX` = Installation directory for **MeTiS**

- `parmetis_PREFIX` = Installation directory for **ParMETIS**

- `scotch_PREFIX` = Installation directory for **SCOTCH**

- `ptscotch_PREFIX` = Installation directory for **PT-SCOTCH**

Then, create a build directory, enter that directory and type:

```
cmake -DCMAKE_BUILD_TYPE=Release -DCMAKE_INSTALL_PREFIX=/path/to/install/sympack -DCMAKE_PREFIX_PATH=/path/to/upcxx/install
 ...OPTIONS... /path/to/sympack

```

The `...OPTIONS...` can be one of the following:

- `-DENABLE_METIS=ON|OFF` to make **MeTiS** ordering available in **symPACK** (`metis_PREFIX` must be set in the environment)

- `-DENABLE_PARMETIS=ON|OFF` to make **ParMETIS** ordering available in **symPACK** (`parmetis_PREFIX` must be set in the environment, `metis_PREFIX` is required as well)

- `-DENABLE_SCOTCH=ON|OFF` to make **SCOTCH** ordering available in **symPACK** (`scotch_PREFIX` must be set in the environment)

- `-DENABLE_PTSCOTCH=ON|OFF` to make **PT-SCOTCH** ordering available in **symPACK** (`ptscotch_PREFIX` must be set in the environment, `scotch_PREFIX` is required as well)

### CUDA Libraries
To enable the GPU mode of **symPACK** for CUDA devices, use the `-DENABLE_CUDA=ON|OFF` build option. Note that this build option is deactivated by default. 

**symPACK's** GPU mode requires CUDA version 6.0 or later. It also requires the `cuBLAS` and `cuSolver` libraries, both of which can be found in CUDA Toolkit 8.0 or later.

### Notes for specific platforms

Some platforms have preconfigured toolchain files which can be used by adding the following option to the `cmake` command:
```
-DCMAKE_TOOLCHAIN_FILE=/path/to/sympack/toolchains/cori.cmake     
(To build on NERSC Cori for instance)

```

A sample toolchain file can be found in `/path/to/sympack/toolchains/build_config.cmake` and customized for the target platform.

## Building symPACK
---------------------------

The `cmake` command will configure the build process, which can now start by typing:
```
make
make install
```

Additionally, standalone drivers for **symPACK** can be built by typing `make examples`

### Build options

**SymPACK** has several optional features. We have already mentioned the options related to sparse matrix ordering. The following list is the complete set of options available when building **symPACK**:

- `-DENABLE_METIS` to enable **MeTiS** ordering (`ON|OFF`)

- `-DENABLE_PARMETIS` to enable **ParMETIS** ordering (`ON|OFF`)

- `-DENABLE_SCOTCH` to enable **SCOTCH** ordering (`ON|OFF`)

- `-DENABLE_PTSCOTCH` to enable **PT-SCOTCH** ordering (`ON|OFF`)

- `-DAMD_IDX_64` to use 64 bit integers for AMD ordering (`ON|OFF`)

- `-DMMD_IDX_64` to use 64 bit integers for MMD ordering (`ON|OFF`)

- `-DRCM_IDX_64` to use 64 bit integers for RCM ordering (`ON|OFF`)

- `-DENABLE_MKL` to enable Intel MKL through the `-mkl` flag (`ON|OFF`)

- `-DENABLE_THREADS` to enable multithreading (`ON|OFF`). `UPCXX_THREADMODE=par` is required during cmake configuration. **SymPACK** implements its own multithreading and as such should be linked with **sequential** BLAS/LAPACK libraries.

- `-DENABLE_CUDA` to enable CUDA mode. This allows certain sufficiently large computations to be offloaded to CUDA devices (`ON|OFF`)

# Running symPACK
---------------------------

**SymPACK** uses both **UPC++** and **MPI**, therefore, some preliminary environment variables may need to be defined on most platforms.
More details can be found in the **UPC++** [documentation](https://bitbucket.org/berkeleylab/upcxx/wiki/docs/mpi-hybrid):
```
export UPCXX_NETWORK=udp #good option on most workstations, but NOT on high performance networks 
export GASNET_SPAWNFN='C'
export GASNET_CSPAWN_CMD='mpirun -np %N %C'
```

To run the standalone **symPACK** driver, for instance on a sample problem from the [Suite Sparse matrix collection](https://sparse.tamu.edu),
the procedure below can be followed:
```
#first download the input matrix (we are using nasa2146 here)
wget https://sparse.tamu.edu/RB/Nasa/nasa2146.tar.gz
#extract the matrix
tar xzf nasa2146.tar.gz

#run sympack
upcxx-run -n 4 -- ./run_sympack -in nasa2146/nasa2146.rb -ordering MMD -nrhs 1
```
For larger problems, like [audikw_1](https://www.cise.ufl.edu/research/sparse/RB/GHS_psdef/audikw_1.tar.gz), it is strongly advised to use more efficient orderings (such as **METIS**).
Note that to run **symPACK**, `upcxx-run` is used in the example above, but on some platforms, such as NERSC Cori,
other launchers may be used to both spawn **MPI** and **UPC++**, such as `srun` if the system is using SLURM.
Moreover, for larger problems, the `-shared-heap XX` argument to `upcxx-run` may be needed (Please refer to **UPC++** documentation).

# GPU Mode Options
---------------------------
**SymPACK** provides several optional command line options that allow the user to configure certain aspects of the GPU mode. They are summarized here:

- `-gpu_mem <SIZE>` controls the amount of memory (in bytes) allocated on the GPU upon calling the `symPACK_cuda_setup()` method. If this option is not specified, **symPACK** will partition a device's memory equally among each process bound to the device.
- `-gpu_blk <SIZE>` controls the size threshold (in bytes) that a factorized diagonal block must exceed for it to be automatically copied to the GPU bound to a remote process when being sent to said remote process. This allows for an intermediate copy to host memory to be bypassed, which can reduce communication overhead for certain problems.
- `{trsm, gemm, potrf, syrk}_limit <SIZE>` controls how many nonzero entries a block must have for the given matrix operation (TRSM, GEMM, POTRF, or SYRK) involving said block to be offloaded to the GPU. The GPU will generally be most beneficial for operations involving large blocks with many nonzeros, but the exact size thresholds for when each operation starts to run faster on the GPU will differ between devices.
- `-gpu_solve` determines if the GPU will be used for **symPACK's** triangular solve routine.

# Publications
--------------------------

## Recent publications

* John Bachan, Scott B. Baden, Steven Hofmeyr, Mathias Jacquelin, Amir Kamil, Dan Bonachea, Paul H. Hargrove, Hadia Ahmed.  
"[**UPC++: A High-Performance Communication Framework for Asynchronous Computation**](https://github.com/symPACK/symPACK/wiki/pubs/ipdps19-upcxx.pdf)" (see Section IV-D-4),    
In *[33rd IEEE International Parallel & Distributed Processing Symposium](https://doi.org/10.1109/IPDPS.2019.00104) ([IPDPS'19](https://www.ipdps.org/ipdps2019/))*, May 20-24, 2019, Rio de Janeiro, Brazil. IEEE. 11 pages.  
https://doi.org/10.25344/S4V88H  
[Talk slides](https://github.com/symPACK/symPACK/wiki/pubs/ipdps19-upcxx-slides.pdf)

## Older publications (based on UPC++ v0.1 version of symPACK)

* Mathias Jacquelin, Yili Zheng, Esmond Ng, Katherine Yelick    
["**An Asynchronous Task-based Fan-Both Sparse Cholesky Solver**"](https://arxiv.org/abs/1608.00044),     
[arXiv:1608.00044](https://arxiv.org/abs/1608.00044) [cs.MS], Aug 2016.    
https://doi.org/10.48550/arXiv.1608.00044

* Mathias Jacquelin, Yili Zheng, Esmond Ng, Katherine Yelick   
["**symPACK: a solver for sparse Symmetric Matrices**"](https://github.com/symPACK/symPACK/wiki/pubs/sc16-sympack-poster.pdf),    
Research Poster at [The International Conference for High Performance Computing, Networking, Storage and Analysis (SC16)](http://sc16.supercomputing.org/sc-archive/tech_poster/poster_files/post222s2-file2.pdf), Nov 2016.


