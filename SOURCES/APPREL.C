

/*
  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  (1) The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  (2) The Software, or any portion of it, may not be compiled for use on any
  operating system OTHER than FreeDOS without written permission from Rex Conn
  <rconn@jpsoft.com>

  (3) The Software, or any portion of it, may not be used in any commercial
  product without written permission from Rex Conn <rconn@jpsoft.com>

  (4) THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/**************************************************************************

   APPREL - Finds relocation table, sorts and compresses it, appends it to
            the end of an EXE file, preserves the original table, and adjusts
            the header and relocation metadata accordingly

   Copyright 1991-1993, J.P. Software Inc., Arlington, MA, USA.  All Rights
   Reserved.

   Author:  Tom Rawson, 3/31/91
            Substantial mods, 7/93

   Usage:
      APPREL [filename] [ignored segments] [-s sizefile]

      filename    name of file to append relocation table to
      -s sizefile measure the relocation payload and generate an assembly
                  include; do not modify the executable.  Relink using
                  the generated size before running APPREL normally.

      If the filename is not given APPREL will prompt for it. An EXE
      extension is assumed unless otherwise specified.

      If an error occurs the message "APPREL failed:  " is printed,
      followed by a specific error message.  See the array errmsg below
      for the individual messages, which are largely self-explanatory.

***************************************************************************/

#include <fcntl.h>
#include <io.h>
#include <sys\types.h>
#include <sys\stat.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <process.h>
#include <string.h>

                           /* size of a relocation table entry */
#define RELOC_SIZE (unsigned int)sizeof(void far *)
                           /* size of EXE header */
#define HEADER_SIZE (unsigned int)sizeof(header)
                           /* size of relocation table elements */
#define MARK_SIZE 4
                           /* loader metadata, serialized little-endian */
#define META_SIG_SIZE 12
#define META_SIZE 20
#define META_OFFSET 12
#define META_PARA 14
#define META_BYTES 16
#define META_CAPACITY 18
#define NB_SIZE (sizeof new_reloc_bytes)
#define SC_SIZE (sizeof seg_cnt)
#define RS_SIZE (sizeof reloc_segs[0])
#define RC_SIZE (sizeof reloc_counts[0])
#define NR_SIZE (sizeof new_reloc[0][0])

                           /* size of read / write buffer */
#define BUF_SIZE 16384
                           /* maximum number of relocation groups */
#define MAX_SEGS 16
                           /* maximum number of entries per group */
#define MAX_SEG_RELOCS 2000
                           /* maximum number of segments to ignore */
#define MAX_IGNORE 16

                           /* make far pointer from segment and offset */
#define MAKEP(seg,off) ((void far *)(((unsigned long)(seg) << 16) | \
   (unsigned int)(off)))

#define ERR_OPEN_IN 1
#define ERR_OPEN_OUT 2
#define ERR_READ_HEAD 3
#define ERR_BAD_EXE 4
#define ERR_SEEK 5
#define ERR_READ 6
#define ERR_READ_CNT 7
#define ERR_WRITE 8
#define ERR_WRITE_CNT 9
#define ERR_CLOSE_IN 10
#define ERR_CLOSE_OUT 11
#define ERR_DEL_BACKUP 12
#define ERR_RENAME_OLD 13
#define ERR_RENAME_NEW 14
#define ERR_IGN_OVER 15
#define ERR_RELOC_SIZE 16
#define ERR_SEG_CNT 17
#define ERR_SEG_RELOCS 18
#define ERR_METADATA 19
#define ERR_CAPACITY 20
#define ERR_SIZEFILE 21
#define ERR_MAX 21
#define ERR_UNKNOWN 22

const char *errmsg[] = {
   "Input file open error",                  /* 1 */
   "Output file open error",                 /* 2 */
   "Header read error",                      /* 3 */
   "Not an EXE file",                        /* 4 */
   "File seek error",                        /* 5 */
   "Input file read error",                  /* 6 */
   "Input file read byte count error",       /* 7 */
   "Output file write error",                /* 8 */
   "Output file write byte count error",     /* 9 */
   "Input file close error",                 /* 10 */
   "Output file close error",                /* 11 */
   "Backup file deletion error",             /* 12 */
   "Old EXE file rename error",              /* 13 */
   "New EXE file rename error",              /* 14 */
   "Too many ignored segments",              /* 15 */
   "Relocation table too large",             /* 16 */
   "Too many relocation groups",             /* 17 */
   "Too many relocations in group",        /* 18 */
   "Missing, duplicate or invalid relocation metadata", /* 19 */
   "Relocation reservation differs from payload; measure and relink", /* 20 */
   "Cannot write relocation size include",   /* 21 */
   "Unknown internal error"                  /* 22 */
};

   /* variables */
