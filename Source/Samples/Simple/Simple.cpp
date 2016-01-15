#include <stdio.h>
#include <stdlib.h>
#include "ImageIO.h"

int main()
{
  FILE* fp;
  int   size;
  int   width;
  int   height;
  ImageIO::Byte*    data;
  ImageIO::ARGB32*  pixels;
  ImageIO::Decoder  decoder;
  ImageIO::Encoder  encoder;

  printf( "Simple ImageIO Sample App\n" );

  fp = fopen( "cats.jpg", "rb" );
  if ( !fp )
  {
    printf( "Unable to open cats.jpg for reading.\n" );
    return 1;
  }
  fseek( fp, 0, SEEK_END );
  size = (int) ftell( fp );
  fseek( fp, 0, SEEK_SET );

  data = new ImageIO::Byte[ size ];
  fread( data, 1, size, fp );
  fclose( fp );

  if ( !decoder.open(data,size) )
  {
    printf( "Input is neither a PNG nor a JPEG.\n" );
    return 1;
  }

  width = decoder.width;
  height = decoder.height;
  printf( "Image is %dx%d\n", width, height );

  pixels = new ImageIO::ARGB32[ width * height ];
  if ( !decoder.decode_argb32(pixels) )
  {
    printf( "Error decoding image.\n" );
    return 1;
  }

  printf( "Image decoded successfully.\n" );

  if (encoder.encode_argb32_png(pixels,width,height))
  {
    printf( "Writing ../../../Build/cats.png\n" );
    fp = fopen( "../../../Build/cats.png", "wb" );
    fwrite( encoder.encoded_bytes, 1, encoder.encoded_byte_count, fp );
    fclose( fp );
  }

  if (encoder.encode_argb32_jpeg(pixels,width,height))
  {
    printf( "Writing ../../../Build/cats.jpg\n" );
    fp = fopen( "../../../Build/cats.jpg", "wb" );
    fwrite( encoder.encoded_bytes, 1, encoder.encoded_byte_count, fp );
    fclose( fp );
  }

  return 0;
}
