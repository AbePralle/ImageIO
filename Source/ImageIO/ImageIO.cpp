//=============================================================================
//  ImageIO.cpp
//
//  v1.0.5 - 2016.01.15 by Abe Pralle
//
//  See README.md for instructions.
//=============================================================================

#include <stdlib.h>
#include <string.h>
#include "ImageIO.h"

namespace PROJECT_NAMESPACE
{

namespace ImageIO
{

//-----------------------------------------------------------------------------
//  Decoder
//-----------------------------------------------------------------------------
Decoder::Decoder()
{
  memset( this, 0, sizeof(Decoder) );
}

Decoder::~Decoder()
{
}

bool Decoder::open( Byte* encoded_bytes, int encoded_byte_count )
{
  format = INVALID;
  if (encoded_byte_count < 4) return 0;  // definitely not an image

  if (encoded_bytes[0] == 0xff)      return open_jpeg( encoded_bytes, encoded_byte_count );
  else if (encoded_bytes[0] == 0x89) return open_png( encoded_bytes, encoded_byte_count );
  else                              return 0;
}


bool Decoder::open_jpeg( Byte* encoded_bytes, int encoded_byte_count )
{
  format = JPEG;
  data = encoded_bytes;
  count = encoded_byte_count;
  jpeg_info.err = jpeg_std_error( (struct jpeg_error_mgr*) this );
  jpeg_error_manager.error_exit = ImageIO_jpeg_error_callback;
  
  if (setjmp(on_error))
  {
    // Caught an error
    jpeg_destroy_decompress( &jpeg_info );
  
    if (buffer) delete buffer;

    format = INVALID;
  
    return false;
  }
  
  jpeg_create_decompress( &jpeg_info );
  jpeg_mem_src( &jpeg_info, data, count );
  jpeg_read_header( &jpeg_info, TRUE );
  
  jpeg_start_decompress( &jpeg_info );
  
  width = jpeg_info.output_width;
  height = jpeg_info.output_height;

  return true;
}


bool Decoder::open_png( Byte* encoded_bytes, int encoded_byte_count )
{
  format = PNG;

  data = encoded_bytes;
  reader = encoded_bytes;
  remaining = encoded_byte_count;

  png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, this,
      ImageIO_png_error_callback, NULL );
  if ( !png_ptr ) return false; // Out of memory

  png_set_add_alpha( png_ptr, 255, PNG_FILLER_AFTER );

  png_info_ptr = png_create_info_struct( png_ptr );
  if ( !png_info_ptr )
  {
    png_destroy_read_struct( &png_ptr, NULL, NULL );
    return false; // Out of memory
  }

  if (setjmp(on_error))
  {
    png_destroy_read_struct( &png_ptr, &png_info_ptr, NULL );
    return false;
  }

  png_set_read_fn( png_ptr, this, ImageIO_png_read_callback );

  // Prepare the reader to ignore all recognized chunks whose data won't be
  // used, i.e., all chunks recognized by libpng except for IHDR, PLTE, IDAT,
  // IEND, tRNS, bKGD, gAMA, and sRGB (small performance improvement).
  {
    static png_byte chunks_to_ignore[] = 
    {
       99,  72,  82,  77, '\0',  // cHRM
      104,  73,  83,  84, '\0',  // hIST
      105,  67,  67,  80, '\0',  // iCCP
      105,  84,  88, 116, '\0',  // iTXt
      111,  70,  70, 115, '\0',  // oFFs
      112,  67,  65,  76, '\0',  // pCAL
      112,  72,  89, 115, '\0',  // pHYs
      115,  66,  73,  84, '\0',  // sBIT
      115,  67,  65,  76, '\0',  // sCAL
      115,  80,  76,  84, '\0',  // sPLT
      115,  84,  69,  82, '\0',  // sTER
      116,  69,  88, 116, '\0',  // tEXt
      116,  73,  77,  69, '\0',  // tIME
      122,  84,  88, 116, '\0'   // zTXt
    };

    png_set_keep_unknown_chunks( png_ptr, PNG_HANDLE_CHUNK_NEVER,
        chunks_to_ignore, sizeof(chunks_to_ignore)/5 );
  }

  png_read_png(
      png_ptr,
      png_info_ptr,
      PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_BGR,
      NULL
  );

  width  = png_get_image_width( png_ptr, png_info_ptr );
  height = png_get_image_height( png_ptr, png_info_ptr );

