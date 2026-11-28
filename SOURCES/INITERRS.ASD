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
          db        065h, 020h, 06Eh, 06Ch, 072h, 068h, 069h, 061h
          db        067h, 073h, 063h, 064h, 074h, 070h, 053h, 062h
          db        066h, 075h, 077h, 06Fh, 045h, 04Dh, 055h, 05Ah
          db        06Bh, 02Fh, 041h, 046h, 047h, 04Ch
          ;
          ; #E_EMSMAP EMS Zuordnungsfehler
          db        E_EMSMAP, 01Ch, 000h, 016h, 017h, 010h, 031h, 091h, 031h
          db        056h, 0D4h, 013h, 04Ah, 0B1h, 022h, 075h, 026h
          ; #E_EMSMSR EMS Tabelle kann nicht gespeichert/gelesen werden
          db        E_EMSMSR, 03Ah, 000h, 016h, 017h, 010h, 030h, 045h, 091h
          db        012h, 055h, 023h, 01Ah, 094h, 043h, 048h, 0C7h
          db        0E3h, 0A2h, 0BFh, 028h, 0C7h, 026h, 0E1h, 0BAh
          db        025h, 02Bh, 024h, 031h, 042h, 06Dh, 024h
          ; #E_SWSEEK Swapdatei suchen fehlgeschlagen
          db        E_SWSEEK, 023h, 000h, 010h, 014h, 09Fh, 0D9h, 0E2h, 083h
          db        0B1h, 03Ch, 072h, 043h, 012h, 027h, 05Ah, 02Bh
          db        0C7h, 059h, 0A2h, 040h
          ; #E_SWWRIT Swapdatei speichern fehlgeschlagen
          db        E_SWWRIT, 025h, 000h, 010h, 014h, 09Fh, 0D9h, 0E2h, 083h
          db        0BFh, 028h, 0C7h, 026h, 043h, 012h, 027h, 05Ah
          db        02Bh, 0C7h, 059h, 0A2h, 040h
          ;
          ; #E_MEMDA Fehler bei der Speicherfreigabe
          db        E_MEMDA, 024h, 000h, 01Dh, 027h, 052h, 063h, 011h, 028h
          db        03Dh, 026h, 031h, 00Fh, 028h, 0C7h, 026h, 012h
          db        062h, 08Ah, 091h, 012h
          ; #E_NOMEM Zu wenig Speicher
          db        E_NOMEM, 015h, 000h, 019h, 013h, 031h, 042h, 048h, 0A3h
          db        010h, 0F2h, 08Ch, 072h, 060h
          ; #E_MODFIT Unzureichender Ladespeicher
          db        E_MODFIT, 020h, 000h, 018h, 040h, 0A7h, 013h, 062h, 08Ch
          db        072h, 04Dh, 026h, 031h, 0F9h, 0D2h, 0BFh, 028h
          db        0C7h, 026h
          ; #E_RELOC Adressentabelle fehlt
          db        E_RELOC, 018h, 000h, 01Ch, 0D6h, 02Bh, 0B2h, 04Eh, 091h
          db        012h, 055h, 023h, 012h, 027h, 05Eh
          ; #E_RLSIZE UngÅltige Grî·e der Relokationstabelle
          db        E_RLSIZE, 034h, 000h, 018h, 04Ah, 001h, 085h, 0E8h, 0A2h
          db        031h, 0E6h, 004h, 090h, 01Eh, 023h, 0D2h, 063h
          db        002h, 052h, 051h, 051h, 0A9h, 0E8h, 015h, 04Bh
          db        0E9h, 011h, 025h, 052h
        ;
          db        0FFh, 0             ;end of table
          ;
InitBad   db        BErrLen, "Illegaler Initialisierungsfehler"
BErrLen   equ       $ - InitBad - 1
          ;
@curseg   ends
          ;
          end

