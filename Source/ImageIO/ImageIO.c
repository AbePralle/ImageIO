//=============================================================================
//  ImageIO.c
//
//  2015.11.20 by Abe Pralle
//=============================================================================

#include <stdlib.h>
#include <string.h>
#include "ImageIO.h"

#ifdef __cplusplus
extern "C"
{
#endif

//-----------------------------------------------------------------------------
//  ImageIODecoder
//-----------------------------------------------------------------------------
ImageIODecoder* ImageIODecoder_init( ImageIODecoder* decoder )
{
  memset( decoder, 0, sizeof(ImageIODecoder) );
  return decoder;
}

ImageIODecoder* ImageIODecoder_retire( ImageIODecoder* decoder )
{
  // No action - present for symmetry with ImageIOEncoder_retire().
  return decoder;
}

ImageIOLogical ImageIODecoder_set_input( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size )
{
  decoder->format = IMAGE_IO_INVALID;
  if (encoded_data_size < 4) return 0;  // definitely not an image

  if (encoded_data[0] == 0xff)      return ImageIODecoder_set_input_jpeg( decoder, encoded_data, encoded_data_size );
  else if (encoded_data[0] == 0x89) return ImageIODecoder_set_input_png( decoder, encoded_data, encoded_data_size );
  else                              return 0;
}


ImageIOLogical ImageIODecoder_set_input_jpeg( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size )
{
  decoder->format = IMAGE_IO_JPEG;
  decoder->data = encoded_data;
  decoder->count = encoded_data_size;

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


ImageIOLogical ImageIODecoder_set_input_png( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size )
{
  decoder->format = IMAGE_IO_PNG;

  decoder->data = encoded_data;
  decoder->reader = encoded_data;
  decoder->remaining = encoded_data_size;

  decoder->png_ptr = png_create_read_struct( PNG_LIBPNG_VER_STRING, decoder,
      ImageIO_png_error_callback, NULL );
  if ( !decoder->png_ptr ) return 0; // Out of memory

  png_set_add_alpha( decoder->png_ptr, 255, PNG_FILLER_AFTER );

  decoder->png_info_ptr = png_create_info_struct( decoder->png_ptr );
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
      PNG_TRANSFORM_STRIP_16 | PNG_TRANSFORM_PACKING | PNG_TRANSFORM_EXPAND | PNG_TRANSFORM_GRAY_TO_RGB | PNG_TRANSFORM_BGR,
      NULL
  );

  decoder->width  = png_get_image_width( decoder->png_ptr, decoder->png_info_ptr );
  decoder->height = png_get_image_height( decoder->png_ptr, decoder->png_info_ptr );

  return 1;  // success
}


ImageIOLogical ImageIODecoder_decode( ImageIODecoder* decoder, ImageIOInteger* bitmap )
{
  switch (decoder->format)
  {
    case IMAGE_IO_JPEG: return ImageIODecoder_decode_jpeg( decoder, bitmap );
    case IMAGE_IO_PNG:  return ImageIODecoder_decode_png( decoder, bitmap );
  }
  return 0;
}


ImageIOLogical ImageIODecoder_decode_jpeg( ImageIODecoder* decoder, ImageIOInteger* bitmap )
{
  int width  = decoder->width;
  int height = decoder->height;
  
  ImageIOInteger* pixels = bitmap - 1;

  if (decoder->format != IMAGE_IO_JPEG) return 0;  // Input was not set up as JPEG.

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
      ImageIOByte* cursor = decoder->buffer;
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
  
    j = decoder->jpeg_info.output_scanline;
  }
  
  free( decoder->buffer );
  decoder->buffer = 0;
  
  jpeg_finish_decompress( &decoder->jpeg_info );
  jpeg_destroy_decompress( &decoder->jpeg_info );

  return 1;
}