  return true;  // success
}


bool Decoder::decode_argb32( ARGB32* bitmap )
{
  switch (format)
  {
    case JPEG: return decode_jpeg_argb32( bitmap );
    case PNG:  return decode_png_argb32( bitmap );
  }
  return 0;
}


bool Decoder::decode_jpeg_argb32( ARGB32* bitmap )
{
  int width  = this->width;
  int height = this->height;
  
  ARGB32* pixels = bitmap - 1;

  if (format != JPEG) return 0;  // Input was not set up as JPEG.

  buffer = new Byte[ width*3 ];
  
  int j = jpeg_info.output_scanline;
  while (j < height)
  {
    Byte* dest = buffer;
    jpeg_read_scanlines( &jpeg_info, &dest, 1 );
  
    // Convert JPEG RGB or grayscale buffer to ImageIO Bitmap ARGB
    if (jpeg_info.jpeg_color_space == JCS_GRAYSCALE)
    {
      Byte* cursor = buffer - 1;
      int i;
      for (i=0; i<width; ++i)
      {
        int k = *(++cursor);
        *(++pixels) = (255<<24) | (k<<16) | (k<<8) | k;
      }
    }
    else
    {
      // RGB Color
      Byte* cursor = buffer;
      int i;
      for (i=0; i<width; ++i)
      {
        int r = cursor[0];
        int g = cursor[1];
        int b = cursor[2];
        *(++pixels) = (255<<24) | (r<<16) | (g<<8) | b;
        cursor += 3;
      }
    }
  
    j = jpeg_info.output_scanline;
  }
  
  delete buffer;
  buffer = 0;
  
  jpeg_finish_decompress( &jpeg_info );
  jpeg_destroy_decompress( &jpeg_info );

  return 1;
}


bool Decoder::decode_png_argb32( ARGB32* bitmap )
{
  int height = this->height;
  png_bytepp row_pointers = png_get_rows( png_ptr, png_info_ptr );
  int row_size = (int) png_get_rowbytes( png_ptr, png_info_ptr );
  ARGB32* pixels = bitmap;

  for (int j=0; j<height; ++j)
  {
    memcpy( pixels, row_pointers[j], row_size );
    pixels += width;
  }

  png_destroy_read_struct( &png_ptr, &png_info_ptr, NULL );
  return 1;
}


//-----------------------------------------------------------------------------
//  Encoder
//-----------------------------------------------------------------------------
Encoder::Encoder()
{
  memset( this, 0, sizeof(Encoder) );
  quality = 75;
}

Encoder::~Encoder()
{
  if (encoded_bytes)
  {
    delete encoded_bytes;
    encoded_bytes = 0;
  }
}

bool Encoder::encode_argb32_jpeg( ARGB32* bitmap, int width, int height )
{
  unsigned long encoded_size;
  JSAMPROW row_pointer;
  Byte* buffer;

  jpeg_info.err = jpeg_std_error( (struct jpeg_error_mgr*) this );
  jpeg_error_manager.error_exit = ImageIO_jpeg_error_callback;

  if (setjmp(on_error))
  {
    // Caught an error
    jpeg_destroy_compress( &jpeg_info );
    return 0;
  }

  jpeg_create_compress(&jpeg_info);

  jpeg_mem_dest( &jpeg_info, &encoded_bytes, &encoded_size );

  jpeg_info.image_width      = width;
  jpeg_info.image_height     = height;
  jpeg_info.input_components = 3;
  jpeg_info.in_color_space   = JCS_RGB;

  jpeg_set_defaults( &jpeg_info);
  jpeg_set_quality( &jpeg_info, quality, TRUE );  // 0..100
  jpeg_start_compress( &jpeg_info, TRUE );

  buffer = new Byte[ width*3 ];
  row_pointer = buffer;

  while (jpeg_info.next_scanline < height)
  {
    int j = jpeg_info.next_scanline;
    int n = width;
    ARGB32* pixels = bitmap + j*width - 1;
    Byte*    dest = buffer;
    while (--n >= 0)
    {
      int color = *(++pixels);
      dest[0] = (Byte) (color >> 16);
      dest[1] = (Byte) (color >> 8);
      dest[2] = (Byte) color;
      dest += 3;
    }

    jpeg_write_scanlines( &jpeg_info, &row_pointer, 1 );
  }

  jpeg_finish_compress( &jpeg_info );

  encoded_byte_count = (int) encoded_size;
  return 1;
}

bool Encoder::encode_argb32_png( ARGB32* bitmap, int width, int height )
{
  int j;
  int color_type;
  int must_delete_bytes;
  int bytes_per_pixel;
  png_bytepp row_pointers;
  png_bytep  bytes;

  png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, this,
      ImageIO_png_error_callback, NULL );
  if ( !png_ptr ) return 0; // Out of memory

  png_info_ptr = png_create_info_struct( png_ptr );
  if ( !png_info_ptr )
  {
    png_destroy_write_struct( &png_ptr, NULL );
    return 0;  // Out of memory
  }

  if (setjmp(on_error))
  {
    png_destroy_write_struct( &png_ptr, &png_info_ptr );
    return 0;
  }

  png_set_write_fn( png_ptr, this, ImageIO_png_write_callback, ImageIO_png_flush_callback );

  if (bitmap_has_translucent_pixels(bitmap,width*height))
  {
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    must_delete_bytes = 0;
    bytes = (png_bytep) bitmap;
    bytes_per_pixel = 4;
  }
  else
  {
    int n = width*height;
    ARGB32* src = bitmap - 1;
    png_bytep dest;

    color_type = PNG_COLOR_TYPE_RGB;
    must_delete_bytes = 1;
    bytes_per_pixel = 3;
    bytes = new png_byte[ width * height * 3 ];
    dest = bytes;
    while (--n >= 0)
    {
      int argb = *(++src);
      dest[0] = (png_byte) argb;
      dest[1] = (png_byte) (argb >> 8);
      dest[2] = (png_byte) (argb >> 16);
      dest += 3;
    }
  }

