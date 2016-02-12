# ImageIO Library

<table>
  <tr>
    <td>Purpose</td>
    <td>Provides a simple API for encoding and decoding JPEG and PNG images in memory.</td>
  </tr>
  <tr>
    <td>Current Version</td>
    <td>1.0.7 - February 12, 2016</td>
  </tr>
  <tr>
    <td>Language</td>
    <td>C++</td>
  </tr>
  <tr>
    <td>Dependencies</td>
    <td>Standard <a href="http://ijg.org/">JPEG</a> and <a href="http://libpng.org/">PNG</a> libraries.</td>
  </tr>
</table>


## Usage

### Project Namespace
The ImageIO library is defined within the nested namespace `PROJECT_NAMESPACE::ImageIO`, where `PROJECT_NAMESPACE` is a `#define` that defaults to `Project`.  Including "ImageIO.h" automatically issues the directive `using namespace PROJECT_NAMESPACE;`.  If you're wanting to use ImageIO in a precompiled library and avoid conflicts with end users who might also use ImageIO, define `PROJECT_NAMESPACE=SomeOtherNamespace` as a compiler flag.

### Decoding JPEG and PNG to ARGB32

```C++
// Input bytes containing encoded JPEG or PNG read from a file or some
// other location.  ImageIO::Byte is equivalent to 'unsigned char'.
ImageIO::Byte*   encoded_bytes = ...;
int            encoded_byte_count = ...;

// Pointer to bitmap data that we'll create once we know the size of the image.
ImageIO::ARGB32* bitmap;  // ARGB32 equivalent to unsigned int32.

// Stack-based decoder struct.
ImageIO::Decoder decoder;

// Initialize the decoder and set the bytes as input
if ( !decoder.open(encoded_bytes, encoded_byte_count) )
{
  // Not a valid JPEG or PNG file - generate error message and abort. 
  ...
}

// We now know the dimensions of the decoded image.  Create a bitmap to decode
// into.
bitmap = new ImageIO::ARGB32[ decoder.width * decoder.height ];

// Decode.
if ( !decoder.decode_argb32(bitmap) )
{
  // Not a valid JPEG or PNG file - generate error message and abort. 
  delete bitmap;
  ...
}

// Finished!  It is up to the developer to eventually delete 'bitmap'
// since it was created outside the ImageIO library.

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
ImageIO::ARGB32* bitmap = ...;
int width = ...;
int height = ...;

// Stack-based encoder struct.
ImageIO::Encoder encoder;

// Initialize the encoder and optionally set an encoding quality.
encoder.quality = 75;  // only affects JPEG encoding; 0..100 (75 default)

// Encode.
encoder.encode_argb32_jpeg( bitmap, width, height );
  // OR
encoder.encode_argb32_png( bitmap, width, height );

// Encoder struct maintains the encoded JPEG or PNG data.  Access the data
// (saving or copying it) before retiring the encoder.
ImageIO::Byte* data = encoder.encoded_bytes;
int count = encoder.encoded_byte_count;
...

// Finished!  It is up to the developer to delete 'bitmap' if necessary.
```

###  Utility Functions
ImageIO provides a few utility functions that can be useful when reading and writing images.

`ImageIO::demultiply_alpha( bitmap:ImageIO::ARGB32*, count:int )`  
The opposite of `ImageIO::premultiply_alpha()` - divides each pixel's color component by its alpha value.
Not often needed but provided for symmetry.

`ImageIO::premultiply_alpha( bitmap:ImageIO::ARGB32*, count:int )`  
Multiplies the Red, Green, and Blue color components of each pixel by its alpha value (conceptually treating the alpha value as a proportional value 0.0 to 1.0).  This offers a significant performance boost during rendering by allowing the computationally simpler blending mode of `SOURCE + DEST*INVERSE_ALPHA` in place of `SOURCE*ALPHA + DEST*INVERSE_ALPHA`.

`ImageIO::swap_red_and_blue( bitmap:ImageIO::ARGB32*, count:int )`  
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

## Change Log

### v1.0.7 - February 12, 2016
-  Removed semicolon after namespace closing bracket to avoid C++-11 warning.

### v1.0.6 - February 2, 2016
-  Fixed namespace bug - moved an #include out of the project namespace.

### v1.0.5 - January 15, 2016
-  Removed debug printing.

### v1.0.4 - January 15, 2016
-  Changed ORG_NAMESPACE to PROJECT_NAMESPACE.

### v1.0.3 - January 14, 2016
-  Converted library to C++.

### v1.0.2 - December 29, 2015
-  API tweak - 'ImageIODecoder_set_input()' renamed to 'ImageIODecoder_open()'.

### v1.0.1 - December 19, 2015
-  Added type cast to prevent an `unsigned long` to `int` conversion warning.

### v1.0.0 - November 25, 2015
-  Original release.
