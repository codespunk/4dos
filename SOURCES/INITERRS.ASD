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
          db        065h, 020h, 06Ch, 06Eh, 072h, 061h, 068h, 069h
          db        067h, 074h, 073h, 063h, 064h, 053h, 066h, 070h
          db        062h, 075h, 077h, 044h, 04Dh, 06Bh, 06Fh, 045h
          db        055h, 05Ah, 076h, 07Ah, 081h, 094h
          ;
          ; #E_XMSMOV XMS Verlagerung fehlgeschlagen
          db        E_XMSMOV, 025h, 000h, 008h, 051h, 06Fh, 030h, 065h, 026h
          db        047h, 0A2h, 061h, 035h, 0A3h, 010h, 028h, 04Ah
          db        02Ch, 0D8h, 047h, 0A2h, 050h
          ; #E_EMSMAP EMS Zuordnungsfehler
          db        E_EMSMAP, 01Bh, 000h, 019h, 016h, 0F3h, 01Bh, 013h, 018h
          db        06Eh, 051h, 035h, 0ACh, 010h, 028h, 042h, 060h
          ; #E_EMSMSR EMS Tabelle kann nicht gespeichert/gelesen werden
          db        E_EMSMSR, 03Bh, 000h, 019h, 016h, 0F3h, 004h, 057h, 012h
          db        024h, 042h, 031h, 077h, 055h, 035h, 09Dh, 08Bh
          db        03Ah, 02Ch, 011h, 029h, 0D8h, 026h, 0B0h, 0F2h
          db        0A2h, 042h, 0C2h, 053h, 014h, 026h, 0E2h, 050h
          ; #E_SWSEEK Swapdatei suchen fehlgeschlagen
          db        E_SWSEEK, 023h, 000h, 0F1h, 047h, 011h, 0E7h, 0B2h, 093h
          db        0C1h, 03Dh, 082h, 053h, 010h, 028h, 04Ah, 02Ch
          db        0D8h, 047h, 0A2h, 050h
          ; #E_SWWRIT Swapdatei speichern fehlgeschlagen
          db        E_SWWRIT, 026h, 000h, 0F1h, 047h, 011h, 0E7h, 0B2h, 093h
          db        0C1h, 012h, 09Dh, 082h, 065h, 031h, 002h, 084h
          db        0A2h, 0CDh, 084h, 07Ah, 025h
          ; #E_SWREAD Swapdatei lesen fehlgeschlagen
          db        E_SWREAD, 021h, 000h, 0F1h, 047h, 011h, 0E7h, 0B2h, 093h
          db        042h, 0C2h, 053h, 010h, 028h, 04Ah, 02Ch, 0D8h
          db        047h, 0A2h, 050h
          ; #E_SWDBAD Disk Swapdatei zerstîrt
          db        E_SWDBAD, 01Dh, 000h, 015h, 09Ch, 017h, 03Fh, 014h, 071h
          db        01Eh, 07Bh, 029h, 031h, 0D2h, 06Ch, 0B1h, 0F6h
          db        0B0h
          ; #E_NOHDL  Datei Handling nicht verfÅgbar
          db        E_NOHDL, 025h, 000h, 015h, 07Bh, 029h, 030h, 084h, 075h
          db        0E4h, 095h, 0A3h, 059h, 0D8h, 0B3h, 01Ch, 026h
          db        010h, 01Eh, 0A1h, 027h, 060h
          ;
          ; #E_MEMDA Fehler bei der Speicherfreigabe
          db        E_MEMDA, 025h, 000h, 006h, 042h, 084h, 026h, 031h, 022h
          db        093h, 0E2h, 063h, 0F1h, 012h, 09Dh, 082h, 061h
          db        006h, 029h, 0A7h, 012h, 020h
          ; #E_NOMEM Zu wenig Speicher
          db        E_NOMEM, 015h, 000h, 01Bh, 013h, 031h, 042h, 059h, 0A3h
          db        0F1h, 012h, 09Dh, 082h, 060h
          ; #E_MODFIT Unzureichender Ladespeicher
          db        E_MODFIT, 021h, 000h, 01Ah, 051h, 0D1h, 036h, 029h, 0D8h
          db        025h, 0E2h, 063h, 00Ch, 047h, 0E2h, 0C1h, 012h
          db        09Dh, 082h, 060h
          ; #E_RELOC Adressentabelle fehlt
          db        E_RELOC, 019h, 000h, 001h, 04Eh, 062h, 0CCh, 025h, 0B7h
          db        012h, 024h, 042h, 031h, 002h, 084h, 0B0h
          ; #E_RLSIZE UngÅltige Grî·e der Relokationstabelle
          db        E_RLSIZE, 033h, 000h, 01Ah, 05Ah, 01Eh, 04Bh, 09Ah, 023h
          db        007h, 046h, 01Fh, 001h, 0E2h, 03Eh, 026h, 030h
          db        025h, 024h, 018h, 017h, 07Bh, 091h, 085h, 0CBh
          db        071h, 022h, 044h, 020h
          ; #E_INHERIT Quelle der vererbten Daten fehlt
          db        E_INHERIT, 027h, 000h, 001h, 051h, 032h, 044h, 023h, 0E2h
          db        063h, 01Ch, 026h, 026h, 012h, 0B2h, 053h, 015h
          db        07Bh, 025h, 031h, 002h, 084h, 0B0h
        ;
          db        0FFh, 0             ;end of table
          ;
InitBad   db        BErrLen, "Illegaler Initialisierungsfehler"
BErrLen   equ       $ - InitBad - 1
          ;
@curseg   ends
          ;
          end

