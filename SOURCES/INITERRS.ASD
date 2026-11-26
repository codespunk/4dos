          title     INITERRS - 4DOS loader transient portion error messages
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  04/14/89

          This module includes only an error message table.  Routines for
          accessing this table are in ERRORMSG.ASM.

          ******************************************************************
          NOTE:  This text has been compressed and copied from 4DLTERRS.TXT.
          Any changes should be made there, not here!
          ******************************************************************

          } end description

          ;
          ; Includes
          ;
          include   trmac.asm           ;general macros
          .cseg     LOAD                ;set loader segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm        ;loader parameters
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, es:nothing, ss:nothing
          ;
          ;
          ; Fatal errors detected by transient part of loader
          ;
          public    InitErrs
          ;
InitErrs  label     byte
          dw        offset InitBad      ;store offset of bad error number msg
          ;
          ; Translation table
          ;
          db        0DEh
          db        065h, 020h, 068h, 069h, 06Ch, 072h, 061h, 06Eh
          db        063h, 067h, 073h, 064h, 074h, 070h, 053h, 062h
          db        066h, 075h, 077h, 055h, 06Fh, 041h, 046h, 047h
          db        04Ch, 052h, 05Ah, 06Bh, 07Ah, 081h
          ;
          ; #E_SWSEEK Swapdatei suchen fehlgeschlagen
          db        E_SWSEEK, 023h, 000h, 010h, 014h, 08Fh, 0D8h, 0E2h, 053h
          db        0C1h, 03Ah, 042h, 093h, 012h, 024h, 06Bh, 02Ch
          db        0A4h, 068h, 0B2h, 090h
          ; #E_SWWRIT Swapdatei speichern fehlgeschlagen
          db        E_SWWRIT, 025h, 000h, 010h, 014h, 08Fh, 0D8h, 0E2h, 053h
          db        0CFh, 025h, 0A4h, 027h, 093h, 012h, 024h, 06Bh
          db        02Ch, 0A4h, 068h, 0B2h, 090h
          ;
          ; #E_MEMDA Fehler bei der Speicherfreigabe
          db        E_MEMDA, 024h, 000h, 018h, 024h, 062h, 073h, 011h, 025h
          db        03Dh, 027h, 031h, 00Fh, 025h, 0A4h, 027h, 012h
          db        072h, 05Bh, 081h, 012h
          ; #E_NOMEM Zu wenig Speicher
          db        E_NOMEM, 015h, 000h, 01Ch, 013h, 031h, 042h, 095h, 0B3h
          db        010h, 0F2h, 05Ah, 042h, 070h
          ; #E_MODFIT Unzureichender Ladespeicher
          db        E_MODFIT, 01Fh, 000h, 015h, 091h, 0E1h, 037h, 025h, 0A4h
          db        029h, 0D2h, 073h, 01Ah, 08Dh, 02Ch, 0F2h, 05Ah
          db        042h, 070h
          ; #E_RELOC Adressentabelle fehlt
          db        E_RELOC, 018h, 000h, 017h, 0D7h, 02Ch, 0C2h, 09Eh, 081h
          db        012h, 066h, 023h, 012h, 024h, 06Eh
          ; #E_RLSIZE UngÅltige Grî·e der Relokationstabelle
          db        E_RLSIZE, 032h, 000h, 015h, 09Bh, 01Fh, 06Eh, 05Bh, 023h
          db        019h, 070h, 049h, 001h, 0E2h, 03Dh, 027h, 031h
          db        0B2h, 061h, 061h, 0D8h, 0E5h, 016h, 09Ch, 0E8h
          db        011h, 026h, 062h
        ;
          db        0FFh, 0             ;end of table
          ;
InitBad   db        BErrLen, "Illegaler Initialisierungsfehler"
BErrLen   equ       $ - InitBad - 1
          ;
@curseg   ends
          ;
          end

