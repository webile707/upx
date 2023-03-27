/* p_sys.cpp -- dos/sys executable format

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
#include "file.h"
#include "filter.h"
#include "packer.h"
#include "p_com.h"
#include "p_sys.h"
#include "linker.h"

static const CLANG_FORMAT_DUMMY_STATEMENT
#include "stub/i086-dos16.sys.h"

/*************************************************************************
//
**************************************************************************/

bool PackSys::canPack() {
    byte buf[128];

    fi->readx(buf, sizeof(buf));
    if (memcmp(buf, "\xff\xff\xff\xff", 4) != 0)
        return false;
    if (!fn_has_ext(fi->getName(), "sys")) // query file name
        return false;
    checkAlreadyPacked(buf, sizeof(buf));
    if (file_size < 1024)
        throwCantPack("file is too small for dos/sys");
    if (file_size > 0x10000)
        throwCantPack("file is too large for dos/sys");
    return true;
}

/*************************************************************************
//
**************************************************************************/

void PackSys::buildLoader(const Filter *ft) {
    initLoader(stub_i086_dos16_sys, sizeof(stub_i086_dos16_sys));
    // clang-format off
    addLoader("SYSMAIN1",
              opt->cpu == opt->CPU_8086 ? "SYSI0861" : "SYSI2861",
              "SYSMAIN2",
              ph.first_offset_found == 1 ? "SYSSBBBP" : "",
              ft->id ? "SYSCALLT" : "",
              "SYSMAIN3,UPX1HEAD,SYSCUTPO,NRV2B160,NRVDDONE,NRVDECO1",
              ph.max_offset_found <= 0xd00 ? "NRVLED00" : "NRVGTD00",
              "NRVDECO2");
    // clang-format on
    if (ft->id) {
        assert(ft->calls > 0);
        addFilter16(ft->id);
    }
    // clang-format off
    addLoader("SYSMAIN5",
              opt->cpu == opt->CPU_8086 ? "SYSI0862" : "SYSI2862",
              "SYSJUMP1");
    // clang-format on
}

void PackSys::patchLoader(OutputFile *fo, byte *loader, int lsize, unsigned calls) {
    const int e_len = getLoaderSectionStart("SYSCUTPO");
    const int d_len = lsize - e_len;
    assert(e_len > 0 && e_len < 128);
    assert(d_len > 0 && d_len < 256);

    if (ph.u_len + d_len + ph.overlap_overhead > 0xfffe)
        throwNotCompressible();

    // use some fields of the original file
    linker->defineSymbol("attribute", get_le16(ibuf + 4));
    linker->defineSymbol("interrupt", get_le16(ibuf + 8));

    unsigned copy_to = ph.u_len + d_len + ph.overlap_overhead;

    linker->defineSymbol("calltrick_calls", calls);
    linker->defineSymbol("copy_source", ph.c_len + lsize - 1);
    linker->defineSymbol("copy_destination", copy_to);
    linker->defineSymbol("neg_e_len", 0 - e_len);
    linker->defineSymbol("NRV2B160", ph.u_len + ph.overlap_overhead + 1);
    linker->defineSymbol("original_strategy", get_le16(ibuf + 6));

    relocateLoader();
    loader = getLoader();

    // some day we could use the relocation stuff for patchPackHeader too..
    patchPackHeader(loader, e_len);
    // write loader + compressed file
    fo->write(loader, e_len);         // entry
    fo->write(obuf, ph.c_len);        // compressed
    fo->write(loader + e_len, d_len); // decompressor
    NO_printf("%-13s: entry        : %8u bytes\n", getName(), e_len);
    NO_printf("%-13s: compressed   : %8u bytes\n", getName(), ph.c_len);
    NO_printf("%-13s: decompressor : %8u bytes\n", getName(), d_len);
}

/* vim:set ts=4 sw=4 et: */
