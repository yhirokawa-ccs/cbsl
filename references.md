# CBSL library references

# Build and link libraries

## C

```bash
export CBSL_INCLUDE_PATH=${CBSL_INSTALLED_PATH}/include
export CBSL_LIBRARY_PATH=${CBSL_INSTALLED_PATH}/lib

cc -c xcbsl_use_code.c -I${CBSL_INCLUDE_PATH}
cc -o xcbsl_use_program ${OBJS} -lcbsl -lzstd -L${CBSL_LIBRARY_PATH}
```

## Fortran

```bash
export CBSL_INCLUDE_PATH=${CBSL_INSTALLED_PATH}/include
export CBSL_LIBRARY_PATH=${CBSL_INSTALLED_PATH}/lib

f95 -c xcbslf_use_code.f95 -I${CBSL_INCLUDE_PATH}
f95 -o xcbslf_use_program ${OBJS} -lcbslf -lcbsl -lzstd -L${CBSL_LIBRARY_PATH}
```

# API

We provide `cbslf` module for using the library in Fortran application.

## Versioning

|Version|Value                  |
|-------|-----------------------|
|major  |release year (ex. 2019)|
|minor  |release month (ex. 5)  |
|patch  |patch version (0-9)    |

### C

```c
#define CBSL_VERSION        // full version (integer)
#define CBSL_VERSION_STRING // full version (string)
#define CBSL_MAJOR_VERSION  // major
#define CBSL_MINOR_VERSION  // minor
#define CBSL_PATCH_VERSION  // patch
```

### Fortran

```fortran
integer(8),   parameter :: CBSL_VERSION
character(*), parameter :: CBSL_VERSION_STRING
integer(4),   parameter :: CBSL_MAJOR_VERSION
integer(4),   parameter :: CBSL_MINOR_VERSION
integer(4),   parameter :: CBSL_PATCH_VERSION
```

## Types

### C

```c
// library context
typedef struct cbsl_ctx_ cbsl_ctx;
```
### Fortran

```fortran
type(cbslf_context)
```

## Constant values

### C

```c
// Context mode
typedef enum
{
  cbsl_load_mode,    // deserialize mode
  cbsl_store_mode,   // serialize mode
  cbsl_unknown_mode  // unknown
}
cbsl_mode;

// Errors
typedef enum
{
  cbsl_success,  // no error
  cbsl_error     // function happens any error
}
cbsl_errors;
```

### Fortran

```fortran
integer(4), parameter :: cbslf_load_mode
integer(4), parameter :: cbslf_store_mode
integer(4), parameter :: cbslf_unknown_mode

integer(4), parameter :: cbslf_success
integer(4), parameter :: cbslf_errors
```

## API

### C