char buffer[BUF_SIZE];                    /* read / write buffer */
char filename[80],                        /* input file name */
     outname[80],                         /* output file name */
     backname[80];                        /* backup file name */
char mark[MARK_SIZE + 1] = "ENDP";        /* marker for relocation table */
static const unsigned char meta_sig[META_SIG_SIZE] = {
   '4', 'D', 'O', 'S', 'R', 'E', 'L', 'O', 'C', 1, 0xA5, 0x5A
};
static unsigned char metadata[META_SIZE];
static long meta_pos;
unsigned int reloc_segs[MAX_SEGS];        /* segment of each relocation group */
unsigned int reloc_counts[MAX_SEGS];      /* entry counts for each segment */
unsigned int new_reloc[MAX_SEGS][MAX_SEG_RELOCS];  /* reorganized relocation table */
unsigned int ignore_segs[MAX_IGNORE];     /* segments to ignore */
int in,                                   /* input exe file handle */
    out;                                  /* output exe file handle */
unsigned int seg_cnt = 0,                 /* segment of each relocation group */
    ignore_cnt = 0,                       /* segments to ignore */
    new_reloc_cnt = 0,                    /* entries in new reloc table */
    new_reloc_bytes;                      /* bytes in new reloc table */
long original_end,                        /* original EOF offset */
    exe_len,                              /* length of EXE portion */
    new_exe_len,                          /* expanded length of file */
    reloc_len,                            /* length of reloc table */
    new_reloc_len;                        /* full length of new reloc table including mark etc. */

struct exe_head {                         /* exe file header */
   char           sigM, sigZ;             /* signature "MZ" */
   unsigned int   last_page_len,          /* last page length */
                  file_pages,             /* pages in file */
                  reloc_cnt,              /* relocation item count */
                  head_para_cnt,          /* paragraphs in header */
                  extra_para_min,         /* minimum space beyond stack */
                  extra_para_max;         /* maximum space beyond stack */
   char far       *stack_init;            /* initial SS:SP */
   unsigned int   checksum;               /* module checksum */
   int            (far *entry)();         /* initial CS:IP */
   unsigned int   reloc_start,            /* relocation table offset */
                  overlay_num;            /* overlay number */
} header;


   /* local prototypes */
void copy_part(unsigned long start_offset, long count);
void write_part(void *buf, int count);
void error(unsigned int errnum);
static void find_metadata(long image_start);
static unsigned int get_word(const unsigned char *p);
static void put_word(unsigned char *p, unsigned int value);
static void write_sizefile(const char *name);


