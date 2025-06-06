#include "../client/client.h"
#include "unzip.h"

/* unzip.h -- IO for uncompress .zip files using zlib
   Version 0.15 beta, Mar 19th, 1998,

   Copyright (C) 1998 Gilles Vollant

   This unzip package allow extract file from .ZIP file, compatible with PKZip 2.04g
     WinZip, InfoZip tools and compatible.
   Encryption and multi volume ZipFile (span) are not supported.
   Old compressions used by old PKZip 1.x are not supported

   THIS IS AN ALPHA VERSION. AT THIS STAGE OF DEVELOPPEMENT, SOMES API OR STRUCTURE
   CAN CHANGE IN FUTURE VERSION !!
   I WAIT FEEDBACK at mail info@winimage.com
   Visit also http://www.winimage.com/zLibDll/unzip.htm for evolution

   Condition of use and distribution are the same than zlib :

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.


*/
/* for more info about .ZIP format, see
      ftp://ftp.cdrom.com/pub/infozip/doc/appnote-970311-iz.zip
   PkWare has also a specification at :
      ftp://ftp.pkware.com/probdesc.zip */

/* zlib.h -- interface of the 'zlib' general purpose compression library
  version 1.1.3, July 9th, 1998

  Copyright (C) 1995-1998 Jean-loup Gailly and Mark Adler

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  Jean-loup Gailly        Mark Adler
  jloup@gzip.org          madler@alumni.caltech.edu


  The data format used by the zlib library is described by RFCs (Request for
  Comments) 1950 to 1952 in the files ftp://ds.internic.net/rfc/rfc1950.txt
  (zlib format), rfc1951.txt (deflate format) and rfc1952.txt (gzip format).
*/

/* zconf.h -- configuration of the zlib compression library
 * Copyright (C) 1995-1998 Jean-loup Gailly.
 * For conditions of distribution and use, see copyright notice in zlib.h
 */


#ifndef _ZCONF_H
    #define _ZCONF_H

    /* Maximum value for memLevel in deflateInit2 */
    #ifndef MAX_MEM_LEVEL
        #ifdef MAXSEG_64K
            #define MAX_MEM_LEVEL 8
        #else
            #define MAX_MEM_LEVEL 9
        #endif
    #endif

    /* Maximum value for windowBits in deflateInit2 and inflateInit2.
     * WARNING: reducing MAX_WBITS makes minigzip unable to extract .gz files
     * created by gzip. (Files created by minigzip can still be extracted by
     * gzip.)
     */
    #ifndef MAX_WBITS
        #define MAX_WBITS 15 /* 32K LZ77 window */
    #endif

/* The memory requirements for deflate are (in bytes):
            (1 << (windowBits+2)) +  (1 << (memLevel+9))
 that is: 128K for windowBits=15  +  128K for memLevel = 8  (default values)
 plus a few kilobytes for small objects. For example, if you want to reduce
 the default memory requirements from 256K to 128K, compile with
     make CFLAGS="-O -DMAX_WBITS=14 -DMAX_MEM_LEVEL=7"
 Of course this will generally degrade compression (there's no free lunch).

   The memory requirements for inflate are (in bytes) 1 << windowBits
 that is, 32K for windowBits=15 (default value) plus a few kilobytes
 for small objects.
*/

/* Type declarations */

    #ifndef OF /* function prototypes */
        #define OF(args) args
    #endif

    #ifndef SEEK_SET
        #define SEEK_SET 0 /* Seek from beginning of file.  */
        #define SEEK_CUR 1 /* Seek from current position.  */
        #define SEEK_END 2 /* Set file pointer to EOF plus "offset" */
    #endif

#endif /* _ZCONF_H */

#define ZLIB_VERSION          "1.1.3"

/*
     The 'zlib' compression library provides in-memory compression and
  decompression functions, including integrity checks of the uncompressed
  data.  This version of the library supports only one compression method
  (deflation) but other algorithms will be added later and will have the same
  stream interface.

     Compression can be done in a single step if the buffers are large
  enough (for example if an input file is mmap'ed), or can be done by
  repeated calls of the compression function.  In the latter case, the
  application must provide more input and/or consume the output
  (providing more output space) before each call.

     The library also supports reading and writing files in gzip (.gz) format
  with an interface similar to that of stdio.

     The library does not install any signal handler. The decoder checks
  the consistency of the compressed data, so the library should never
  crash even in case of corrupted input.
*/

/*
   The application must update next_in and avail_in when avail_in has
   dropped to zero. It must update next_out and avail_out when avail_out
   has dropped to zero. The application must initialize zalloc, zfree and
   opaque before calling the init function. All other fields are set by the
   compression library and must not be updated by the application.

   The opaque value provided by the application will be passed as the first
   parameter for calls of zalloc and zfree. This can be useful for custom
   memory management. The compression library attaches no meaning to the
   opaque value.

   zalloc must return Z_NULL if there is not enough memory for the object.
   If zlib is used in a multi-threaded application, zalloc and zfree must be
   thread safe.

   On 16-bit systems, the functions zalloc and zfree must be able to allocate
   exactly 65536 bytes, but will not be required to allocate more than this
   if the symbol MAXSEG_64K is defined (see zconf.h). WARNING: On MSDOS,
   pointers returned by zalloc for objects of exactly 65536 bytes *must*
   have their offset normalized to zero. The default allocation function
   provided by this library ensures this (see zutil.c). To reduce memory
   requirements and avoid any allocation of 64K objects, at the expense of
   compression ratio, compile the library with -DMAX_WBITS=14 (see zconf.h).

   The fields total_in and total_out can be used for statistics or
   progress reports. After compression, total_in holds the total size of
   the uncompressed data and may be saved for use in the decompressor
   (particularly if the decompressor wants to decompress everything in
   a single step).
*/

/* constants */

#define Z_NO_FLUSH            0
#define Z_PARTIAL_FLUSH       1 /* will be removed, use Z_SYNC_FLUSH instead */
#define Z_SYNC_FLUSH          2
#define Z_FULL_FLUSH          3
#define Z_FINISH              4
/* Allowed flush values; see deflate() below for details */

#define Z_OK                  0
#define Z_STREAM_END          1
#define Z_NEED_DICT           2
#define Z_ERRNO               (-1)
#define Z_STREAM_ERROR        (-2)
#define Z_DATA_ERROR          (-3)
#define Z_MEM_ERROR           (-4)
#define Z_BUF_ERROR           (-5)
#define Z_VERSION_ERROR       (-6)
/* Return codes for the compression/decompression functions. Negative
 * values are errors, positive values are used for special but normal events.
 */

#define Z_NO_COMPRESSION      0
#define Z_BEST_SPEED          1
#define Z_BEST_COMPRESSION    9
#define Z_DEFAULT_COMPRESSION (-1)
/* compression levels */

#define Z_FILTERED            1
#define Z_HUFFMAN_ONLY        2
#define Z_DEFAULT_STRATEGY    0
/* compression strategy; see deflateInit2() below for details */

#define Z_BINARY              0
#define Z_ASCII               1
#define Z_UNKNOWN             2
/* Possible values of the data_type field */

#define Z_DEFLATED            8
/* The deflate compression method (the only one supported in this version) */

#define Z_NULL                0 /* for initializing zalloc, zfree, opaque */

#define zlib_version          zlibVersion()
/* for compatibility with versions < 1.0.2 */

/* basic functions */

// static const char * zlibVersion OF(());
/* The application can compare zlibVersion and ZLIB_VERSION for consistency.
   If the first character differs, the library code actually used is
   not compatible with the zlib.h header file used by the application.
   This check is automatically made by deflateInit and inflateInit.
 */

/*
int32 deflateInit OF((z_streamp strm, int32 level));

     Initializes the internal stream state for compression. The fields
   zalloc, zfree and opaque must be initialized before by the caller.
   If zalloc and zfree are set to Z_NULL, deflateInit updates them to
   use default allocation functions.

     The compression level must be Z_DEFAULT_COMPRESSION, or between 0 and 9:
   1 gives best speed, 9 gives best compression, 0 gives no compression at
   all (the input data is simply copied a block at a time).
   Z_DEFAULT_COMPRESSION requests a default compromise between speed and
   compression (currently equivalent to level 6).

     deflateInit returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_STREAM_ERROR if level is not a valid compression level,
   Z_VERSION_ERROR if the zlib library version (zlib_version) is incompatible
   with the version assumed by the caller (ZLIB_VERSION).
   msg is set to null if there is no error message.  deflateInit does not
   perform any compression: this will be done by deflate().
*/


// static int32 deflate OF((z_streamp strm, int32 flush));
/*
    deflate compresses as much data as possible, and stops when the input
  buffer becomes empty or the output buffer becomes full. It may introduce some
  output latency (reading input without producing any output) except when
  forced to flush.

    The detailed semantics are as follows. deflate performs one or both of the
  following actions:

  - Compress more input starting at next_in and update next_in and avail_in
    accordingly. If not all input can be processed (because there is not
    enough room in the output buffer), next_in and avail_in are updated and
    processing will resume at this point for the next call of deflate().

  - Provide more output starting at next_out and update next_out and avail_out
    accordingly. This action is forced if the parameter flush is non zero.
    Forcing flush frequently degrades the compression ratio, so this parameter
    should be set only when necessary (in interactive applications).
    Some output may be provided even if flush is not set.

  Before the call of deflate(), the application should ensure that at least
  one of the actions is possible, by providing more input and/or consuming
  more output, and updating avail_in or avail_out accordingly; avail_out
  should never be zero before the call. The application can consume the
  compressed output when it wants, for example when the output buffer is full
  (avail_out == 0), or after each call of deflate(). If deflate returns Z_OK
  and with zero avail_out, it must be called again after making room in the
  output buffer because there might be more output pending.

    If the parameter flush is set to Z_SYNC_FLUSH, all pending output is
  flushed to the output buffer and the output is aligned on a uint8 boundary, so
  that the decompressor can get all input data available so far. (In particular
  avail_in is zero after the call if enough output space has been provided
  before the call.)  Flushing may degrade compression for some compression
  algorithms and so it should be used only when necessary.

    If flush is set to Z_FULL_FLUSH, all output is flushed as with
  Z_SYNC_FLUSH, and the compression state is reset so that decompression can
  restart from this point if previous compressed data has been damaged or if
  random access is desired. Using Z_FULL_FLUSH too often can seriously degrade
  the compression.

    If deflate returns with avail_out == 0, this function must be called again
  with the same value of the flush parameter and more output space (updated
  avail_out), until the flush is complete (deflate returns with non-zero
  avail_out).

    If the parameter flush is set to Z_FINISH, pending input is processed,
  pending output is flushed and deflate returns with Z_STREAM_END if there
  was enough output space; if deflate returns with Z_OK, this function must be
  called again with Z_FINISH and more output space (updated avail_out) but no
  more input data, until it returns with Z_STREAM_END or an error. After
  deflate has returned Z_STREAM_END, the only possible operations on the
  stream are deflateReset or deflateEnd.

    Z_FINISH can be used immediately after deflateInit if all the compression
  is to be done in a single step. In this case, avail_out must be at least
  0.1% larger than avail_in plus 12 bytes.  If deflate does not return
  Z_STREAM_END, then it must be called again as described above.

    deflate() sets strm->adler to the adler32 checksum of all input read
  so (that is, total_in bytes).

    deflate() may update data_type if it can make a good guess about
  the input data type (Z_ASCII or Z_BINARY). In doubt, the data is considered
  binary. This field is only for information purposes and does not affect
  the compression algorithm in any manner.

    deflate() returns Z_OK if some progress has been made (more input
  processed or more output produced), Z_STREAM_END if all input has been
  consumed and all output has been produced (only when flush is set to
  Z_FINISH), Z_STREAM_ERROR if the stream state was inconsistent (for example
  if next_in or next_out was NULL), Z_BUF_ERROR if no progress is possible
  (for example avail_in or avail_out was zero).
*/


// static int32 deflateEnd OF((z_streamp strm));
/*
     All dynamically allocated data structures for this stream are freed.
   This function discards any unprocessed input and does not flush any
   pending output.

     deflateEnd returns Z_OK if success, Z_STREAM_ERROR if the
   stream state was inconsistent, Z_DATA_ERROR if the stream was freed
   prematurely (some input or output was discarded). In the error case,
   msg may be set but then points to a static string (which must not be
   deallocated).
*/


/*
int32 inflateInit OF((z_streamp strm));

     Initializes the internal stream state for decompression. The fields
   next_in, avail_in, zalloc, zfree and opaque must be initialized before by
   the caller. If next_in is not Z_NULL and avail_in is large enough (the exact
   value depends on the compression method), inflateInit determines the
   compression method from the zlib header and allocates all data structures
   accordingly; otherwise the allocation will be deferred to the first call of
   inflate.  If zalloc and zfree are set to Z_NULL, inflateInit updates them to
   use default allocation functions.

     inflateInit returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_VERSION_ERROR if the zlib library version is incompatible with the
   version assumed by the caller.  msg is set to null if there is no error
   message. inflateInit does not perform any decompression apart from reading
   the zlib header if present: this will be done by inflate().  (So next_in and
   avail_in may be modified, but next_out and avail_out are unchanged.)
*/


static int32 inflate OF((z_streamp strm, int32 flush));
/*
    inflate decompresses as much data as possible, and stops when the input
  buffer becomes empty or the output buffer becomes full. It may some
  introduce some output latency (reading input without producing any output)
  except when forced to flush.

  The detailed semantics are as follows. inflate performs one or both of the
  following actions:

  - Decompress more input starting at next_in and update next_in and avail_in
    accordingly. If not all input can be processed (because there is not
    enough room in the output buffer), next_in is updated and processing
    will resume at this point for the next call of inflate().

  - Provide more output starting at next_out and update next_out and avail_out
    accordingly.  inflate() provides as much output as possible, until there
    is no more input data or no more space in the output buffer (see below
    about the flush parameter).

  Before the call of inflate(), the application should ensure that at least
  one of the actions is possible, by providing more input and/or consuming
  more output, and updating the next_* and avail_* values accordingly.
  The application can consume the uncompressed output when it wants, for
  example when the output buffer is full (avail_out == 0), or after each
  call of inflate(). If inflate returns Z_OK and with zero avail_out, it
  must be called again after making room in the output buffer because there
  might be more output pending.

    If the parameter flush is set to Z_SYNC_FLUSH, inflate flushes as much
  output as possible to the output buffer. The flushing behavior of inflate is
  not specified for values of the flush parameter other than Z_SYNC_FLUSH
  and Z_FINISH, but the current implementation actually flushes as much output
  as possible anyway.

    inflate() should normally be called until it returns Z_STREAM_END or an
  error. However if all decompression is to be performed in a single step
  (a single call of inflate), the parameter flush should be set to
  Z_FINISH. In this case all pending input is processed and all pending
  output is flushed; avail_out must be large enough to hold all the
  uncompressed data. (The size of the uncompressed data may have been saved
  by the compressor for this purpose.) The next operation on this stream must
  be inflateEnd to deallocate the decompression state. The use of Z_FINISH
  is never required, but can be used to inform inflate that a faster routine
  may be used for the single inflate() call.

     If a preset dictionary is needed at this point (see inflateSetDictionary
  below), inflate sets strm-adler to the adler32 checksum of the
  dictionary chosen by the compressor and returns Z_NEED_DICT; otherwise
  it sets strm->adler to the adler32 checksum of all output produced
  so (that is, total_out bytes) and returns Z_OK, Z_STREAM_END or
  an error code as described below. At the end of the stream, inflate()
  checks that its computed adler32 checksum is equal to that saved by the
  compressor and returns Z_STREAM_END only if the checksum is correct.

    inflate() returns Z_OK if some progress has been made (more input processed
  or more output produced), Z_STREAM_END if the end of the compressed data has
  been reached and all uncompressed output has been produced, Z_NEED_DICT if a
  preset dictionary is needed at this point, Z_DATA_ERROR if the input data was
  corrupted (input stream not conforming to the zlib format or incorrect
  adler32 checksum), Z_STREAM_ERROR if the stream structure was inconsistent
  (for example if next_in or next_out was NULL), Z_MEM_ERROR if there was not
  enough memory, Z_BUF_ERROR if no progress is possible or if there was not
  enough room in the output buffer when Z_FINISH is used. In the Z_DATA_ERROR
  case, the application may then call inflateSync to look for a good
  compression block.
*/


static int32 inflateEnd OF((z_streamp strm));
/*
     All dynamically allocated data structures for this stream are freed.
   This function discards any unprocessed input and does not flush any
   pending output.

     inflateEnd returns Z_OK if success, Z_STREAM_ERROR if the stream state
   was inconsistent. In the error case, msg may be set but then points to a
   static string (which must not be deallocated).
*/

/* Advanced functions */

/*
    The following functions are needed only in some special applications.
*/

/*
int32 deflateInit2 OF((z_streamp strm,
                                     int32  level,
                                     int32  method,
                                     int32  windowBits,
                                     int32  memLevel,
                                     int32  strategy));

     This is another version of deflateInit with more compression options. The
   fields next_in, zalloc, zfree and opaque must be initialized before by
   the caller.

     The method parameter is the compression method. It must be Z_DEFLATED in
   this version of the library.

     The windowBits parameter is the base two logarithm of the window size
   (the size of the history buffer).  It should be in the range 8..15 for this
   version of the library. Larger values of this parameter result in better
   compression at the expense of memory usage. The default value is 15 if
   deflateInit is used instead.

     The memLevel parameter specifies how much memory should be allocated
   for the internal compression state. memLevel=1 uses minimum memory but
   is slow and reduces compression ratio; memLevel=9 uses maximum memory
   for optimal speed. The default value is 8. See zconf.h for total memory
   usage as a function of windowBits and memLevel.

     The strategy parameter is used to tune the compression algorithm. Use the
   value Z_DEFAULT_STRATEGY for normal data, Z_FILTERED for data produced by a
   filter (or predictor), or Z_HUFFMAN_ONLY to force Huffman encoding only (no
   string match).  Filtered data consists mostly of small values with a
   somewhat random distribution. In this case, the compression algorithm is
   tuned to compress them better. The effect of Z_FILTERED is to force more
   Huffman coding and less string matching; it is somewhat intermediate
   between Z_DEFAULT and Z_HUFFMAN_ONLY. The strategy parameter only affects
   the compression ratio but not the correctness of the compressed output even
   if it is not set appropriately.

      deflateInit2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_STREAM_ERROR if a parameter is invalid (such as an invalid
   method). msg is set to null if there is no error message.  deflateInit2 does
   not perform any compression: this will be done by deflate().
*/

/*
static int32 deflateSetDictionary OF((z_streamp strm,
                                             const uint8 *dictionary,
                                             uint32  dictLength));
*/
/*
     Initializes the compression dictionary from the given uint8 sequence
   without producing any compressed output. This function must be called
   immediately after deflateInit, deflateInit2 or deflateReset, before any
   call of deflate. The compressor and decompressor must use exactly the same
   dictionary (see inflateSetDictionary).

     The dictionary should consist of strings (uint8 sequences) that are likely
   to be encountered later in the data to be compressed, with the most commonly
   used strings preferably put towards the end of the dictionary. Using a
   dictionary is most useful when the data to be compressed is int16 and can be
   predicted with good accuracy; the data can then be compressed better than
   with the default empty dictionary.

     Depending on the size of the compression data structures selected by
   deflateInit or deflateInit2, a part of the dictionary may in effect be
   discarded, for example if the dictionary is larger than the window size in
   deflate or deflate2. Thus the strings most likely to be useful should be
   put at the end of the dictionary, not at the front.

     Upon return of this function, strm->adler is set to the Adler32 value
   of the dictionary; the decompressor may later use this value to determine
   which dictionary has been used by the compressor. (The Adler32 value
   applies to the whole dictionary even if only a subset of the dictionary is
   actually used by the compressor.)

     deflateSetDictionary returns Z_OK if success, or Z_STREAM_ERROR if a
   parameter is invalid (such as NULL dictionary) or the stream state is
   inconsistent (for example if deflate has already been called for this stream
   or if the compression method is bsort). deflateSetDictionary does not
   perform any compression: this will be done by deflate().
*/