  png_set_IHDR( png_ptr, png_info_ptr, width, height, 8,
       color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );

  row_pointers = new png_bytep[ height ];
  for (j=0; j<height; ++j)
  {
    row_pointers[j] = bytes + j*width*bytes_per_pixel;
  }

  png_set_rows( png_ptr, png_info_ptr, row_pointers );

  png_write_png( png_ptr, png_info_ptr, PNG_TRANSFORM_BGR, 0 );

  delete row_pointers;
  if (must_delete_bytes) delete bytes;

  return 1;
}

void Encoder::reserve( int additional_count )
{
  if ( !encoded_bytes )
  {
    if (additional_count < 16*1024) additional_count = 16*1024;
    encoded_bytes = new Byte[ additional_count ];
    capacity = additional_count;
  }
  else
  {
    int required = encoded_byte_count + additional_count;
    if (required > capacity)
    {
      int double_size = capacity * 2;
      if (double_size > required) required = double_size;

      {
        Byte* expanded_data = new Byte[ required ];
        memcpy( expanded_data, encoded_bytes, encoded_byte_count );
        delete encoded_bytes;
        encoded_bytes = expanded_data;
        capacity = required;
      }
    }
  }
}

void Encoder::write( Byte* bytes, int count )
{
  reserve( count );
  memcpy( encoded_bytes + encoded_byte_count, bytes, count );
  encoded_byte_count += count;
}

//-----------------------------------------------------------------------------
//  ImageIO Utility
//-----------------------------------------------------------------------------
bool bitmap_has_translucent_pixels( ARGB32* bitmap, int count )
{
  ARGB32* cur = bitmap - 1;
  int c = count;
  while (--c >= 0)
  {
    if ((*(++cur) & 0xff000000) != 0xff000000) return 1;
  }
  return 0;
}

void demultiply_alpha( ARGB32* bitmap, int count )
{
  ARGB32* cur = bitmap - 1;
  int c = count;
  while (--c >= 0)
  {
    int color = *(++cur);
    int a = (color >> 24) & 255;
    if (a && a != 255)
    {
      int r = (((color >> 16) & 255) * 255) / a;
      int g = (((color >> 8) & 255) * 255) / a;
      int b = ((color & 255) * 255) / a;
      *cur = (a << 24) | (r << 16) | (g << 8) | b;
    }
    // else fully opaque or transparent pixel is unchanged
  }
}

void premultiply_alpha( ARGB32* bitmap, int count )
{
  ARGB32* cur = bitmap - 1;
  int c = count;
  while (--c >= 0)
  {
    int color = *(++cur);
    int a = (color >> 24) & 255;
    if (a)
    {
      if (a != 255)
      {
        int r = (((color >> 16) & 255) * a) / 255;
        int g = (((color >> 8) & 255) * a) / 255;
        int b = ((color & 255) * a) / 255;
        *cur = (a << 24) | (r << 16) | (g << 8) | b;
      }
      // else opaque pixel is unchanged
    }
    else
    {
      *cur = 0;
    }
  }
}

void swap_red_and_blue( ARGB32* bitmap, int count )
{
  ARGB32* cur = bitmap - 1;
  int c = count;
  while (--c >= 0)
  {
    int color = *(++cur);
    *cur = (color & 0xff00ff00) | ((color>>16)&0xff) | ((color<<16)&0xff0000);
  }
}

// Callbacks
void ImageIO_jpeg_error_callback( j_common_ptr jpeg_info )
{
  (*jpeg_info->err->output_message)( jpeg_info );

  longjmp( ((Decoder*)jpeg_info->err)->on_error, 1 );
}

void ImageIO_png_error_callback( png_structp png_ptr, png_const_charp msg )
{
  fprintf(stderr, "libpng error: %s\n", msg);
  fflush(stderr);

  longjmp( ((Decoder*)png_get_io_ptr(png_ptr))->on_error, 1 );
}

void ImageIO_png_read_callback( png_structp png_ptr, png_bytep bytes, png_size_t count )
{
  Decoder* decoder = (Decoder*) png_get_io_ptr( png_ptr );
  if (count > decoder->remaining)
  {
    printf(" out of bytes\n");
    count = (png_size_t) decoder->remaining;
  }

  if (count)
  {
    memcpy( bytes, decoder->reader, count );
    decoder->reader    += count;
    decoder->remaining -= count;
  }
}

void ImageIO_png_write_callback( png_structp png_ptr, png_bytep bytes, png_size_t count )
{
  Encoder* encoder = (Encoder*) png_get_io_ptr( png_ptr );
  encoder->write( (Byte*) bytes, (int) count );
}

void ImageIO_png_flush_callback( png_structp png_ptr )
{
  // No action
}

};  // namespace ImageIO

};  // namespace PROJECT_NAMESPACE