main(int argc, char **argv)
{

   unsigned int *rtptr;
   unsigned int ign_seg, rcnt, scnt;
   unsigned int chunk_entries, chunk_bytes;
   int ign_flag;
   const char *sizefile = NULL;
   long image_start, image_offset;
   unsigned long payload;

   // sign on and parse command line

   printf("\nAPPREL 1.11  Copyright 1991-1995, JP Software Inc., All Rights Reserved.\n");

   // get file name if not there
   if (--argc > 0)
      strcpy(filename, *(++argv));
   else {
      printf("\nEnter file name: ");
      scanf("%s", filename);
   }

   // scan for segments to ignore
   while (--argc > 0) {
      ++argv;
      if (strcmp(*argv, "-s") == 0) {
         if (sizefile != NULL || --argc <= 0)
            error(ERR_SIZEFILE);
         sizefile = *(++argv);
         continue;
      }
      if (sscanf(*argv, "%x", &ign_seg) != 1)
         break;
      if (ignore_cnt >= MAX_IGNORE)
         error(ERR_IGN_OVER);
      ignore_segs[ignore_cnt++] = ign_seg;
   }

//    for (scnt = 0; scnt < ignore_cnt; scnt++)
//       printf("Ignore segment %d is %.04X\n", scnt, ignore_segs[scnt]);

   // fix up file names with proper extensions etc.
   if (strrchr(filename,'.') == NULL)
      strcpy(&filename[strlen(filename)], ".EXE");    /* add EXE extension */
   strcpy(outname, filename);                         /* copy name */
   strcpy(strrchr(outname,'.'), ".EX$");              /* add EX$ extension */
   strcpy(backname, filename);                        /* copy name */
   strcpy(strrchr(backname,'.'), ".BAK");             /* add BAK extension */

   // open input file, read header, check for "MZ" signature
   if ((in = open(filename, O_RDONLY | O_BINARY)) < 0)
      error(ERR_OPEN_IN);
   if (read(in, (char *)&header, HEADER_SIZE) != HEADER_SIZE)
      error(ERR_READ_HEAD);
   if (header.sigM != 'M' || header.sigZ != 'Z')
      error(ERR_BAD_EXE);

   // Determine the declared executable extent, excluding overlays/debug data.
   if ((original_end = lseek(in, 0L, SEEK_END)) == -1L)
      error(ERR_SEEK);
   if (header.file_pages == 0 || header.last_page_len >= 512)
      error(ERR_BAD_EXE);
   exe_len = ((long)header.file_pages * 512L) -
      (long)((header.last_page_len == 0) ? 0 : (512 - header.last_page_len));
   image_start = (long)header.head_para_cnt * 16L;
   if (image_start < HEADER_SIZE || image_start > exe_len ||
       exe_len > original_end || header.reloc_start < HEADER_SIZE ||
       (long)header.reloc_start + (long)header.reloc_cnt * RELOC_SIZE > image_start)
      error(ERR_BAD_EXE);

   // check count
   if (header.reloc_cnt == 0) {
      printf("\nNo relocation table entries present, APPREL aborted\n");
      return 1;
   } else
      printf("\n%u relocation entries found\n", header.reloc_cnt);

   // Seek to the original relocation table.
   // Use long arithmetic to avoid 16-bit multiplication overflow.
   reloc_len = (long)header.reloc_cnt * RELOC_SIZE;
   if (lseek(in, (unsigned long)(header.reloc_start), SEEK_SET) == -1L)
      error(ERR_SEEK);

   // Read complete relocation entries in buffer-sized chunks.
   // Preserve the original entry order and segment grouping.
   chunk_entries = 0;
   for (rcnt = header.reloc_cnt; rcnt > 0; rcnt--) {
      if (chunk_entries == 0) {
         chunk_entries = (rcnt > BUF_SIZE / RELOC_SIZE)
            ? BUF_SIZE / RELOC_SIZE : rcnt;
         chunk_bytes = chunk_entries * RELOC_SIZE;
         if (read(in, buffer, (int)chunk_bytes) != (int)chunk_bytes)
            error(ERR_READ);
         rtptr = (unsigned int *)(&buffer);
      }
      chunk_entries--;

// printf("Entry %.04X:%.04X -- ", rtptr[1], rtptr[0]);
      // see if this entry is in the ignore list
      ign_flag = 0;
      for (scnt = 0; scnt < ignore_cnt; scnt++) {
         if (rtptr[1] == ignore_segs[scnt]) {
            ign_flag++;
// printf("ignored\n");
            break;
         }
      }

      // if we aren't ignoring this entry, add it to the new table
      if (ign_flag == 0) {

         // Find a group for this segment that still has room.
         // Reloc processes groups independently, so a segment may appear
         // more than once without changing the serialized format.
         for (scnt = 0; scnt < seg_cnt; scnt++) {
            if (rtptr[1] == reloc_segs[scnt] &&
                reloc_counts[scnt] < MAX_SEG_RELOCS)
               break;
         }

         // Start another group if the segment is new or all its groups are full.
         if (scnt == seg_cnt) {
// printf("NEW segment %d -- ", scnt);
            if (seg_cnt >= MAX_SEGS)
               error(ERR_SEG_CNT);
            reloc_segs[seg_cnt++] = rtptr[1];
         }
// else printf("old segment %d -- ", scnt);

         // now add this entry to the group
         if (reloc_counts[scnt] >= MAX_SEG_RELOCS) {
            printf("\nError on segment %.04X", reloc_segs[scnt]);
            error(ERR_SEG_RELOCS);
         }
// printf("adding as entry %d\n", reloc_counts[scnt]);
         new_reloc[scnt][(reloc_counts[scnt]++)] = rtptr[0];
         new_reloc_cnt++;

      }

      // point to next entry
      rtptr += 2;

   }

   printf("%u relocation entries copied\n", new_reloc_cnt);

   // calculate length of all elements of new table
   payload = SC_SIZE + (unsigned long)seg_cnt * (RS_SIZE + RC_SIZE);
   for (scnt = 0; scnt < seg_cnt; scnt++) {
      payload += (unsigned long)reloc_counts[scnt] * NR_SIZE;
   }
   // A normalized far pointer can begin at offset 15.  Keep the entire
   // serialized table in one 64 KiB window, including ENDP and length.
   if (payload > 65536UL - 15UL - MARK_SIZE - NB_SIZE)
      error(ERR_RELOC_SIZE);
   new_reloc_bytes = (unsigned int)payload;
   new_reloc_len = (long)MARK_SIZE + NB_SIZE + new_reloc_bytes;

   find_metadata(image_start);
   printf("%u relocation bytes required\n", new_reloc_bytes);
   if (sizefile != NULL) {
      write_sizefile(sizefile);
      if (close(in) < 0)
         error(ERR_CLOSE_IN);
      return 0;
   }
   if (get_word(metadata + META_CAPACITY) != new_reloc_bytes) {
      printf("Linked reservation: %u bytes\n", get_word(metadata + META_CAPACITY));
      error(ERR_CAPACITY);
   }

   // Encode the normalized load-relative address (excluding MZ header).
   image_offset = exe_len - image_start;
   if (image_offset + new_reloc_len > 0xFFFF0L)
      error(ERR_RELOC_SIZE);
   put_word(metadata + META_OFFSET, (unsigned int)(image_offset & 15L));
   put_word(metadata + META_PARA, (unsigned int)(image_offset >> 4));
   put_word(metadata + META_BYTES, new_reloc_bytes);

//    printf("%d total segments \n\n", seg_cnt);
//    for (scnt = 0; scnt < seg_cnt; scnt++) {
//       printf("Segment %d is %.04X, with %d entries:\n", scnt, reloc_segs[scnt], reloc_counts[scnt]);
//       for (rcnt = 0; rcnt < reloc_counts[scnt]; rcnt++)
//          printf("    %.04X", new_reloc[scnt][rcnt]);
//       printf("\n");
//    }

//    for (scnt = 0; scnt < seg_cnt; scnt++) {
//       for (rcnt = 0; rcnt < reloc_counts[scnt]; rcnt++)
//          printf("%.04X:%.04X\n", reloc_segs[scnt], new_reloc[scnt][rcnt]);
//    }

   // open output file
	if ((out = open(outname, (O_CREAT | O_TRUNC | O_RDWR | O_BINARY),
         (S_IREAD | S_IWRITE))) < 0)
      error(ERR_OPEN_OUT);

   // Adjust header lengths for relocation table info removed at start of
   // file, and new relocation table added to end of file.  This gets the
   // header adjusted before we write it, avoiding a rewrite.

//   new_exe_len = exe_len - reloc_len + new_reloc_len;
   new_exe_len = exe_len + new_reloc_len;
//   header.reloc_cnt = 0;
   header.file_pages = (unsigned int)((new_exe_len + 511L) / 512L);
   header.last_page_len = (unsigned int)(new_exe_len % 512L);
// printf("Old len %lX, new len %lX, pages %X, leftover %X\n", exe_len, new_exe_len, header.file_pages, header.last_page_len);

   // copy the new header and the EXE portion of the file (skipping the old
   // relocation table)
   if (write(out, (char *)&header, HEADER_SIZE) != HEADER_SIZE)
      error(ERR_WRITE);
//   copy_part(((unsigned long)HEADER_SIZE + reloc_len), exe_len - HEADER_SIZE - reloc_len);
   // Patch the record while copying the image; do not modify the input.
   copy_part((unsigned long)HEADER_SIZE, meta_pos - HEADER_SIZE);
   write_part(metadata, META_SIZE);
   copy_part((unsigned long)(meta_pos + META_SIZE),
      exe_len - meta_pos - META_SIZE);

   // Write the mark, the total and segment counts, and the segments, counts,
   // and data for the new relocation table
   write_part(&mark, MARK_SIZE);
   write_part(&new_reloc_bytes, NB_SIZE);
   write_part(&seg_cnt, SC_SIZE);
   for (scnt = 0; scnt < seg_cnt; scnt++) {
      write_part(&(reloc_segs[scnt]), RS_SIZE);
      write_part(&(reloc_counts[scnt]), RC_SIZE);
      write_part(&(new_reloc[scnt][0]), (reloc_counts[scnt] * NR_SIZE));
   }

   // copy the rest of the file (debugger info etc. beyond the EXE portion)
   copy_part(exe_len, (original_end - exe_len));


   // delete any backup file, rename the old file to the backup name, and
   // rename the new file to an EXE
   if (close(in) < 0)
      error(ERR_CLOSE_IN);
   if (close(out) < 0)
      error(ERR_CLOSE_OUT);
   if ((unlink(backname) != 0) && (errno != ENOENT)) // file not found is OK
      error(ERR_DEL_BACKUP);
   if (rename(filename,backname) != 0)
      error(ERR_RENAME_OLD);
   if (rename(outname,filename) != 0)
      error(ERR_RENAME_NEW);

   return 0;                              /* all done */
}


