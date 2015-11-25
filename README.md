# ImageIO Library

<table>
  <tr>
    <td>Purpose</td>
    <td>Provides a simple API for encoding and decoding JPEG and PNG images in memory.</td>
  </tr>
  <tr>
    <td>Current Version</td>
    <td>1.0.0 - November 25, 2015</td>
  </tr>
  <tr>
    <td>Language</td>
    <td>C</td>
  </tr>
  <tr>
    <td>Dependencies</td>
    <td>Standard <a href="http://ijg.org/">JPEG</a> and <a href="http://libpng.org/">PNG</a> libraries.</td>
  </tr>
</table>


## Usage

### Decoding JPEG and PNG to ARGB32

```C++
// Input bytes containing encoded JPEG or PNG read from a file or some
// other location.  ImageIOByte is equivalent to 'unsigned char'.
ImageIOByte*   encoded_bytes = ...;
int            encoded_byte_count = ...;

// Pointer to bitmap data that we'll create once we know the size of the image.
ImageIOARGB32* bitmap;  // ARGB32 equivalent to unsigned int32.

// Stack-based decoder struct.
ImageIODecoder decoder;

// Initialize the decoder and set the bytes as input
ImageIODecoder_init( &decoder );
if ( !ImageIODecoder_set_input(&decoder, encoded_bytes, encoded_byte_count) )
{
  // Not a valid JPEG or PNG file - generate error message and abort. 
  ...
}

// We now know the dimensions of the decoded image.  Create a bitmap to decode
// into.
bitmap = new ImageIOARGB32[ decoder.width * decoder.height ];

// Decode.
if ( !ImageIODecoder_decode_argb32(&decoder, bitmap) )
{
  // Not a valid JPEG or PNG file - generate error message and abort. 
  delete bitmap;
  ...
}

// Finished - retire the decoder and delete 'encoded_bytes' if necessary.
// It is up to the developer to eventually delete 'bitmap'.
ImageIODecoder_retire( &decoder );

// ---- Working with the bitmap data ----
// bitmap[0]                 - The leftmost pixel of the top row
// bitmap[decoder.width-1]   - The rightmost pixel of the top row
// bitmap[y*decoder.width+x] - The pixel at (x,y) in raster order
// (bitmap[i] >> 24) & 255   - The alpha value (0-255) of pixel i.
// (bitmap[i] >> 16) & 255   - The red value (0-255) of pixel i.
// (bitmap[i] >> 8) & 255    - The green value (0-255) of pixel i.
// bitmap[i] & 255           - The blue value (0-255) of pixel i.
```


### Encoding JPEG and PNG from ARGB32

```C++
// Pointer to existing ARGB32 bitmap data to be encoded as JPEG or PNG.
ImageIOARGB32* bitmap = ...;
int width = ...;
int height = ...;

// Stack-based encoder struct.
ImageIOEncoder encoder;

// Initialize the encoder and optionally set an encoding quality.
ImageIOEncoder_init( &encoder );
encoder.quality = 75;  // only affects JPEG encoding; 0..100 (75 default)

// Encode.
ImageIOEncoder_encode_argb32_jpeg( &encoder, bitmap, width, height );
  // OR
ImageIOEncoder_encode_argb32_png( &encoder, bitmap, width, height );

// Encoder struct maintains the encoded JPEG or PNG data.  Access the data
// (saving or copying it) before retiring the encoder.
ImageIOByte* data = encoder.encoded_bytes;
int count = encoder.encoded_byte_count;
...

// Finished - retire the encoder, allowing it to delete its internal data.
// It is up to the developer to delete 'bitmap' if necessary.
ImageIOEncoder_retire( &encoder );
```

###  Utility Functions
ImageIO provides a few utility functions that can be useful when reading and writing images.

`ImageIO_demultiply_alpha( bitmap:ImageIOARGB32*, count:int )`  
The opposite of `ImageIO_premultiply_alpha()` - divides each pixel's color component by its alpha value.
Not often needed but provided for symmetry.

`ImageIO_premultiply_alpha( bitmap:ImageIOARGB32*, count:int )`  
Multiplies the Red, Green, and Blue color components of each pixel by its alpha value (conceptually treating the alpha value as a proportional value 0.0 to 1.0).  This offers a significant performance boost during rendering by allowing the computationally simpler blending mode of `SOURCE + DEST*INVERSE_ALPHA` in place of `SOURCE*ALPHA + DEST*INVERSE_ALPHA`.

`ImageIO_swap_red_and_blue( bitmap:ImageIOARGB32*, count:int )`  
Swaps the Red and Blue color components of each pixel, transforming each ARGB value into the ABGR format favored by Open GL.


### Compiling and Linking

#### ImageIO Library
1.  Compile ImageIO.c directly in with your program.

2.  Ensure that ImageIO.h is in your project's Include Path.

#### JPEG Library
1.  Download and configure the latest JPEG reference library from [http://ijg.org/]()

2.  Ensure that the JPEG .h files are in your project's Include Path.

3.  Either link against JPEG as a separate library or compile the following JPEG files in with your project:

```
jaricom.c
jcapimin.c
jcapistd.c
jcarith.c
jccoefct.c
jccolor.c
jcdctmgr.c
jchuff.c
jcinit.c
jcmainct.c
jcmarker.c
jcmaster.c
jcomapi.c
jcparam.c
jcprepct.c
jcsample.c
jctrans.c
jdapimin.c
jdapistd.c
jdarith.c
jdatadst.c
jdatasrc.c
jdcoefct.c
jdcolor.c
jddctmgr.c
jdhuff.c
jdinput.c
jdmainct.c
jdmarker.c
jdmaster.c
jdmerge.c
jdpostct.c
jdsample.c
jdtrans.c
jerror.c
jfdctflt.c
jfdctfst.c
jfdctint.c
jidctflt.c
jidctfst.c
jidctint.c
jmemansi.c
jmemmgr.c
jquant1.c
jquant2.c
jutils.c
```


#### PNG Library
1.  Download and configure the latest PNG reference library from [http://libpng.org/]()

2.  Ensure that the PNG .h files are in your project's Include Path.

3.  Either link against PNG as a separate library or compile the following PNG files in with your project:

```
png.c
pngerror.c
pngget.c
pngmem.c
pngpread.c
pngread.c
pngrio.c
pngrtran.c
pngrutil.c
pngset.c
pngtrans.c
pngwio.c
pngwrite.c
pngwtran.c
pngwutil.c
```
