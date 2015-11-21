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

typedef struct ImageIODecoder
{
  struct jpeg_error_mgr jpeg_error_manager;  // this line MUST come first (implicit in JPEG callback)
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

ImageIOLogical ImageIODecoder_init( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size );
ImageIOLogical ImageIODecoder_init_jpeg( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size );
ImageIOLogical ImageIODecoder_init_png( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size );
ImageIOLogical ImageIODecoder_decode_argb32( ImageIODecoder* decoder, ImageIOInteger* bitmap, int pixel_count );
ImageIOLogical ImageIODecoder_decode_jpeg_argb32( ImageIODecoder* decoder, ImageIOInteger* bitmap, int pixel_count );
ImageIOLogical ImageIODecoder_decode_png_argb32( ImageIODecoder* decoder, ImageIOInteger* bitmap, int pixel_count );

void ImageIO_demultiply_alpha( ImageIOInteger* data, int count );
void ImageIO_premultiply_alpha( ImageIOInteger* data, int count );
void ImageIO_swap_red_and_blue( ImageIOInteger* data, int count );

void ImageIO_jpeg_error_callback( j_common_ptr jpeg_info );
void ImageIO_png_error_callback( png_structp png_ptr, png_const_charp msg );
void ImageIO_png_read_callback( png_structp png_ptr, png_bytep data, png_size_t count );

#endif // IMAGE_IO