// Read and write the metadata words explicitly, independent of packing.
static unsigned int get_word(const unsigned char *p) {
   return (unsigned int)p[0] | ((unsigned int)p[1] << 8);
}

static void put_word(unsigned char *p, unsigned int value) {
   p[0] = (unsigned char)value;
   p[1] = (unsigned char)(value >> 8);
}

// Scan only the declared load image, overlapping blocks so that records
// crossing a buffer boundary are not missed.  A processed image is rejected.
static void find_metadata(long image_start) {
   long pos = image_start;
   unsigned int count, i;

   meta_pos = -1L;
   while (exe_len - pos >= META_SIZE) {
      count = (exe_len - pos > BUF_SIZE) ? BUF_SIZE :
         (unsigned int)(exe_len - pos);
      if (lseek(in, pos, SEEK_SET) == -1L)
         error(ERR_SEEK);
      if (read(in, buffer, count) != (int)count)
         error(ERR_READ_CNT);
      for (i = 0; i <= count - META_SIZE; i++) {
         if (memcmp(buffer + i, meta_sig, META_SIG_SIZE) == 0) {
            if (meta_pos != -1L)
               error(ERR_METADATA);
            meta_pos = pos + i;
            memcpy(metadata, buffer + i, META_SIZE);
         }
      }
      pos += count - META_SIZE + 1;
   }
   if (meta_pos == -1L || get_word(metadata + META_OFFSET) != 0 ||
       get_word(metadata + META_PARA) != 0 ||
       get_word(metadata + META_BYTES) != 0 ||
       get_word(metadata + META_CAPACITY) < SC_SIZE)
      error(ERR_METADATA);
}

