/* This is a wrapper for inflating useing miniz_tinfl
@YX Hao
#201712 v0.1
*/
#pragma once
#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/stat.h>
#include "defs.h"
#include "miniz.h"

#define IN_BUF_SIZE (1024*512)
#define OUT_BUF_SIZE (1024*512)

#if !defined(min)
    #define max(a ,b) (((a) > (b)) ? (a) : (b))
    #define min(a, b) (((a) < (b)) ? (a) : (b))
#endif

#if !defined(__min)
    #define __min min
    #define __max max
#endif

int inflate_fp(FILE *fp_r, FILE *fp_w) {
    static uint8_t pbuf_in[IN_BUF_SIZE];
    static uint8_t pbuf_out[OUT_BUF_SIZE];
    size_t infile_size, infile_remaining;
    size_t avail_in, in_bytes, total_in;
    size_t avail_out, out_bytes, total_out;
    uint8_t *pnext_in, *pnext_out;
    tinfl_status status;
    tinfl_decompressor inflator;
    //
    if (fp_r == fp_w) {
        fprintf(stderr, "Output file is the same as input file!\n");
        return 1;
    }
    //
    infile_size = _filelength(fileno(fp_r));
    infile_remaining = infile_size;
    pnext_in  = pbuf_in;
    pnext_out = pbuf_out;
    avail_in  = 0;
    avail_out = OUT_BUF_SIZE;
    total_in = 0;
    total_out = 0;
    //
    tinfl_init(&inflator);
    //
    for ( ; ; ) {
        if (!avail_in) { // pbuf_in left size
            size_t n = min(IN_BUF_SIZE, infile_remaining);
            //
            if (fread(pbuf_in, 1, n, fp_r) != n) {
                fprintf(stderr, "Failed reading from input file!\n");
                return 1;
            }
            //
            avail_in = n;
            pnext_in = pbuf_in;
            infile_remaining -= n;
        }
        //
        in_bytes  = avail_in;
        out_bytes = avail_out;
        status = tinfl_decompress(&inflator, pnext_in, &in_bytes,
                    pbuf_out, pnext_out, &out_bytes,
                    (infile_remaining ? TINFL_FLAG_HAS_MORE_INPUT : 0)
                    | TINFL_FLAG_PARSE_ZLIB_HEADER);
        //
        avail_in -= in_bytes;
        pnext_in += in_bytes;
        total_in += in_bytes;
        //
        avail_out -= out_bytes;
        pnext_out += out_bytes;
        total_out += out_bytes;
        //
        if ((status <= TINFL_STATUS_DONE) || (!avail_out)) {
            // Output buffer is full or decompression is done
            size_t n = OUT_BUF_SIZE - avail_out;
            if (fwrite(pbuf_out, 1, n, fp_w) != n) {
                fprintf(stderr, "Failed writing to output file!\n");
                return 1;
            }
            pnext_out = pbuf_out;
            avail_out = OUT_BUF_SIZE;
        }

        // done or went wrong
        if (status <= TINFL_STATUS_DONE) {
            if (status == TINFL_STATUS_DONE) {
                break;
            }
            else {
                fprintf(stderr, "Decompression failed with status %i!\n", status);
                return 1;
            }
        }
    }
    //
    return 0;
}

int inflate_f(char *f_r, char *f_w) {
    FILE *fp_r, *fp_w;
    int ret;
    //
    if (stricmp(f_r, f_w) == 0) {
        fprintf(stderr, "Output file is the same as input file!\n");
        return 1;
    }
    //
    ret = 1;
    fp_r = fopen(f_r, "rb");
    //
    if (!fp_r) {
        fprintf(stderr, "Can't open file: %s!\n", f_r);
    }
    else {
        fp_w = fopen(f_w, "wb");
        if (!fp_w) {
            fprintf(stderr, "Can't open file: %s!\n", f_w);
        }
        else {
            ret = inflate_fp(fp_r, fp_w);
            fflush(fp_w);
            fclose(fp_w);
        }
        fclose(fp_r);
    }
    //
    return ret;
}

#if defined(__CYGWIN__)
int _getmbcp()
{
    return 0;
}

int _filelength(int handle)
{
	struct stat	fi;
	if (fstat(handle, &fi) == -1)
	{
		return -1;
	}
	return fi.st_size;
}

