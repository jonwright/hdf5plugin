# /*##########################################################################
#
# Copyright (c) 2019 European Synchrotron Radiation Facility
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
#
# THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
# IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
# FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
# AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
# LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
# OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
# THE SOFTWARE.
#
# ###########################################################################*/
/* This provides replacement for HDF5 functions/variables used by filters.
 *
 * Those replacement provides no-op functions by default and if init_filter
 * is called it provides access to HDF5 functions/variables through dynamic
 * loading.
 * This is useful on Linux/macOS to avoid linking the plugin with a dedicated
 * HDF5 library.
 */
#include "hdf5.h"
#include "H5PLextern.h"

#ifdef _WIN32
#include <windows.h>
#define DL_OPEN(libname) LoadLibraryExW(libname, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)
#define DL_SYM(handle, symbol) GetProcAddress(handle, symbol)

#else
#include <dlfcn.h>
#define DL_OPEN(libname) dlopen(libname, RTLD_LAZY | RTLD_LOCAL)
#define DL_SYM(handle, symbol) dlsym(handle, symbol)

#endif /*_WIN32*/



/*Function types*/
/*H5*/
typedef herr_t (*DL_func_H5open)(void);
/*H5E*/
typedef herr_t (* DL_func_H5Epush1)(
    const char *file, const char *func, unsigned line,
    H5E_major_t maj, H5E_minor_t min, const char *str);
/*H5P*/
typedef herr_t (* DL_func_H5Pget_filter_by_id2)(hid_t plist_id, H5Z_filter_t id,
    unsigned int *flags/*out*/, size_t *cd_nelmts/*out*/,
    unsigned cd_values[]/*out*/, size_t namelen, char name[]/*out*/,
    unsigned *filter_config/*out*/);
typedef int (* DL_func_H5Pget_chunk)(
	hid_t plist_id, int max_ndims, hsize_t dim[]/*out*/);
typedef herr_t (* DL_func_H5Pmodify_filter)(
    hid_t plist_id, H5Z_filter_t filter,
    unsigned int flags, size_t cd_nelmts,
    const unsigned int cd_values[/*cd_nelmts*/]);
/*H5T*/
typedef size_t (* DL_func_H5Tget_size)(
    hid_t type_id);
typedef H5T_class_t (* DL_func_H5Tget_class)(hid_t type_id);
typedef hid_t (* DL_func_H5Tget_super)(hid_t type);
typedef herr_t (* DL_func_H5Tclose)(hid_t type_id);
/*H5Z*/
typedef herr_t (* DL_func_H5Zregister)(
    const void *cls);


static struct {
    /*H5*/
    DL_func_H5open H5open;
    /*H5E*/
    DL_func_H5Epush1 H5Epush1;
    /*H5P*/
    DL_func_H5Pget_filter_by_id2 H5Pget_filter_by_id2;
    DL_func_H5Pget_chunk H5Pget_chunk;
    DL_func_H5Pmodify_filter H5Pmodify_filter;
    /*H5T*/
    DL_func_H5Tget_size H5Tget_size;
    DL_func_H5Tget_class H5Tget_class;
    DL_func_H5Tget_super H5Tget_super;
    DL_func_H5Tclose H5Tclose;
    /*H5T*/
    DL_func_H5Zregister H5Zregister;
} DL_H5Functions = {
    NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL};


/*HDF5 variables*/
hid_t H5E_CANTREGISTER_g = -1;
hid_t H5E_CALLBACK_g = -1;
hid_t H5E_PLINE_g = -1;
hid_t H5E_ERR_CLS_g = -1;


static bool is_init = false;


/* Handle difference between dlopen and LoadLibraryExW */
#ifdef _WIN32
typedef const wchar_t* libname_t; /* Using utf-16-le encoding */
#else
typedef const char* libname_t;
#endif /*_WIN32*/


/* Initialize the dynamic loading of symbols and register the plugin
 *
 * libname: Name of the DLL from which to load libHDF5 symbols
 * Returns: a value < 0 if an error occured
 */
