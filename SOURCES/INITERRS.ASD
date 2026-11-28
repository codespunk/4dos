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
          db        065h, 020h, 06Ch, 061h, 06Eh, 068h, 067h, 072h
          db        069h, 073h, 074h, 063h, 064h, 053h, 066h, 070h
          db        062h, 077h, 075h, 04Dh, 044h, 045h, 046h, 06Bh
          db        06Fh, 094h, 055h, 058h, 05Ah, 076h
          ;
          ; #E_DAEMS  EMS Freigabe fehlgeschlagen
          db        E_DAEMS, 020h, 000h, 017h, 015h, 0F3h, 018h, 092h, 0A8h
          db        051h, 022h, 031h, 002h, 074h, 082h, 0BDh, 074h
          db        058h, 026h
          ; #E_DAXMS  XMS Freigabe fehlgeschlagen
          db        E_DAXMS, 020h, 000h, 01Dh, 015h, 0F3h, 018h, 092h, 0A8h
          db        051h, 022h, 031h, 002h, 074h, 082h, 0BDh, 074h
          db        058h, 026h
          ; #E_XMSMOV XMS Verlagerung fehlgeschlagen
          db        E_XMSMOV, 024h, 000h, 01Dh, 015h, 0F3h, 006h, 052h, 094h
          db        058h, 029h, 014h, 068h, 031h, 002h, 074h, 082h
          db        0BDh, 074h, 058h, 026h
          ; #E_EMSMAP EMS Zuordnungsfehler
          db        E_EMSMAP, 01Bh, 000h, 017h, 015h, 0F3h, 01Eh, 014h, 01Ah
          db        09Eh, 061h, 046h, 08Bh, 010h, 027h, 042h, 090h
          ; #E_EMSMSR EMS Tabelle kann nicht gespeichert/gelesen werden
          db        E_EMSMSR, 03Bh, 000h, 017h, 015h, 0F3h, 004h, 055h, 012h
          db        024h, 042h, 031h, 095h, 066h, 036h, 0ADh, 07Ch
          db        038h, 02Bh, 011h, 02Ah, 0D7h, 029h, 0C0h, 0F2h
          db        082h, 042h, 0B2h, 063h, 013h, 029h, 0E2h, 060h
          ; #E_SWSEEK Swapdatei suchen fehlgeschlagen
          db        E_SWSEEK, 023h, 000h, 0F1h, 035h, 011h, 0E5h, 0C2h, 0A3h
          db        0B1h, 04Dh, 072h, 063h, 010h, 027h, 048h, 02Bh
          db        0D7h, 045h, 082h, 060h
          ; #E_SWWRIT Swapdatei speichern fehlgeschlagen
          db        E_SWWRIT, 026h, 000h, 0F1h, 035h, 011h, 0E5h, 0C2h, 0A3h
          db        0B1h, 012h, 0ADh, 072h, 096h, 031h, 002h, 074h
          db        082h, 0BDh, 074h, 058h, 026h
          ; #E_SWREAD Swapdatei lesen fehlgeschlagen
          db        E_SWREAD, 021h, 000h, 0F1h, 035h, 011h, 0E5h, 0C2h, 0A3h
          db        042h, 0B2h, 063h, 010h, 027h, 048h, 02Bh, 0D7h
          db        045h, 082h, 060h
          ; #E_SWDBAD Disk Swapdatei zerstîrt
          db        E_SWDBAD, 01Eh, 000h, 016h, 0ABh, 019h, 03Fh, 013h, 051h
          db        01Eh, 05Ch, 02Ah, 030h, 0A7h, 029h, 0BCh, 01Bh
          db        09Ch
          ; #E_NOHDL  Datei Handling nicht verfÅgbar
          db        E_NOHDL, 026h, 000h, 016h, 05Ch, 02Ah, 030h, 084h, 056h
          db        0E4h, 0A6h, 083h, 06Ah, 0D7h, 0C3h, 01Fh, 029h
          db        010h, 001h, 088h, 012h, 059h
          ;
          ; #E_MEMDA Fehler bei der Speicherfreigabe
          db        E_MEMDA, 024h, 000h, 018h, 027h, 042h, 093h, 012h, 02Ah
          db        03Eh, 029h, 03Fh, 011h, 02Ah, 0D7h, 029h, 010h
          db        092h, 0A8h, 051h, 022h
          ; #E_NOMEM Zu wenig Speicher
          db        E_NOMEM, 015h, 000h, 01Eh, 014h, 031h, 032h, 06Ah, 083h
          db        0F1h, 012h, 0ADh, 072h, 090h
          ; #E_MODFIT Unzureichender Ladespeicher
          db        E_MODFIT, 022h, 000h, 01Ch, 060h, 0A7h, 014h, 092h, 0ADh
          db        072h, 06Eh, 029h, 030h, 0C4h, 05Eh, 02Bh, 011h
          db        02Ah, 0D7h, 029h
          ; #E_RELOC Adressentabelle fehlt
          db        E_RELOC, 019h, 000h, 001h, 04Eh, 092h, 0BBh, 026h, 0C5h
          db        012h, 024h, 042h, 031h, 002h, 074h, 0C0h
          ; #E_RLSIZE UngÅltige Grî·e der Relokationstabelle
          db        E_RLSIZE, 034h, 000h, 01Ch, 068h, 001h, 084h, 0CAh, 082h
          db        030h, 074h, 091h, 0B0h, 01Eh, 023h, 0E2h, 093h
          db        002h, 052h, 041h, 0A1h, 095h, 0CAh, 01Ah, 06Bh
          db        0C5h, 012h, 024h, 042h
          ; #E_INHERIT Quelle der vererbten Daten fehlt
          db        E_INHERIT, 027h, 000h, 001h, 051h, 042h, 044h, 023h, 0E2h
          db        093h, 01Fh, 029h, 029h, 012h, 0C2h, 063h, 016h
          db        05Ch, 026h, 031h, 002h, 074h, 0C0h
          ; #E_DSKCLS Swapdatei schlie·en fehlgeschlagen
          db        E_DSKCLS, 027h, 000h, 0F1h, 035h, 011h, 0E5h, 0C2h, 0A3h
          db        0BDh, 074h, 0A2h, 001h, 0E2h, 063h, 010h, 027h
          db        048h, 02Bh, 0D7h, 045h, 082h, 060h
          ; #E_DSKDEL Swapdatei lîschen fehlgeschlagen
          db        E_DSKDEL, 024h, 000h, 0F1h, 035h, 011h, 0E5h, 0C2h, 0A3h
          db        041h, 0BBh, 0D7h, 026h, 031h, 002h, 074h, 082h
          db        0BDh, 074h, 058h, 026h
        ;
          db        0FFh, 0             ;end of table
          ;
InitBad   db        BErrLen, "Illegaler Initialisierungsfehler"
BErrLen   equ       $ - InitBad - 1
          ;
@curseg   ends
          ;
          end

