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
  if ( !ImageIODecoder_decode_argb32(&decoder,pixels) )
  {
    printf( "Error decoding image.\n" );
    return 1;
  }

  printf( "Image decoded successfully.\n" );

  return 0;
}