/*
static int32 deflateCopy OF((z_streamp dest,
                                    z_streamp source));
*/
/*
     Sets the destination stream as a complete copy of the source stream.

     This function can be useful when several compression strategies will be
   tried, for example when there are several ways of pre-processing the input
   data with a filter. The streams that will be discarded should then be freed
   by calling deflateEnd.  Note that deflateCopy duplicates the internal
   compression state which can be quite large, so this strategy is slow and
   can consume lots of memory.

     deflateCopy returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_STREAM_ERROR if the source stream state was inconsistent
   (such as zalloc being NULL). msg is left unchanged in both source and
   destination.
*/

// static int32 deflateReset OF((z_streamp strm));
/*
     This function is equivalent to deflateEnd followed by deflateInit,
   but does not free and reallocate all the internal compression state.
   The stream will keep the same compression level and any other attributes
   that may have been set by deflateInit2.

      deflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent (such as zalloc or state being NULL).
*/

/*
static int32 deflateParams OF((z_streamp strm,
                      int32 level,
                      int32 strategy));
*/
/*
     Dynamically update the compression level and compression strategy.  The
   interpretation of level and strategy is as in deflateInit2.  This can be
   used to switch between compression and straight copy of the input data, or
   to switch to a different kind of input data requiring a different
   strategy. If the compression level is changed, the input available so far
   is compressed with the old level (and may be flushed); the new level will
   take effect only at the next call of deflate().

     Before the call of deflateParams, the stream state must be set as for
   a call of deflate(), since the currently available input may have to
   be compressed and flushed. In particular, strm->avail_out must be non-zero.

     deflateParams returns Z_OK if success, Z_STREAM_ERROR if the source
   stream state was inconsistent or if a parameter was invalid, Z_BUF_ERROR
   if strm->avail_out was zero.
*/

/*
int32 inflateInit2 OF((z_streamp strm,
                                     int32  windowBits));

     This is another version of inflateInit with an extra parameter. The
   fields next_in, avail_in, zalloc, zfree and opaque must be initialized
   before by the caller.

     The windowBits parameter is the base two logarithm of the maximum window
   size (the size of the history buffer).  It should be in the range 8..15 for
   this version of the library. The default value is 15 if inflateInit is used
   instead. If a compressed stream with a larger window size is given as
   input, inflate() will return with the error code Z_DATA_ERROR instead of
   trying to allocate a larger window.

      inflateInit2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_STREAM_ERROR if a parameter is invalid (such as a negative
   memLevel). msg is set to null if there is no error message.  inflateInit2
   does not perform any decompression apart from reading the zlib header if
   present: this will be done by inflate(). (So next_in and avail_in may be
   modified, but next_out and avail_out are unchanged.)
*/

/*
static int32 inflateSetDictionary OF((z_streamp strm,
                                             const uint8 *dictionary,
                                             uint32  dictLength));
*/
/*
     Initializes the decompression dictionary from the given uncompressed uint8
   sequence. This function must be called immediately after a call of inflate
   if this call returned Z_NEED_DICT. The dictionary chosen by the compressor
   can be determined from the Adler32 value returned by this call of
   inflate. The compressor and decompressor must use exactly the same
   dictionary (see deflateSetDictionary).

     inflateSetDictionary returns Z_OK if success, Z_STREAM_ERROR if a
   parameter is invalid (such as NULL dictionary) or the stream state is
   inconsistent, Z_DATA_ERROR if the given dictionary doesn't match the
   expected one (incorrect Adler32 value). inflateSetDictionary does not
   perform any decompression: this will be done by subsequent calls of
   inflate().
*/

// static int32 inflateSync OF((z_streamp strm));
/*
    Skips invalid compressed data until a full flush point (see above the
  description of deflate with Z_FULL_FLUSH) can be found, or until all
  available input is skipped. No output is provided.

    inflateSync returns Z_OK if a full flush point has been found, Z_BUF_ERROR
  if no more input was provided, Z_DATA_ERROR if no flush point has been found,
  or Z_STREAM_ERROR if the stream structure was inconsistent. In the success
  case, the application may save the current current value of total_in which
  indicates where valid compressed data was found. In the error case, the
  application may repeatedly call inflateSync, providing more input each time,
  until success or end of the input data.
*/

static int32 inflateReset OF((z_streamp strm));
/*
     This function is equivalent to inflateEnd followed by inflateInit,
   but does not free and reallocate all the internal decompression state.
   The stream will keep attributes that may have been set by inflateInit2.

      inflateReset returns Z_OK if success, or Z_STREAM_ERROR if the source
   stream state was inconsistent (such as zalloc or state being NULL).
*/


/* utility functions */

/*
     The following utility functions are implemented on top of the
   basic stream-oriented functions. To simplify the interface, some
   default options are assumed (compression level and memory usage,
   standard memory allocation functions). The source code of these
   utility functions can easily be modified if you need special options.
*/

/*
static int32 compress OF((uint8 *dest,   uint32 *destLen,
                                 const uint8 *source, uint32 sourceLen));
*/
/*
     Compresses the source buffer into the destination buffer.  sourceLen is
   the uint8 length of the source buffer. Upon entry, destLen is the total
   size of the destination buffer, which must be at least 0.1% larger than
   sourceLen plus 12 bytes. Upon exit, destLen is the actual size of the
   compressed buffer.
     This function can be used to compress a whole file at once if the
   input file is mmap'ed.
     compress returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_BUF_ERROR if there was not enough room in the output
   buffer.
*/

/*
static int32 compress2 OF((uint8 *dest,   uint32 *destLen,
                                  const uint8 *source, uint32 sourceLen,
                                  int32 level));
*/
/*
     Compresses the source buffer into the destination buffer. The level
   parameter has the same meaning as in deflateInit.  sourceLen is the uint8
   length of the source buffer. Upon entry, destLen is the total size of the
   destination buffer, which must be at least 0.1% larger than sourceLen plus
   12 bytes. Upon exit, destLen is the actual size of the compressed buffer.

     compress2 returns Z_OK if success, Z_MEM_ERROR if there was not enough
   memory, Z_BUF_ERROR if there was not enough room in the output buffer,
   Z_STREAM_ERROR if the level parameter is invalid.
*/

/*
static int32 uncompress OF((uint8 *dest,   uint32 *destLen,
                                   const uint8 *source, uint32 sourceLen));
*/
/*
     Decompresses the source buffer into the destination buffer.  sourceLen is
   the uint8 length of the source buffer. Upon entry, destLen is the total
   size of the destination buffer, which must be large enough to hold the
   entire uncompressed data. (The size of the uncompressed data must have
   been saved previously by the compressor and transmitted to the decompressor
   by some mechanism outside the scope of this compression library.)
   Upon exit, destLen is the actual size of the compressed buffer.
     This function can be used to decompress a whole file at once if the
   input file is mmap'ed.

     uncompress returns Z_OK if success, Z_MEM_ERROR if there was not
   enough memory, Z_BUF_ERROR if there was not enough room in the output
   buffer, or Z_DATA_ERROR if the input data was corrupted.
*/


typedef uint8* gzFile;

gzFile gzopen OF((const char* path, const char* mode));
/*
     Opens a gzip (.gz) file for reading or writing. The mode parameter
   is as in fopen ("rb" or "wb") but can also include a compression level
   ("wb9") or a strategy: 'f' for filtered data as in "wb6f", 'h' for
   Huffman only compression as in "wb1h". (See the description
   of deflateInit2 for more information about the strategy parameter.)

     gzopen can be used to read a file which is not in gzip format; in this
   case gzread will directly read from the file without decompression.

     gzopen returns NULL if the file could not be opened or if there was
   insufficient memory to allocate the (de)compression state; errno
   can be checked to distinguish the two cases (if errno is zero, the
   zlib error is Z_MEM_ERROR).  */

gzFile gzdopen OF((int32 fd, const char* mode));
/*
     gzdopen() associates a gzFile with the file descriptor fd.  File
   descriptors are obtained from calls like open, dup, creat, pipe or
   fileno (in the file has been previously opened with fopen).
   The mode parameter is as in gzopen.
     The next call of gzclose on the returned gzFile will also close the
   file descriptor fd, just like fclose(fdopen(fd), mode) closes the file
   descriptor fd. If you want to keep fd open, use gzdopen(dup(fd), mode).
     gzdopen returns NULL if there was insufficient memory to allocate
   the (de)compression state.
*/

int32 gzsetparams OF((gzFile file, int32 level, int32 strategy));
/*
     Dynamically update the compression level or strategy. See the description
   of deflateInit2 for the meaning of these parameters.
     gzsetparams returns Z_OK if success, or Z_STREAM_ERROR if the file was not
   opened for writing.
*/

int32 gzread OF((gzFile file, uint8* buf, uint32 len));
/*
     Reads the given number of uncompressed bytes from the compressed file.
   If the input file was not in gzip format, gzread copies the given number
   of bytes into the buffer.
     gzread returns the number of uncompressed bytes actually read (0 for
   end of file, -1 for error). */

int32 gzwrite OF((gzFile file, const uint8* buf, uint32 len));
/*
     Writes the given number of uncompressed bytes into the compressed file.
   gzwrite returns the number of uncompressed bytes actually written
   (0 in case of error).
*/

int32 QDECL gzprintf OF((gzFile file, const char* format, ...));
/*
     Converts, formats, and writes the args to the compressed file under
   control of the format string, as in fprintf. gzprintf returns the number of
   uncompressed bytes actually written (0 in case of error).
*/

int32 gzputs OF((gzFile file, const char* s));
/*
      Writes the given null-terminated string to the compressed file, excluding
   the terminating null character.
      gzputs returns the number of characters written, or -1 in case of error.
*/

char* gzgets OF((gzFile file, char* buf, int32 len));
/*
      Reads bytes from the compressed file until len-1 characters are read, or
   a newline character is read and transferred to buf, or an end-of-file
   condition is encountered.  The string is then terminated with a null
   character.
      gzgets returns buf, or Z_NULL in case of error.
*/

int32 gzputc OF((gzFile file, int32 c));
/*
      Writes c, converted to an uint8, into the compressed file.
   gzputc returns the value that was written, or -1 in case of error.
*/

int32 gzgetc OF((gzFile file));
/*
      Reads one uint8 from the compressed file. gzgetc returns this uint8
   or -1 in case of end of file or error.
*/

int32 gzflush OF((gzFile file, int32 flush));
/*
     Flushes all pending output into the compressed file. The parameter
   flush is as in the deflate() function. The return value is the zlib
   error number (see function gzerror below). gzflush returns Z_OK if
   the flush parameter is Z_FINISH and all output could be flushed.
     gzflush should be called only when strictly necessary because it can
   degrade compression.
*/

int32 gzseek OF((gzFile file, int32 offset, int32 whence));
/*
      Sets the starting position for the next gzread or gzwrite on the
   given compressed file. The offset represents a number of bytes in the
   uncompressed data stream. The whence parameter is defined as in lseek(2);
   the value SEEK_END is not supported.
     If the file is opened for reading, this function is emulated but can be
   extremely slow. If the file is opened for writing, only forward seeks are
   supported; gzseek then compresses a sequence of zeroes up to the new
   starting position.

      gzseek returns the resulting offset location as measured in bytes from
   the beginning of the uncompressed stream, or -1 in case of error, in
   particular if the file is opened for writing and the new starting position
   would be before the current position.
*/

int32 gzrewind OF((gzFile file));
/*
     Rewinds the given file. This function is supported only for reading.

   gzrewind(file) is equivalent to (int32)gzseek(file, 0L, SEEK_SET)
*/

int32 gztell OF((gzFile file));
/*
     Returns the starting position for the next gzread or gzwrite on the
   given compressed file. This position represents a number of bytes in the
   uncompressed data stream.

   gztell(file) is equivalent to gzseek(file, 0L, SEEK_CUR)
*/

int32 gzeof OF((gzFile file));
/*
     Returns 1 when EOF has previously been detected reading the given
   input stream, otherwise zero.
*/

int32 gzclose OF((gzFile file));
/*
     Flushes all pending output if necessary, closes the compressed file
   and deallocates all the (de)compression state. The return value is the zlib
   error number (see function gzerror below).
*/

// static const char * gzerror OF((gzFile file, int32 *errnum));
/*
     Returns the error message for the last error which occurred on the
   given compressed file. errnum is set to zlib error number. If an
   error occurred in the file system and not in the compression library,
   errnum is set to Z_ERRNO and the application may consult errno
   to get the exact error code.
*/

/* checksum functions */

/*
     These functions are not related to compression but are exported
   anyway because they might be useful in applications using the
   compression library.
*/

static uint32 adler32 OF((uint32 adler, const uint8* buf, uint32 len));

/*
     Update a running Adler-32 checksum with the bytes buf[0..len-1] and
   return the updated checksum. If buf is NULL, this function returns
   the required initial value for the checksum.
   An Adler-32 checksum is almost as reliable as a CRC32 but can be computed
   much faster. Usage example:

     uint32 adler = adler32(0L, Z_NULL, 0);

     while (read_buffer(buffer, length) != EOF) {
       adler = adler32(adler, buffer, length);
     }
     if (adler != original_adler) error();
*/

/* various hacks, don't look :) */

/* deflateInit and inflateInit are macros to allow checking the zlib version
 * and the compiler's view of z_stream:
 */
/*
static int32 deflateInit_ OF((z_streamp strm, int32 level,
                                     const char *version, int32 stream_size));
static int32 inflateInit_ OF((z_streamp strm,
                                     const char *version, int32 stream_size));
static int32 deflateInit2_ OF((z_streamp strm, int32  level, int32  method,
                                      int32 windowBits, int32 memLevel,
                                      int32 strategy, const char *version,
                                      int32 stream_size));
*/
static int32 inflateInit2_ OF((z_streamp strm, int32 windowBits, const char* version, int32 stream_size));

#define deflateInit(strm, level) \
    deflateInit_((strm), (level), ZLIB_VERSION, sizeof(z_stream))
#define inflateInit(strm) \
    inflateInit_((strm), ZLIB_VERSION, sizeof(z_stream))
#define deflateInit2(strm, level, method, windowBits, memLevel, strategy) \
    deflateInit2_((strm), (level), (method), (windowBits), (memLevel), (strategy), ZLIB_VERSION, sizeof(z_stream))
#define inflateInit2(strm, windowBits) \
    inflateInit2_((strm), (windowBits), ZLIB_VERSION, sizeof(z_stream))


// static const char   * zError           OF((int32 err));
// static int32            inflateSyncPoint OF((z_streamp z));
// static const uint32 * get_crc_table    OF(());

typedef uint8  uch;
typedef uint16 ush;
typedef uint32 ulg;

// static const char *z_errmsg[10]; /* indexed by 2-zlib_error */
/* (size given to avoid silly warnings with Visual C++) */

#define ERR_MSG(err) z_errmsg[Z_NEED_DICT - (err)]

#define ERR_RETURN(strm, err) \
    return (strm->msg = (char*) ERR_MSG(err), (err))
/* To be used only when the state is known to be valid */

/* common constants */

#ifndef DEF_WBITS
    #define DEF_WBITS MAX_WBITS
#endif
/* default windowBits for decompression. MAX_WBITS is for compression only */

#if MAX_MEM_LEVEL >= 8
    #define DEF_MEM_LEVEL 8
#else
    #define DEF_MEM_LEVEL MAX_MEM_LEVEL
#endif
/* default memLevel */

#define STORED_BLOCK 0
#define STATIC_TREES 1
#define DYN_TREES    2
/* The three kinds of block type */

#define MIN_MATCH    3
#define MAX_MATCH    258
/* The minimum and maximum match lengths */

#define PRESET_DICT  0x20 /* preset dictionary flag in zlib header */

                          /* target dependencies */

/* Common defaults */

#ifndef OS_CODE
    #define OS_CODE 0x03 /* assume Unix */
#endif

#ifndef F_OPEN
    #define F_OPEN(name, mode) fopen((name), (mode))
#endif

/* functions */

#ifdef HAVE_STRERROR
extern char* strerror OF((int32));
    #define zstrerror(errnum) strerror(errnum)
#else
    #define zstrerror(errnum) ""
#endif

#define zmemcpy             Com_Memcpy
#define zmemcmp             memcmp
#define zmemzero(dest, len) Com_Memset(dest, 0, len)

/* Diagnostic functions */
#ifdef _ZIP_DEBUG_
int32 z_verbose = 0;
    #define Assert(cond, msg) assert(cond);
//{if(!(cond)) Sys_Error(msg);}
    #define Trace(x)            \
        {                       \
            if (z_verbose >= 0) \
                Sys_Error x;    \
        }
    #define Tracev(x)          \
        {                      \
            if (z_verbose > 0) \
                Sys_Error x;   \
        }
    #define Tracevv(x)         \
        {                      \
            if (z_verbose > 1) \
                Sys_Error x;   \
        }
    #define Tracec(c, x)              \
        {                             \
            if (z_verbose > 0 && (c)) \
                Sys_Error x;          \
        }
    #define Tracecv(c, x)             \
        {                             \
            if (z_verbose > 1 && (c)) \
                Sys_Error x;          \
        }
#else
    #define Assert(cond, msg)
    #define Trace(x)
    #define Tracev(x)
    #define Tracevv(x)
    #define Tracec(c, x)
    #define Tracecv(c, x)
#endif


typedef uint32(*check_func) OF((uint32 check, const uint8* buf, uint32 len));
static uint8* zcalloc OF((uint8 * opaque, uint32 items, uint32 size));
static void zcfree    OF((uint8 * opaque, uint8* ptr));

#define ZALLOC(strm, items, size) \
    (*((strm)->zalloc))((strm)->opaque, (items), (size))
#define ZFREE(strm, addr) (*((strm)->zfree))((strm)->opaque, (uint8*) (addr))
#define TRY_FREE(s, p)   \
    {                    \
        if (p)           \
            ZFREE(s, p); \
    }


#if !defined(unix) && !defined(CASESENSITIVITYDEFAULT_YES) && !defined(CASESENSITIVITYDEFAULT_NO)
    #define CASESENSITIVITYDEFAULT_NO
#endif


#ifndef UNZ_BUFSIZE
    #define UNZ_BUFSIZE (65536)
#endif

#ifndef UNZ_MAXFILENAMEINZIP
    #define UNZ_MAXFILENAMEINZIP (256)
#endif

#ifndef ALLOC
    #define ALLOC(size) (Z_Malloc(size))
#endif
#ifndef TRYFREE
    #define TRYFREE(p)     \
        {                  \
            if (p)         \
                Z_Free(p); \
        }
#endif

#define SIZECENTRALDIRITEM (0x2e)
#define SIZEZIPLOCALHEADER (0x1e)


