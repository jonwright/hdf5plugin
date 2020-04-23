// $Id: jpegls_decompress_segfault.c 778 2016-03-09 07:56:29Z delaunay $
// =============================================================
//
// PROJECT : FCICOMP
//
// AUTHOR : THALES Services
//
// Copyright 2015 EUMETSAT
//
// =============================================================
// HISTORY :
//
// VERSION:1.0.1:NCR:FCICOMP-8:09/03/2016:Add the copyright notice in the header
//
// END-HISTORY
// =============================================================

#include <stdlib.h>
#include <stdio.h>
#include <signal.h>

#include "fcicomp_jpegls.h"
#include "fcicomp_errors.h"
#include "fcicomp_options.h"




// Provoke a segmentation fault in the decompression of a test image
int jpeglsDecompressSegFault(int argc, char* argv[]) {
	// ------------------------------------------
	// Parse the input arguments
	// ------------------------------------------

	// Check the number of input arguments
	if (argc != FCI_TWO) {
		ERR_TEST(EXIT_FAILURE, INVALID_NUMBER_ARGUMENTS)
	}

	// Input FileName
	char * inFile = argv[FCI_ONE];

	// ------------------------------------------
	// Read the input JLS file
	// ------------------------------------------

	// Open the file
	FILE * pFile = fopen(inFile, "rb");
	if (pFile == NULL) {
		ERR_TEST(EXIT_FAILURE, CANNOT_OPEN_FILE_R);
	}

	// Obtain file size
	fseek(pFile, 0, SEEK_END);
	size_t inSize = ftell(pFile);
	// Replace the pointer at the beginning of the file
	rewind(pFile);

	// Allocate memory to read the compressed file
	char *inBuf = (char *) malloc(inSize);
	if (inBuf == NULL) {
		ERR_TEST(EXIT_FAILURE, MEMORY_ALLOCATION_ERROR);
	}

	// Read the compressed file
	size_t nBytesRead = fread(inBuf, 1, inSize, pFile);
	if (nBytesRead != inSize) {
		ERR_TEST(EXIT_FAILURE, ERROR_READING_FILE);
	}

	// Close the file
	fclose(pFile);

	// ------------------------------------------
	// Read the input JLS header
	// ------------------------------------------

	// Define the JPEG-LS coding parameters
	jls_parameters_t jlsParams = { 0, 0, 0, 0,{ 0, 0, 0, 0, 0}};

	// Read JPEG-LS header to get the image parameters
	int samples = 0;
	int lines = 0;
	if (jpeglsReadHeader(inBuf, inSize, &samples, &lines, &jlsParams) != 0) {
		ERR_TEST(EXIT_FAILURE, "Error reading the JPEG-LS header!\n");
	}

	// ------------------------------------------
	// Perform JPEG-LS decompression
	// ------------------------------------------

	// Number of samples in the input image
	size_t nSamples = samples * lines;
	// Number of bytes
	int bpp = jlsParams.bit_per_sample;
	int nBytes = (bpp > 0) ? (((bpp - 1) / 8) + 1) : 0;
	// Size of the output buffer
	size_t outSize = nSamples * nBytes;

	// Provoke the segmentation fault: the output buffer does not correspond
	// to the outSize parameter provided to jpeglsDecompress!
	short *outBuf = NULL;

	// Decompress
	if (jpeglsDecompress(outBuf, outSize, inBuf, inSize) != FJLS_SEGFAULT_CAUGHT) {
		// As we provoke an error, jpeglsDecompress should return a positive error code
		ERR_TEST(EXIT_FAILURE, "The expected error has not occurred!\n");
	}

	// The expected error has happened => exit with the success status
	return EXIT_SUCCESS;
}
