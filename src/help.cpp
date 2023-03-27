/* help.cpp --

   This file is part of the UPX executable compressor.

   Copyright (C) 1996-2023 Markus Franz Xaver Johannes Oberhumer
   Copyright (C) 1996-2023 Laszlo Molnar
   All Rights Reserved.

   UPX and the UCL library are free software; you can redistribute them
   and/or modify them under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of
   the License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; see the file COPYING.
   If not, write to the Free Software Foundation, Inc.,
   59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.

   Markus F.X.J. Oberhumer              Laszlo Molnar
   <markus@oberhumer.com>               <ezerotven+github@gmail.com>
 */

#include "conf.h"
#include "packmast.h"
#include "packer.h"
#include "compress/compress.h" // upx_ucl_version_string()

/*************************************************************************
// header
**************************************************************************/

// also see UPX_CONFIG_DISABLE_GITREV in CMakeLists.txt
#if defined(UPX_VERSION_GITREV)
const char gitrev[] = UPX_VERSION_GITREV;
#else
const char gitrev[1] = {0};
#endif

void show_header(void) {
    FILE *f = con_term;
    int fg;

    static bool header_done;
    if (header_done)
        return;
    header_done = true;

    fg = con_fg(f, FG_GREEN);
    // clang-format off
    con_fprintf(f,
                "                       Ultimate Packer for eXecutables\n"
                "                          Copyright (C) 1996 - " UPX_VERSION_YEAR "\n"
#if defined(UPX_VERSION_GITREV)
                "UPX git-%6.6s%c"
#else
                "UPX %-11s"
#endif
                " Markus Oberhumer, Laszlo Molnar & John Reiser  %14s\n\n",
#if defined(UPX_VERSION_GITREV)
                gitrev,
                (sizeof(gitrev)-1 > 6 && gitrev[sizeof(gitrev)-2] == '+') ? '+' : ' ',
#else
                UPX_VERSION_STRING,
#endif
                UPX_VERSION_DATE);
    // clang-format on
    fg = con_fg(f, fg);
    UNUSED(fg);
}

/*************************************************************************
// usage
**************************************************************************/

void show_usage(void) {
    FILE *f = con_term;

    con_fprintf(f, "Usage: %s [-123456789dlthVL] [-qvfk] [-o file] %sfile..\n", progname,
#if (ACC_OS_DOS32) && defined(__DJGPP__)
                "[@]");
#else
                "");
#endif
}

/*************************************************************************
// util
**************************************************************************/

namespace {
struct PackerNames {
    struct Entry {
        const char *fname;
        const char *sname;
    };
    Entry names[64];
    size_t names_count;
    const Options *o;
    PackerNames() : names_count(0), o(nullptr) {}
    void add(const Packer *p) {
        assert(names_count < 64);
        names[names_count].fname = p->getFullName(o);
        names[names_count].sname = p->getName();
        names_count++;
    }
    static Packer *visit(Packer *p, void *user) {
        PackerNames *self = (PackerNames *) user;
        self->add(p);
        delete p;
        return nullptr;
    }
    static int __acc_cdecl_qsort cmp_fname(const void *a, const void *b) {
        return strcmp(((const Entry *) a)->fname, ((const Entry *) b)->fname);
    }
    static int __acc_cdecl_qsort cmp_sname(const void *a, const void *b) {
        return strcmp(((const Entry *) a)->sname, ((const Entry *) b)->sname);
    }
};

static void show_all_packers(FILE *f, int verbose) {
    Options o;
    o.reset();
    PackerNames pn;
    pn.o = &o;
    PackMaster::visitAllPackers(PackerNames::visit, nullptr, &o, &pn);
    qsort(pn.names, pn.names_count, sizeof(PackerNames::Entry), PackerNames::cmp_fname);
    size_t pos = 0;
    for (size_t i = 0; i < pn.names_count; ++i) {
        const char *fn = pn.names[i].fname;
        const char *sn = pn.names[i].sname;
        if (verbose > 0) {
            con_fprintf(f, "    %-36s %s\n", fn, sn);
        } else {
            size_t fl = strlen(fn);
            if (pos == 0) {
                con_fprintf(f, "  %s", fn);
                pos = 2 + fl;
            } else if (pos + 1 + fl > 80) {
                con_fprintf(f, "\n  %s", fn);
                pos = 2 + fl;
            } else {
                con_fprintf(f, " %s", fn);
                pos += 1 + fl;
            }
        }
    }
    if (verbose <= 0 && pn.names_count)
        con_fprintf(f, "\n");
}
} // namespace

