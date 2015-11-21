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
  int          format;
  int          width;
  int          height;

  int          count;
  int          remaining;
  ImageIOByte* data;

  ImageIOByte* buffer;

  struct jpeg_decompress_struct jpeg_info;

  struct ErrorInfo
  {
    struct jpeg_error_mgr jpeg_error_manager;
    jmp_buf               on_error;
  } error_info;
} ImageIODecoder;

ImageIOLogical ImageIODecoder_init( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size );
ImageIOLogical ImageIODecoder_init_jpeg( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size );
ImageIOLogical ImageIODecoder_init_png( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size );
ImageIOLogical ImageIODecoder_decode_argb32( ImageIODecoder* decoder, ImageIOInteger* pixels, int pixel_count );
ImageIOLogical ImageIODecoder_decode_jpeg_argb32( ImageIODecoder* decoder, ImageIOInteger* pixels, int pixel_count );
ImageIOLogical ImageIODecoder_decode_png_argb32( ImageIODecoder* decoder, ImageIOInteger* pixels, int pixel_count );

void ImageIO_demultiply_alpha( ImageIOInteger* data, int count );
void ImageIO_premultiply_alpha( ImageIOInteger* data, int count );
void ImageIO_swap_red_and_blue( ImageIOInteger* data, int count );

void ImageIO_jpeg_error_callback( j_common_ptr jpeg_info );

#endif // IMAGE_IO