ImageIOLogical ImageIODecoder_decode_png( ImageIODecoder* decoder, ImageIOInteger* bitmap )
{
  int height = decoder->height;
  png_bytepp row_pointers = png_get_rows( decoder->png_ptr, decoder->png_info_ptr );
  int row_size = (int) png_get_rowbytes( decoder->png_ptr, decoder->png_info_ptr );
  ImageIOInteger* pixels = bitmap;

  for (int j=0; j<height; ++j)
  {
    memcpy( pixels, row_pointers[j], row_size );
    pixels += decoder->width;
  }

  png_destroy_read_struct( &decoder->png_ptr, &decoder->png_info_ptr, NULL );
  return 1;
}


//-----------------------------------------------------------------------------
//  ImageIOEncoder
//-----------------------------------------------------------------------------
ImageIOEncoder* ImageIOEncoder_init( ImageIOEncoder* encoder )
{
  memset( encoder, 0, sizeof(ImageIOEncoder) );
  encoder->quality = 75;
  return encoder;
}

ImageIOEncoder* ImageIOEncoder_retire( ImageIOEncoder* encoder )
{
  if (encoder->encoded_data)
  {
    free( encoder->encoded_data );
    encoder->encoded_data = 0;
  }

  return encoder;
}

ImageIOLogical ImageIOEncoder_encode_jpeg( ImageIOEncoder* encoder, ImageIOInteger* bitmap, int width, int height )
{
  unsigned long encoded_size;
  JSAMPROW row_pointer;
  ImageIOByte* buffer;

  encoder->jpeg_info.err = jpeg_std_error( (struct jpeg_error_mgr*) encoder );
  encoder->jpeg_error_manager.error_exit = ImageIO_jpeg_error_callback;

  if (setjmp(encoder->on_error))
  {
    // Caught an error
    jpeg_destroy_compress( &encoder->jpeg_info );
  
    //if (encoder->buffer) free( encoder->buffer );

    return 0;
  }

  jpeg_create_compress(&encoder->jpeg_info);

  jpeg_mem_dest( &encoder->jpeg_info, &encoder->encoded_data, &encoded_size );

  encoder->jpeg_info.image_width      = width;
  encoder->jpeg_info.image_height     = height;
  encoder->jpeg_info.input_components = 3;
  encoder->jpeg_info.in_color_space   = JCS_RGB;

  jpeg_set_defaults( &encoder->jpeg_info);
  jpeg_set_quality( &encoder->jpeg_info, encoder->quality, 1 );  // 0..100
  jpeg_start_compress( &encoder->jpeg_info, 1 );

  buffer = malloc( width*3 );
  row_pointer = buffer;

  while (encoder->jpeg_info.next_scanline < height)
  {
    int j = encoder->jpeg_info.next_scanline;
    int n = width;
    ImageIOInteger* pixels = bitmap + j*width - 1;
    ImageIOByte*    dest = buffer;
    while (--n >= 0)
    {
      int color = *(++pixels);
      dest[0] = (ImageIOByte) (color >> 16);
      dest[1] = (ImageIOByte) (color >> 8);
      dest[2] = (ImageIOByte) color;
      dest += 3;
    }

    jpeg_write_scanlines( &encoder->jpeg_info, &row_pointer, 1 );
  }

  jpeg_finish_compress( &encoder->jpeg_info );

  encoder->encoded_data_size = encoded_size;
  return 1;
}