/* ===========================================================================
     Read a uint8 from a gz_stream; update next_in and avail_in. Return EOF
   for end of file.
   IN assertion: the stream s has been sucessfully opened for reading.
*/

/*
static int32 unzlocal_getByte(FILE *fin,int32 *pi)
{
    uint8 c;
    int32 err = fread(&c, 1, 1, fin);
    if (err==1)
    {
        *pi = (int32)c;
        return UNZ_OK;
    }
    else
    {
        if (ferror(fin))
            return UNZ_ERRNO;
        else
            return UNZ_EOF;
    }
}
*/

/* ===========================================================================
   Reads a long in LSB order from the given gz_stream. Sets
*/
static int32 unzlocal_getShort(FILE* fin, uint32* pX)
{
    int16 v;

    fread(&v, sizeof(v), 1, fin);

    *pX = (v);
    return UNZ_OK;

    /*
        uint32 x ;
        int32 i;
        int32 err;

        err = unzlocal_getByte(fin,&i);
        x = (uint32)i;

        if (err==UNZ_OK)
            err = unzlocal_getByte(fin,&i);
        x += ((uint32)i)<<8;

        if (err==UNZ_OK)
            *pX = x;
        else
            *pX = 0;
        return err;
    */
}

static int32 unzlocal_getLong(FILE* fin, uint32* pX)
{
    int32 v;

    fread(&v, sizeof(v), 1, fin);

    *pX = (v);
    return UNZ_OK;

    /*
        uint32 x ;
        int32 i;
        int32 err;

        err = unzlocal_getByte(fin,&i);
        x = (uint32)i;

        if (err==UNZ_OK)
            err = unzlocal_getByte(fin,&i);
        x += ((uint32)i)<<8;

        if (err==UNZ_OK)
            err = unzlocal_getByte(fin,&i);
        x += ((uint32)i)<<16;

        if (err==UNZ_OK)
            err = unzlocal_getByte(fin,&i);
        x += ((uint32)i)<<24;

        if (err==UNZ_OK)
            *pX = x;
        else
            *pX = 0;
        return err;
    */
}


/* My own strcmpi / strcasecmp */
static int32 strcmpcasenosensitive_internal(const char* fileName1, const char* fileName2)
{
    for (;;) {
        char c1 = *(fileName1++);
        char c2 = *(fileName2++);
        if ((c1 >= 'a') && (c1 <= 'z')) {
            c1 -= 0x20;
        }
        if ((c2 >= 'a') && (c2 <= 'z')) {
            c2 -= 0x20;
        }
        if (c1 == '\0') {
            return ((c2 == '\0') ? 0 : -1);
        }
        if (c2 == '\0') {
            return 1;
        }
        if (c1 < c2) {
            return -1;
        }
        if (c1 > c2) {
            return 1;
        }
    }
}


#ifdef CASESENSITIVITYDEFAULT_NO
    #define CASESENSITIVITYDEFAULTVALUE 2
#else
    #define CASESENSITIVITYDEFAULTVALUE 1
#endif

#ifndef STRCMPCASENOSENTIVEFUNCTION
    #define STRCMPCASENOSENTIVEFUNCTION strcmpcasenosensitive_internal
#endif

/*
   Compare two filename (fileName1,fileName2).
   If iCaseSenisivity = 1, comparision is case sensitivity (like strcmp)
   If iCaseSenisivity = 2, comparision is not case sensitivity (like strcmpi
                                                                or strcasecmp)
   If iCaseSenisivity = 0, case sensitivity is defaut of your operating system
        (like 1 on Unix, 2 on Windows)

*/
extern int32 unzStringFileNameCompare(const char* fileName1, const char* fileName2, int32 iCaseSensitivity)
{
    if (iCaseSensitivity == 0) {
        iCaseSensitivity = CASESENSITIVITYDEFAULTVALUE;
    }

    if (iCaseSensitivity == 1) {
        return strcmp(fileName1, fileName2);
    }

    return STRCMPCASENOSENTIVEFUNCTION(fileName1, fileName2);
}

#define BUFREADCOMMENT (0x400)

/*
  Locate the Central directory of a zipfile (at the end, just before
    the global comment)
*/
extern uint32 unzlocal_SearchCentralDir(FILE* fin)
{
    uint8* buf;
    uint32 uSizeFile;
    uint32 uBackRead;
    uint32 uMaxBack = 0xffff; /* maximum size of global comment */
    uint32 uPosFound = 0;

    if (fseek(fin, 0, SEEK_END) != 0) {
        return 0;
    }


    uSizeFile = ftell(fin);

    if (uMaxBack > uSizeFile) {
        uMaxBack = uSizeFile;
    }

    buf = (uint8*) ALLOC(BUFREADCOMMENT + 4);
    if (buf == NULL) {
        return 0;
    }

    uBackRead = 4;
    while (uBackRead < uMaxBack) {
        uint32 uReadSize, uReadPos;
        int32  i;
        if (uBackRead + BUFREADCOMMENT > uMaxBack) {
            uBackRead = uMaxBack;
        } else {
            uBackRead += BUFREADCOMMENT;
        }
        uReadPos = uSizeFile - uBackRead;

        uReadSize = ((BUFREADCOMMENT + 4) < (uSizeFile - uReadPos)) ? (BUFREADCOMMENT + 4) : (uSizeFile - uReadPos);
        if (fseek(fin, uReadPos, SEEK_SET) != 0) {
            break;
        }

        if (fread(buf, (uint32) uReadSize, 1, fin) != 1) {
            break;
        }

        for (i = (int32) uReadSize - 3; (i--) > 0;) {
            if (((*(buf + i)) == 0x50) && ((*(buf + i + 1)) == 0x4b) && ((*(buf + i + 2)) == 0x05) && ((*(buf + i + 3)) == 0x06)) {
                uPosFound = uReadPos + i;
                break;
            }
        }

        if (uPosFound != 0) {
            break;
        }
    }
    TRYFREE(buf);
    return uPosFound;
}

extern unzFile unzReOpen(const char* path, unzFile file)
{
    unz_s* s;
    FILE*  fin;

    fin = fopen(path, "rb");
    if (fin == NULL) {
        return NULL;
    }

    s = (unz_s*) ALLOC(sizeof(unz_s));
    Com_Memcpy(s, (unz_s*) file, sizeof(unz_s));

    s->file = fin;
    return (unzFile) s;
}

