//=============================================================================
//  ImageIO.c
//
//  2015.11.20 by Abe Pralle
//=============================================================================

#include <stdlib.h>
#include <string.h>
#include "ImageIO.h"

ImageIOLogical ImageIODecoder_init( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size )
{
  decoder->format = IMAGE_IO_INVALID;
  if (encoded_data_size < 4) return 0;  // definitely not an image

  if (encoded_data[0] == 0xff)      ImageIODecoder_init_jpeg( decoder, encoded_data, encoded_data_size );
  else if (encoded_data[0] == 0x89) ImageIODecoder_init_png( decoder, encoded_data, encoded_data_size );
  else                              return 0;

  return 1;
}


ImageIOLogical ImageIODecoder_init_jpeg( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size )
{
  decoder->format = IMAGE_IO_JPEG;
  decoder->data = encoded_data;
  decoder->count = encoded_data_size;
  decoder->buffer = 0;

  decoder->jpeg_info.err = jpeg_std_error( (struct jpeg_error_mgr*) decoder );
  decoder->jpeg_error_manager.error_exit = ImageIO_jpeg_error_callback;
  
  if (setjmp(decoder->on_error))
  {
    // Caught an error
    jpeg_destroy_decompress( &decoder->jpeg_info );
  
    if (decoder->buffer) free( decoder->buffer );

    decoder->format = IMAGE_IO_INVALID;
  
    return 0;
  }
  
  jpeg_create_decompress( &decoder->jpeg_info );
  jpeg_mem_src( &decoder->jpeg_info, decoder->data, decoder->count );
  jpeg_read_header( &decoder->jpeg_info, TRUE );
  
  jpeg_start_decompress( &decoder->jpeg_info );
  
  decoder->width = decoder->jpeg_info.output_width;
  decoder->height = decoder->jpeg_info.output_height;

  return 1;
}


ImageIOLogical ImageIODecoder_init_png( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size )
{
  decoder->format = IMAGE_IO_PNG;

  decoder->data = encoded_data;
  decoder->remaining = encoded_data_size;

  decoder->png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, decoder,
      ImageIO_png_error_callback, NULL );
  if ( !decoder->png_ptr ) return 0; // Out of memory

  png_set_add_alpha( decoder->png_ptr, 255, PNG_FILLER_AFTER );

  decoder->png_info_ptr = png_create_info_struct(decoder->png_ptr);
  if ( !decoder->png_info_ptr )
  {
    png_destroy_read_struct( &decoder->png_ptr, NULL, NULL );
    return 0;  // Out of memory
  }

  if (setjmp(decoder->on_error))
  {
    png_destroy_read_struct( &decoder->png_ptr, &decoder->png_info_ptr, NULL );
    return 0;
  }

  png_set_read_fn( decoder->png_ptr, decoder, ImageIO_png_read_callback );

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

    png_set_keep_unknown_chunks( decoder->png_ptr, PNG_HANDLE_CHUNK_NEVER,
        chunks_to_ignore, sizeof(chunks_to_ignore)/5 );
  }

  png_read_png(
      decoder->png_ptr,
      decoder->png_info_ptr,
      PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_GRAY_TO_RGB,
      NULL
  );

  decoder->width  = png_get_image_width( decoder->png_ptr, decoder->png_info_ptr );
  decoder->height = png_get_image_height( decoder->png_ptr, decoder->png_info_ptr );

  return 1;  // success
}


ImageIOLogical ImageIODecoder_decode_argb32( ImageIODecoder* decoder, ImageIOInteger* bitmap, int pixel_count )
{
  switch (decoder->format)
  {
    case IMAGE_IO_JPEG: return ImageIODecoder_decode_jpeg_argb32( decoder, bitmap, pixel_count );
    case IMAGE_IO_PNG:  return ImageIODecoder_decode_png_argb32( decoder, bitmap, pixel_count );
  }
  return 0;
}