void _splitpath (
        const CHAR *path,
        CHAR *drive,
        CHAR *dir,
        CHAR *fname,
        CHAR *ext
        )
{
        CHAR *p;
        CHAR* last_slash = NULL;
        CHAR* dot = NULL;
        unsigned len;

        /* we assume that the path argument has the following form, where any
         * or all of the components may be missing.
         *
         *  <drive><dir><fname><ext>
         *
         * and each of the components has the following expected form(s)
         *
         *  drive:
         *  0 to _MAX_DRIVE-1 characters, the last of which, if any, is a
         *  ':'
         *  dir:
         *  0 to _MAX_DIR-1 characters in the form of an absolute path
         *  (leading '/' or '\') or relative path, the last of which, if
         *  any, must be a '/' or '\'.  E.g -
         *  absolute path:
         *      \top\next\last\     ; or
         *      /top/next/last/
         *  relative path:
         *      top\next\last\  ; or
         *      top/next/last/
         *  Mixed use of '/' and '\' within a path is also tolerated
         *  fname:
         *  0 to _MAX_FNAME-1 characters not including the '.' character
         *  ext:
         *  0 to _MAX_EXT-1 characters where, if any, the first must be a
         *  '.'
         *
         */

        /* extract drive letter and :, if any */

        if ((strlen(path) >= (_MAX_DRIVE - 2)) && (*(path + _MAX_DRIVE - 2) == _T(':'))) {
            if (drive) {
                strncpy(drive, path, _MAX_DRIVE - 1);
                *(drive + _MAX_DRIVE-1) = _T('\0');
            }
            path += _MAX_DRIVE - 1;
        }
        else if (drive) {
            *drive = _T('\0');
        }

        /* extract path string, if any.  Path now points to the first character
         * of the path, if any, or the filename or extension, if no path was
         * specified.  Scan ahead for the last occurence, if any, of a '/' or
         * '\' path separator character.  If none is found, there is no path.
         * We will also note the last '.' character found, if any, to aid in
         * handling the extension.
         */

        for (last_slash = NULL, p = (CHAR *)path; *p; p++) {
#ifdef _MBCS
            if (_ISLEADBYTE (*p))
                p++;
            else {
#endif  /* _MBCS */
            if (*p == _T('/') || *p == _T('\\'))
                /* point to one beyond for later copy */
                last_slash = p + 1;
            else if (*p == _T('.'))
                dot = p;
#ifdef _MBCS
            }
#endif  /* _MBCS */
        }

        if (last_slash) {

            /* found a path - copy up through last_slash or max. characters
             * allowed, whichever is smaller
             */

            if (dir) {
                len = __min(((char *)last_slash - (char *)path) / sizeof(CHAR),
                    (_MAX_DIR - 1));
                strncpy(dir, path, len);
                *(dir + len) = _T('\0');
            }
            path = last_slash;
        }
        else if (dir) {

            /* no path found */

            *dir = _T('\0');
        }

        /* extract file name and extension, if any.  Path now points to the
         * first character of the file name, if any, or the extension if no
         * file name was given.  Dot points to the '.' beginning the extension,
         * if any.
         */

        if (dot && (dot >= path)) {
            /* found the marker for an extension - copy the file name up to
             * the '.'.
             */
            if (fname) {
                len = __min(((char *)dot - (char *)path) / sizeof(CHAR),
                    (_MAX_FNAME - 1));
                strncpy(fname, path, len);
                *(fname + len) = _T('\0');
            }
            /* now we can get the extension - remember that p still points
             * to the terminating nul character of path.
             */
            if (ext) {
                len = __min(((char *)p - (char *)dot) / sizeof(CHAR),
                    (_MAX_EXT - 1));
                strncpy(ext, dot, len);
                *(ext + len) = _T('\0');
            }
        }
        else {
            /* found no extension, give empty extension and copy rest of
             * string into fname.
             */
            if (fname) {
                len = __min(((char *)p - (char *)path) / sizeof(CHAR),
                    (_MAX_FNAME - 1));
                strncpy(fname, path, len);
                *(fname + len) = _T('\0');
            }
            if (ext) {
                *ext = _T('\0');
            }
        }
}

void _makepath (
        CHAR *path,
        const CHAR *drive,
        const CHAR *dir,
        const CHAR *fname,
        const CHAR *ext
        )
{
        const CHAR *p;

        /* we assume that the arguments are in the following form (although we
         * do not diagnose invalid arguments or illegal filenames (such as
         * names longer than 8.3 or with illegal characters in them)
         *
         *  drive:
         *      A           ; or
         *      A:
         *  dir:
         *      \top\next\last\     ; or
         *      /top/next/last/     ; or
         *      either of the above forms with either/both the leading
         *      and trailing / or \ removed.  Mixed use of '/' and '\' is
         *      also tolerated
         *  fname:
         *      any valid file name
         *  ext:
         *      any valid extension (none if empty or null )
         */

        /* copy drive */

        if (drive && *drive) {
                *path++ = *drive;
                *path++ = _T(':');
        }

        /* copy dir */

        if ((p = dir) && *p) {
                do {
                        *path++ = *p++;
                }
                while (*p);
#ifdef _MBCS
                if (*(p=_mbsdec(dir,p)) != _T('/') && *p != _T('\\')) {
#else  /* _MBCS */
                if (*(p-1) != _T('/') && *(p-1) != _T('\\')) {
#endif  /* _MBCS */
                        *path++ = _T('\\');
                }
        }

        /* copy fname */

        if (p = fname) {
                while (*p) {
                        *path++ = *p++;
                }
        }

        /* copy ext, including 0-terminator - check to see if a '.' needs
         * to be inserted.
         */

        if (p = ext) {
                if (*p && *p != _T('.')) {
                        *path++ = _T('.');
                }
                while (*path++ = *p++)
                        ;
        }
        else {
                /* better add the 0-terminator */
                *path = _T('\0');
        }
}

#endif
