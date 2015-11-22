#ifndef IMAGE_IO
#define IMAGE_IO
//=============================================================================
//  ImageIO.h
//
//  2015.11.20 by Abe Pralle
//=============================================================================

#if defined(_WIN32)
  #include <windows.h>
  typedef __int32          ImageIOInteger;
  typedef unsigned char    ImageIOByte;
  typedef int              ImageIOLogical;
#else
  #include <stdint.h>
  typedef int32_t          ImageIOInteger;
  typedef uint8_t          ImageIOByte;
  typedef int              ImageIOLogical;
#endif

#define IMAGE_IO_INVALID 0
#define IMAGE_IO_PNG     1
#define IMAGE_IO_JPEG    2

#include <stdio.h>
#include <setjmp.h>
#include "jpeglib.h"
#include "png.h"

#ifdef __cplusplus
extern "C"
{
#endif

//-----------------------------------------------------------------------------
//  ImageIODecoder
//-----------------------------------------------------------------------------
typedef struct ImageIODecoder
{
  // These two lines MUST come first and second (implicit in JPEG callbacks)
  struct jpeg_error_mgr jpeg_error_manager;
  jmp_buf               on_error;

  int          format;
  int          width;
  int          height;

  ImageIOByte* data;
  int          count;

  ImageIOByte* reader;
  int          remaining;

  ImageIOByte* buffer;

  struct jpeg_decompress_struct jpeg_info;

  png_structp  png_ptr;
  png_infop    png_info_ptr;
} ImageIODecoder;

ImageIODecoder* ImageIODecoder_init( ImageIODecoder* decoder );
ImageIODecoder* ImageIODecoder_retire( ImageIODecoder* decoder );
ImageIOLogical  ImageIODecoder_set_input( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size );
ImageIOLogical  ImageIODecoder_set_input_jpeg( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size );
ImageIOLogical  ImageIODecoder_set_input_png( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size );
ImageIOLogical  ImageIODecoder_decode( ImageIODecoder* decoder, ImageIOInteger* bitmap );
ImageIOLogical  ImageIODecoder_decode_jpeg( ImageIODecoder* decoder, ImageIOInteger* bitmap );
ImageIOLogical  ImageIODecoder_decode_png( ImageIODecoder* decoder, ImageIOInteger* bitmap );


//-----------------------------------------------------------------------------
//  ImageIOEncoder
//-----------------------------------------------------------------------------
typedef struct ImageIOEncoder
{
  // These two lines MUST come first and second (implicit in JPEG callbacks)
  struct jpeg_error_mgr jpeg_error_manager;
  jmp_buf               on_error;

  // BEGIN PUBLIC
  int          quality;   // JPEG quality 0..100.  Default 75
  ImageIOByte* encoded_data;
  int          encoded_data_size;
  // END PUBLIC

  ImageIOByte* writer;
  int          capacity;

  //ImageIOByte* buffer;

  struct jpeg_compress_struct jpeg_info;

  png_structp  png_ptr;
  png_infop    png_info_ptr;
} ImageIOEncoder;

ImageIOEncoder* ImageIOEncoder_init( ImageIOEncoder* encoder );
ImageIOEncoder* ImageIOEncoder_retire( ImageIOEncoder* encoder );
ImageIOLogical  ImageIOEncoder_encode_jpeg( ImageIOEncoder* encoder, ImageIOInteger* bitmap, int width, int height );
ImageIOLogical  ImageIOEncoder_encode_png( ImageIOEncoder* encoder, ImageIOInteger* bitmap, int width, int height );
void            ImageIOEncoder_reserve( ImageIOEncoder* encoder, int additional_count );
void            ImageIOEncoder_write( ImageIOEncoder* encoder, ImageIOByte* bytes, int count );

//-----------------------------------------------------------------------------
//  ImageIO Utility
//-----------------------------------------------------------------------------
ImageIOLogical ImageIO_bitmap_has_translucent_pixels( ImageIOInteger* bitmap, int count );

void ImageIO_demultiply_alpha( ImageIOInteger* bitmap, int count );
void ImageIO_premultiply_alpha( ImageIOInteger* bitmap, int count );
void ImageIO_swap_red_and_blue( ImageIOInteger* bitmap, int count );

void ImageIO_jpeg_error_callback( j_common_ptr jpeg_info );
void ImageIO_png_error_callback( png_structp png_ptr, png_const_charp msg );
void ImageIO_png_read_callback( png_structp png_ptr, png_bytep data, png_size_t count );
void ImageIO_png_write_callback( png_structp png_ptr, png_bytep data, png_size_t count );
void ImageIO_png_flush_callback( png_structp png_ptr );

#ifdef __cplusplus
} // end extern "C"
#endif

#endif // IMAGE_IO