/*************************************************************************
// help
**************************************************************************/

void show_help(int verbose) {
    FILE *f = con_term;
    int fg;

    show_header();
    show_usage();

    // clang-format off
    fg = con_fg(f, FG_YELLOW);
    con_fprintf(f, "\nCommands:\n");
    fg = con_fg(f, fg);
    con_fprintf(f,
                "  -1     compress faster                   -9    compress better\n"
                "%s"
                "  -d     decompress                        -l    list compressed file\n"
                "  -t     test compressed file              -V    display version number\n"
                "  -h     give %s help                    -L    display software license\n%s",
                verbose == 0 ? "" : "  --best compress best (can be slow for big files)\n",
                verbose == 0 ? "more" : "this", verbose == 0 ? "" : "\n");

    fg = con_fg(f, FG_YELLOW);
    con_fprintf(f, "Options:\n");
    fg = con_fg(f, fg);

    con_fprintf(f,
                "  -q     be quiet                          -v    be verbose\n"
                "  -oFILE write output to 'FILE'\n"
                //"  -f     force overwrite of output files and compression of suspicious files\n"
                "  -f     force compression of suspicious files\n"
                "%s%s"
                , (verbose == 0) ? "  -k     keep backup files\n" : ""
#if 1
                , (verbose > 0) ? "  --no-color, --mono, --color, --no-progress   change look\n" : ""
#else
                , ""
#endif
                );

    if (verbose > 0)
    {
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "\nCompression tuning options:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --lzma              try LZMA [slower but tighter than NRV]\n"
                    "  --brute             try all available compression methods & filters [slow]\n"
                    "  --ultra-brute       try even more compression variants [very slow]\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Backup options:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  -k, --backup        keep backup files\n"
                    "  --no-backup         no backup files [default]\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Overlay options:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --overlay=copy      copy any extra data attached to the file [default]\n"
                    "  --overlay=strip     strip any extra data attached to the file [DANGEROUS]\n"
                    "  --overlay=skip      don't compress a file with an overlay\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Options for djgpp2/coff:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --coff              produce COFF output [default: EXE]\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Options for dos/com:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --8086              make compressed com work on any 8086\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Options for dos/exe:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --8086              make compressed exe work on any 8086\n"
                    "  --no-reloc          put no relocations in to the exe header\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Options for dos/sys:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --8086              make compressed sys work on any 8086\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Options for ps1/exe:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --8-bit             uses 8 bit size compression [default: 32 bit]\n"
                    "  --8mib-ram          8 megabyte memory limit [default: 2 MiB]\n"
                    "  --boot-only         disables client/host transfer compatibility\n"
                    "  --no-align          don't align to 2048 bytes [enables: --console-run]\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Options for watcom/le:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --le                produce LE output [default: EXE]\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Options for win32/pe, win64/pe & rtm32/pe:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --compress-exports=0    do not compress the export section\n"
                    "  --compress-exports=1    compress the export section [default]\n"
                    "  --compress-icons=0      do not compress any icons\n"
                    "  --compress-icons=1      compress all but the first icon\n"
                    "  --compress-icons=2      compress all but the first icon directory [default]\n"
                    "  --compress-icons=3      compress all icons\n"
                    "  --compress-resources=0  do not compress any resources at all\n"
                    "  --keep-resource=list    do not compress resources specified by list\n"
                    "  --strip-relocs=0        do not strip relocations\n"
                    "  --strip-relocs=1        strip relocations [default]\n"
                    "\n");
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "Options for linux/elf:\n");
        fg = con_fg(f, fg);
        con_fprintf(f,
                    "  --preserve-build-id     copy .gnu.note.build-id to compressed output\n"
                    "\n");
    }
    // clang-format on

    con_fprintf(f, "file..   executables to (de)compress\n");

    if (verbose > 0) {
        fg = con_fg(f, FG_YELLOW);
        con_fprintf(f, "\nThis version supports:\n");
        fg = con_fg(f, fg);
        show_all_packers(f, verbose);
    } else {
        con_fprintf(f, "\nType '%s --help' for more detailed help.\n", progname);
    }

    con_fprintf(f, "\nUPX comes with ABSOLUTELY NO WARRANTY; "
                   "for details visit https://upx.github.io\n");