// Generate the exact reservation for both copies of SERVDATA.ASM.
static void write_sizefile(const char *name) {
   FILE *fp;
   int failed;

   if ((fp = fopen(name, "w")) == NULL)
      error(ERR_SIZEFILE);
   failed = (fprintf(fp, "; Generated by APPREL -s; do not edit.\n"
      "RELOCSIZE equ %u\n", new_reloc_bytes) < 0);
   if (fclose(fp) != 0 || failed)
      error(ERR_SIZEFILE);
}

// Copy part of the input file to the output file, beginning at location
// start_offset in the input file and at the current position in the
// output file

void copy_part(unsigned long start_offset, long count) {

   int   bytes_req,                       /* bytes requested on read */
         bytes_read,                      /* bytes actually read */
         bytes_written;                   /* bytes actually written */

   if (lseek(in, start_offset, SEEK_SET) == -1L)
      error(ERR_SEEK);
   for ( ; count > 0; count -= (unsigned long)bytes_read) {
      bytes_req = (int)((count > (long)BUF_SIZE) ? BUF_SIZE : (int)count);
      if ((bytes_read = read(in, buffer, bytes_req)) <= 0)
         error(ERR_READ);
      if (bytes_read != bytes_req)
         error(ERR_READ_CNT);
      if ((bytes_written = write(out, buffer, bytes_read)) < 0)
         error(ERR_WRITE);
      if (bytes_written != bytes_read)
         error(ERR_WRITE_CNT);
   }
}


// Write some information to the output file from a buffer

void write_part(void *buf, int count) {

   int bytes_written;                     /* bytes actually written */

   if ((bytes_written = write(out, buf, count)) < 0)
      error(ERR_WRITE);
   if (bytes_written != count)
      error(ERR_WRITE_CNT);
}


// Handle internal errors
void error(unsigned int errnum) {
   if (errnum > ERR_MAX)
      errnum = ERR_UNKNOWN;
   printf("\nAPPREL failed:  %s\n", errmsg[errnum - 1]);
   exit(1);
}
