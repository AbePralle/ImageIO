IMAGE_IO_VERSION := 1.0.0

LIB_PNG  := lpng1619
LIB_JPEG := jpeg-9a

CCFLAGS := -Wall
CCFLAGS += -ISource/$(LIB_PNG)
CCFLAGS += -ISource/$(LIB_JPEG)

LIB_PNG_SOURCE := \
  $(LIB_PNG)/png.c \
  $(LIB_PNG)/pngerror.c \
  $(LIB_PNG)/pngget.c \
  $(LIB_PNG)/pngmem.c \
  $(LIB_PNG)/pngpread.c \
  $(LIB_PNG)/pngread.c \
  $(LIB_PNG)/pngrio.c \
  $(LIB_PNG)/pngrtran.c \
  $(LIB_PNG)/pngrutil.c \
  $(LIB_PNG)/pngset.c \
  $(LIB_PNG)/pngtrans.c \
  $(LIB_PNG)/pngwio.c \
  $(LIB_PNG)/pngwrite.c \
  $(LIB_PNG)/pngwtran.c \
  $(LIB_PNG)/pngwutil.c

LIB_JPEG_SOURCE := \
	$(LIB_JPEG)/jaricom.c \
	$(LIB_JPEG)/jcapimin.c \
	$(LIB_JPEG)/jcapistd.c \
	$(LIB_JPEG)/jcarith.c \
	$(LIB_JPEG)/jccoefct.c \
	$(LIB_JPEG)/jccolor.c \
	$(LIB_JPEG)/jcdctmgr.c \
	$(LIB_JPEG)/jchuff.c \
	$(LIB_JPEG)/jcinit.c \
	$(LIB_JPEG)/jcmainct.c \
	$(LIB_JPEG)/jcmarker.c \
	$(LIB_JPEG)/jcmaster.c \
	$(LIB_JPEG)/jcomapi.c \
	$(LIB_JPEG)/jcparam.c \
	$(LIB_JPEG)/jcprepct.c \
	$(LIB_JPEG)/jcsample.c \
	$(LIB_JPEG)/jctrans.c \
	$(LIB_JPEG)/jdapimin.c \
	$(LIB_JPEG)/jdapistd.c \
	$(LIB_JPEG)/jdarith.c \
	$(LIB_JPEG)/jdatadst.c \
	$(LIB_JPEG)/jdatasrc.c \
	$(LIB_JPEG)/jdcoefct.c \
	$(LIB_JPEG)/jdcolor.c \
	$(LIB_JPEG)/jddctmgr.c \
	$(LIB_JPEG)/jdhuff.c \
	$(LIB_JPEG)/jdinput.c \
	$(LIB_JPEG)/jdmainct.c \
	$(LIB_JPEG)/jdmarker.c \
	$(LIB_JPEG)/jdmaster.c \
	$(LIB_JPEG)/jdmerge.c \
	$(LIB_JPEG)/jdpostct.c \
	$(LIB_JPEG)/jdsample.c \
	$(LIB_JPEG)/jdtrans.c \
	$(LIB_JPEG)/jerror.c \
	$(LIB_JPEG)/jfdctflt.c \
	$(LIB_JPEG)/jfdctfst.c \
	$(LIB_JPEG)/jfdctint.c \
	$(LIB_JPEG)/jidctflt.c \
	$(LIB_JPEG)/jidctfst.c \
	$(LIB_JPEG)/jidctint.c \
	$(LIB_JPEG)/jmemansi.c \
	$(LIB_JPEG)/jmemmgr.c \
	$(LIB_JPEG)/jquant1.c \
	$(LIB_JPEG)/jquant2.c \
	$(LIB_JPEG)/jutils.c

LIB_PNG_O_FILES  := $(LIB_PNG_SOURCE:%.c=Build/%.o)
LIB_JPEG_O_FILES := $(LIB_JPEG_SOURCE:%.c=Build/%.o)

all: build
	
build: Build/libimageio.a

Build/libimageio.a: Build Source/$(LIB_PNG) Source/$(LIB_JPEG) Build/ImageIO/ImageIO.o $(LIB_PNG_O_FILES) $(LIB_JPEG_O_FILES)
	ar rcs Build/libimageio.a Build/ImageIO/ImageIO.o Build/$(LIB_PNG)/*.o Build/$(LIB_JPEG)/*.o

Build:
	mkdir -p Build

Source/$(LIB_PNG):
	@echo "No PNG library found in the Source/ folder.  Install the latest lpngXXXX from"
	@echo "http://libpng.org/ and/or set the LIB_PNG variable at the top of the Makefile."
	@false

Source/$(LIB_JPEG):
	@echo "No JPEG library found in the Source/ folder.  Install the latest jpeg-XX from"
	@echo "http://ijg.org/ and/or set the LIB_JPEG variable at the top of the Makefile."
	@false

Build/ImageIO/ImageIO.o: Source/ImageIO/ImageIO.cpp Source/ImageIO/ImageIO.h
	@mkdir -p Build/ImageIO
	gcc $(CCFLAGS) -c Source/ImageIO/ImageIO.cpp -o Build/ImageIO/ImageIO.o

Build/%.o: Source/%.c
	@mkdir -p Build/$(LIB_PNG)
	@mkdir -p Build/$(LIB_JPEG)
	gcc $(CCFLAGS) -c $< -o $@

product: build Product/ImageIO-$(IMAGE_IO_VERSION).zip

Product/ImageIO-$(IMAGE_IO_VERSION).zip: Source/ImageIO/ImageIO.h Source/ImageIO/ImageIO.cpp
	rm -rf Product/ImageIO-$(IMAGE_IO_VERSION)
	mkdir -p Product/ImageIO-$(IMAGE_IO_VERSION)
	cp -r Source/ImageIO Product/ImageIO-$(IMAGE_IO_VERSION)
	cd Product && zip -r ImageIO-$(IMAGE_IO_VERSION).zip ImageIO-$(IMAGE_IO_VERSION)

clean:
	rm -rf Build/*
