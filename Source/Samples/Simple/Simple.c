#include <stdio.h>
#include <stdlib.h>
#include "ImageIO.h"

int main()
{
  FILE* fp;
  int   size;
  int   width;
  int   height;
  ImageIOByte*    data;
  ImageIOInteger* pixels;
  ImageIODecoder  decoder;
  ImageIOEncoder  encoder;

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

  data = malloc( size );
  fread( data, 1, size, fp );
  fclose( fp );

  ImageIODecoder_init( &decoder );
  if ( !ImageIODecoder_set_input(&decoder,data,size) )
  {
    printf( "Input is neither a PNG nor a JPEG.\n" );
    return 1;
  }

  width = decoder.width;
  height = decoder.height;
  printf( "Image is %dx%d\n", width, height );

  pixels = malloc( width * height * sizeof(ImageIOInteger) );
  if ( !ImageIODecoder_decode(&decoder,pixels) )
  {
    printf( "Error decoding image.\n" );
    return 1;
  }

  printf( "Image decoded successfully.\n" );

  ImageIOEncoder_init( &encoder );
  if (ImageIOEncoder_encode_png(&encoder,pixels,width,height))
  {
    printf( "Writing ../../../Build/cats.png\n" );
    fp = fopen( "../../../Build/cats.png", "wb" );
    fwrite( encoder.encoded_data, 1, encoder.encoded_data_size, fp );
    fclose( fp );
  }

  printf( "Encoded size: %d\n", encoder.encoded_data_size );

  ImageIOEncoder_retire( &encoder );

  return 0;
}
