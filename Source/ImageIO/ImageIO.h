//=============================================================================
//  ImageIO.h
//
//  v1.0.3 - 2016.01.14 by Abe Pralle
//
//  See README.md for instructions.
//=============================================================================
#ifndef IMAGE_IO_H
#define IMAGE_IO_H

#ifndef ORG_NAMESPACE
  #define ORG_NAMESPACE Org
#endif

#include <stdio.h>
#include <setjmp.h>
#include "jpeglib.h"
#include "png.h"

namespace ORG_NAMESPACE
{

namespace ImageIO
{

#if defined(_WIN32)
  #include <windows.h>
  typedef __uint32         ARGB32;
  typedef unsigned char    Byte;
#else
  #include <stdint.h>
  typedef uint32_t         ARGB32;
  typedef uint8_t          Byte;
#endif

//-----------------------------------------------------------------------------
//  Decoder
//-----------------------------------------------------------------------------
struct Decoder
{
  // These two lines MUST come first (implicit in JPEG callbacks)
  struct jpeg_error_mgr jpeg_error_manager;
  jmp_buf               on_error;

  // EXTERNAL
  int          width;
  int          height;
  // END EXTERNAL

  int          format;

  Byte* data;
  int          count;

  Byte* reader;
  int          remaining;

  Byte* buffer;

  struct jpeg_decompress_struct jpeg_info;

  png_structp  png_ptr;
  png_infop    png_info_ptr;

  static const int INVALID = 0;
  static const int PNG     = 1;
  static const int JPEG    = 2;

  // METHODS
  Decoder();
  ~Decoder();
  bool open( Byte* encoded_bytes, int encoded_byte_count );
  bool open_jpeg( Byte* encoded_bytes, int encoded_byte_count );
  bool open_png( Byte* encoded_bytes, int encoded_byte_count );
  bool decode_argb32( ARGB32* bitmap );
  bool decode_jpeg_argb32( ARGB32* bitmap );
  bool decode_png_argb32( ARGB32* bitmap );
};


//-----------------------------------------------------------------------------
//  Encoder
//-----------------------------------------------------------------------------
struct Encoder
{
  // These two lines MUST come first (implicit in JPEG callbacks)
  struct jpeg_error_mgr jpeg_error_manager;
  jmp_buf               on_error;

  // EXTERNAL
  int          quality;   // JPEG quality 0..100.  Default 75
  Byte* encoded_bytes;
  int          encoded_byte_count;
  // END EXTERNAL

  Byte* writer;
  int          capacity;

  struct jpeg_compress_struct jpeg_info;

  png_structp  png_ptr;
  png_infop    png_info_ptr;


  // METHODS
  Encoder();
  ~Encoder();
  bool encode_argb32_jpeg( ARGB32* bitmap, int width, int height );
  bool encode_argb32_png( ARGB32* bitmap, int width, int height );

  // INTERNAL USE
  void reserve( int additional_count );
  void write( Byte* bytes, int count );
};


//-----------------------------------------------------------------------------
//  ImageIO Utility
//-----------------------------------------------------------------------------
bool bitmap_has_translucent_pixels( ARGB32* bitmap, int count );

void demultiply_alpha( ARGB32* bitmap, int count );
void premultiply_alpha( ARGB32* bitmap, int count );
void swap_red_and_blue( ARGB32* bitmap, int count );

void ImageIO_jpeg_error_callback( j_common_ptr jpeg_info );
void ImageIO_png_error_callback( png_structp png_ptr, png_const_charp msg );
void ImageIO_png_read_callback( png_structp png_ptr, png_bytep data, png_size_t count );
void ImageIO_png_write_callback( png_structp png_ptr, png_bytep data, png_size_t count );
void ImageIO_png_flush_callback( png_structp png_ptr );

}; // namespace ImageIO

};  // namespace ORG_NAMESPACE;

using namespace ORG_NAMESPACE;

#endif // IMAGE_IO_H
