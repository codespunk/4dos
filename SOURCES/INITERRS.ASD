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
          db        065h, 020h, 06Ch, 061h, 06Eh, 072h, 067h, 068h
          db        069h, 073h, 074h, 063h, 064h, 053h, 066h, 062h
          db        070h, 075h, 077h, 04Dh, 044h, 045h, 046h, 06Bh
          db        06Fh, 055h, 058h, 05Ah, 076h, 07Ah
          ;
          ; #E_DAEMS  EMS Freigabe fehlgeschlagen
          db        E_DAEMS, 020h, 000h, 017h, 015h, 0F3h, 018h, 072h, 0A8h
          db        051h, 012h, 031h, 002h, 094h, 082h, 0BDh, 094h
          db        058h, 026h
          ; #E_DAXMS  XMS Freigabe fehlgeschlagen
          db        E_DAXMS, 020h, 000h, 01Ch, 015h, 0F3h, 018h, 072h, 0A8h
          db        051h, 012h, 031h, 002h, 094h, 082h, 0BDh, 094h
          db        058h, 026h
          ; #E_XMSMOV XMS Verlagerung fehlgeschlagen
          db        E_XMSMOV, 024h, 000h, 01Ch, 015h, 0F3h, 006h, 052h, 074h
          db        058h, 027h, 013h, 068h, 031h, 002h, 094h, 082h
          db        0BDh, 094h, 058h, 026h
          ; #E_EMSMAP EMS Zuordnungsfehler
          db        E_EMSMAP, 01Bh, 000h, 017h, 015h, 0F3h, 01Dh, 013h, 01Ah
          db        07Eh, 061h, 036h, 08Bh, 010h, 029h, 042h, 070h
          ; #E_EMSMSR EMS Tabelle kann nicht gespeichert/gelesen werden
          db        E_EMSMSR, 03Bh, 000h, 017h, 015h, 0F3h, 004h, 055h, 011h
          db        024h, 042h, 031h, 095h, 066h, 036h, 0ADh, 09Ch
          db        038h, 02Bh, 012h, 02Ah, 0D9h, 027h, 0C0h, 0F2h
          db        082h, 042h, 0B2h, 063h, 014h, 027h, 0E2h, 060h
          ; #E_SWSEEK Swapdatei suchen fehlgeschlagen
          db        E_SWSEEK, 023h, 000h, 0F1h, 045h, 012h, 0E5h, 0C2h, 0A3h
          db        0B1h, 03Dh, 092h, 063h, 010h, 029h, 048h, 02Bh
          db        0D9h, 045h, 082h, 060h
          ; #E_SWWRIT Swapdatei speichern fehlgeschlagen
          db        E_SWWRIT, 026h, 000h, 0F1h, 045h, 012h, 0E5h, 0C2h, 0A3h
          db        0B1h, 022h, 0ADh, 092h, 076h, 031h, 002h, 094h
          db        082h, 0BDh, 094h, 058h, 026h
          ; #E_SWREAD Swapdatei lesen fehlgeschlagen
          db        E_SWREAD, 021h, 000h, 0F1h, 045h, 012h, 0E5h, 0C2h, 0A3h
          db        042h, 0B2h, 063h, 010h, 029h, 048h, 02Bh, 0D9h
          db        045h, 082h, 060h
          ; #E_SWDBAD Disk Swapdatei zerstîrt
          db        E_SWDBAD, 01Eh, 000h, 016h, 0ABh, 019h, 03Fh, 014h, 051h
          db        02Eh, 05Ch, 02Ah, 031h, 0F2h, 07Bh, 0C0h, 049h
          db        07Ch
          ; #E_NOHDL  Datei Handling nicht verfÅgbar
          db        E_NOHDL, 026h, 000h, 016h, 05Ch, 02Ah, 030h, 084h, 056h
          db        0E4h, 0A6h, 083h, 06Ah, 0D9h, 0C3h, 01Eh, 027h
          db        010h, 001h, 088h, 011h, 057h
          ;
          ; #E_MEMDA Fehler bei der Speicherfreigabe
          db        E_MEMDA, 024h, 000h, 018h, 029h, 042h, 073h, 011h, 02Ah
          db        03Eh, 027h, 03Fh, 012h, 02Ah, 0D9h, 027h, 010h
          db        072h, 0A8h, 051h, 012h
          ; #E_NOMEM Zu wenig Speicher
          db        E_NOMEM, 015h, 000h, 01Dh, 013h, 031h, 042h, 06Ah, 083h
          db        0F1h, 022h, 0ADh, 092h, 070h
          ; #E_MODFIT Unzureichender Ladespeicher
          db        E_MODFIT, 021h, 000h, 01Bh, 061h, 0F1h, 037h, 02Ah, 0D9h
          db        026h, 0E2h, 073h, 00Ch, 045h, 0E2h, 0B1h, 022h
          db        0ADh, 092h, 070h
          ; #E_RELOC Adressentabelle fehlt
          db        E_RELOC, 019h, 000h, 001h, 04Eh, 072h, 0BBh, 026h, 0C5h
          db        011h, 024h, 042h, 031h, 002h, 094h, 0C0h
          ; #E_RLSIZE UngÅltige Grî·e der Relokationstabelle
          db        E_RLSIZE, 035h, 000h, 01Bh, 068h, 001h, 084h, 0CAh, 082h
          db        030h, 074h, 070h, 049h, 001h, 0E2h, 03Eh, 027h
          db        030h, 025h, 024h, 01Ah, 019h, 05Ch, 0A1h, 0A6h
          db        0BCh, 051h, 012h, 044h, 020h
          ; #E_INHERIT Quelle der vererbten Daten fehlt
          db        E_INHERIT, 027h, 000h, 001h, 051h, 032h, 044h, 023h, 0E2h
          db        073h, 01Eh, 027h, 027h, 011h, 0C2h, 063h, 016h
          db        05Ch, 026h, 031h, 002h, 094h, 0C0h
        ;
          db        0FFh, 0             ;end of table
          ;
InitBad   db        BErrLen, "Illegaler Initialisierungsfehler"
BErrLen   equ       $ - InitBad - 1
          ;
@curseg   ends
          ;
          end