/*
  Open a Zip file. path contain the full pathname (by example,
     on a Windows NT computer "c:\\test\\zlib109.zip" or on an Unix computer
     "zlib/zlib109.zip".
     If the zipfile cannot be opened (file don't exist or in not valid), the
       return value is NULL.
     Else, the return value is a unzFile Handle, usable with other function
       of this unzip package.
*/
extern unzFile unzOpen(const char* path)
{
    unz_s  us;
    unz_s* s;
    uint32 central_pos, uL;
    FILE*  fin;

    uint32 number_disk;         /* number of the current dist, used for
                                  spaning ZIP, unsupported, always 0*/
    uint32 number_disk_with_CD; /* number the the disk with central dir, used
                                  for spaning ZIP, unsupported, always 0*/
    uint32 number_entry_CD;     /* total number of entries in
                                  the central dir
                                  (same than number_entry on nospan) */

    int32 err = UNZ_OK;

    fin = fopen(path, "rb");
    if (fin == NULL) {
        return NULL;
    }

    central_pos = unzlocal_SearchCentralDir(fin);
    if (central_pos == 0) {
        err = UNZ_ERRNO;
    }

    if (fseek(fin, central_pos, SEEK_SET) != 0) {
        err = UNZ_ERRNO;
    }

    /* the signature, already checked */
    if (unzlocal_getLong(fin, &uL) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    /* number of this disk */
    if (unzlocal_getShort(fin, &number_disk) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    /* number of the disk with the start of the central directory */
    if (unzlocal_getShort(fin, &number_disk_with_CD) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    /* total number of entries in the central dir on this disk */
    if (unzlocal_getShort(fin, &us.gi.number_entry) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    /* total number of entries in the central dir */
    if (unzlocal_getShort(fin, &number_entry_CD) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if ((number_entry_CD != us.gi.number_entry) || (number_disk_with_CD != 0) || (number_disk != 0)) {
        err = UNZ_BADZIPFILE;
    }

    /* size of the central directory */
    if (unzlocal_getLong(fin, &us.size_central_dir) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    /* offset of start of central directory with respect to the
          starting disk number */
    if (unzlocal_getLong(fin, &us.offset_central_dir) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    /* zipfile comment length */
    if (unzlocal_getShort(fin, &us.gi.size_comment) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if ((central_pos < us.offset_central_dir + us.size_central_dir) && (err == UNZ_OK)) {
        err = UNZ_BADZIPFILE;
    }

    if (err != UNZ_OK) {
        fclose(fin);
        return NULL;
    }

    us.file = fin;
    us.byte_before_the_zipfile = central_pos - (us.offset_central_dir + us.size_central_dir);
    us.central_pos = central_pos;
    us.pfile_in_zip_read = NULL;


    s = (unz_s*) ALLOC(sizeof(unz_s));
    *s = us;
    //    unzGoToFirstFile((unzFile)s);
    return (unzFile) s;
}


/*
  Close a ZipFile opened with unzipOpen.
  If there is files inside the .Zip opened with unzipOpenCurrentFile (see later),
    these files MUST be closed with unzipCloseCurrentFile before call unzipClose.
  return UNZ_OK if there is no problem. */
extern int32 unzClose(unzFile file)
{
    unz_s* s;
    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;

    if (s->pfile_in_zip_read != NULL) {
        unzCloseCurrentFile(file);
    }

    fclose(s->file);
    TRYFREE(s);
    return UNZ_OK;
}


/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem. */
extern int32 unzGetGlobalInfo(unzFile file, unz_global_info* pglobal_info)
{
    unz_s* s;
    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    *pglobal_info = s->gi;
    return UNZ_OK;
}


/*
   Translate date/time from Dos format to tm_unz (readable more easilty)
*/
static void unzlocal_DosDateToTmuDate(uint32 ulDosDate, tm_unz* ptm)
{
    uint32 uDate;
    uDate = (uint32) (ulDosDate >> 16);
    ptm->tm_mday = (uint32) (uDate & 0x1f);
    ptm->tm_mon = (uint32) ((((uDate) & 0x1E0) / 0x20) - 1);
    ptm->tm_year = (uint32) (((uDate & 0x0FE00) / 0x0200) + 1980);

    ptm->tm_hour = (uint32) ((ulDosDate & 0xF800) / 0x800);
    ptm->tm_min = (uint32) ((ulDosDate & 0x7E0) / 0x20);
    ptm->tm_sec = (uint32) (2 * (ulDosDate & 0x1f));
}

/*
  Get Info about the current file in the zipfile, with internal only info
*/
static int32 unzlocal_GetCurrentFileInfoInternal(unzFile file, unz_file_info* pfile_info, unz_file_info_internal* pfile_info_internal, char* szFileName, uint32 fileNameBufferSize, void* extraField, uint32 extraFieldBufferSize, char* szComment, uint32 commentBufferSize)
{
    unz_s*                 s;
    unz_file_info          file_info;
    unz_file_info_internal file_info_internal;
    int32                  err = UNZ_OK;
    uint32                 uMagic;
    int32                  lSeek = 0;

    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    if (fseek(s->file, s->pos_in_central_dir + s->byte_before_the_zipfile, SEEK_SET) != 0) {
        err = UNZ_ERRNO;
    }


    /* we check the magic */
    if (err == UNZ_OK) {
        if (unzlocal_getLong(s->file, &uMagic) != UNZ_OK) {
            err = UNZ_ERRNO;
        } else if (uMagic != 0x02014b50) {
            err = UNZ_BADZIPFILE;
        }
    }
    if (unzlocal_getShort(s->file, &file_info.version) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getShort(s->file, &file_info.version_needed) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getShort(s->file, &file_info.flag) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getShort(s->file, &file_info.compression_method) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getLong(s->file, &file_info.dosDate) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    unzlocal_DosDateToTmuDate(file_info.dosDate, &file_info.tmu_date);

    if (unzlocal_getLong(s->file, &file_info.crc) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getLong(s->file, &file_info.compressed_size) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getLong(s->file, &file_info.uncompressed_size) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getShort(s->file, &file_info.size_filename) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getShort(s->file, &file_info.size_file_extra) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getShort(s->file, &file_info.size_file_comment) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getShort(s->file, &file_info.disk_num_start) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getShort(s->file, &file_info.internal_fa) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getLong(s->file, &file_info.external_fa) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getLong(s->file, &file_info_internal.offset_curfile) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    lSeek += file_info.size_filename;
    if ((err == UNZ_OK) && (szFileName != NULL)) {
        uint32 uSizeRead;
        if (file_info.size_filename < fileNameBufferSize) {
            *(szFileName + file_info.size_filename) = '\0';
            uSizeRead = file_info.size_filename;
        } else {
            uSizeRead = fileNameBufferSize;
        }

        if ((file_info.size_filename > 0) && (fileNameBufferSize > 0)) {
            if (fread(szFileName, (uint32) uSizeRead, 1, s->file) != 1) {
                err = UNZ_ERRNO;
            }
        }
        lSeek -= uSizeRead;
    }


    if ((err == UNZ_OK) && (extraField != NULL)) {
        uint32 uSizeRead;
        if (file_info.size_file_extra < extraFieldBufferSize) {
            uSizeRead = file_info.size_file_extra;
        } else {
            uSizeRead = extraFieldBufferSize;
        }

        if (lSeek != 0) {
            if (fseek(s->file, lSeek, SEEK_CUR) == 0) {
                lSeek = 0;
            } else {
                err = UNZ_ERRNO;
            }
        }
        if ((file_info.size_file_extra > 0) && (extraFieldBufferSize > 0)) {
            if (fread(extraField, (uint32) uSizeRead, 1, s->file) != 1) {
                err = UNZ_ERRNO;
            }
        }
        lSeek += file_info.size_file_extra - uSizeRead;
    } else {
        lSeek += file_info.size_file_extra;
    }


    if ((err == UNZ_OK) && (szComment != NULL)) {
        uint32 uSizeRead;
        if (file_info.size_file_comment < commentBufferSize) {
            *(szComment + file_info.size_file_comment) = '\0';
            uSizeRead = file_info.size_file_comment;
        } else {
            uSizeRead = commentBufferSize;
        }

        if (lSeek != 0) {
            if (fseek(s->file, lSeek, SEEK_CUR) == 0) {
                lSeek = 0;
            } else {
                err = UNZ_ERRNO;
            }
        }
        if ((file_info.size_file_comment > 0) && (commentBufferSize > 0)) {
            if (fread(szComment, (uint32) uSizeRead, 1, s->file) != 1) {
                err = UNZ_ERRNO;
            }
        }
        lSeek += file_info.size_file_comment - uSizeRead;
    } else {
        lSeek += file_info.size_file_comment;
    }

    if ((err == UNZ_OK) && (pfile_info != NULL)) {
        *pfile_info = file_info;
    }

    if ((err == UNZ_OK) && (pfile_info_internal != NULL)) {
        *pfile_info_internal = file_info_internal;
    }

    return err;
}


/*
  Write info about the ZipFile in the *pglobal_info structure.
  No preparation of the structure is needed
  return UNZ_OK if there is no problem.
*/
extern int32 unzGetCurrentFileInfo(unzFile file, unz_file_info* pfile_info, char* szFileName, uint32 fileNameBufferSize, void* extraField, uint32 extraFieldBufferSize, char* szComment, uint32 commentBufferSize)
{
    return unzlocal_GetCurrentFileInfoInternal(file, pfile_info, NULL, szFileName, fileNameBufferSize, extraField, extraFieldBufferSize, szComment, commentBufferSize);
}

/*
  Set the current file of the zipfile to the first file.
  return UNZ_OK if there is no problem
*/
extern int32 unzGoToFirstFile(unzFile file)
{
    int32  err = UNZ_OK;
    unz_s* s;
    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    s->pos_in_central_dir = s->offset_central_dir;
    s->num_file = 0;
    err = unzlocal_GetCurrentFileInfoInternal(file, &s->cur_file_info, &s->cur_file_info_internal, NULL, 0, NULL, 0, NULL, 0);
    s->current_file_ok = (err == UNZ_OK);
    return err;
}


/*
  Set the current file of the zipfile to the next file.
  return UNZ_OK if there is no problem
  return UNZ_END_OF_LIST_OF_FILE if the actual file was the latest.
*/
extern int32 unzGoToNextFile(unzFile file)
{
    unz_s* s;
    int32  err;

    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    if (!s->current_file_ok) {
        return UNZ_END_OF_LIST_OF_FILE;
    }
    if (s->num_file + 1 == s->gi.number_entry) {
        return UNZ_END_OF_LIST_OF_FILE;
    }

    s->pos_in_central_dir += SIZECENTRALDIRITEM + s->cur_file_info.size_filename + s->cur_file_info.size_file_extra + s->cur_file_info.size_file_comment;
    s->num_file++;
    err = unzlocal_GetCurrentFileInfoInternal(file, &s->cur_file_info, &s->cur_file_info_internal, NULL, 0, NULL, 0, NULL, 0);
    s->current_file_ok = (err == UNZ_OK);
    return err;
}

/*
  Get the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/
extern int32 unzGetCurrentFileInfoPosition(unzFile file, uint32* pos)
{
    unz_s* s;

    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;

    *pos = s->pos_in_central_dir;
    return UNZ_OK;
}

/*
  Set the position of the info of the current file in the zip.
  return UNZ_OK if there is no problem
*/
extern int32 unzSetCurrentFileInfoPosition(unzFile file, uint32 pos)
{
    unz_s* s;
    int32  err;

    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;

    s->pos_in_central_dir = pos;
    err = unzlocal_GetCurrentFileInfoInternal(file, &s->cur_file_info, &s->cur_file_info_internal, NULL, 0, NULL, 0, NULL, 0);
    s->current_file_ok = (err == UNZ_OK);
    return UNZ_OK;
}

/*
  Try locate the file szFileName in the zipfile.
  For the iCaseSensitivity signification, see unzipStringFileNameCompare

  return value :
  UNZ_OK if the file is found. It becomes the current file.
  UNZ_END_OF_LIST_OF_FILE if the file is not found
*/
extern int32 unzLocateFile(unzFile file, const char* szFileName, int32 iCaseSensitivity)
{
    unz_s* s;
    int32  err;


    uint32 num_fileSaved;
    uint32 pos_in_central_dirSaved;


    if (file == NULL) {
        return UNZ_PARAMERROR;
    }

    if (strlen(szFileName) >= UNZ_MAXFILENAMEINZIP) {
        return UNZ_PARAMERROR;
    }

    s = (unz_s*) file;
    if (!s->current_file_ok) {
        return UNZ_END_OF_LIST_OF_FILE;
    }

    num_fileSaved = s->num_file;
    pos_in_central_dirSaved = s->pos_in_central_dir;

    err = unzGoToFirstFile(file);

    while (err == UNZ_OK) {
        char szCurrentFileName[UNZ_MAXFILENAMEINZIP + 1];
        unzGetCurrentFileInfo(file, NULL, szCurrentFileName, sizeof(szCurrentFileName) - 1, NULL, 0, NULL, 0);
        if (unzStringFileNameCompare(szCurrentFileName, szFileName, iCaseSensitivity) == 0) {
            return UNZ_OK;
        }
        err = unzGoToNextFile(file);
    }

    s->num_file = num_fileSaved;
    s->pos_in_central_dir = pos_in_central_dirSaved;
    return err;
}


/*
  Read the static header of the current zipfile
  Check the coherency of the static header and info in the end of central
        directory about this file
  store in *piSizeVar the size of extra info in static header
        (filename and size of extra field data)
*/
static int32 unzlocal_CheckCurrentFileCoherencyHeader(unz_s* s, uint32* piSizeVar, uint32* poffset_local_extrafield, uint32* psize_local_extrafield)
{
    uint32 uMagic, uData, uFlags;
    uint32 size_filename;
    uint32 size_extra_field;
    int32  err = UNZ_OK;

    *piSizeVar = 0;
    *poffset_local_extrafield = 0;
    *psize_local_extrafield = 0;

    if (fseek(s->file, s->cur_file_info_internal.offset_curfile + s->byte_before_the_zipfile, SEEK_SET) != 0) {
        return UNZ_ERRNO;
    }


    if (err == UNZ_OK) {
        if (unzlocal_getLong(s->file, &uMagic) != UNZ_OK) {
            err = UNZ_ERRNO;
        } else if (uMagic != 0x04034b50) {
            err = UNZ_BADZIPFILE;
        }
    }
    if (unzlocal_getShort(s->file, &uData) != UNZ_OK) {
        err = UNZ_ERRNO;
    }
    /*
        else if ((err==UNZ_OK) && (uData!=s->cur_file_info.wVersion))
            err=UNZ_BADZIPFILE;
    */
    if (unzlocal_getShort(s->file, &uFlags) != UNZ_OK) {
        err = UNZ_ERRNO;
    }

    if (unzlocal_getShort(s->file, &uData) != UNZ_OK) {
        err = UNZ_ERRNO;
    } else if ((err == UNZ_OK) && (uData != s->cur_file_info.compression_method)) {
        err = UNZ_BADZIPFILE;
    }

    if ((err == UNZ_OK) && (s->cur_file_info.compression_method != 0) && (s->cur_file_info.compression_method != Z_DEFLATED)) {
        err = UNZ_BADZIPFILE;
    }

    if (unzlocal_getLong(s->file, &uData) != UNZ_OK) { /* date/time */
        err = UNZ_ERRNO;
    }

    if (unzlocal_getLong(s->file, &uData) != UNZ_OK) { /* crc */
        err = UNZ_ERRNO;
    } else if ((err == UNZ_OK) && (uData != s->cur_file_info.crc) && ((uFlags & 8) == 0)) {
        err = UNZ_BADZIPFILE;
    }

    if (unzlocal_getLong(s->file, &uData) != UNZ_OK) { /* size compr */
        err = UNZ_ERRNO;
    } else if ((err == UNZ_OK) && (uData != s->cur_file_info.compressed_size) && ((uFlags & 8) == 0)) {
        err = UNZ_BADZIPFILE;
    }

    if (unzlocal_getLong(s->file, &uData) != UNZ_OK) { /* size uncompr */
        err = UNZ_ERRNO;
    } else if ((err == UNZ_OK) && (uData != s->cur_file_info.uncompressed_size) && ((uFlags & 8) == 0)) {
        err = UNZ_BADZIPFILE;
    }


    if (unzlocal_getShort(s->file, &size_filename) != UNZ_OK) {
        err = UNZ_ERRNO;
    } else if ((err == UNZ_OK) && (size_filename != s->cur_file_info.size_filename)) {
        err = UNZ_BADZIPFILE;
    }

    *piSizeVar += (uint32) size_filename;

    if (unzlocal_getShort(s->file, &size_extra_field) != UNZ_OK) {
        err = UNZ_ERRNO;
    }
    *poffset_local_extrafield = s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER + size_filename;
    *psize_local_extrafield = (uint32) size_extra_field;

    *piSizeVar += (uint32) size_extra_field;

    return err;
}

/*
  Open for reading data the current file in the zipfile.
  If there is no error and the file is opened, the return value is UNZ_OK.
*/
extern int32 unzOpenCurrentFile(unzFile file)
{
    int32                    err = UNZ_OK;
    int32                    Store;
    uint32                   iSizeVar;
    unz_s*                   s;
    file_in_zip_read_info_s* pfile_in_zip_read_info;
    uint32                   offset_local_extrafield; /* offset of the static extra field */
    uint32                   size_local_extrafield;   /* size of the static extra field */

    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    if (!s->current_file_ok) {
        return UNZ_PARAMERROR;
    }

    if (s->pfile_in_zip_read != NULL) {
        unzCloseCurrentFile(file);
    }

    if (unzlocal_CheckCurrentFileCoherencyHeader(s, &iSizeVar, &offset_local_extrafield, &size_local_extrafield) != UNZ_OK) {
        return UNZ_BADZIPFILE;
    }

    pfile_in_zip_read_info = (file_in_zip_read_info_s*)
        ALLOC(sizeof(file_in_zip_read_info_s));
    if (pfile_in_zip_read_info == NULL) {
        return UNZ_INTERNALERROR;
    }

    pfile_in_zip_read_info->read_buffer = (char*) ALLOC(UNZ_BUFSIZE);
    pfile_in_zip_read_info->offset_local_extrafield = offset_local_extrafield;
    pfile_in_zip_read_info->size_local_extrafield = size_local_extrafield;
    pfile_in_zip_read_info->pos_local_extrafield = 0;

    if (pfile_in_zip_read_info->read_buffer == NULL) {
        TRYFREE(pfile_in_zip_read_info);
        return UNZ_INTERNALERROR;
    }

    pfile_in_zip_read_info->stream_initialised = 0;

    if ((s->cur_file_info.compression_method != 0) && (s->cur_file_info.compression_method != Z_DEFLATED)) {
        err = UNZ_BADZIPFILE;
    }
    Store = s->cur_file_info.compression_method == 0;

    pfile_in_zip_read_info->crc32_wait = s->cur_file_info.crc;
    pfile_in_zip_read_info->crc32 = 0;
    pfile_in_zip_read_info->compression_method =
        s->cur_file_info.compression_method;
    pfile_in_zip_read_info->file = s->file;
    pfile_in_zip_read_info->byte_before_the_zipfile = s->byte_before_the_zipfile;

    pfile_in_zip_read_info->stream.total_out = 0;

    if (!Store) {
        pfile_in_zip_read_info->stream.zalloc = (alloc_func) 0;
        pfile_in_zip_read_info->stream.zfree = (free_func) 0;
        pfile_in_zip_read_info->stream.opaque = (uint8*) 0;

        err = inflateInit2(&pfile_in_zip_read_info->stream, -MAX_WBITS);
        if (err == Z_OK) {
            pfile_in_zip_read_info->stream_initialised = 1;
        }
        /* windowBits is passed < 0 to tell that there is no zlib header.
         * Note that in this case inflate *requires* an extra "dummy" uint8
         * after the compressed stream in order to complete decompression and
         * return Z_STREAM_END.
         * In unzip, i don't wait absolutely Z_STREAM_END because I known the
         * size of both compressed and uncompressed data
         */
    }
    pfile_in_zip_read_info->rest_read_compressed =
        s->cur_file_info.compressed_size;
    pfile_in_zip_read_info->rest_read_uncompressed =
        s->cur_file_info.uncompressed_size;


    pfile_in_zip_read_info->pos_in_zipfile =
        s->cur_file_info_internal.offset_curfile + SIZEZIPLOCALHEADER + iSizeVar;

    pfile_in_zip_read_info->stream.avail_in = (uint32) 0;


    s->pfile_in_zip_read = pfile_in_zip_read_info;
    return UNZ_OK;
}


/*
  Read bytes from the current file.
  buf contain buffer where data must be copied
  len the size of buf.

  return the number of uint8 copied if somes bytes are copied
  return 0 if the end of file was reached
  return <0 with error code if there is an error
    (UNZ_ERRNO for IO error, or zLib error for uncompress error)
*/
extern int32 unzReadCurrentFile(unzFile file, void* buf, uint32 len)
{
    int32                    err = UNZ_OK;
    uint32                   iRead = 0;
    unz_s*                   s;
    file_in_zip_read_info_s* pfile_in_zip_read_info;
    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    pfile_in_zip_read_info = s->pfile_in_zip_read;

    if (pfile_in_zip_read_info == NULL) {
        return UNZ_PARAMERROR;
    }


    if ((pfile_in_zip_read_info->read_buffer == NULL)) {
        return UNZ_END_OF_LIST_OF_FILE;
    }
    if (len == 0) {
        return 0;
    }

    pfile_in_zip_read_info->stream.next_out = (uint8*) buf;

    pfile_in_zip_read_info->stream.avail_out = (uint32) len;

    if (len > pfile_in_zip_read_info->rest_read_uncompressed) {
        pfile_in_zip_read_info->stream.avail_out =
            (uint32) pfile_in_zip_read_info->rest_read_uncompressed;
    }

    while (pfile_in_zip_read_info->stream.avail_out > 0) {
        if ((pfile_in_zip_read_info->stream.avail_in == 0) && (pfile_in_zip_read_info->rest_read_compressed > 0)) {
            uint32 uReadThis = UNZ_BUFSIZE;
            if (pfile_in_zip_read_info->rest_read_compressed < uReadThis) {
                uReadThis = (uint32) pfile_in_zip_read_info->rest_read_compressed;
            }
            if (uReadThis == 0) {
                return UNZ_EOF;
            }
            if (s->cur_file_info.compressed_size == pfile_in_zip_read_info->rest_read_compressed) {
                if (fseek(pfile_in_zip_read_info->file, pfile_in_zip_read_info->pos_in_zipfile + pfile_in_zip_read_info->byte_before_the_zipfile, SEEK_SET) != 0) {
                    return UNZ_ERRNO;
                }
            }
            if (fread(pfile_in_zip_read_info->read_buffer, uReadThis, 1, pfile_in_zip_read_info->file) != 1) {
                return UNZ_ERRNO;
            }
            pfile_in_zip_read_info->pos_in_zipfile += uReadThis;

            pfile_in_zip_read_info->rest_read_compressed -= uReadThis;

            pfile_in_zip_read_info->stream.next_in =
                (uint8*) pfile_in_zip_read_info->read_buffer;
            pfile_in_zip_read_info->stream.avail_in = (uint32) uReadThis;
        }

        if (pfile_in_zip_read_info->compression_method == 0) {
            uint32 uDoCopy, i;
            if (pfile_in_zip_read_info->stream.avail_out < pfile_in_zip_read_info->stream.avail_in) {
                uDoCopy = pfile_in_zip_read_info->stream.avail_out;
            } else {
                uDoCopy = pfile_in_zip_read_info->stream.avail_in;
            }

            for (i = 0; i < uDoCopy; i++) {
                *(pfile_in_zip_read_info->stream.next_out + i) =
                    *(pfile_in_zip_read_info->stream.next_in + i);
            }

            //            pfile_in_zip_read_info->crc32 = crc32(pfile_in_zip_read_info->crc32,
            //                                pfile_in_zip_read_info->stream.next_out,
            //                                uDoCopy);
            pfile_in_zip_read_info->rest_read_uncompressed -= uDoCopy;
            pfile_in_zip_read_info->stream.avail_in -= uDoCopy;
            pfile_in_zip_read_info->stream.avail_out -= uDoCopy;
            pfile_in_zip_read_info->stream.next_out += uDoCopy;
            pfile_in_zip_read_info->stream.next_in += uDoCopy;
            pfile_in_zip_read_info->stream.total_out += uDoCopy;
            iRead += uDoCopy;
        } else {
            uint32       uTotalOutBefore, uTotalOutAfter;
            const uint8* bufBefore;
            uint32       uOutThis;
            int32        flush = Z_SYNC_FLUSH;

            uTotalOutBefore = pfile_in_zip_read_info->stream.total_out;
            bufBefore = pfile_in_zip_read_info->stream.next_out;

            /*
            if ((pfile_in_zip_read_info->rest_read_uncompressed ==
                     pfile_in_zip_read_info->stream.avail_out) &&
                (pfile_in_zip_read_info->rest_read_compressed == 0))
                flush = Z_FINISH;
            */
            err = inflate(&pfile_in_zip_read_info->stream, flush);

            uTotalOutAfter = pfile_in_zip_read_info->stream.total_out;
            uOutThis = uTotalOutAfter - uTotalOutBefore;

            //            pfile_in_zip_read_info->crc32 =
            //                crc32(pfile_in_zip_read_info->crc32,bufBefore,
            //                        (uint32)(uOutThis));

            pfile_in_zip_read_info->rest_read_uncompressed -=
                uOutThis;

            iRead += (uint32) (uTotalOutAfter - uTotalOutBefore);

            if (err == Z_STREAM_END) {
                return (iRead == 0) ? UNZ_EOF : iRead;
            }
            if (err != Z_OK) {
                break;
            }
        }
    }

    if (err == Z_OK) {
        return iRead;
    }
    return err;
}


/*
  Give the current position in uncompressed data
*/
extern int32 unztell(unzFile file)
{
    unz_s*                   s;
    file_in_zip_read_info_s* pfile_in_zip_read_info;
    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    pfile_in_zip_read_info = s->pfile_in_zip_read;

    if (pfile_in_zip_read_info == NULL) {
        return UNZ_PARAMERROR;
    }

    return (int32) pfile_in_zip_read_info->stream.total_out;
}


/*
  return 1 if the end of file was reached, 0 elsewhere
*/
extern int32 unzeof(unzFile file)
{
    unz_s*                   s;
    file_in_zip_read_info_s* pfile_in_zip_read_info;
    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    pfile_in_zip_read_info = s->pfile_in_zip_read;

    if (pfile_in_zip_read_info == NULL) {
        return UNZ_PARAMERROR;
    }

    if (pfile_in_zip_read_info->rest_read_uncompressed == 0) {
        return 1;
    } else {
        return 0;
    }
}


/*
  Read extra field from the current file (opened by unzOpenCurrentFile)
  This is the static-header version of the extra field (sometimes, there is
    more info in the static-header version than in the central-header)

  if buf==NULL, it return the size of the static extra field that can be read

  if buf!=NULL, len is the size of the buffer, the extra header is copied in
    buf.
  the return value is the number of bytes copied in buf, or (if <0)
    the error code
*/
extern int32 unzGetLocalExtrafield(unzFile file, void* buf, uint32 len)
{
    unz_s*                   s;
    file_in_zip_read_info_s* pfile_in_zip_read_info;
    uint32                   read_now;
    uint32                   size_to_read;

    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    pfile_in_zip_read_info = s->pfile_in_zip_read;

    if (pfile_in_zip_read_info == NULL) {
        return UNZ_PARAMERROR;
    }

    size_to_read = (pfile_in_zip_read_info->size_local_extrafield - pfile_in_zip_read_info->pos_local_extrafield);

    if (buf == NULL) {
        return (int32) size_to_read;
    }

    if (len > size_to_read) {
        read_now = (uint32) size_to_read;
    } else {
        read_now = (uint32) len;
    }

    if (read_now == 0) {
        return 0;
    }

    if (fseek(pfile_in_zip_read_info->file, pfile_in_zip_read_info->offset_local_extrafield + pfile_in_zip_read_info->pos_local_extrafield, SEEK_SET) != 0) {
        return UNZ_ERRNO;
    }

    if (fread(buf, (uint32) size_to_read, 1, pfile_in_zip_read_info->file) != 1) {
        return UNZ_ERRNO;
    }

    return (int32) read_now;
}

/*
  Close the file in zip opened with unzipOpenCurrentFile
  Return UNZ_CRCERROR if all the file was read but the CRC is not good
*/
extern int32 unzCloseCurrentFile(unzFile file)
{
    int32 err = UNZ_OK;

    unz_s*                   s;
    file_in_zip_read_info_s* pfile_in_zip_read_info;
    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;
    pfile_in_zip_read_info = s->pfile_in_zip_read;

    if (pfile_in_zip_read_info == NULL) {
        return UNZ_PARAMERROR;
    }

    /*
        if (pfile_in_zip_read_info->rest_read_uncompressed == 0)
        {
            if (pfile_in_zip_read_info->crc32 != pfile_in_zip_read_info->crc32_wait)
                err=UNZ_CRCERROR;
        }
    */

    TRYFREE(pfile_in_zip_read_info->read_buffer);
    pfile_in_zip_read_info->read_buffer = NULL;
    if (pfile_in_zip_read_info->stream_initialised) {
        inflateEnd(&pfile_in_zip_read_info->stream);
    }

    pfile_in_zip_read_info->stream_initialised = 0;
    TRYFREE(pfile_in_zip_read_info);

    s->pfile_in_zip_read = NULL;

    return err;
}


/*
  Get the global comment string of the ZipFile, in the szComment buffer.
  uSizeBuf is the size of the szComment buffer.
  return the number of uint8 copied or an error code <0
*/
extern int32 unzGetGlobalComment(unzFile file, char* szComment, uint32 uSizeBuf)
{
    unz_s* s;
    uint32 uReadThis;
    if (file == NULL) {
        return UNZ_PARAMERROR;
    }
    s = (unz_s*) file;

    uReadThis = uSizeBuf;
    if (uReadThis > s->gi.size_comment) {
        uReadThis = s->gi.size_comment;
    }

    if (fseek(s->file, s->central_pos + 22, SEEK_SET) != 0) {
        return UNZ_ERRNO;
    }

    if (uReadThis > 0) {
        *szComment = '\0';
        if (fread(szComment, (uint32) uReadThis, 1, s->file) != 1) {
            return UNZ_ERRNO;
        }
    }

    if ((szComment != NULL) && (uSizeBuf > s->gi.size_comment)) {
        *(szComment + s->gi.size_comment) = '\0';
    }
    return (int32) uReadThis;
}

/* infblock.h -- header to use infblock.c
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

struct inflate_blocks_state;
typedef struct inflate_blocks_state inflate_blocks_statef;

static inflate_blocks_statef* inflate_blocks_new OF((
    z_streamp  z,
    check_func c, /* check function */
    uint32     w
));               /* window size */

static int32 inflate_blocks OF((
    inflate_blocks_statef*,
    z_streamp,
    int32
)); /* initial return code */

static void inflate_blocks_reset OF((
    inflate_blocks_statef*,
    z_streamp,
    uint32*
) ); /* check value on output */

static int32 inflate_blocks_free OF((
    inflate_blocks_statef*,
    z_streamp
));

/* simplify the use of the inflate_huft type with some defines */
#define exop word.what.Exop
#define bits word.what.Bits

/* Table for deflate from PKZIP's appnote.txt. */
static const uint32 border[] = {/* Order of the bit length code lengths */
                                16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15
};

/* inftrees.h -- header to use inftrees.c
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

/* Huffman code lookup table entry--this entry is four bytes for machines
   that have 16-bit pointers (e.g. PC's in the small or medium model). */

typedef struct inflate_huft_s inflate_huft;

struct inflate_huft_s
{
    union
    {
        struct
        {
            uint8 Exop; /* number of extra bits or operation */
            uint8 Bits; /* number of bits in this code or subcode */
        } what;
        uint32 pad;     /* pad structure to a power of 2 (4 bytes for */
    } word;             /*  16-bit, 8 bytes for 32-bit int32's) */
    uint32 base;        /* literal, length base, distance base,
                         or table offset */
};

/* Maximum size of dynamic tree.  The maximum found in a long but non-
   exhaustive search was 1004 huft structures (850 for length/literals
   and 154 for distances, the latter actually the result of an
   exhaustive search).  The actual maximum is not known, but the
   value below is more than safe. */
#define MANY 1440

static int32 inflate_trees_bits OF((
    uint32*,        /* 19 code lengths */
    uint32*,        /* bits tree desired/actual depth */
    inflate_huft**, /* bits tree result */
    inflate_huft*,  /* space for trees */
    z_streamp
));                 /* for messages */

static int32 inflate_trees_dynamic OF((
    uint32,         /* number of literal/length codes */
    uint32,         /* number of distance codes */
    uint32*,        /* that many (total) code lengths */
    uint32*,        /* literal desired/actual bit depth */
    uint32*,        /* distance desired/actual bit depth */
    inflate_huft**, /* literal/length tree result */
    inflate_huft**, /* distance tree result */
    inflate_huft*,  /* space for trees */
    z_streamp
));                 /* for messages */

static int32 inflate_trees_fixed OF((
    uint32*,        /* literal desired/actual bit depth */
    uint32*,        /* distance desired/actual bit depth */
    inflate_huft**, /* literal/length tree result */
    inflate_huft**, /* distance tree result */
    z_streamp
));                 /* for memory allocation */


/* infcodes.h -- header to use infcodes.c
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

struct inflate_codes_state;
typedef struct inflate_codes_state inflate_codes_statef;

static inflate_codes_statef* inflate_codes_new OF((
    uint32, uint32,
    inflate_huft*, inflate_huft*,
    z_streamp
));

static int32 inflate_codes OF((
    inflate_blocks_statef*,
    z_streamp,
    int32
));

static void inflate_codes_free OF((
    inflate_codes_statef*,
    z_streamp
));

/* infutil.h -- types and macros common to blocks and codes
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

#ifndef _INFUTIL_H
    #define _INFUTIL_H

typedef enum
{
    TYPE,   /* get type bits (3, including end bit) */
    LENS,   /* get lengths for stored */
    STORED, /* processing stored block */
    TABLE,  /* get table lengths */
    BTREE,  /* get bit lengths tree for a dynamic block */
    DTREE,  /* get length, distance trees for a dynamic block */
    CODES,  /* processing fixed or dynamic block */
    DRY,    /* output remaining window bytes */
    DONE,   /* finished last block, done */
    BAD
} /* got a data error--stuck here */
inflate_block_mode;

/* inflate blocks semi-private state */
struct inflate_blocks_state
{
    /* mode */
    inflate_block_mode mode; /* current inflate_block mode */

    /* mode dependent information */
    union
    {
        uint32 left; /* if STORED, bytes left to copy */
        struct
        {
            uint32        table; /* table lengths (14 bits) */
            uint32        index; /* index into blens (or border) */
            uint32*       blens; /* bit lengths of codes */
            uint32        bb;    /* bit length tree depth */
            inflate_huft* tb;    /* bit length decoding tree */
        } trees;                 /* if DTREE, decoding info for trees */
        struct
        {
            inflate_codes_statef* codes;
        } decode; /* if CODES, current state */
    } sub;        /* submode */
    uint32 last;  /* true if this block is the last block */

    /* mode independent information */
    uint32        bitk;    /* bits in bit buffer */
    uint32        bitb;    /* bit buffer */
    inflate_huft* hufts;   /* single malloc for tree space */
    uint8*        window;  /* sliding window */
    uint8*        end;     /* one uint8 after sliding window */
    uint8*        read;    /* window read pointer */
    uint8*        write;   /* window write pointer */
    check_func    checkfn; /* check function */
    uint32        check;   /* check on output */
};


    /* defines for inflate input/output */
    /*   update pointers and return */
    #define UPDBITS      \
        {                \
            s->bitb = b; \
            s->bitk = k; \
        }
    #define UPDIN                          \
        {                                  \
            z->avail_in = n;               \
            z->total_in += p - z->next_in; \
            z->next_in = p;                \
        }
    #define UPDOUT        \
        {                 \
            s->write = q; \
        }
    #define UPDATE {UPDBITS UPDIN UPDOUT}
    #define LEAVE                                 \
        {                                         \
            UPDATE return inflate_flush(s, z, r); \
        }
    /*   get bytes and bits */
    #define LOADIN           \
        {                    \
            p = z->next_in;  \
            n = z->avail_in; \
            b = s->bitb;     \
            k = s->bitk;     \
        }
    #define NEEDBYTE      \
        {                 \
            if (n)        \
                r = Z_OK; \
            else          \
                LEAVE     \
        }
    #define NEXTBYTE (n--, *p++)
    #define NEEDBITS(j)                        \
        {                                      \
            while (k < (j)) {                  \
                NEEDBYTE;                      \
                b |= ((uint32) NEXTBYTE) << k; \
                k += 8;                        \
            }                                  \
        }
    #define DUMPBITS(j) \
        {               \
            b >>= (j);  \
            k -= (j);   \
        }
    /*   output bytes */
    #define WAVAIL (uint32)(q < s->read ? s->read - q - 1 : s->end - q)
    #define LOADOUT              \
        {                        \
            q = s->write;        \
            m = (uint32) WAVAIL; \
        }
    #define WRAP                                       \
        {                                              \
            if (q == s->end && s->read != s->window) { \
                q = s->window;                         \
                m = (uint32) WAVAIL;                   \
            }                                          \
        }
    #define FLUSH                              \
        {                                      \
            UPDOUT r = inflate_flush(s, z, r); \
            LOADOUT                            \
        }
    #define NEEDOUT                              \
        {                                        \
            if (m == 0) {                        \
                WRAP if (m == 0)                 \
                {                                \
                    FLUSH WRAP if (m == 0) LEAVE \
                }                                \
            }                                    \
            r = Z_OK;                            \
        }
    #define OUTBYTE(a)          \
        {                       \
            *q++ = (uint8) (a); \
            m--;                \
        }
    /*   load static pointers */
    #define LOAD {LOADIN LOADOUT}

/* masks for lower bits (size given to avoid silly warnings with Visual C++) */
static uint32 inflate_mask[17] = {
    0x0000,
    0x0001, 0x0003, 0x0007, 0x000f, 0x001f, 0x003f, 0x007f, 0x00ff,
    0x01ff, 0x03ff, 0x07ff, 0x0fff, 0x1fff, 0x3fff, 0x7fff, 0xffff
};

/* copy as much as possible from the sliding window to the output area */
static int32 inflate_flush OF((
    inflate_blocks_statef*,
    z_streamp,
    int32
));

#endif


/*
   Notes beyond the 1.93a appnote.txt:

   1. Distance pointers never point before the beginning of the output
      stream.
   2. Distance pointers can point back across blocks, up to 32k away.
   3. There is an implied maximum of 7 bits for the bit length table and
      15 bits for the actual data.
   4. If only one code exists, then it is encoded using one bit.  (Zero
      would be more efficient, but perhaps a little confusing.)  If two
      codes exist, they are coded using one bit each (0 and 1).
   5. There is no way of sending zero distance codes--a dummy must be
      sent if there are none.  (History: a pre 2.0 version of PKZIP would
      store blocks with no distance codes, but this was discovered to be
      too harsh a criterion.)  Valid only for 1.93a.  2.04c does allow
      zero distance codes, which is sent as one code of zero bits in
      length.
   6. There are up to 286 literal/length codes.  Code 256 represents the
      end-of-block.  Note however that the static length tree defines
      288 codes just to fill out the Huffman codes.  Codes 286 and 287
      cannot be used though, since there is no length base or extra bits
      defined for them.  Similarily, there are up to 30 distance codes.
      However, static trees define 32 codes (all 5 bits) to fill out the
      Huffman codes, but the last two had better not show up in the data.
   7. Unzip can check dynamic Huffman blocks for complete code sets.
      The exception is that a single code would not be complete (see #4).
   8. The five bits following the block type is really the number of
      literal codes sent minus 257.
   9. Length codes 8,16,16 are interpreted as 13 length codes of 8 bits
      (1+6+6).  Therefore, to output three times the length, you output
      three codes (1+1+1), whereas to output four times the same length,
      you only need two codes (1+3).  Hmm.
  10. In the tree reconstruction algorithm, Code = Code + Increment
      only if BitLength(i) is not zero.  (Pretty obvious.)
  11. Correction: 4 Bits: # of Bit Length codes - 4     (4 - 19)
  12. Note: length code 284 can represent 227-258, but length code 285
      really is 258.  The last length deserves its own, int16 code
      since it gets used a lot in very redundant files.  The length
      258 is special since 258 - 3 (the min match length) is 255.
  13. The literal/length and distance code bit lengths are read as a
      single stream of lengths.  It is possible (and advantageous) for
      a repeat code (16, 17, or 18) to go across the boundary between
      the two sets of lengths.
 */


void inflate_blocks_reset(inflate_blocks_statef* s, z_streamp z, uint32* c)
{
    if (c != Z_NULL) {
        *c = s->check;
    }
    if (s->mode == BTREE || s->mode == DTREE) {
        ZFREE(z, s->sub.trees.blens);
    }
    if (s->mode == CODES) {
        inflate_codes_free(s->sub.decode.codes, z);
    }
    s->mode = TYPE;
    s->bitk = 0;
    s->bitb = 0;
    s->read = s->write = s->window;
    if (s->checkfn != Z_NULL) {
        z->adler = s->check = (*s->checkfn)(0L, (const uint8*) Z_NULL, 0);
    }
    Tracev(("inflate:   blocks reset\n"));
}


inflate_blocks_statef* inflate_blocks_new(z_streamp z, check_func c, uint32 w)
{
    inflate_blocks_statef* s;

    if ((s = (inflate_blocks_statef*) ZALLOC(z, 1, sizeof(struct inflate_blocks_state))) == Z_NULL) {
        return s;
    }
    if ((s->hufts =
             (inflate_huft*) ZALLOC(z, sizeof(inflate_huft), MANY))
        == Z_NULL) {
        ZFREE(z, s);
        return Z_NULL;
    }
    if ((s->window = (uint8*) ZALLOC(z, 1, w)) == Z_NULL) {
        ZFREE(z, s->hufts);
        ZFREE(z, s);
        return Z_NULL;
    }
    s->end = s->window + w;
    s->checkfn = c;
    s->mode = TYPE;
    Tracev(("inflate:   blocks allocated\n"));
    inflate_blocks_reset(s, z, Z_NULL);
    return s;
}


int32 inflate_blocks(inflate_blocks_statef* s, z_streamp z, int32 r)
{
    uint32 t; /* temporary storage */
    uint32 b; /* bit buffer */
    uint32 k; /* bits in bit buffer */
    uint8* p; /* input data pointer */
    uint32 n; /* bytes available there */
    uint8* q; /* output window write pointer */
    uint32 m; /* bytes to end of window or read pointer */

    /* copy input/output information to locals (UPDATE macro restores) */
    LOAD

        /* process input based on current state */
        while (1) switch (s->mode)
    {
    case TYPE:
        NEEDBITS(3)
        t = (uint32) b & 7;
        s->last = t & 1;
        switch (t >> 1) {
        case 0: /* stored */
            Tracev(("inflate:     stored block%s\n", s->last ? " (last)" : ""));
            DUMPBITS(3)
            t = k & 7;      /* go to uint8 boundary */
            DUMPBITS(t)
            s->mode = LENS; /* get length of stored block */
            break;
        case 1:             /* fixed */
            Tracev(("inflate:     fixed codes block%s\n", s->last ? " (last)" : ""));
            {
                uint32        bl, bd;
                inflate_huft *tl, *td;
                inflate_trees_fixed(&bl, &bd, &tl, &td, z);
                s->sub.decode.codes = inflate_codes_new(bl, bd, tl, td, z);
                if (s->sub.decode.codes == Z_NULL) {
                    r = Z_MEM_ERROR;
                    LEAVE
                }
            }
            DUMPBITS(3)
            s->mode = CODES;
            break;
        case 2: /* dynamic */
            Tracev(("inflate:     dynamic codes block%s\n", s->last ? " (last)" : ""));
            DUMPBITS(3)
            s->mode = TABLE;
            break;
        case 3: /* illegal */
            DUMPBITS(3)
            s->mode = BAD;
            z->msg = (char*) "invalid block type";
            r = Z_DATA_ERROR;
            LEAVE
        }
        break;
    case LENS:
        NEEDBITS(32)
        if ((((~b) >> 16) & 0xffff) != (b & 0xffff)) {
            s->mode = BAD;
            z->msg = (char*) "invalid stored block lengths";
            r = Z_DATA_ERROR;
            LEAVE
        }
        s->sub.left = (uint32) b & 0xffff;
        b = k = 0; /* dump bits */
        Tracev(("inflate:       stored length %u\n", s->sub.left));
        s->mode = s->sub.left ? STORED : (s->last ? DRY : TYPE);
        break;
    case STORED:
        if (n == 0) {
            LEAVE
        }
        NEEDOUT
        t = s->sub.left;
        if (t > n) {
            t = n;
        }
        if (t > m) {
            t = m;
        }
        zmemcpy(q, p, t);
        p += t;
        n -= t;
        q += t;
        m -= t;
        if ((s->sub.left -= t) != 0) {
            break;
        }
        Tracev(("inflate:       stored end, %lu total out\n", z->total_out + (q >= s->read ? q - s->read : (s->end - s->read) + (q - s->window))));
        s->mode = s->last ? DRY : TYPE;
        break;
    case TABLE:
        NEEDBITS(14)
        s->sub.trees.table = t = (uint32) b & 0x3fff;
#ifndef PKZIP_BUG_WORKAROUND
        if ((t & 0x1f) > 29 || ((t >> 5) & 0x1f) > 29) {
            s->mode = BAD;
            z->msg = (char*) "too many length or distance symbols";
            r = Z_DATA_ERROR;
            LEAVE
        }
#endif
        t = 258 + (t & 0x1f) + ((t >> 5) & 0x1f);
        if ((s->sub.trees.blens = (uint32*) ZALLOC(z, t, sizeof(uint32))) == Z_NULL) {
            r = Z_MEM_ERROR;
            LEAVE
        }
        DUMPBITS(14)
        s->sub.trees.index = 0;
        Tracev(("inflate:       table sizes ok\n"));
        s->mode = BTREE;
    case BTREE:
        while (s->sub.trees.index < 4 + (s->sub.trees.table >> 10)) {
            NEEDBITS(3)
            s->sub.trees.blens[border[s->sub.trees.index++]] = (uint32) b & 7;
            DUMPBITS(3)
        }
        while (s->sub.trees.index < 19) {
            s->sub.trees.blens[border[s->sub.trees.index++]] = 0;
        }
        s->sub.trees.bb = 7;
        t = inflate_trees_bits(s->sub.trees.blens, &s->sub.trees.bb, &s->sub.trees.tb, s->hufts, z);
        if (t != Z_OK) {
            ZFREE(z, s->sub.trees.blens);
            r = t;
            if (r == Z_DATA_ERROR) {
                s->mode = BAD;
            }
            LEAVE
        }
        s->sub.trees.index = 0;
        Tracev(("inflate:       bits tree ok\n"));
        s->mode = DTREE;
    case DTREE:
        while (t = s->sub.trees.table,
               s->sub.trees.index < 258 + (t & 0x1f) + ((t >> 5) & 0x1f)) {
            inflate_huft* h;
            uint32        i, j, c;

            t = s->sub.trees.bb;
            NEEDBITS(t)
            h = s->sub.trees.tb + ((uint32) b & inflate_mask[t]);
            t = h->bits;
            c = h->base;
            if (c < 16) {
                DUMPBITS(t)
                s->sub.trees.blens[s->sub.trees.index++] = c;
            } else /* c == 16..18 */
            {
                i = c == 18 ? 7 : c - 14;
                j = c == 18 ? 11 : 3;
                NEEDBITS(t + i)
                DUMPBITS(t)
                j += (uint32) b & inflate_mask[i];
                DUMPBITS(i)
                i = s->sub.trees.index;
                t = s->sub.trees.table;
                if (i + j > 258 + (t & 0x1f) + ((t >> 5) & 0x1f) || (c == 16 && i < 1)) {
                    ZFREE(z, s->sub.trees.blens);
                    s->mode = BAD;
                    z->msg = (char*) "invalid bit length repeat";
                    r = Z_DATA_ERROR;
                    LEAVE
                }
                c = c == 16 ? s->sub.trees.blens[i - 1] : 0;
                do {
                    s->sub.trees.blens[i++] = c;
                } while (--j);
                s->sub.trees.index = i;
            }
        }
        s->sub.trees.tb = Z_NULL;
        {
            uint32                bl, bd;
            inflate_huft *        tl, *td;
            inflate_codes_statef* c;

            tl = NULL;
            td = NULL;
            bl = 9; /* must be <= 9 for lookahead assumptions */
            bd = 6; /* must be <= 9 for lookahead assumptions */
            t = s->sub.trees.table;
            t = inflate_trees_dynamic(257 + (t & 0x1f), 1 + ((t >> 5) & 0x1f), s->sub.trees.blens, &bl, &bd, &tl, &td, s->hufts, z);
            ZFREE(z, s->sub.trees.blens);
            if (t != Z_OK) {
                if (t == (uint32) Z_DATA_ERROR) {
                    s->mode = BAD;
                }
                r = t;
                LEAVE
            }
            Tracev(("inflate:       trees ok\n"));
            if ((c = inflate_codes_new(bl, bd, tl, td, z)) == Z_NULL) {
                r = Z_MEM_ERROR;
                LEAVE
            }
            s->sub.decode.codes = c;
        }
        s->mode = CODES;
    case CODES:
        UPDATE
        if ((r = inflate_codes(s, z, r)) != Z_STREAM_END) {
            return inflate_flush(s, z, r);
        }
        r = Z_OK;
        inflate_codes_free(s->sub.decode.codes, z);
        LOAD
            Tracev(("inflate:       codes end, %lu total out\n", z->total_out + (q >= s->read ? q - s->read : (s->end - s->read) + (q - s->window))));
        if (!s->last) {
            s->mode = TYPE;
            break;
        }
        s->mode = DRY;
    case DRY:
        FLUSH
        if (s->read != s->write) {
            LEAVE
        }
        s->mode = DONE;
    case DONE:
        r = Z_STREAM_END;
        LEAVE
    case BAD:
        r = Z_DATA_ERROR;
        LEAVE
    default:
        r = Z_STREAM_ERROR;
        LEAVE
    }
}


int32 inflate_blocks_free(inflate_blocks_statef* s, z_streamp z)
{
    inflate_blocks_reset(s, z, Z_NULL);
    ZFREE(z, s->window);
    ZFREE(z, s->hufts);
    ZFREE(z, s);
    Tracev(("inflate:   blocks freed\n"));
    return Z_OK;
}

/* And'ing with mask[n] masks the lower n bits */
// static uint32 inflate_mask[17]; // defined behiend


/* copy as much as possible from the sliding window to the output area */
int32 inflate_flush(inflate_blocks_statef* s, z_streamp z, int32 r)
{
    uint32 n;
    uint8* p;
    uint8* q;

    /* static copies of source and destination pointers */
    p = z->next_out;
    q = s->read;

    /* compute number of bytes to copy as as end of window */
    n = (uint32) ((q <= s->write ? s->write : s->end) - q);
    if (n > z->avail_out) {
        n = z->avail_out;
    }
    if (n && r == Z_BUF_ERROR) {
        r = Z_OK;
    }

    /* update counters */
    z->avail_out -= n;
    z->total_out += n;

    /* update check information */
    if (s->checkfn != Z_NULL) {
        z->adler = s->check = (*s->checkfn)(s->check, q, n);
    }

    /* copy as as end of window */
    zmemcpy(p, q, n);
    p += n;
    q += n;

    /* see if more to copy at beginning of window */
    if (q == s->end) {
        /* wrap pointers */
        q = s->window;
        if (s->write == s->end) {
            s->write = s->window;
        }

        /* compute bytes to copy */
        n = (uint32) (s->write - q);
        if (n > z->avail_out) {
            n = z->avail_out;
        }
        if (n && r == Z_BUF_ERROR) {
            r = Z_OK;
        }

        /* update counters */
        z->avail_out -= n;
        z->total_out += n;

        /* update check information */
        if (s->checkfn != Z_NULL) {
            z->adler = s->check = (*s->checkfn)(s->check, q, n);
        }

        /* copy */
        zmemcpy(p, q, n);
        p += n;
        q += n;
    }

    /* update pointers */
    z->next_out = p;
    s->read = q;

    /* done */
    return r;
}

/*
  If you use the zlib library in a product, an acknowledgment is welcome
  in the documentation of your product. If for some reason you cannot
  include such an acknowledgment, I would appreciate that you keep this
  copyright string in the executable of your product.
 */

/* simplify the use of the inflate_huft type with some defines */
#define exop word.what.Exop
#define bits word.what.Bits


static int32 huft_build OF((
    uint32*,        /* code lengths in bits */
    uint32,         /* number of codes */
    uint32,         /* number of "simple" codes */
    const uint32*,  /* list of base values for non-simple codes */
    const uint32*,  /* list of extra bits for non-simple codes */
    inflate_huft**, /* result: starting table */
    uint32*,        /* maximum lookup bits (returns actual) */
    inflate_huft*,  /* space for trees */
    uint32*,        /* hufts used in space */
    uint32*
) );                /* space for values */

/* Tables for deflate from PKZIP's appnote.txt. */
static const uint32 cplens[31] = {/* Copy lengths for literal codes 257..285 */
                                  3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258, 0, 0
};
/* see note #13 above about 258 */
static const uint32 cplext[31] = {/* Extra bits for literal codes 257..285 */
                                  0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0, 112, 112
}; /* 112==invalid */
static const uint32 cpdist[30] = {/* Copy offsets for distance codes 0..29 */
                                  1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577
};
static const uint32 cpdext[30] = {/* Extra bits for distance codes */
                                  0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13
};

/*
   Huffman code decoding is performed using a multi-level table lookup.
   The fastest way to decode is to simply build a lookup table whose
   size is determined by the longest code.  However, the time it takes
   to build this table can also be a factor if the data being decoded
   is not very int32.  The most common codes are necessarily the
   shortest codes, so those codes dominate the decoding time, and hence
   the speed.  The idea is you can have a shorter table that decodes the
   shorter, more probable codes, and then point to subsidiary tables for
   the longer codes.  The time it costs to decode the longer codes is
   then traded against the time it takes to make longer tables.

   This results of this trade are in the variables lbits and dbits
   below.  lbits is the number of bits the first level table for literal/
   length codes can decode in one step, and dbits is the same thing for
   the distance codes.  Subsequent tables are also less than or equal to
   those sizes.  These values may be adjusted either when all of the
   codes are shorter than that, in which case the longest code length in
   bits is used, or when the shortest code is *longer* than the requested
   table size, in which case the length of the shortest code in bits is
   used.

   There are two different values for the two tables, since they code a
   different number of possibilities each.  The literal/length table
   codes 286 possible values, or in a flat code, a little over eight
   bits.  The distance table codes 30 possible values, or a little less
   than five bits, flat.  The optimum values for speed end up being
   about one bit more than those, so lbits is 8+1 and dbits is 5+1.
   The optimum values may differ though from machine to machine, and
   possibly even between compilers.  Your mileage may vary.
 */


/* If BMAX needs to be larger than 16, then h and x[] should be uint32. */
#define BMAX 15 /* maximum bit length of any code */

static int32 huft_build(uint32* b, uint32 n, uint32 s, const uint32* d, const uint32* e, inflate_huft** t, uint32* m, inflate_huft* hp, uint32* hn, uint32* v)
// uint32 *b;               /* code lengths in bits (all assumed <= BMAX) */
// uint32 n;                 /* number of codes (assumed <= 288) */
// uint32 s;                 /* number of simple-valued codes (0..s-1) */
// const uint32 *d;         /* list of base values for non-simple codes */
// const uint32 *e;         /* list of extra bits for non-simple codes */
// inflate_huft ** t;        /* result: starting table */
// uint32 *m;               /* maximum lookup bits, returns actual */
// inflate_huft *hp;       /* space for trees */
// uint32 *hn;               /* hufts used in space */
// uint32 *v;               /* working area: values in order of bit length */
/* Given a list of code lengths and a maximum table size, make a set of
   tables to decode that set of codes.  Return Z_OK on success, Z_BUF_ERROR
   if the given code set is incomplete (the tables are still built in this
   case), Z_DATA_ERROR if the input is invalid (an over-subscribed set of
   lengths), or Z_MEM_ERROR if not enough memory. */
{
    uint32                a;           /* counter for codes of length k */
    uint32                c[BMAX + 1]; /* bit length count table */
    uint32                f;           /* i repeats in table every f entries */
    int32                 g;           /* maximum code length */
    int32                 h;           /* table level */
    register uint32       i;           /* counter, current code */
    register uint32       j;           /* counter */
    register int32        k;           /* number of bits in current code */
    int32                 l;           /* bits per table (returned in m) */
    uint32                mask;        /* (1 << w) - 1, to avoid cc -O bug on HP */
    register uint32*      p;           /* pointer into c[], b[], or v[] */
    inflate_huft*         q;           /* points to current table */
    struct inflate_huft_s r;           /* table entry for structure assignment */
    inflate_huft*         u[BMAX];     /* table stack */
    register int32        w;           /* bits before this table == (l * h) */
    uint32                x[BMAX + 1]; /* bit offsets, then code stack */
    uint32*               xp;          /* pointer into x */
    int32                 y;           /* number of dummy codes added */
    uint32                z;           /* number of entries in current table */


    /* Generate counts for each bit length */
    p = c;
#define C0 *p++ = 0;
#define C2 C0 C0 C0 C0
#define C4 C2 C2 C2 C2
    C4 /* clear c[]--assume BMAX+1 is 16 */
        p = b;
    i = n;
    do {
        c[*p++]++; /* assume all entries <= BMAX */
    } while (--i);
    if (c[0] == n) /* null input--all zero length codes */
    {
        *t = (inflate_huft*) Z_NULL;
        *m = 0;
        return Z_OK;
    }


    /* Find minimum and maximum length, bound *m by those */
    l = *m;
    for (j = 1; j <= BMAX; j++) {
        if (c[j]) {
            break;
        }
    }
    k = j; /* minimum code length */
    if ((uint32) l < j) {
        l = j;
    }
    for (i = BMAX; i; i--) {
        if (c[i]) {
            break;
        }
    }
    g = i; /* maximum code length */
    if ((uint32) l > i) {
        l = i;
    }
    *m = l;


    /* Adjust last length count to fill out codes, if needed */
    for (y = 1 << j; j < i; j++, y <<= 1) {
        if ((y -= c[j]) < 0) {
            return Z_DATA_ERROR;
        }
    }
    if ((y -= c[i]) < 0) {
        return Z_DATA_ERROR;
    }
    c[i] += y;


    /* Generate starting offsets into the value table for each length */
    x[1] = j = 0;
    p = c + 1;
    xp = x + 2;
    while (--i) { /* note that i == g from above */
        *xp++ = (j += *p++);
    }


    /* Make a table of values in order of bit lengths */
    p = b;
    i = 0;
    do {
        if ((j = *p++) != 0) {
            v[x[j]++] = i;
        }
    } while (++i < n);
    n = x[g]; /* set n to length of v */


    /* Generate the Huffman codes and for each, make the table entries */
    x[0] = i = 0;                  /* first Huffman code is zero */
    p = v;                         /* grab values in bit order */
    h = -1;                        /* no tables yet--level -1 */
    w = -l;                        /* bits decoded == (l * h) */
    u[0] = (inflate_huft*) Z_NULL; /* just to keep compilers happy */
    q = (inflate_huft*) Z_NULL;    /* ditto */
    z = 0;                         /* ditto */

    /* go through the bit lengths (k already is bits in shortest code) */
    for (; k <= g; k++) {
        a = c[k];
        while (a--) {
            /* here i is the Huffman code of length k bits for value *p */
            /* make tables up to required level */
            while (k > w + l) {
                h++;
                w += l; /* previous table always l bits */

                /* compute minimum size table less than or equal to l bits */
                z = g - w;
                z = z > (uint32) l ? l : z;         /* table size upper limit */
                if ((f = 1 << (j = k - w)) > a + 1) /* try a k-w bit table */
                {                                   /* too few codes for k-w bit table */
                    f -= a + 1;                     /* deduct codes from patterns left */
                    xp = c + k;
                    if (j < z) {
                        while (++j < z) /* try smaller tables up to z bits */
                        {
                            if ((f <<= 1) <= *++xp) {
                                break; /* enough codes to use up j bits */
                            }
                            f -= *xp;  /* else deduct codes from patterns */
                        }
                    }
                }
                z = 1 << j; /* table entries for j-bit table */

                /* allocate new table */
                if (*hn + z > MANY) {   /* (note: doesn't matter for fixed) */
                    return Z_MEM_ERROR; /* not enough memory */
                }
                u[h] = q = hp + *hn;
                *hn += z;

                /* connect to last table, if there is one */
                if (h) {
                    x[h] = i;                             /* save pattern for backing up */
                    r.bits = (uint8) l;                   /* bits to dump before this table */
                    r.exop = (uint8) j;                   /* bits in this table */
                    j = i >> (w - l);
                    r.base = (uint32) (q - u[h - 1] - j); /* offset to this table */
                    u[h - 1][j] = r;                      /* connect to last table */
                } else {
                    *t = q;                               /* first table is returned result */
                }
            }

            /* set up table entry in r */
            r.bits = (uint8) (k - w);
            if (p >= v + n) {
                r.exop = 128 + 64;                         /* out of values--invalid code */
            } else if (*p < s) {
                r.exop = (uint8) (*p < 256 ? 0 : 32 + 64); /* 256 is end-of-block */
                r.base = *p++;                             /* simple code is just the value */
            } else {
                r.exop = (uint8) (e[*p - s] + 16 + 64);    /* non-simple--look up in lists */
                r.base = d[*p++ - s];
            }

            /* fill code-like entries with r */
            f = 1 << (k - w);
            for (j = i >> w; j < z; j += f) {
                q[j] = r;
            }

            /* backwards increment the k-bit code i */
            for (j = 1 << (k - 1); i & j; j >>= 1) {
                i ^= j;
            }
            i ^= j;

            /* backup over finished tables */
            mask = (1 << w) - 1; /* needed on HP, cc -O bug */
            while ((i & mask) != x[h]) {
                h--;             /* don't need to update q */
                w -= l;
                mask = (1 << w) - 1;
            }
        }
    }


    /* Return Z_BUF_ERROR if we were given an incomplete table */
    return y != 0 && g != 1 ? Z_BUF_ERROR : Z_OK;
}


int32 inflate_trees_bits(uint32* c, uint32* bb, inflate_huft** tb, inflate_huft* hp, z_streamp z)
// uint32 *c;               /* 19 code lengths */
// uint32 *bb;              /* bits tree desired/actual depth */
// inflate_huft * *tb; /* bits tree result */
// inflate_huft *hp;       /* space for trees */
// z_streamp z;            /* for messages */
{
    int32   r;
    uint32  hn = 0; /* hufts used in space */
    uint32* v;      /* work area for huft_build */

    if ((v = (uint32*) ZALLOC(z, 19, sizeof(uint32))) == Z_NULL) {
        return Z_MEM_ERROR;
    }
    r = huft_build(c, 19, 19, (uint32*) Z_NULL, (uint32*) Z_NULL, tb, bb, hp, &hn, v);
    if (r == Z_DATA_ERROR) {
        z->msg = (char*) "oversubscribed dynamic bit lengths tree";
    } else if (r == Z_BUF_ERROR || *bb == 0) {
        z->msg = (char*) "incomplete dynamic bit lengths tree";
        r = Z_DATA_ERROR;
    }
    ZFREE(z, v);
    return r;
}


int32 inflate_trees_dynamic(uint32 nl, uint32 nd, uint32* c, uint32* bl, uint32* bd, inflate_huft** tl, inflate_huft** td, inflate_huft* hp, z_streamp z)
// uint32 nl;                /* number of literal/length codes */
// uint32 nd;                /* number of distance codes */
// uint32 *c;               /* that many (total) code lengths */
// uint32 *bl;              /* literal desired/actual bit depth */
// uint32 *bd;              /* distance desired/actual bit depth */
// inflate_huft * *tl; /* literal/length tree result */
// inflate_huft * *td; /* distance tree result */
// inflate_huft *hp;       /* space for trees */
// z_streamp z;            /* for messages */
{
    int32   r;
    uint32  hn = 0; /* hufts used in space */
    uint32* v;      /* work area for huft_build */

    /* allocate work area */
    if ((v = (uint32*) ZALLOC(z, 288, sizeof(uint32))) == Z_NULL) {
        return Z_MEM_ERROR;
    }

    /* build literal/length tree */
    r = huft_build(c, nl, 257, cplens, cplext, tl, bl, hp, &hn, v);
    if (r != Z_OK || *bl == 0) {
        if (r == Z_DATA_ERROR) {
            z->msg = (char*) "oversubscribed literal/length tree";
        } else if (r != Z_MEM_ERROR) {
            z->msg = (char*) "incomplete literal/length tree";
            r = Z_DATA_ERROR;
        }
        ZFREE(z, v);
        return r;
    }

    /* build distance tree */
    r = huft_build(c + nl, nd, 0, cpdist, cpdext, td, bd, hp, &hn, v);
    if (r != Z_OK || (*bd == 0 && nl > 257)) {
        if (r == Z_DATA_ERROR) {
            z->msg = (char*) "oversubscribed distance tree";
        } else if (r == Z_BUF_ERROR) {
#ifdef PKZIP_BUG_WORKAROUND
            r = Z_OK;
        }
#else
            z->msg = (char*) "incomplete distance tree";
            r = Z_DATA_ERROR;
        } else if (r != Z_MEM_ERROR) {
            z->msg = (char*) "empty distance tree with lengths";
            r = Z_DATA_ERROR;
        }
        ZFREE(z, v);
        return r;
#endif
    }

    /* done */
    ZFREE(z, v);
    return Z_OK;
}

/* inffixed.h -- table for decoding fixed codes
 * Generated automatically by the maketree.c program
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

static uint32       fixed_bl = 9;
static uint32       fixed_bd = 5;
static inflate_huft fixed_tl[] = {
    {{{96, 7}}, 256}, {{{0, 8}}, 80}, {{{0, 8}}, 16}, {{{84, 8}}, 115}, {{{82, 7}}, 31}, {{{0, 8}}, 112}, {{{0, 8}}, 48}, {{{0, 9}}, 192}, {{{80, 7}}, 10}, {{{0, 8}}, 96}, {{{0, 8}}, 32}, {{{0, 9}}, 160}, {{{0, 8}}, 0}, {{{0, 8}}, 128}, {{{0, 8}}, 64}, {{{0, 9}}, 224}, {{{80, 7}}, 6}, {{{0, 8}}, 88}, {{{0, 8}}, 24}, {{{0, 9}}, 144}, {{{83, 7}}, 59}, {{{0, 8}}, 120}, {{{0, 8}}, 56}, {{{0, 9}}, 208}, {{{81, 7}}, 17}, {{{0, 8}}, 104}, {{{0, 8}}, 40}, {{{0, 9}}, 176}, {{{0, 8}}, 8}, {{{0, 8}}, 136}, {{{0, 8}}, 72}, {{{0, 9}}, 240}, {{{80, 7}}, 4}, {{{0, 8}}, 84}, {{{0, 8}}, 20}, {{{85, 8}}, 227}, {{{83, 7}}, 43}, {{{0, 8}}, 116}, {{{0, 8}}, 52}, {{{0, 9}}, 200}, {{{81, 7}}, 13}, {{{0, 8}}, 100}, {{{0, 8}}, 36}, {{{0, 9}}, 168}, {{{0, 8}}, 4}, {{{0, 8}}, 132}, {{{0, 8}}, 68}, {{{0, 9}}, 232}, {{{80, 7}}, 8}, {{{0, 8}}, 92}, {{{0, 8}}, 28}, {{{0, 9}}, 152}, {{{84, 7}}, 83}, {{{0, 8}}, 124}, {{{0, 8}}, 60}, {{{0, 9}}, 216}, {{{82, 7}}, 23}, {{{0, 8}}, 108}, {{{0, 8}}, 44}, {{{0, 9}}, 184}, {{{0, 8}}, 12}, {{{0, 8}}, 140}, {{{0, 8}}, 76}, {{{0, 9}}, 248}, {{{80, 7}}, 3}, {{{0, 8}}, 82}, {{{0, 8}}, 18}, {{{85, 8}}, 163}, {{{83, 7}}, 35}, {{{0, 8}}, 114}, {{{0, 8}}, 50}, {{{0, 9}}, 196}, {{{81, 7}}, 11}, {{{0, 8}}, 98}, {{{0, 8}}, 34}, {{{0, 9}}, 164}, {{{0, 8}}, 2}, {{{0, 8}}, 130}, {{{0, 8}}, 66}, {{{0, 9}}, 228}, {{{80, 7}}, 7}, {{{0, 8}}, 90}, {{{0, 8}}, 26}, {{{0, 9}}, 148}, {{{84, 7}}, 67}, {{{0, 8}}, 122}, {{{0, 8}}, 58}, {{{0, 9}}, 212}, {{{82, 7}}, 19}, {{{0, 8}}, 106}, {{{0, 8}}, 42}, {{{0, 9}}, 180}, {{{0, 8}}, 10}, {{{0, 8}}, 138}, {{{0, 8}}, 74}, {{{0, 9}}, 244}, {{{80, 7}}, 5}, {{{0, 8}}, 86}, {{{0, 8}}, 22}, {{{192, 8}}, 0}, {{{83, 7}}, 51}, {{{0, 8}}, 118}, {{{0, 8}}, 54}, {{{0, 9}}, 204}, {{{81, 7}}, 15}, {{{0, 8}}, 102}, {{{0, 8}}, 38}, {{{0, 9}}, 172}, {{{0, 8}}, 6}, {{{0, 8}}, 134}, {{{0, 8}}, 70}, {{{0, 9}}, 236}, {{{80, 7}}, 9}, {{{0, 8}}, 94}, {{{0, 8}}, 30}, {{{0, 9}}, 156}, {{{84, 7}}, 99}, {{{0, 8}}, 126}, {{{0, 8}}, 62}, {{{0, 9}}, 220}, {{{82, 7}}, 27}, {{{0, 8}}, 110}, {{{0, 8}}, 46}, {{{0, 9}}, 188}, {{{0, 8}}, 14}, {{{0, 8}}, 142}, {{{0, 8}}, 78}, {{{0, 9}}, 252}, {{{96, 7}}, 256}, {{{0, 8}}, 81}, {{{0, 8}}, 17}, {{{85, 8}}, 131}, {{{82, 7}}, 31}, {{{0, 8}}, 113}, {{{0, 8}}, 49}, {{{0, 9}}, 194}, {{{80, 7}}, 10}, {{{0, 8}}, 97}, {{{0, 8}}, 33}, {{{0, 9}}, 162}, {{{0, 8}}, 1}, {{{0, 8}}, 129}, {{{0, 8}}, 65}, {{{0, 9}}, 226}, {{{80, 7}}, 6}, {{{0, 8}}, 89}, {{{0, 8}}, 25}, {{{0, 9}}, 146}, {{{83, 7}}, 59}, {{{0, 8}}, 121}, {{{0, 8}}, 57}, {{{0, 9}}, 210}, {{{81, 7}}, 17}, {{{0, 8}}, 105}, {{{0, 8}}, 41}, {{{0, 9}}, 178}, {{{0, 8}}, 9}, {{{0, 8}}, 137}, {{{0, 8}}, 73}, {{{0, 9}}, 242}, {{{80, 7}}, 4}, {{{0, 8}}, 85}, {{{0, 8}}, 21}, {{{80, 8}}, 258}, {{{83, 7}}, 43}, {{{0, 8}}, 117}, {{{0, 8}}, 53}, {{{0, 9}}, 202}, {{{81, 7}}, 13}, {{{0, 8}}, 101}, {{{0, 8}}, 37}, {{{0, 9}}, 170}, {{{0, 8}}, 5}, {{{0, 8}}, 133}, {{{0, 8}}, 69}, {{{0, 9}}, 234}, {{{80, 7}}, 8}, {{{0, 8}}, 93}, {{{0, 8}}, 29}, {{{0, 9}}, 154}, {{{84, 7}}, 83}, {{{0, 8}}, 125}, {{{0, 8}}, 61}, {{{0, 9}}, 218}, {{{82, 7}}, 23}, {{{0, 8}}, 109}, {{{0, 8}}, 45}, {{{0, 9}}, 186}, {{{0, 8}}, 13}, {{{0, 8}}, 141}, {{{0, 8}}, 77}, {{{0, 9}}, 250}, {{{80, 7}}, 3}, {{{0, 8}}, 83}, {{{0, 8}}, 19}, {{{85, 8}}, 195}, {{{83, 7}}, 35}, {{{0, 8}}, 115}, {{{0, 8}}, 51}, {{{0, 9}}, 198}, {{{81, 7}}, 11}, {{{0, 8}}, 99}, {{{0, 8}}, 35}, {{{0, 9}}, 166}, {{{0, 8}}, 3}, {{{0, 8}}, 131}, {{{0, 8}}, 67}, {{{0, 9}}, 230}, {{{80, 7}}, 7}, {{{0, 8}}, 91}, {{{0, 8}}, 27}, {{{0, 9}}, 150}, {{{84, 7}}, 67}, {{{0, 8}}, 123}, {{{0, 8}}, 59}, {{{0, 9}}, 214}, {{{82, 7}}, 19}, {{{0, 8}}, 107}, {{{0, 8}}, 43}, {{{0, 9}}, 182}, {{{0, 8}}, 11}, {{{0, 8}}, 139}, {{{0, 8}}, 75}, {{{0, 9}}, 246}, {{{80, 7}}, 5}, {{{0, 8}}, 87}, {{{0, 8}}, 23}, {{{192, 8}}, 0}, {{{83, 7}}, 51}, {{{0, 8}}, 119}, {{{0, 8}}, 55}, {{{0, 9}}, 206}, {{{81, 7}}, 15}, {{{0, 8}}, 103}, {{{0, 8}}, 39}, {{{0, 9}}, 174}, {{{0, 8}}, 7}, {{{0, 8}}, 135}, {{{0, 8}}, 71}, {{{0, 9}}, 238}, {{{80, 7}}, 9}, {{{0, 8}}, 95}, {{{0, 8}}, 31}, {{{0, 9}}, 158}, {{{84, 7}}, 99}, {{{0, 8}}, 127}, {{{0, 8}}, 63}, {{{0, 9}}, 222}, {{{82, 7}}, 27}, {{{0, 8}}, 111}, {{{0, 8}}, 47}, {{{0, 9}}, 190}, {{{0, 8}}, 15}, {{{0, 8}}, 143}, {{{0, 8}}, 79}, {{{0, 9}}, 254}, {{{96, 7}}, 256}, {{{0, 8}}, 80}, {{{0, 8}}, 16}, {{{84, 8}}, 115}, {{{82, 7}}, 31}, {{{0, 8}}, 112}, {{{0, 8}}, 48}, {{{0, 9}}, 193}, {{{80, 7}}, 10}, {{{0, 8}}, 96}, {{{0, 8}}, 32}, {{{0, 9}}, 161}, {{{0, 8}}, 0}, {{{0, 8}}, 128}, {{{0, 8}}, 64}, {{{0, 9}}, 225}, {{{80, 7}}, 6}, {{{0, 8}}, 88}, {{{0, 8}}, 24}, {{{0, 9}}, 145}, {{{83, 7}}, 59}, {{{0, 8}}, 120}, {{{0, 8}}, 56}, {{{0, 9}}, 209}, {{{81, 7}}, 17}, {{{0, 8}}, 104}, {{{0, 8}}, 40}, {{{0, 9}}, 177}, {{{0, 8}}, 8}, {{{0, 8}}, 136}, {{{0, 8}}, 72}, {{{0, 9}}, 241}, {{{80, 7}}, 4}, {{{0, 8}}, 84}, {{{0, 8}}, 20}, {{{85, 8}}, 227}, {{{83, 7}}, 43}, {{{0, 8}}, 116}, {{{0, 8}}, 52}, {{{0, 9}}, 201}, {{{81, 7}}, 13}, {{{0, 8}}, 100}, {{{0, 8}}, 36}, {{{0, 9}}, 169}, {{{0, 8}}, 4}, {{{0, 8}}, 132}, {{{0, 8}}, 68}, {{{0, 9}}, 233}, {{{80, 7}}, 8}, {{{0, 8}}, 92}, {{{0, 8}}, 28}, {{{0, 9}}, 153}, {{{84, 7}}, 83}, {{{0, 8}}, 124}, {{{0, 8}}, 60}, {{{0, 9}}, 217}, {{{82, 7}}, 23}, {{{0, 8}}, 108}, {{{0, 8}}, 44}, {{{0, 9}}, 185}, {{{0, 8}}, 12}, {{{0, 8}}, 140}, {{{0, 8}}, 76}, {{{0, 9}}, 249}, {{{80, 7}}, 3}, {{{0, 8}}, 82}, {{{0, 8}}, 18}, {{{85, 8}}, 163}, {{{83, 7}}, 35}, {{{0, 8}}, 114}, {{{0, 8}}, 50}, {{{0, 9}}, 197}, {{{81, 7}}, 11}, {{{0, 8}}, 98}, {{{0, 8}}, 34}, {{{0, 9}}, 165}, {{{0, 8}}, 2}, {{{0, 8}}, 130}, {{{0, 8}}, 66}, {{{0, 9}}, 229}, {{{80, 7}}, 7}, {{{0, 8}}, 90}, {{{0, 8}}, 26}, {{{0, 9}}, 149}, {{{84, 7}}, 67}, {{{0, 8}}, 122}, {{{0, 8}}, 58}, {{{0, 9}}, 213}, {{{82, 7}}, 19}, {{{0, 8}}, 106}, {{{0, 8}}, 42}, {{{0, 9}}, 181}, {{{0, 8}}, 10}, {{{0, 8}}, 138}, {{{0, 8}}, 74}, {{{0, 9}}, 245}, {{{80, 7}}, 5}, {{{0, 8}}, 86}, {{{0, 8}}, 22}, {{{192, 8}}, 0}, {{{83, 7}}, 51}, {{{0, 8}}, 118}, {{{0, 8}}, 54}, {{{0, 9}}, 205}, {{{81, 7}}, 15}, {{{0, 8}}, 102}, {{{0, 8}}, 38}, {{{0, 9}}, 173}, {{{0, 8}}, 6}, {{{0, 8}}, 134}, {{{0, 8}}, 70}, {{{0, 9}}, 237}, {{{80, 7}}, 9}, {{{0, 8}}, 94}, {{{0, 8}}, 30}, {{{0, 9}}, 157}, {{{84, 7}}, 99}, {{{0, 8}}, 126}, {{{0, 8}}, 62}, {{{0, 9}}, 221}, {{{82, 7}}, 27}, {{{0, 8}}, 110}, {{{0, 8}}, 46}, {{{0, 9}}, 189}, {{{0, 8}}, 14}, {{{0, 8}}, 142}, {{{0, 8}}, 78}, {{{0, 9}}, 253}, {{{96, 7}}, 256}, {{{0, 8}}, 81}, {{{0, 8}}, 17}, {{{85, 8}}, 131}, {{{82, 7}}, 31}, {{{0, 8}}, 113}, {{{0, 8}}, 49}, {{{0, 9}}, 195}, {{{80, 7}}, 10}, {{{0, 8}}, 97}, {{{0, 8}}, 33}, {{{0, 9}}, 163}, {{{0, 8}}, 1}, {{{0, 8}}, 129}, {{{0, 8}}, 65}, {{{0, 9}}, 227}, {{{80, 7}}, 6}, {{{0, 8}}, 89}, {{{0, 8}}, 25}, {{{0, 9}}, 147}, {{{83, 7}}, 59}, {{{0, 8}}, 121}, {{{0, 8}}, 57}, {{{0, 9}}, 211}, {{{81, 7}}, 17}, {{{0, 8}}, 105}, {{{0, 8}}, 41}, {{{0, 9}}, 179}, {{{0, 8}}, 9}, {{{0, 8}}, 137}, {{{0, 8}}, 73}, {{{0, 9}}, 243}, {{{80, 7}}, 4}, {{{0, 8}}, 85}, {{{0, 8}}, 21}, {{{80, 8}}, 258}, {{{83, 7}}, 43}, {{{0, 8}}, 117}, {{{0, 8}}, 53}, {{{0, 9}}, 203}, {{{81, 7}}, 13}, {{{0, 8}}, 101}, {{{0, 8}}, 37}, {{{0, 9}}, 171}, {{{0, 8}}, 5}, {{{0, 8}}, 133}, {{{0, 8}}, 69}, {{{0, 9}}, 235}, {{{80, 7}}, 8}, {{{0, 8}}, 93}, {{{0, 8}}, 29}, {{{0, 9}}, 155}, {{{84, 7}}, 83}, {{{0, 8}}, 125}, {{{0, 8}}, 61}, {{{0, 9}}, 219}, {{{82, 7}}, 23}, {{{0, 8}}, 109}, {{{0, 8}}, 45}, {{{0, 9}}, 187}, {{{0, 8}}, 13}, {{{0, 8}}, 141}, {{{0, 8}}, 77}, {{{0, 9}}, 251}, {{{80, 7}}, 3}, {{{0, 8}}, 83}, {{{0, 8}}, 19}, {{{85, 8}}, 195}, {{{83, 7}}, 35}, {{{0, 8}}, 115}, {{{0, 8}}, 51}, {{{0, 9}}, 199}, {{{81, 7}}, 11}, {{{0, 8}}, 99}, {{{0, 8}}, 35}, {{{0, 9}}, 167}, {{{0, 8}}, 3}, {{{0, 8}}, 131}, {{{0, 8}}, 67}, {{{0, 9}}, 231}, {{{80, 7}}, 7}, {{{0, 8}}, 91}, {{{0, 8}}, 27}, {{{0, 9}}, 151}, {{{84, 7}}, 67}, {{{0, 8}}, 123}, {{{0, 8}}, 59}, {{{0, 9}}, 215}, {{{82, 7}}, 19}, {{{0, 8}}, 107}, {{{0, 8}}, 43}, {{{0, 9}}, 183}, {{{0, 8}}, 11}, {{{0, 8}}, 139}, {{{0, 8}}, 75}, {{{0, 9}}, 247}, {{{80, 7}}, 5}, {{{0, 8}}, 87}, {{{0, 8}}, 23}, {{{192, 8}}, 0}, {{{83, 7}}, 51}, {{{0, 8}}, 119}, {{{0, 8}}, 55}, {{{0, 9}}, 207}, {{{81, 7}}, 15}, {{{0, 8}}, 103}, {{{0, 8}}, 39}, {{{0, 9}}, 175}, {{{0, 8}}, 7}, {{{0, 8}}, 135}, {{{0, 8}}, 71}, {{{0, 9}}, 239}, {{{80, 7}}, 9}, {{{0, 8}}, 95}, {{{0, 8}}, 31}, {{{0, 9}}, 159}, {{{84, 7}}, 99}, {{{0, 8}}, 127}, {{{0, 8}}, 63}, {{{0, 9}}, 223}, {{{82, 7}}, 27}, {{{0, 8}}, 111}, {{{0, 8}}, 47}, {{{0, 9}}, 191}, {{{0, 8}}, 15}, {{{0, 8}}, 143}, {{{0, 8}}, 79}, {{{0, 9}}, 255}
};
static inflate_huft fixed_td[] = {
    {{{80, 5}}, 1}, {{{87, 5}}, 257}, {{{83, 5}}, 17}, {{{91, 5}}, 4097}, {{{81, 5}}, 5}, {{{89, 5}}, 1025}, {{{85, 5}}, 65}, {{{93, 5}}, 16385}, {{{80, 5}}, 3}, {{{88, 5}}, 513}, {{{84, 5}}, 33}, {{{92, 5}}, 8193}, {{{82, 5}}, 9}, {{{90, 5}}, 2049}, {{{86, 5}}, 129}, {{{192, 5}}, 24577}, {{{80, 5}}, 2}, {{{87, 5}}, 385}, {{{83, 5}}, 25}, {{{91, 5}}, 6145}, {{{81, 5}}, 7}, {{{89, 5}}, 1537}, {{{85, 5}}, 97}, {{{93, 5}}, 24577}, {{{80, 5}}, 4}, {{{88, 5}}, 769}, {{{84, 5}}, 49}, {{{92, 5}}, 12289}, {{{82, 5}}, 13}, {{{90, 5}}, 3073}, {{{86, 5}}, 193}, {{{192, 5}}, 24577}
};

int32 inflate_trees_fixed(uint32* bl, uint32* bd, inflate_huft** tl, inflate_huft** td, z_streamp z)
// uint32 *bl;               /* literal desired/actual bit depth */
// uint32 *bd;               /* distance desired/actual bit depth */
// inflate_huft * *tl;  /* literal/length tree result */
// inflate_huft * *td;  /* distance tree result */
// z_streamp z;             /* for memory allocation */
{
    *bl = fixed_bl;
    *bd = fixed_bd;
    *tl = fixed_tl;
    *td = fixed_td;
    return Z_OK;
}

/* simplify the use of the inflate_huft type with some defines */
#define exop word.what.Exop
#define bits word.what.Bits

/* macros for bit input with no checking and for returning unused bytes */
#define GRABBITS(j)                        \
    {                                      \
        while (k < (j)) {                  \
            b |= ((uint32) NEXTBYTE) << k; \
            k += 8;                        \
        }                                  \
    }
#define UNGRAB                         \
    {                                  \
        c = z->avail_in - n;           \
        c = (k >> 3) < c ? k >> 3 : c; \
        n += c;                        \
        p -= c;                        \
        k -= c << 3;                   \
    }

/* Called with number of bytes left to write in window at least 258
   (the maximum string length) and number of input bytes available
   at least ten.  The ten bytes are six bytes for the longest length/
   distance pair plus four bytes for overloading the bit buffer. */

static int32 inflate_fast(uint32 bl, uint32 bd, inflate_huft* tl, inflate_huft* td, inflate_blocks_statef* s, z_streamp z)
{
    inflate_huft* t;  /* temporary pointer */
    uint32        e;  /* extra bits or operation */
    uint32        b;  /* bit buffer */
    uint32        k;  /* bits in bit buffer */
    uint8*        p;  /* input data pointer */
    uint32        n;  /* bytes available there */
    uint8*        q;  /* output window write pointer */
    uint32        m;  /* bytes to end of window or read pointer */
    uint32        ml; /* mask for literal/length tree */
    uint32        md; /* mask for distance tree */
    uint32        c;  /* bytes to copy */
    uint32        d;  /* distance back to copy from */
    uint8*        r;  /* copy source pointer */

    /* load input, output, bit values */
    LOAD

        /* initialize masks */
        ml = inflate_mask[bl];
    md = inflate_mask[bd];

    /* do until not enough input or output space for fast loop */
    do { /* assume called with m >= 258 && n >= 10 */
        /* get literal/length code */
        GRABBITS(20) /* max bits for literal/length code */
        if ((e = (t = tl + ((uint32) b & ml))->exop) == 0) {
            DUMPBITS(t->bits)
            Tracevv((t->base >= 0x20 && t->base < 0x7f ? "inflate:         * literal '%c'\n" : "inflate:         * literal 0x%02x\n", t->base));
            *q++ = (uint8) t->base;
            m--;
            continue;
        }
        do {
            DUMPBITS(t->bits)
            if (e & 16) {
                /* get extra bits for length */
                e &= 15;
                c = t->base + ((uint32) b & inflate_mask[e]);
                DUMPBITS(e)
                Tracevv(("inflate:         * length %u\n", c));

                /* decode distance base of block to copy */
                GRABBITS(15); /* max bits for distance code */
                e = (t = td + ((uint32) b & md))->exop;
                do {
                    DUMPBITS(t->bits)
                    if (e & 16) {
                        /* get extra bits to add to distance base */
                        e &= 15;
                        GRABBITS(e) /* get extra bits (up to 13) */
                        d = t->base + ((uint32) b & inflate_mask[e]);
                        DUMPBITS(e)
                        Tracevv(("inflate:         * distance %u\n", d));

                        /* do the copy */
                        m -= c;
                        if ((uint32) (q - s->window) >= d) /* offset before dest */
                        {                                  /*  just copy */
                            r = q - d;
                            *q++ = *r++;
                            c--;                              /* minimum count is three, */
                            *q++ = *r++;
                            c--;                              /*  so unroll loop a little */
                        } else                                /* else offset after destination */
                        {
                            e = d - (uint32) (q - s->window); /* bytes from offset to end */
                            r = s->end - e;                   /* pointer to offset */
                            if (c > e)                        /* if source crosses, */
                            {
                                c -= e;                       /* copy to end of window */
                                do {
                                    *q++ = *r++;
                                } while (--e);
                                r = s->window; /* copy rest from start of window */
                            }
                        }
                        do { /* copy all or what's left */
                            *q++ = *r++;
                        } while (--c);
                        break;
                    } else if ((e & 64) == 0) {
                        t += t->base;
                        e = (t += ((uint32) b & inflate_mask[e]))->exop;
                    } else {
                        z->msg = (char*) "invalid distance code";
                        UNGRAB
                        UPDATE
                        return Z_DATA_ERROR;
                    }
                } while (1);
                break;
            }
            if ((e & 64) == 0) {
                t += t->base;
                if ((e = (t += ((uint32) b & inflate_mask[e]))->exop) == 0) {
                    DUMPBITS(t->bits)
                    Tracevv((t->base >= 0x20 && t->base < 0x7f ? "inflate:         * literal '%c'\n" : "inflate:         * literal 0x%02x\n", t->base));
                    *q++ = (uint8) t->base;
                    m--;
                    break;
                }
            } else if (e & 32) {
                Tracevv(("inflate:         * end of block\n"));
                UNGRAB
                UPDATE
                return Z_STREAM_END;
            } else {
                z->msg = (char*) "invalid literal/length code";
                UNGRAB
                UPDATE
                return Z_DATA_ERROR;
            }
        } while (1);
    } while (m >= 258 && n >= 10);

    /* not enough input or output--restore pointers and return */
    UNGRAB
    UPDATE
    return Z_OK;
}

/* infcodes.c -- process literals and length/distance pairs
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* simplify the use of the inflate_huft type with some defines */
#define exop word.what.Exop
#define bits word.what.Bits

typedef enum
{          /* waiting for "i:"=input, "o:"=output, "x:"=nothing */
  START,   /* x: set up for LEN */
  LEN,     /* i: get length/literal/eob next */
  LENEXT,  /* i: getting length extra (have base) */
  DIST,    /* i: get distance next */
  DISTEXT, /* i: getting distance extra */
  COPY,    /* o: copying bytes in window, waiting for space */
  LIT,     /* o: got literal, waiting for output space */
  WASH,    /* o: got eob, possibly still output waiting */
  END,     /* x: got eob and all data flushed */
  BADCODE
} /* x: got error */
inflate_codes_mode;

/* inflate codes private state */
struct inflate_codes_state
{
    /* mode */
    inflate_codes_mode mode; /* current inflate_codes mode */

    /* mode dependent information */
    uint32 len;
    union
    {
        struct
        {
            inflate_huft* tree; /* pointer into tree */
            uint32        need; /* bits needed */
        } code;                 /* if LEN or DIST, where in tree */
        uint32 lit;             /* if LIT, literal */
        struct
        {
            uint32 get;  /* bits to get for extra */
            uint32 dist; /* distance back to copy from */
        } copy;          /* if EXT or COPY, where and how much */
    } sub;               /* submode */

    /* mode independent information */
    uint8         lbits; /* ltree bits decoded per branch */
    uint8         dbits; /* dtree bits decoder per branch */
    inflate_huft* ltree; /* literal/length/eob tree */
    inflate_huft* dtree; /* distance tree */
};


inflate_codes_statef* inflate_codes_new(uint32 bl, uint32 bd, inflate_huft* tl, inflate_huft* td, z_streamp z)
{
    inflate_codes_statef* c;

    if ((c = (inflate_codes_statef*)
             ZALLOC(z, 1, sizeof(struct inflate_codes_state)))
        != Z_NULL) {
        c->mode = START;
        c->lbits = (uint8) bl;
        c->dbits = (uint8) bd;
        c->ltree = tl;
        c->dtree = td;
        Tracev(("inflate:       codes new\n"));
    }
    return c;
}


int32 inflate_codes(inflate_blocks_statef* s, z_streamp z, int32 r)
{
    uint32                j;                       /* temporary storage */
    inflate_huft*         t;                       /* temporary pointer */
    uint32                e;                       /* extra bits or operation */
    uint32                b;                       /* bit buffer */
    uint32                k;                       /* bits in bit buffer */
    uint8*                p;                       /* input data pointer */
    uint32                n;                       /* bytes available there */
    uint8*                q;                       /* output window write pointer */
    uint32                m;                       /* bytes to end of window or read pointer */
    uint8*                f;                       /* pointer to copy strings from */
    inflate_codes_statef* c = s->sub.decode.codes; /* codes state */

    /* copy input/output information to locals (UPDATE macro restores) */
    LOAD

        /* process input and output based on current state */
        while (1) switch (c->mode)
    {           /* waiting for "i:"=input, "o:"=output, "x:"=nothing */
    case START: /* x: set up for LEN */
#ifndef SLOW
        if (m >= 258 && n >= 10) {
            UPDATE
            r = inflate_fast(c->lbits, c->dbits, c->ltree, c->dtree, s, z);
            LOAD if (r != Z_OK)
            {
                c->mode = r == Z_STREAM_END ? WASH : BADCODE;
                break;
            }
        }
#endif /* !SLOW */
        c->sub.code.need = c->lbits;
        c->sub.code.tree = c->ltree;
        c->mode = LEN;
    case LEN: /* i: get length/literal/eob next */
        j = c->sub.code.need;
        NEEDBITS(j)
        t = c->sub.code.tree + ((uint32) b & inflate_mask[j]);
        DUMPBITS(t->bits)
        e = (uint32) (t->exop);
        if (e == 0) /* literal */
        {
            c->sub.lit = t->base;
            Tracevv((t->base >= 0x20 && t->base < 0x7f ? "inflate:         literal '%c'\n" : "inflate:         literal 0x%02x\n", t->base));
            c->mode = LIT;
            break;
        }
        if (e & 16) /* length */
        {
            c->sub.copy.get = e & 15;
            c->len = t->base;
            c->mode = LENEXT;
            break;
        }
        if ((e & 64) == 0) /* next table */
        {
            c->sub.code.need = e;
            c->sub.code.tree = t + t->base;
            break;
        }
        if (e & 32) /* end of block */
        {
            Tracevv(("inflate:         end of block\n"));
            c->mode = WASH;
            break;
        }
        c->mode = BADCODE; /* invalid code */
        z->msg = (char*) "invalid literal/length code";
        r = Z_DATA_ERROR;
        LEAVE
    case LENEXT: /* i: getting length extra (have base) */
        j = c->sub.copy.get;
        NEEDBITS(j)
        c->len += (uint32) b & inflate_mask[j];
        DUMPBITS(j)
        c->sub.code.need = c->dbits;
        c->sub.code.tree = c->dtree;
        Tracevv(("inflate:         length %u\n", c->len));
        c->mode = DIST;
    case DIST: /* i: get distance next */
        j = c->sub.code.need;
        NEEDBITS(j)
        t = c->sub.code.tree + ((uint32) b & inflate_mask[j]);
        DUMPBITS(t->bits)
        e = (uint32) (t->exop);
        if (e & 16) /* distance */
        {
            c->sub.copy.get = e & 15;
            c->sub.copy.dist = t->base;
            c->mode = DISTEXT;
            break;
        }
        if ((e & 64) == 0) /* next table */
        {
            c->sub.code.need = e;
            c->sub.code.tree = t + t->base;
            break;
        }
        c->mode = BADCODE; /* invalid code */
        z->msg = (char*) "invalid distance code";
        r = Z_DATA_ERROR;
        LEAVE
    case DISTEXT: /* i: getting distance extra */
        j = c->sub.copy.get;
        NEEDBITS(j)
        c->sub.copy.dist += (uint32) b & inflate_mask[j];
        DUMPBITS(j)
        Tracevv(("inflate:         distance %u\n", c->sub.copy.dist));
        c->mode = COPY;
    case COPY:     /* o: copying bytes in window, waiting for space */
#ifndef __TURBOC__ /* Turbo C bug for following expression */
        f = (uint32) (q - s->window) < c->sub.copy.dist ? s->end - (c->sub.copy.dist - (q - s->window)) : q - c->sub.copy.dist;
#else
        f = q - c->sub.copy.dist;
        if ((uint32) (q - s->window) < c->sub.copy.dist) {
            f = s->end - (c->sub.copy.dist - (uint32) (q - s->window));
        }
#endif
        while (c->len) {
            NEEDOUT
            OUTBYTE(*f++)
            if (f == s->end) {
                f = s->window;
            }
            c->len--;
        }
        c->mode = START;
        break;
    case LIT: /* o: got literal, waiting for output space */
        NEEDOUT
        OUTBYTE(c->sub.lit)
        c->mode = START;
        break;
    case WASH:     /* o: got eob, possibly more output */
        if (k > 7) /* return unused uint8, if any */
        {
            Assert(k < 16, "inflate_codes grabbed too many bytes")
                k -= 8;
            n++;
            p--; /* can always return one */
        }
        FLUSH
        if (s->read != s->write) {
            LEAVE
        }
        c->mode = END;
    case END:
        r = Z_STREAM_END;
        LEAVE
    case BADCODE: /* x: got error */
        r = Z_DATA_ERROR;
        LEAVE
    default:
        r = Z_STREAM_ERROR;
        LEAVE
    }
#ifdef NEED_DUMMY_RETURN
    return Z_STREAM_ERROR; /* Some dumb compilers complain without this */
#endif
}


void inflate_codes_free(inflate_codes_statef* c, z_streamp z)
{
    ZFREE(z, c);
    Tracev(("inflate:       codes free\n"));
}

/* adler32.c -- compute the Adler-32 checksum of a data stream
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

#define BASE 65521L /* largest prime smaller than 65536 */
#define NMAX 5552
/* NMAX is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1 */

#undef DO1
#undef DO2
#undef DO4
#undef DO8

#define DO1(buf, i)   \
    {                 \
        s1 += buf[i]; \
        s2 += s1;     \
    }
#define DO2(buf, i) \
    DO1(buf, i);    \
    DO1(buf, i + 1);
#define DO4(buf, i) \
    DO2(buf, i);    \
    DO2(buf, i + 2);
#define DO8(buf, i) \
    DO4(buf, i);    \
    DO4(buf, i + 4);
#define DO16(buf) \
    DO8(buf, 0);  \
    DO8(buf, 8);

/* ========================================================================= */
static uint32 adler32(uint32 adler, const uint8* buf, uint32 len)
{
    uint32 s1 = adler & 0xffff;
    uint32 s2 = (adler >> 16) & 0xffff;
    int32  k;

    if (buf == Z_NULL) {
        return 1L;
    }

    while (len > 0) {
        k = len < NMAX ? len : NMAX;
        len -= k;
        while (k >= 16) {
            DO16(buf);
            buf += 16;
            k -= 16;
        }
        if (k != 0) {
            do {
                s1 += *buf++;
                s2 += s1;
            } while (--k);
        }
        s1 %= BASE;
        s2 %= BASE;
    }
    return (s2 << 16) | s1;
}


/* infblock.h -- header to use infblock.c
 * Copyright (C) 1995-1998 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

static inflate_blocks_statef* inflate_blocks_new OF((
    z_streamp  z,
    check_func c, /* check function */
    uint32     w
));               /* window size */

static int32 inflate_blocks OF((
    inflate_blocks_statef*,
    z_streamp,
    int32
)); /* initial return code */

static void inflate_blocks_reset OF((
    inflate_blocks_statef*,
    z_streamp,
    uint32*
) ); /* check value on output */

static int32 inflate_blocks_free OF((
    inflate_blocks_statef*,
    z_streamp
));

typedef enum
{
    imMETHOD, /* waiting for method uint8 */
    imFLAG,   /* waiting for flag uint8 */
    imDICT4,  /* four dictionary check bytes to go */
    imDICT3,  /* three dictionary check bytes to go */
    imDICT2,  /* two dictionary check bytes to go */
    imDICT1,  /* one dictionary check uint8 to go */
    imDICT0,  /* waiting for inflateSetDictionary */
    imBLOCKS, /* decompressing blocks */
    imCHECK4, /* four check bytes to go */
    imCHECK3, /* three check bytes to go */
    imCHECK2, /* two check bytes to go */
    imCHECK1, /* one check uint8 to go */
    imDONE,   /* finished check, done */
    imBAD
} /* got an error--stay here */
inflate_mode;

/* inflate private state */
struct internal_state
{
    /* mode */
    inflate_mode mode; /* current inflate mode */

    /* mode dependent information */
    union
    {
        uint32 method; /* if FLAGS, method uint8 */
        struct
        {
            uint32 was;  /* computed check value */
            uint32 need; /* stream check value */
        } check;         /* if CHECK, check values to compare */
        uint32 marker;   /* if BAD, inflateSync's marker bytes count */
    } sub;               /* submode */

    /* mode independent information */
    int32                  nowrap; /* flag for no wrapper */
    uint32                 wbits;  /* log2(window size)  (8..15, defaults to 15) */
    inflate_blocks_statef* blocks; /* current inflate_blocks state */
};


int32 inflateReset(z_streamp z)
{
    if (z == Z_NULL || z->state == Z_NULL) {
        return Z_STREAM_ERROR;
    }
    z->total_in = z->total_out = 0;
    z->msg = Z_NULL;
    z->state->mode = z->state->nowrap ? imBLOCKS : imMETHOD;
    inflate_blocks_reset(z->state->blocks, z, Z_NULL);
    Tracev(("inflate: reset\n"));
    return Z_OK;
}


int32 inflateEnd(z_streamp z)
{
    if (z == Z_NULL || z->state == Z_NULL || z->zfree == Z_NULL) {
        return Z_STREAM_ERROR;
    }
    if (z->state->blocks != Z_NULL) {
        inflate_blocks_free(z->state->blocks, z);
    }
    ZFREE(z, z->state);
    z->state = Z_NULL;
    Tracev(("inflate: end\n"));
    return Z_OK;
}


int32 inflateInit2_(z_streamp z, int32 w, const char* version, int32 stream_size)
{
    if (version == Z_NULL || version[0] != ZLIB_VERSION[0] || stream_size != sizeof(z_stream)) {
        return Z_VERSION_ERROR;
    }

    /* initialize state */
    if (z == Z_NULL) {
        return Z_STREAM_ERROR;
    }
    z->msg = Z_NULL;
    if (z->zalloc == Z_NULL) {
        z->zalloc = (void* (*) (void*, uint32, uint32)) zcalloc;
        z->opaque = (uint8*) 0;
    }
    if (z->zfree == Z_NULL) {
        z->zfree = (void (*)(void*, void*)) zcfree;
    }
    if ((z->state = (struct internal_state*)
             ZALLOC(z, 1, sizeof(struct internal_state)))
        == Z_NULL) {
        return Z_MEM_ERROR;
    }
    z->state->blocks = Z_NULL;

    /* handle undocumented nowrap option (no zlib header or check) */
    z->state->nowrap = 0;
    if (w < 0) {
        w = -w;
        z->state->nowrap = 1;
    }

    /* set window size */
    if (w < 8 || w > 15) {
        inflateEnd(z);
        return Z_STREAM_ERROR;
    }
    z->state->wbits = (uint32) w;

    /* create inflate_blocks state */
    if ((z->state->blocks =
             inflate_blocks_new(z, z->state->nowrap ? Z_NULL : adler32, (uint32) 1 << w))
        == Z_NULL) {
        inflateEnd(z);
        return Z_MEM_ERROR;
    }
    Tracev(("inflate: allocated\n"));

    /* reset state */
    inflateReset(z);
    return Z_OK;
}

#define iNEEDBYTE             \
    {                         \
        if (z->avail_in == 0) \
            return r;         \
        r = f;                \
    }
#define iNEXTBYTE (z->avail_in--, z->total_in++, *z->next_in++)

int32 inflate(z_streamp z, int32 f)
{
    int32  r;
    uint32 b;

    if (z == Z_NULL || z->state == Z_NULL || z->next_in == Z_NULL) {
        return Z_STREAM_ERROR;
    }
    f = f == Z_FINISH ? Z_BUF_ERROR : Z_OK;
    r = Z_BUF_ERROR;
    while (1) {
        switch (z->state->mode) {
        case imMETHOD:
            iNEEDBYTE if (((z->state->sub.method = iNEXTBYTE) & 0xf) != Z_DEFLATED)
            {
                z->state->mode = imBAD;
                z->msg = (char*) "unknown compression method";
                z->state->sub.marker = 5; /* can't try inflateSync */
                break;
            }
            if ((z->state->sub.method >> 4) + 8 > z->state->wbits) {
                z->state->mode = imBAD;
                z->msg = (char*) "invalid window size";
                z->state->sub.marker = 5; /* can't try inflateSync */
                break;
            }
            z->state->mode = imFLAG;
        case imFLAG:
            iNEEDBYTE
                b = iNEXTBYTE;
            if (((z->state->sub.method << 8) + b) % 31) {
                z->state->mode = imBAD;
                z->msg = (char*) "incorrect header check";
                z->state->sub.marker = 5; /* can't try inflateSync */
                break;
            }
            Tracev(("inflate: zlib header ok\n"));
            if (!(b & PRESET_DICT)) {
                z->state->mode = imBLOCKS;
                break;
            }
            z->state->mode = imDICT4;
        case imDICT4:
            iNEEDBYTE
                z->state->sub.check.need = (uint32) iNEXTBYTE << 24;
            z->state->mode = imDICT3;
        case imDICT3:
            iNEEDBYTE
                z->state->sub.check.need += (uint32) iNEXTBYTE << 16;
            z->state->mode = imDICT2;
        case imDICT2:
            iNEEDBYTE
                z->state->sub.check.need += (uint32) iNEXTBYTE << 8;
            z->state->mode = imDICT1;
        case imDICT1:
            iNEEDBYTE
                z->state->sub.check.need += (uint32) iNEXTBYTE;
            z->adler = z->state->sub.check.need;
            z->state->mode = imDICT0;
            return Z_NEED_DICT;
        case imDICT0:
            z->state->mode = imBAD;
            z->msg = (char*) "need dictionary";
            z->state->sub.marker = 0; /* can try inflateSync */
            return Z_STREAM_ERROR;
        case imBLOCKS:
            r = inflate_blocks(z->state->blocks, z, r);
            if (r == Z_DATA_ERROR) {
                z->state->mode = imBAD;
                z->state->sub.marker = 0; /* can try inflateSync */
                break;
            }
            if (r == Z_OK) {
                r = f;
            }
            if (r != Z_STREAM_END) {
                return r;
            }
            r = f;
            inflate_blocks_reset(z->state->blocks, z, &z->state->sub.check.was);
            if (z->state->nowrap) {
                z->state->mode = imDONE;
                break;
            }
            z->state->mode = imCHECK4;
        case imCHECK4:
            iNEEDBYTE
                z->state->sub.check.need = (uint32) iNEXTBYTE << 24;
            z->state->mode = imCHECK3;
        case imCHECK3:
            iNEEDBYTE
                z->state->sub.check.need += (uint32) iNEXTBYTE << 16;
            z->state->mode = imCHECK2;
        case imCHECK2:
            iNEEDBYTE
                z->state->sub.check.need += (uint32) iNEXTBYTE << 8;
            z->state->mode = imCHECK1;
        case imCHECK1:
            iNEEDBYTE
                z->state->sub.check.need += (uint32) iNEXTBYTE;

            if (z->state->sub.check.was != z->state->sub.check.need) {
                z->state->mode = imBAD;
                z->msg = (char*) "incorrect data check";
                z->state->sub.marker = 5; /* can't try inflateSync */
                break;
            }
            Tracev(("inflate: zlib check ok\n"));
            z->state->mode = imDONE;
        case imDONE:
            return Z_STREAM_END;
        case imBAD:
            return Z_DATA_ERROR;
        default:
            return Z_STREAM_ERROR;
        }
    }
#ifdef NEED_DUMMY_RETURN
    return Z_STREAM_ERROR; /* Some dumb compilers complain without this */
#endif
}

uint8* zcalloc(uint8* opaque, uint32 items, uint32 size)
{
    if (opaque) {
        items += size - size; /* make compiler happy */
    }
    return (uint8*) Z_Malloc(items * size);
}

void zcfree(uint8* opaque, uint8* ptr)
{
    Z_Free(ptr);
    if (opaque) {
        return; /* make compiler happy */
    }
}