#if DEBUG || TESTING
    fg = con_fg(f, FG_RED);
    con_fprintf(f, "\nWARNING: this version is compiled with"
#if DEBUG
                   " -DDEBUG"
#endif
#if TESTING
                   " -DTESTING"
#endif
                   "\n");
    fg = con_fg(f, fg);
#endif

    UNUSED(fg);
}

/*************************************************************************
// license
**************************************************************************/

void show_license(void) {
    FILE *f = con_term;

    show_header();

    // clang-format off
    con_fprintf(f,
        "   This program may be used freely, and you are welcome to\n"
        "   redistribute it under certain conditions.\n"
        "\n"
        "   This program is distributed in the hope that it will be useful,\n"
        "   but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
        "   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
        "   UPX License Agreements for more details.\n"
        "\n"
        "   You should have received a copy of the UPX License Agreements\n"
        "   along with this program; see the files COPYING and LICENSE.\n"
        "   If not, visit one of the following pages:\n"
        "\n"
    );
    int fg = con_fg(f, FG_CYAN);
    con_fprintf(f,
        "        https://upx.github.io\n"
        "        https://www.oberhumer.com/opensource/upx/\n"
    );
    (void) con_fg(f, FG_ORANGE);
    con_fprintf(f,
        "\n"
        "   Markus F.X.J. Oberhumer              Laszlo Molnar\n"
        "   <markus@oberhumer.com>               <ezerotven+github@gmail.com>\n"
    );
    // clang-format on
    fg = con_fg(f, fg);
    UNUSED(fg);
}

/*************************************************************************
// version
**************************************************************************/

void show_version(bool one_line) {
    FILE *fp = stdout;
    const char *v;

#if defined(UPX_VERSION_GIT_DESCRIBE)
    fprintf(fp, "upx %s\n", UPX_VERSION_GIT_DESCRIBE);
#elif defined(UPX_VERSION_GITREV)
    fprintf(fp, "upx %s\n", UPX_VERSION_STRING "-git-" UPX_VERSION_GITREV);
#else
    fprintf(fp, "upx %s\n", UPX_VERSION_STRING);
#endif
    if (one_line)
        return;

#if (WITH_NRV)
    v = upx_nrv_version_string();
    if (v != nullptr && v[0])
        fprintf(fp, "NRV data compression library %s\n", v);
#endif
#if (WITH_UCL)
    v = upx_ucl_version_string();
    if (v != nullptr && v[0])
        fprintf(fp, "UCL data compression library %s\n", v);
#endif
#if (WITH_ZLIB)
    v = upx_zlib_version_string();
    if (v != nullptr && v[0])
        fprintf(fp, "zlib data compression library %s\n", v);
#endif
#if (WITH_LZMA)
    v = upx_lzma_version_string();
    if (v != nullptr && v[0])
        fprintf(fp, "LZMA SDK version %s\n", v);
#endif
#if (WITH_ZSTD)
    v = upx_zstd_version_string();
    if (v != nullptr && v[0])
        fprintf(fp, "zstd data compression library %s\n", v);
#endif
#if !defined(DOCTEST_CONFIG_DISABLE)
    fprintf(fp, "doctest C++ testing framework version %s\n", DOCTEST_VERSION_STR);
#endif
    // clang-format off
    fprintf(fp, "Copyright (C) 1996-2023 Markus Franz Xaver Johannes Oberhumer\n");
    fprintf(fp, "Copyright (C) 1996-2023 Laszlo Molnar\n");
    fprintf(fp, "Copyright (C) 2000-2023 John F. Reiser\n");
    fprintf(fp, "Copyright (C) 2002-2023 Jens Medoch\n");
#if (WITH_ZLIB)
    fprintf(fp, "Copyright (C) 1995" "-2022 Jean-loup Gailly and Mark Adler\n");
#endif
#if (WITH_LZMA)
    fprintf(fp, "Copyright (C) 1999" "-2006 Igor Pavlov\n");
#endif
#if (WITH_ZSTD)
    // see vendor/zstd/LICENSE; main author is Yann Collet
    fprintf(fp, "Copyright (C) 2015" "-2023 Meta Platforms, Inc. and affiliates\n");
#endif
#if !defined(DOCTEST_CONFIG_DISABLE)
    fprintf(fp, "Copyright (C) 2016" "-2021 Viktor Kirilov\n");
#endif
    fprintf(fp, "UPX comes with ABSOLUTELY NO WARRANTY; for details type '%s -L'.\n", progname);
    // clang-format on
}

/* vim:set ts=4 sw=4 et: */
