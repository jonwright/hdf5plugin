HDF5 compression filter for JPEG compression
============================================ 
* This is a lossy compression filter.  
* It provides a user-specified "quality factor" to control the trade-off of size versus accuracy.
* The officially assigned HDF5 filter number for this filter is 32019.

Author
======
* Mark Rivers, University of Chicago (rivers@cars.uchicago.edu)

Requirements
============
* libjpeg   This library is available as a package for most Linux distributions, and source code is available from https://www.ijg.org/.

Restrictions
============
  * Only 8-bit unsigned data arrays are supported.
  * Arrays must be either:
    * 2-D monochromatic [NumColumns, NumRows] 
    * 3-D RGB [3, NumColumns, NumRows]
  * Chunking must be set to the size of one entire image so the filter is called once for each image.

Installing the JPEG filter plugin
=================================
Instead of just linking this JPEG filter into your HDF5 application, it is possible to install
it as a system-wide HDF5 plugin (with HDF5 1.8.11 or later).  This is useful because it allows
*every* HDF5-using program on your system to transparently read JPEG-compressed HDF5 files.

As described in the [HDF5 plugin documentation](https://portal.hdfgroup.org/display/HDF5/HDF5+Dynamically+Loaded+Filters), 
you just need to compile the JPEG plugin into a shared library and
copy it to the plugin directory (which defaults to ``/usr/local/hdf5/lib/plugin`` on non-Windows systems).
You can also install in any other location and define the environment variable ``HDF5_PLUGIN_PATH`` to point to that directory.

Following the ``Compiling`` instructions below produces a ``libjpeg_h5plugin.so`` shared library 
file (or ``.dylib``/``.dll`` on Mac/Windows), that you can copy to the HDF5 plugin directory.

To *write* JPEG-compressed HDF5 files, on the other hand, an HDF5 using program must be
specially modified to enable the JPEG filter when writing HDF5 datasets, as described below.


Linking the JPEG filter directly into your program
==================================================
Instead of (or in addition to) installing the JPEG plugin system-wide as
described above, you can also link the JPEG filter directly into your
application.  Although this only makes the JPEG filter available in
your application (as opposed to other HDF5-using applications), it
is useful in cases where installing the plugin is inconvenient.  Compile
the JPEG filter as described above, but link ``libjpeg_h5filter.so``
(generated by ``make``) directly into your program.

In order to register JPEG in your HDF5 application, you then need
to call a function in jpeg_h5filter.h, with the following signature::

    int jpeg_register_h5filter()

Calling this will register the filter with the HDF5 library.

A non-negative return value indicates success.  If the registration
fails, an error is pushed onto the current error stack and a negative
value is returned.

An example C program (``src/example.c``) is included which demonstrates
the proper use of the filter.  It takes a single optional argument, which
is the JPEG quality factor.  If it is omitted, 100 is used.

This filter has been tested against HDF5 versions 1.10.1.

Using the JPEG filter in your application
=========================================

Assuming the filter is installed (either by a system-wide plugin or registered
directly in your program as described above), your application can transparently
*read* HDF5 files with JPEG-compressed datasets.  (The HDF5 library will detect
that the dataset is JPEG-compressed and invoke the filter automatically).

To *write* an HDF5 file with a JPEG-compressed dataset, you call the
[H5Pset_filter](https://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetFilter) function
on the property list of the dataset you are creating, and pass ``JPEG_H5FILTER``
(defined in ``jpeg_h5filter.h``) for the ``filter_id`` parameter.   In addition, HDF5
only supports compression for "chunked" datasets; this just means that you need to
call [H5Pset_chunk](https://www.hdfgroup.org/HDF5/doc/RM/RM_H5P.html#Property-SetChunk) to
specify a chunk size.  The chunking must be set to the size of a single image for the JPEG filter to
work properly.

When calling ``H5Pset_filter`` for compression it must be called with cd_nelmts=4 and cd_values as follows:
  * cd_values[0] = quality factor (1-100)
  * cd_values[1] = numColumns
  * cd_values[2] = numRows
  * cd_values[3] = 0=Mono, 1=RGB

Compiling
=========
The filter consists of a single ``src/jpeg_h5filter.c`` source file and
``src/jpeg_h5filter.h`` header, which will need the JPEG library
installed to work. The JPEG library needs to be installed on your system.

Assuming you have [cmake](http://www.cmake.org/) and other standard
Unix build tools installed, do
```
    mkdir build
    cd build
    cmake ..
    make
```
There is also an example `build_linux` script in the top-level directory.  It does the above
steps (except ``mkdir build``) and passes flags to cmake to define the locations of the hdf5
and jpeg libraries on your system.  In the example script ``libjpeg.so`` and ``libhdf5.so`` from
the anaconda3 distribution are used, because the system versions in /usr/lib64 were not
current enough.  You will need to edit this file for your site if you chose to use it.

This generates the library and plugin files required above in the ``build``
directory.

Running the Example Program
===========================
The example program generates an HDF5 file with 10 arrays, each 1024x512.
The values start at 0 in element [0,0] and increase by 1 in each array element.
Because the array is unsigned 8-bit integer the values wrap back to 0 after they reach 255.

The example program takes a single option argument which is the JPEG quality. It defaults to 100.
The program writes the HDF5 file, closes it, re-opens it, and reads the data back in.
It prints the percentage of values that do not exactly match the original values, due to the lossy nature
of the compression.

Here is the output with quality=100, 80, 60, 40, 20, and 1.
```
corvette:~/devel/jpegHDF5>build/example 100
Success, JPEG quality=100, percent of differing array elements=0.000000
corvette:~/devel/jpegHDF5>build/example 80
Success, JPEG quality=80, percent of differing array elements=16.406250
corvette:~/devel/jpegHDF5>build/example 60
Success, JPEG quality=60, percent of differing array elements=38.671875
corvette:~/devel/jpegHDF5>build/example 40
Success, JPEG quality=40, percent of differing array elements=65.625000
corvette:~/devel/jpegHDF5>build/example 20
Success, JPEG quality=20, percent of differing array elements=78.906250
corvette:~/devel/jpegHDF5>build/example 1
Success, JPEG quality=1, percent of differing array elements=96.484375
```

Below is the output of ``h5dump`` on the output file of the example test program.
``h5dump`` was not built with the JPEG decompressor, but is using the dynamic plugin that is built in this package.
This was done by setting the environment variable ``HDF5_PLUGIN_PATH`` to the ``build`` directory of this project:
```
corvette:~/devel/jpegHDF5>echo $HDF5_PLUGIN_PATH
/home/epics/devel/jpegHDF5/build
```

This is the output of h5dump when the quality is 100. The compression factor is 10.5.
The compression was actually lossless in this case, all array elements are the same before and after compression.
This is due to the nature of the actual data, and is not generally the case when quality=100.
```
corvette:~/devel/jpegHDF5>h5dump --properties example.h5 | more
HDF5 "example.h5" {
GROUP "/" {
   DATASET "dset" {
      DATATYPE  H5T_STD_U8LE
      DATASPACE  SIMPLE { ( 10, 512, 1024 ) / ( 10, 512, 1024 ) }
      STORAGE_LAYOUT {
         CHUNKED ( 1, 512, 1024 )
         SIZE 497380 (10.541:1 COMPRESSION)
      }
      FILTERS {
         USER_DEFINED_FILTER {
            FILTER_ID 32019
            COMMENT jpeg; see https://github.com/CARS-UChicago/jpegHDF5
            PARAMS { 100 1024 512 0 }
         }
      }
      FILLVALUE {
         FILL_TIME H5D_FILL_TIME_IFSET
         VALUE  H5D_FILL_VALUE_DEFAULT
      }
      ALLOCATION_TIME {
         H5D_ALLOC_TIME_INCR
      }
      DATA {
      (0,0,0): 0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17,
      (0,0,18): 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32,
      (0,0,33): 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43, 44, 45, 46, 47,
      (0,0,48): 48, 49, 50, 51, 52, 53, 54, 55, 56, 57, 58, 59, 60, 61, 62,
      (0,0,63): 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77,
      (0,0,78): 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92,
      (0,0,93): 93, 94, 95, 96, 97, 98, 99, 100, 101, 102, 103, 104, 105,
      (0,0,106): 106, 107, 108, 109, 110, 111, 112, 113, 114, 115, 116, 117,
      (0,0,118): 118, 119, 120, 121, 122, 123, 124, 125, 126, 127, 128, 129,
      (0,0,130): 130, 131, 132, 133, 134, 135, 136, 137, 138, 139, 140, 141,
      (0,0,142): 142, 143, 144, 145, 146, 147, 148, 149, 150, 151, 152, 153,
      (0,0,154): 154, 155, 156, 157, 158, 159, 160, 161, 162, 163, 164, 165,
      (0,0,166): 166, 167, 168, 169, 170, 171, 172, 173, 174, 175, 176, 177,
      (0,0,178): 178, 179, 180, 181, 182, 183, 184, 185, 186, 187, 188, 189,
      (0,0,190): 190, 191, 192, 193, 194, 195, 196, 197, 198, 199, 200, 201,
      (0,0,202): 202, 203, 204, 205, 206, 207, 208, 209, 210, 211, 212, 213,
      (0,0,214): 214, 215, 216, 217, 218, 219, 220, 221, 222, 223, 224, 225,
      (0,0,226): 226, 227, 228, 229, 230, 231, 232, 233, 234, 235, 236, 237,
      (0,0,238): 238, 239, 240, 241, 242, 243, 244, 245, 246, 247, 248, 249,
      ...
```

This is the output of h5dump when the quality is 60.  The compression factor is 35.1.
There are repeated and skipped values in the output.
```
corvette:~/devel/jpegHDF5>h5dump --properties example.h5 | more
HDF5 "example.h5" {
GROUP "/" {
   DATASET "dset" {
      DATATYPE  H5T_STD_U8LE
      DATASPACE  SIMPLE { ( 10, 512, 1024 ) / ( 10, 512, 1024 ) }
      STORAGE_LAYOUT {
         CHUNKED ( 1, 512, 1024 )
         SIZE 149220 (35.135:1 COMPRESSION)
      }
      FILTERS {
         USER_DEFINED_FILTER {
            FILTER_ID 32019
            COMMENT jpeg; see https://github.com/CARS-UChicago/jpegHDF5
            PARAMS { 60 1024 512 0 }
         }
      }
      FILLVALUE {
         FILL_TIME H5D_FILL_TIME_IFSET
         VALUE  H5D_FILL_VALUE_DEFAULT
      }
      ALLOCATION_TIME {
         H5D_ALLOC_TIME_INCR
      }
      DATA {
      (0,0,0): 0, 0, 1, 2, 3, 5, 6, 6, 8, 8, 9, 10, 12, 13, 14, 14, 16, 16,
      (0,0,18): 17, 19, 20, 21, 22, 22, 24, 25, 25, 27, 28, 29, 30, 30, 32,
      (0,0,33): 33, 34, 35, 36, 37, 38, 38, 40, 41, 42, 43, 44, 45, 46, 47,
      (0,0,48): 49, 49, 50, 51, 52, 53, 54, 55, 57, 57, 58, 59, 60, 62, 62,
      (0,0,63): 63, 65, 65, 66, 67, 68, 70, 71, 71, 73, 73, 74, 75, 77, 78,
      (0,0,78): 79, 79, 81, 81, 82, 84, 85, 86, 87, 87, 89, 90, 90, 92, 93,
      (0,0,93): 94, 95, 95, 96, 96, 97, 98, 99, 101, 101, 102, 104, 104, 105,
      (0,0,107): 106, 107, 109, 110, 110, 112, 112, 113, 114, 116, 117, 118,
      (0,0,119): 118, 120, 120, 121, 123, 124, 125, 126, 126, 128, 129, 129,
      (0,0,131): 131, 132, 133, 134, 134, 136, 137, 138, 139, 140, 141, 142,
      (0,0,143): 142, 144, 145, 146, 147, 148, 149, 150, 151, 153, 153, 154,
      (0,0,155): 155, 156, 157, 158, 159, 161, 161, 162, 163, 164, 166, 166,
      (0,0,167): 167, 169, 169, 170, 171, 172, 174, 175, 175, 177, 177, 178,
      (0,0,179): 179, 181, 182, 183, 183, 185, 185, 186, 188, 189, 190, 191,
      (0,0,191): 191, 193, 194, 194, 196, 197, 198, 199, 199, 200, 200, 201,
      (0,0,203): 202, 203, 205, 205, 206, 208, 208, 209, 210, 211, 213, 214,
      (0,0,215): 214, 216, 216, 217, 218, 220, 221, 222, 222, 224, 224, 225,
      (0,0,227): 227, 228, 229, 230, 230, 232, 233, 233, 235, 236, 237, 238,
      (0,0,239): 238, 240, 241, 242, 243, 244, 245, 246, 246, 248, 249, 250,
      ...
```

Performance
===========

The performance of the filter was measured under the following conditions:
* 1024x1024 8-bit monochromatic images
* Streaming 2000 images to a single HDF5 file
* JPEG quality=90

The filter plugin can write 100 frames/s (100 MB/s) under these conditions.

If the JPEG compression is done in an external function using 5 threads, and the precompressed images are
written to a single HDF5 file using *direct chunk write* then it can do 500 frames/s.  This is limited by the
speed of the external compression function with 5 threads, not by the speed of the HDF5 file writing.

Both types of files can be read with the plugin provided in this package.

Acknowledgments
===============
The cmake file and this documentation borrow heavily from the [hdf-blosc repository](https://github.com/Blosc/hdf5-blosc).