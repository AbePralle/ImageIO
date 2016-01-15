//=============================================================================
//  ImageIO.h
//
//  v1.0.2 - 2015.12.29 by Abe Pralle
//
//  See README.md for instructions.
//=============================================================================
#pragma once

#ifndef IMAGE_IO_NAMESPACE
  #define IMAGE_IO_NAMESPACE ImageIO
#endif

#include <stdio.h>
#include <setjmp.h>
#include "jpeglib.h"
#include "png.h"

namespace IMAGE_IO_NAMESPACE
{

#if defined(_WIN32)
  #include <windows.h>
  typedef __uint32         ImageIOARGB32;
  typedef unsigned char    ImageIOByte;
#else
  #include <stdint.h>
  typedef uint32_t         ImageIOARGB32;
  typedef uint8_t          ImageIOByte;
#endif

#define IMAGE_IO_INVALID 0
#define IMAGE_IO_PNG     1
#define IMAGE_IO_JPEG    2

//-----------------------------------------------------------------------------
//  ImageIODecoder
//-----------------------------------------------------------------------------
struct ImageIODecoder
{
  // These two lines MUST come first (implicit in JPEG callbacks)
  struct jpeg_error_mgr jpeg_error_manager;
  jmp_buf               on_error;

  // EXTERNAL
  int          width;
  int          height;
  // END EXTERNAL

  int          format;

  ImageIOByte* data;
  int          count;

  ImageIOByte* reader;
  int          remaining;

  ImageIOByte* buffer;

  struct jpeg_decompress_struct jpeg_info;

  png_structp  png_ptr;
  png_infop    png_info_ptr;

  // METHODS
  ImageIODecoder();
  ~ImageIODecoder();
  bool open( ImageIOByte* encoded_bytes, int encoded_byte_count );
  bool open_jpeg( ImageIOByte* encoded_bytes, int encoded_byte_count );
  bool open_png( ImageIOByte* encoded_bytes, int encoded_byte_count );
  bool decode_argb32( ImageIOARGB32* bitmap );
  bool decode_jpeg_argb32( ImageIOARGB32* bitmap );
  bool decode_png_argb32( ImageIOARGB32* bitmap );
};


//-----------------------------------------------------------------------------
//  ImageIOEncoder
//-----------------------------------------------------------------------------
typedef struct ImageIOEncoder
{
  // These two lines MUST come first (implicit in JPEG callbacks)
  struct jpeg_error_mgr jpeg_error_manager;
  jmp_buf               on_error;

  // EXTERNAL
  int          quality;   // JPEG quality 0..100.  Default 75
  ImageIOByte* encoded_bytes;
  int          encoded_byte_count;
  // END EXTERNAL

  ImageIOByte* writer;
  int          capacity;

  struct jpeg_compress_struct jpeg_info;

  png_structp  png_ptr;
  png_infop    png_info_ptr;
} ImageIOEncoder;

ImageIOEncoder* ImageIOEncoder_init( ImageIOEncoder* encoder );
ImageIOEncoder* ImageIOEncoder_retire( ImageIOEncoder* encoder );
bool  ImageIOEncoder_encode_argb32_jpeg( ImageIOEncoder* encoder, ImageIOARGB32* bitmap, int width, int height );
bool  ImageIOEncoder_encode_argb32_png( ImageIOEncoder* encoder, ImageIOARGB32* bitmap, int width, int height );

// INTERNAL USE
void            ImageIOEncoder_reserve( ImageIOEncoder* encoder, int additional_count );
void            ImageIOEncoder_write( ImageIOEncoder* encoder, ImageIOByte* bytes, int count );

//-----------------------------------------------------------------------------
//  ImageIO Utility
//-----------------------------------------------------------------------------
bool ImageIO_bitmap_has_translucent_pixels( ImageIOARGB32* bitmap, int count );

void ImageIO_demultiply_alpha( ImageIOARGB32* bitmap, int count );
void ImageIO_premultiply_alpha( ImageIOARGB32* bitmap, int count );
void ImageIO_swap_red_and_blue( ImageIOARGB32* bitmap, int count );

void ImageIO_jpeg_error_callback( j_common_ptr jpeg_info );
void ImageIO_png_error_callback( png_structp png_ptr, png_const_charp msg );
void ImageIO_png_read_callback( png_structp png_ptr, png_bytep data, png_size_t count );
void ImageIO_png_write_callback( png_structp png_ptr, png_bytep data, png_size_t count );
void ImageIO_png_flush_callback( png_structp png_ptr );

};  // namespace IMAGE_IO_NAMESPACE