ImageIOLogical ImageIODecoder_decode_jpeg_argb32( ImageIODecoder* decoder, ImageIOInteger* bitmap, int pixel_count )
{
  int width  = decoder->width;
  int height = decoder->height;
  
  ImageIOInteger* pixels = bitmap - 1;

  if (decoder->format != IMAGE_IO_JPEG) return 0;  // Input was not set up as JPEG.
  if (pixel_count < width*height) return 0;  // buffer too small

  decoder->buffer = (ImageIOByte*) malloc( width*3 );
  
  int j = decoder->jpeg_info.output_scanline;
  while (j < height)
  {
    ImageIOByte* dest = decoder->buffer;
    jpeg_read_scanlines( &decoder->jpeg_info, &dest, 1 );
  
    // Convert JPEG RGB or grayscale buffer to ImageIO Bitmap ARGB
    if (decoder->jpeg_info.jpeg_color_space == JCS_GRAYSCALE)
    {
      ImageIOByte* cursor = decoder->buffer - 1;
      for (int i=0; i<width; ++i)
      {
        int k = *(++cursor);
        *(++pixels) = (255<<24) | (k<<16) | (k<<8) | k;
      }
    }
    else
    {
      // RGB Color
      ImageIOByte* cursor = decoder->buffer;
      for (int i=0; i<width; ++i)
      {
        int r = cursor[0];
        int g = cursor[1];
        int b = cursor[2];
        *(++pixels) = (255<<24) | (r<<16) | (g<<8) | b;
        cursor += 3;
      }
    }
  
    j = decoder->jpeg_info.output_scanline;
  }
  
  free( decoder->buffer );
  decoder->buffer = 0;
  
  jpeg_finish_decompress( &decoder->jpeg_info );
  jpeg_destroy_decompress( &decoder->jpeg_info );

  return 1;
}


ImageIOLogical ImageIODecoder_decode_png_argb32( ImageIODecoder* decoder, ImageIOInteger* bitmap, int pixel_count )
{
  png_bytepp row_pointers = png_get_rows( decoder->png_ptr, decoder->png_info_ptr );
  int row_size = (int) png_get_rowbytes( decoder->png_ptr, decoder->png_info_ptr );
  RogueInteger* pixels = bitmap;

  for (int j=0; j<height; ++j)
  {
    memcpy( pixels, row_pointers[j], row_size );
    pixels += decoder->width;
  }

  // Premultiply the alpha and transform ABGR into ARGB
  RogueInteger* cur = $this->bitmap->pixels->data->integers - 1;
  for (int i=decoder->width*height; --i>=0; )
  {
    RogueInteger abgr = *(++cur);
    RogueInteger a = (abgr >> 24) & 255;
    RogueInteger r, g, b;

    if (a)
    {
      b = (abgr >> 16) & 255;
      g = (abgr >> 8)  & 255;
      r = abgr & 255;
      if (a != 255)
      {
        r = (r * a) / 255;
        g = (g * a) / 255;
        b = (b * a) / 255;
      }
    }
    else
    {
      r = g = b = 0;
    }

    *cur = (a<<24) | (r<<16) | (g<<8) | b;
  }

  png_destroy_read_struct( &decoder->png_ptr, &decoder->png_info_ptr, NULL );
  return 0;
}


void ImageIO_demultiply_alpha( ImageIOInteger* data, int count )
{
  ImageIOInteger* cur = data - 1;
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

void ImageIO_premultiply_alpha( ImageIOInteger* data, int count )
{
  ImageIOInteger* cur = data - 1;
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

void ImageIO_swap_red_and_blue( ImageIOInteger* data, int count )
{
  ImageIOInteger* cur = data - 1;
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

  longjmp( ((ImageIODecoder*)jpeg_info->err)->on_error, 1 );
}

void ImageIO_png_error_callback( png_structp png_ptr, png_const_charp msg )
{
  fprintf(stderr, "libpng error: %s\n", msg);
  fflush(stderr);

  longjmp( ((ImageIODecoder*)png_get_io_ptr(png_ptr))->on_error, 1 );
}

void ImageIO_png_read_callback( png_structp png_ptr, png_bytep data, png_size_t count )
{
  ImageIODecoder* decoder = (ImageIODecoder*) png_get_io_ptr( png_ptr );
  if (count > decoder->remaining) count = (png_size_t) decoder->remaining;

  if (count)
  {
    memcpy( data, decoder->reader, count );
    decoder->reader    += count;
    decoder->remaining -= count;
  }
}

