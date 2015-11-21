//=============================================================================
//  ImageIO.c
//
//  2015.11.20 by Abe Pralle
//=============================================================================

#include "ImageIO.h"

ImageIOLogical ImageIODecoder_init( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size )
{
  decoder->type = IMAGE_IO_INVALID;
  if (encoded_data_size < 4) return 0;  // definitely not an image

  if (encoded_data[0] == 0xff)      ImageIODecoder_init_jpeg( decoder, encoded_data, encoded_data_size );
  else if (encoded_data[0] == 0x89) ImageIODecoder_init_png( decoder, encoded_data, encoded_data_size );
  else                              return 0;

  return 1;
}


ImageIOLogical ImageIODecoder_init_jpeg( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size )
{
  decoder->type = IMAGE_IO_JPEG;
  decoder->data = encoded_data;
  decoder->count = encoded_data_size;
  decoder->remaining = encoded_data_size;
  decoder->buffer = 0;

  decoder->jpeg_info.err = jpeg_std_error( (struct jpeg_error_mgr*) &decoder->error_info );
  decoder->error_info.jpeg_error_manager.error_exit = ImageIO_jpeg_error_callback;
  
  if (setjmp(decoder->error_info.on_error))
  {
    // Caught an error
    jpeg_destroy_decompress( &decoder->jpeg_info );
  
    if (decoder->buffer) free( buffer );

    decoder->type = IMAGE_IO_INVALID;
  
    return 0;
  }
  
  jpeg_create_decompress( &decoder->jpeg_info );
  jpeg_mem_src( &decoder->jpeg_info, $bytes->data->bytes, $bytes->count );
  jpeg_read_header( &decoder->jpeg_info, TRUE );
  
  jpeg_start_decompress( &decoder->jpeg_info );
  
  decoder->width = decoder->jpeg_info.output_width;
  decoder->height = decoder->jpeg_info.output_height;

  return 1;
}


ImageIOLogical ImageIODecoder_init_png( ImageIODecoder* decoder, ImageIOByte* encoded_data, int encoded_data_size )
{
}


ImageIOLogical ImageIODecoder_decode_argb32( ImageIODecoder* decoder, ImageIOInteger* pixels, int pixel_count )
{
  switch (decoder->type)
  {
    case IMAGE_IO_JPEG: return ImageIODecoder_decode_jpeg_argb32( decoder, pixels, pixel_count );
    case IMAGE_IO_PNG:  return ImageIODecoder_decode_png_argb32( decoder, pixels, pixel_count );
  }
  return 0;
}


ImageIOLogical ImageIODecoder_decode_jpeg_argb32( ImageIODecoder* decoder, ImageIOInteger* pixels, int pixel_count )
{
  int width  = decoder->width;
  int height = decoder->height;
  
  ImageIOInteger* pixels = pixels - 1;
  if (pixel_count < width*height) return 0;  // buffer too small

  decoder->buffer = (RogueByte*) malloc( width*3 );
  
  int j = decoder->jpeg_info.output_scanline;
  while (j < height)
  {
    RogueByte* dest = decoder->buffer;
    jpeg_read_scanlines( &decoder->jpeg_info, &dest, 1 );
  
    // Convert JPEG RGB or grayscale buffer to Rogue Bitmap ARGB
    if (decoder->jpeg_info.jpeg_color_space == JCS_GRAYSCALE)
    {
      RogueByte* cursor = decoder->buffer - 1;
      for (int i=0; i<width; ++i)
      {
        int k = *(++cursor);
        *(++pixels) = (255<<24) | (k<<16) | (k<<8) | k;
      }
    }
    else
    {
      // RGB Color
      RogueByte* cursor = decoder->buffer;
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


ImageIOLogical ImageIODecoder_decode_png_argb32( ImageIODecoder* decoder, ImageIOInteger* pixels, int pixel_count )
{
  return 1;
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

  longjmp( ((struct ImageIODecoder::ErrorInfo*)jpeg_info->err)->on_error, 1 );
}