```c
/*
   [brief]
        open serialized data file stream.
   [arguments]
        open_mode: specifies load/store mode
        path     : data file path
   [return]
        success  : return context pointer
        fail     : NULL pointer
 */
cbsl_ctx* cbsl_open(cbsl_mode open_mode, char* path);

/*
   [brief]
        close serialized data file stream.
        this function calls cbsl_flush() before closing stream.
   [arguments]
        ctx      : context pointer
   [return]
        success  : cbsl_success
        fail     : cbsl_error
 */
cbsl_errors cbsl_close(cbsl_ctx* ctx);

/*
   [brief]
        flush file stream.
        this function executes data compression and write to file stream.
   [arguments]
        ctx      : context pointer
   [return]
        success  : cbsl_success
        fail     : cbsl_error
 */
cbsl_errors cbsl_flush(cbsl_ctx* ctx);

/*
   [brief]
        the data stores with compression.
        this function stores internal buffer to stored data.
        When the stored data fills enough size to compress, this function calls cbsl_flush().
   [arguments]
        ctx      : context pointer
        data     : write data pointer (must be allocated)
        size     : byte size of data
   [return]
        success  : cbsl_success
        fail     : cbsl_error
 */
cbsl_errors cbsl_write(cbsl_ctx* ctx, const void* data, uint64_t size);

/*
   [brief]
        the data loads with decompression.
        this function decompresses data from a file stream, and loads from internal decompressed buffer.
   [arguments]
        ctx      : context pointer
        data     : read data pointer (must be allocated)
        size     : byte size of data
   [return]
        success  : cbsl_success
        fail     : cbsl_error
 */
cbsl_errors cbsl_read(cbsl_ctx* ctx, void* data, uint64_t size);

/*
   [brief]
        the data loads/stores with compression/decompression.
        this function calls cbsl_read()/cbsl_write() by context mode to help the implementation of checkpoint/restart in the application.
   [argumetns]
        ctx      : context pointer
        data     : read/write data pointer (must be allocated)
        size     : byte size of data
   [return]
        success  : cbsl_success
        fail     : cbsl_error
 */
cbsl_errors cbsl_record(cbsl_ctx* ctx, void* data, uint64_t size);

/*
   [brief]
        this is a specialized function of cbsl_record() for heap allocated array.
        the function loads/stores array data size [bytes] and array values from/to a file stream.
        an array will be allocated on heap memory by malloc() routine if `data` is null pointer.
   [arguments]
        ctx      : context pointer
        data     : read/write data pointer (it accepts NULL pointer)
        size     : byte size of data
   [return]
        success  : cbsl_success
        fail     : cbsl_error
 */
cbsl_errors cbsl_record_heap(cbsl_ctx* ctx, void** data, uint64_t* size);

/*
   [brief]
        gets context mode.
   [arguments]
        ctx      : context pointer
   [return]
        success  : context mode
 */
cbsl_mode cbsl_get_mode(cbsl_ctx* ctx);

/*
   [brief]
        sets zstd compression level of serialized data.
   [arguments]
        ctx      : context pointer
        clevel   : compression level 1-22 (zstd 1.4.0)
   [return]
        success  : cbsl_success
        fail     : cbsl_error
 */
cbsl_errors cbsl_set_compression_level(cbsl_ctx* ctx, int clevel);

/*
   [brief]
        gets zstd compression level of serialized data.
   [arguments]
        ctx      : context pointer
   [return]
        success  : compression level 1-22 (zstd 1.4.0)
        fail     : -1
 */
int cbsl_get_compression_level(cbsl_ctx* ctx);

/*
   [brief]
        sets training data sets. it use the dictionary of zstd.
   [arguments]
        ctx      : context pointer
        src      : traning source
        sample_size: size of a sample (>= 8)
        total_size : total size of a source
   [return]
        success  : cbsl_success
        fail     : cbsl_error
   [note]
        this function is usable if `cbsl_get_mode(ctx) == cbsl_store_mode`.
        they refer up to 100 KiB of a source data from `src` head address.
        for creating the efficient dictionary, zstd may prefers a largest array in your application.
        the function must be invoked before `cbsl_write()` or `cbsl_record()`.
 */
cbsl_errors cbsl_train_data(cbsl_ctx* ctx, const void* src, size_t sample_size, uint64_t total_size);
```

### Fortran