int init_filter(libname_t libname)
{
    int retval = -1;
  	void * handle;

    handle = DL_OPEN(libname);

    if (handle != NULL) {
        /*H5*/
        DL_H5Functions.H5open = (DL_func_H5open)DL_SYM(handle, "H5open");
        /*H5E*/
        DL_H5Functions.H5Epush1 = (DL_func_H5Epush1)DL_SYM(handle, "H5Epush1");
        /*H5P*/
        DL_H5Functions.H5Pget_filter_by_id2 = (DL_func_H5Pget_filter_by_id2)DL_SYM(handle, "H5Pget_filter_by_id2");
        DL_H5Functions.H5Pget_chunk = (DL_func_H5Pget_chunk)DL_SYM(handle, "H5Pget_chunk");
        DL_H5Functions.H5Pmodify_filter = (DL_func_H5Pmodify_filter)DL_SYM(handle, "H5Pmodify_filter");
        /*H5T*/
        DL_H5Functions.H5Tget_size = (DL_func_H5Tget_size)DL_SYM(handle, "H5Tget_size");
        DL_H5Functions.H5Tget_class = (DL_func_H5Tget_class)DL_SYM(handle, "H5Tget_class");
        DL_H5Functions.H5Tget_super = (DL_func_H5Tget_super)DL_SYM(handle, "H5Tget_super");
        DL_H5Functions.H5Tclose = (DL_func_H5Tclose)DL_SYM(handle, "H5Tclose");
        /*H5Z*/
        DL_H5Functions.H5Zregister = (DL_func_H5Zregister)DL_SYM(handle, "H5Zregister");

        /*Variables*/
#ifndef _WIN32 /* This crashes on Windows... */
        H5E_CANTREGISTER_g = *((hid_t *)DL_SYM(handle, "H5E_CANTREGISTER_g"));
        H5E_CALLBACK_g = *((hid_t *)DL_SYM(handle, "H5E_CALLBACK_g"));
        H5E_PLINE_g = *((hid_t *)DL_SYM(handle, "H5E_PLINE_g"));
        H5E_ERR_CLS_g = *((hid_t *)DL_SYM(handle, "H5E_ERR_CLS_g"));
#endif /*_WIN32*/

        /*Register plugin*/
        retval = H5Zregister(H5PLget_plugin_info());

        is_init = true;
    }

    return retval;
};


#define CALL(fallback, func, ...)\
    if(DL_H5Functions.func != NULL) {\
        return DL_H5Functions.func(__VA_ARGS__);\
    } else {\
        return fallback;\
    }


/*Function wrappers*/
/*H5*/
herr_t H5open(void)
{
CALL(0, H5open)
};

/*H5E*/
herr_t H5Epush1(const char *file, const char *func, unsigned line,
    H5E_major_t maj, H5E_minor_t min, const char *str)
{
CALL(0, H5Epush1, file, func, line, maj, min, str)
}

/*H5P*/
herr_t H5Pget_filter_by_id2(hid_t plist_id, H5Z_filter_t id,
    unsigned int *flags/*out*/, size_t *cd_nelmts/*out*/,
    unsigned cd_values[]/*out*/, size_t namelen, char name[]/*out*/,
    unsigned *filter_config/*out*/)
{
CALL(0, H5Pget_filter_by_id2, plist_id, id, flags, cd_nelmts, cd_values, namelen, name, filter_config)
}

int H5Pget_chunk(hid_t plist_id, int max_ndims, hsize_t dim[]/*out*/)
{
CALL(0, H5Pget_chunk, plist_id, max_ndims, dim)
}

herr_t H5Pmodify_filter(hid_t plist_id, H5Z_filter_t filter,
    unsigned int flags, size_t cd_nelmts,
    const unsigned int cd_values[/*cd_nelmts*/])
{
CALL(0, H5Pmodify_filter, plist_id, filter, flags, cd_nelmts, cd_values)
}

/*H5T*/
size_t H5Tget_size(hid_t type_id)
{
CALL(0, H5Tget_size, type_id)
}

H5T_class_t H5Tget_class(hid_t type_id)
{
CALL(H5T_NO_CLASS, H5Tget_class, type_id)
}


hid_t H5Tget_super(hid_t type)
{
CALL(0, H5Tget_super, type)
}

herr_t H5Tclose(hid_t type_id)
{
CALL(0, H5Tclose, type_id)
}

/*H5Z*/
herr_t H5Zregister(const void *cls)
{
CALL(-1, H5Zregister, cls)
}