ImageIOLogical ImageIOEncoder_encode_png( ImageIOEncoder* encoder, ImageIOInteger* bitmap, int width, int height )
{
  int j;
  int color_type;
  int must_delete_bytes;
  int bytes_per_pixel;
  png_bytepp row_pointers;
  png_bytep  bytes;

  encoder->png_ptr = png_create_write_struct( PNG_LIBPNG_VER_STRING, encoder,
      ImageIO_png_error_callback, NULL );
  if ( !encoder->png_ptr ) return 0; // Out of memory

  encoder->png_info_ptr = png_create_info_struct( encoder->png_ptr );
  if ( !encoder->png_info_ptr )
  {
    png_destroy_write_struct( &encoder->png_ptr, NULL );
    return 0;  // Out of memory
  }

  if (setjmp(encoder->on_error))
  {
    png_destroy_write_struct( &encoder->png_ptr, &encoder->png_info_ptr );
    return 0;
  }

  png_set_write_fn( encoder->png_ptr, encoder, ImageIO_png_write_callback, ImageIO_png_flush_callback );

  if (ImageIO_bitmap_has_translucent_pixels(bitmap,width*height))
  {
    color_type = PNG_COLOR_TYPE_RGB_ALPHA;
    must_delete_bytes = 0;
    bytes = (png_bytep) bitmap;
    bytes_per_pixel = 4;
  }
  else
  {
    int n = width*height;
    ImageIOInteger* src = bitmap - 1;
    png_bytep dest;

    color_type = PNG_COLOR_TYPE_RGB;
    must_delete_bytes = 1;
    bytes_per_pixel = 3;
    bytes = malloc( width * height * 3 );
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

  png_set_IHDR( encoder->png_ptr, encoder->png_info_ptr, width, height, 8,
       color_type, PNG_INTERLACE_NONE, PNG_COMPRESSION_TYPE_DEFAULT, PNG_FILTER_TYPE_DEFAULT );

  row_pointers = malloc( sizeof(png_bytep) * height );
  for (j=0; j<height; ++j)
  {
    row_pointers[j] = bytes + j*width*bytes_per_pixel;
  }

  png_set_rows( encoder->png_ptr, encoder->png_info_ptr, row_pointers );

  png_write_png( encoder->png_ptr, encoder->png_info_ptr, PNG_TRANSFORM_BGR, 0 );

  free( row_pointers );
  if (must_delete_bytes) free( bytes );

  return 1;
}

void ImageIOEncoder_reserve( ImageIOEncoder* encoder, int additional_count )
{
  if ( !encoder->encoded_data )
  {
    if (additional_count < 16*1024) additional_count = 16*1024;
    encoder->encoded_data = malloc( additional_count );
    encoder->capacity = additional_count;
  }
  else
  {
    int required = encoder->encoded_data_size + additional_count;
    if (required > encoder->capacity)
    {
      int double_size = encoder->capacity * 2;
      if (double_size > required) required = double_size;

      {
        ImageIOByte* expanded_data = malloc( required );
        memcpy( expanded_data, encoder->encoded_data, encoder->encoded_data_size );
        free( encoder->encoded_data );
        encoder->encoded_data = expanded_data;
        encoder->capacity = required;
      }
    }
  }
}

void ImageIOEncoder_write( ImageIOEncoder* encoder, ImageIOByte* bytes, int count )
{
  ImageIOEncoder_reserve( encoder, count );
  memcpy( encoder->encoded_data + encoder->encoded_data_size, bytes, count );
  encoder->encoded_data_size += count;
}

//-----------------------------------------------------------------------------
//  ImageIO Utility
//-----------------------------------------------------------------------------
ImageIOLogical ImageIO_bitmap_has_translucent_pixels( ImageIOInteger* bitmap, int count )
{
  ImageIOInteger* cur = bitmap - 1;
  int c = count;
  while (--c >= 0)
  {
    if ((*(++cur) & 0xff000000) != 0xff000000) return 1;
  }
  return 0;
}

void ImageIO_demultiply_alpha( ImageIOInteger* bitmap, int count )
{
  ImageIOInteger* cur = bitmap - 1;
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

void ImageIO_premultiply_alpha( ImageIOInteger* bitmap, int count )
{
  ImageIOInteger* cur = bitmap - 1;
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

void ImageIO_swap_red_and_blue( ImageIOInteger* bitmap, int count )
{
  ImageIOInteger* cur = bitmap - 1;
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

void ImageIO_png_read_callback( png_structp png_ptr, png_bytep bytes, png_size_t count )
{
  ImageIODecoder* decoder = (ImageIODecoder*) png_get_io_ptr( png_ptr );
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
  ImageIOEncoder* encoder = (ImageIOEncoder*) png_get_io_ptr( png_ptr );
  ImageIOEncoder_write( encoder, (ImageIOByte*) bytes, (int) count );
}

void ImageIO_png_flush_callback( png_structp png_ptr )
{
  // No action
}

#ifdef __cplusplus
} // end extern "C"
#endif
