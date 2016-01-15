ROOT_FOLDER := ../../..
BUILD_FOLDER := $(ROOT_FOLDER)/Build
OUTPUT_FOLDER := $(BUILD_FOLDER)/Samples/Simple

CC_FLAGS := -Wall
CC_FLAGS := -lz
CC_FLAGS += -I ../../ImageIO
CC_FLAGS += -I $(shell ls -d ../../lpng* | head -1)
CC_FLAGS += -I $(shell ls -d ../../jpeg* | head -1)

all: makelibrary build run

makelibrary:
	make -C $(ROOT_FOLDER)

build: $(OUTPUT_FOLDER) $(OUTPUT_FOLDER)/simple

$(OUTPUT_FOLDER):
	mkdir -p $(OUTPUT_FOLDER)

$(OUTPUT_FOLDER)/simple: $(OUTPUT_FOLDER)/simple.o $(BUILD_FOLDER)/libimageio.a
	g++ $(CC_FLAGS) $(OUTPUT_FOLDER)/simple.o -L $(BUILD_FOLDER) -limageio -o $(OUTPUT_FOLDER)/simple

$(OUTPUT_FOLDER)/simple.o: Simple.cpp $(BUILD_FOLDER)/libimageio.a
	g++ $(CC_FLAGS) -c Simple.cpp -o $(OUTPUT_FOLDER)/simple.o

run:
	$(OUTPUT_FOLDER)/simple