```fortran
!
! [brief]
!      open serialized data file stream.
! [arguments]
!      open_mode: specifies load/store mode
!      path     : data file path
!      errcode  : error code
! [return]
!      success  : return context pointer
!      fail     : NULL pointer
!
function cbslf_open(open_mode, path, errcode) result(ctx)
  integer(4),   intent(in)          :: open_mode
  character(*), intent(in)          :: path
  integer(4), intent(out), optional :: errcode
  type(cbslf_context)               :: ctx
end function

!
! [brief]
!      close serialized data file stream.
!      this subroutine calls cbslf_flush() before closing stream.
! [arguments]
!      ctx      : context pointer
!      errcode  : error code
!
subroutine cbslf_close(ctx, errcode)
  type(cbslf_context), intent(in)   :: ctx
  integer(4), intent(out), optional :: errcode
end subroutine

!
! [brief]
!      flush file stream.
!      this subroutine executes data compression and write to file stream.
! [arguments]
!      ctx      : context pointer
!      errcode  : error code
!
subroutine cbslf_flush(ctx, errcode)
  type(cbslf_context), intent(in)   :: ctx
  integer(4), intent(out), optional :: errcode
end subroutine

!
! [brief]
!      this is a generic interface.
!      the data stores with compression.
!      this subroutine stores internal buffer to stored data.
!      When the stored data fills enough size to compress, this subroutine calls cbslf_flush().
! [supported types]
!      scalar   : logical, character(*)
!      +array   : integer(4), integer(8), real(4), real(8), complex(4), complex(8) by up to 7-dimensional array
! [arguments]
!      ctx      : context pointer
!      data     : write data (array must be allocated)
!      errcode  : error code
!
interface cbslf_write(ctx, data, errcode)
  type(cbslf_context), intent(in)   :: ctx
  GENERIC_TYPE,         intent(in)  :: data
  integer(4), intent(out), optional :: errcode
end interface

!
! [brief]
!      this is a generic interface.
!      the data loads with decompression.
!      this subroutine decompresses data from a file stream, and loads from internal decompressed buffer.
! [supported types]
!      scalar   : logical, character(*)
!      +array   : integer(4), integer(8), real(4), real(8), complex(4), complex(8) by 7-dimensional array
! [arguments]
!      ctx      : context pointer
!      data     : write data (array must be allocated)
!      errcode  : error code
!
interface cbslf_read(ctx, data, errcode)
  type(cbslf_context), intent(in)   :: ctx
  GENERIC_TYPE,         intent(out) :: data
  integer(4), intent(out), optional :: errcode
end interface

!
! [brief]
!      this is a generic interface.
!      the data loads/stores with compression/decompression.
!      this subroutine calls cbsl_read()/cbsl_write() by context mode to help the implementation of checkpoint/restart in the application.
! [supported types]
!      scalar   : logical, character(*)
!      +array   : integer(4), integer(8), real(4), real(8), complex(4), complex(8) by up to 7-dimensional array
! [arguments]
!      ctx      : context pointer
!      data     : read/write data (array must be allocated)
!      size     : byte size of data
!      errcode  : error code (return value)
!
interface cbslf_record(ctx, data, errcode)
  type(cbslf_context), intent(in)     :: ctx
  GENERIC_TYPE,         intent(inout) :: data
  integer(4), intent(out), optional   :: errcode
end interface

!
! [brief]
!      this is a generic interface for allocatable array.
!      the data loads/stores with compression/decompression.
!      this subroutine calls cbsl_read()/cbsl_write() by context mode to help the implementation of checkpoint/restart in the application.
!      this subroutine behaves as like as cbsl_record_heap() in C API.
!      an array will be allocated on the memory if it is not allocated.
! [supported types]
!      array    : integer(4), integer(8), real(4), real(8), complex(4), complex(8) by up to 7-dimensional array
! [arguments]
!      ctx      : context pointer
!      data     : read/write data (accepts not allocated array)
!      size     : byte size of data
!      errcode  : error code
!
interface cbslf_record_heap(ctx, data, errcode)
  type(cbslf_context), intent(in)     :: ctx
  GENERIC_TYPE,         intent(inout) :: data
  integer(4), intent(out), optional   :: errcode
end interface

!
! [brief]
!      gets context mode.
! [arguments]
!      ctx      : context pointer
!      errcode  : error code (return value)
! [return]
!      success  : context mode
!
function cbslf_get_mode(ctx, errcode) result(mode)
  type(cbslf_context), intent(in)   :: ctx
  integer(4), intent(out), optional :: errcode
  integer(4)                        :: mode
end function

!
! [brief]
!      sets zstd compression level of serialized data.
! [arguments]
!      ctx      : context pointer
!      clevel   : compression level 1-22 (zstd 1.4.0)
!      errcode  : error code (return value)
!
subroutine cbslf_set_compression_level(ctx, clevel)
  type(cbslf_context), intent(in)   :: ctx
  ingeter(4),           intent(in)  :: clevel
  integer(4), intent(out), optional :: errcode
end subroutine

!
! [brief]
!      gets zstd compression level of serialized data.
! [arguments]
!      ctx      : context pointer
! [return]
!      success  : compression level 1-22 (zstd 1.4.0)
!      fail     : -1
!
function cbslf_get_compression_level(ctx, errcode) result(clevel)
  type(cbslf_context), intent(in)   :: ctx
  integer(4), intent(out), optional :: errcode
  ingeter(4),                       :: clevel
end function

!
! [brief]
!      this is a generic interface.
!      sets training data sets. it use the dictionary of zstd.
!      please read C API note.
! [supported types of src]
!      integer(4), integer(8), real(4), real(8), complex(4), complex(8) by up to 7-dimensional array
! [arguments]
!      ctx      : context pointer
!      src      : traning source
!      sample_size: size of a sample (> 8)
!      total_size : total size of a source
!      errcode  : error code (return value)
!
interface cbslf_train_data(ctx, src, sample_size, total_size, errcode)
  type(cbslf_context), intent(in)   :: ctx
  GENERIC_TYPE,        intent(in)   :: src
  integer(4),          intent(in)   :: sample_size
  integer(4),          intent(in)   :: total_size
  integer(4), intent(out), optional :: errcode
end interface
```

## Data format

```
BDATA: binary data                  [1 byte]
BSIZE: total size of binary data    [4 bytes]
RSIZE: rank size of a fortran array [4 bytes]
NRANK: number of rank size of a fortran array

sizeof(TYPE):     gets byte size of TYPE
Array-value-type: element type of an array
```

### Header

```
[BDATA * 8]         : cbsl library version (64-bit integer)
[BDATA * 100 * 1024]: the dictionary of zstd
```

### C API

```
Scalar data: HEAD -> [BDATA * sizeof(Scalar)] -> TAIL
Array data : HEAD -> [BSIZE][BDATA * sizeof(Array-value-type) * BSIZE] -> TAIL
```

### Fortran API

```
Scalar data: HEAD -> [BDATA * sizeof(Scalar)] -> TAIL
Array data : HEAD -> [BSIZE][ARRAY_LOWER_BOUNDS][ARRAY_UPPER_BOUNDS][BDATA * sizeof(Array-value-type) * BSIZE] -> TAIL

ARRAY_LOWER_BOUNDS: lower bounds of a fortran array [RSIZE * NRANK]
ARRAY_UPPER_BOUNDS: upper bounds of a fortran array [RSIZE * NRANK]
```
