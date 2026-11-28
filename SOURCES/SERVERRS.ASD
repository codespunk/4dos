          ; Translated by Fritz 04-14-92  corr. 10.9.1992


          title     SERVERRS - 4DOS high-memory server error messages
          ;
          page      ,132                ;make wide listing
          ;
          comment   }

          Copyright 1989, 1990, J.P. Software, All Rights Reserved

          Author:  Tom Rawson  04/14/89

          This module includes only an error message table.  Routines for
          accessing this table are in ERRORMSG.ASM.

          ******************************************************************
          NOTE:  This text has been compressed and copied from SERVERRS.TXT.
          Any changes should be made there, not here!
          ******************************************************************

          } end description

          ;
          ; Includes
          ;
          include   trmac.asm           ;general macros
          .cseg     SERV                ;set server segment if not defined
                                        ;  externally
          include   model.inc           ;Spontaneous Assembly memory models
          include   4dlparms.asm        ;loader parameters
          ;
          ;
          .defcode  ,1                  ;set up code segment, no DGROUP
          ;
          assume    cs:@curseg, ds:nothing, ss:nothing, es:nothing
          ;
          ;
          ; Fatal errors detected by resident part of loader
          ;
          public    ServErrs
          ;
ServErrs  label     byte
          dw        offset ServBad      ;store offset of bad error number msg
          ;
          ; Translation table
          ;
          db        0DEh
          db        065h, 020h, 06Ch, 061h, 06Eh, 067h, 068h, 072h
          db        069h, 073h, 074h, 053h, 063h, 066h, 070h, 06Fh
          db        075h, 062h, 054h, 064h, 077h, 045h, 049h, 02Ah
          db        02Eh, 04Dh, 04Eh, 04Fh, 044h, 046h
          ;
          ; #E_TFREE  Speicherfreigabe fehlgeschlagen
          db        E_TFREE, 021h, 000h, 0D1h, 002h, 0AEh, 082h, 09Fh, 092h
          db        0A7h, 051h, 032h, 03Fh, 028h, 047h, 02Bh, 0E8h
          db        045h, 072h, 060h
          ; #E_TALLOC Speicherbelegung fehlgeschlagen
          db        E_TALLOC, 022h, 000h, 0D1h, 002h, 0AEh, 082h, 091h, 032h
          db        042h, 071h, 026h, 073h, 0F2h, 084h, 072h, 0BEh
          db        084h, 057h, 026h
          ; #E_QROOT  Versuch, die primÑre Shell zu verlassen
          db        E_QROOT, 037h, 000h, 006h, 052h, 09Bh, 012h, 0E8h, 00Ch
          db        023h, 015h, 0A2h, 031h, 009h, 0A0h, 0D6h, 004h
          db        089h, 023h, 0D8h, 024h, 043h, 00Ah, 071h, 023h
          db        006h, 072h, 094h, 05Bh, 0B2h, 060h
          ; #E_DAEMS  EMS Freigabe fehlgeschlagen
          db        E_DAEMS, 01Fh, 000h, 017h, 01Bh, 0D3h, 01Fh, 092h, 0A7h
          db        051h, 032h, 03Fh, 028h, 047h, 02Bh, 0E8h, 045h
          db        072h, 060h
          ; #E_DAXMS  XMS Freigabe fehlgeschlagen
          db        E_DAXMS, 020h, 000h, 008h, 051h, 0BDh, 031h, 0F9h, 02Ah
          db        075h, 013h, 023h, 0F2h, 084h, 072h, 0BEh, 084h
          db        057h, 026h
          ; #E_XMSMOV XMS Verlagerung fehlgeschlagen
          db        E_XMSMOV, 024h, 000h, 008h, 051h, 0BDh, 030h, 065h, 029h
          db        045h, 072h, 091h, 026h, 073h, 0F2h, 084h, 072h
          db        0BEh, 084h, 057h, 026h
          ; #E_EMSMAP EMS Zuordnungsfehler
          db        E_EMSMAP, 01Ch, 000h, 017h, 01Bh, 0D3h, 00Ah, 051h, 021h
          db        019h, 015h, 061h, 026h, 07Bh, 0F2h, 084h, 029h
          ; #E_EMSMSR EMS Tabelle kann nicht gespeichert/gelesen werden
          db        E_EMSMSR, 03Ch, 000h, 017h, 01Bh, 0D3h, 014h, 051h, 032h
          db        044h, 023h, 00Bh, 065h, 066h, 036h, 0AEh, 08Ch
          db        037h, 02Bh, 010h, 02Ah, 0E8h, 029h, 0C0h, 0F2h
          db        072h, 042h, 0B2h, 063h, 016h, 029h, 015h, 026h
          ; #E_SWSEEK Swapdatei suchen fehlgeschlagen
          db        E_SWSEEK, 023h, 000h, 0D1h, 065h, 010h, 015h, 05Ch, 02Ah
          db        03Bh, 012h, 0E8h, 026h, 03Fh, 028h, 047h, 02Bh
          db        0E8h, 045h, 072h, 060h
          ; #E_SWWRIT Swapdatei speichern fehlgeschlagen
          db        E_SWWRIT, 026h, 000h, 0D1h, 065h, 010h, 015h, 05Ch, 02Ah
          db        03Bh, 010h, 02Ah, 0E8h, 029h, 063h, 0F2h, 084h
          db        072h, 0BEh, 084h, 057h, 026h
          ; #E_SWREAD Swapdatei lesen fehlgeschlagen
          db        E_SWREAD, 021h, 000h, 0D1h, 065h, 010h, 015h, 05Ch, 02Ah
          db        034h, 02Bh, 026h, 03Fh, 028h, 047h, 02Bh, 0E8h
          db        045h, 072h, 060h
          ; #E_SWDBAD Disk Swapdatei zerstîrt
          db        E_SWDBAD, 021h, 000h, 01Eh, 0ABh, 00Bh, 063h, 0D1h, 065h
          db        010h, 015h, 05Ch, 02Ah, 030h, 0A7h, 029h, 0BCh
          db        004h, 099h, 0C0h
          ; #E_NOHDL  Datei Handling nicht verfÅgbar
          db        E_NOHDL, 027h, 000h, 01Eh, 05Ch, 02Ah, 030h, 084h, 056h
          db        015h, 04Ah, 067h, 036h, 0AEh, 08Ch, 030h, 067h
          db        029h, 0F0h, 018h, 071h, 035h, 090h
          ;
          ; 4DOS signon messages
          ;
          ; #244 Copyright 1988-1992  JP Software Inc. & Computer Solutions Software GmbH
          db        244, 07Dh, 000h, 003h, 041h, 011h, 000h, 097h, 09Ah
          db        078h, 0C3h, 001h, 030h, 093h, 008h, 030h, 083h
          db        00Dh, 020h, 013h, 009h, 030h, 093h, 002h, 033h
          db        030h, 0A4h, 000h, 053h, 0D1h, 01Fh, 0C1h, 065h
          db        092h, 031h, 086h, 0E1h, 0A3h, 006h, 023h, 003h
          db        041h, 010h, 0D6h, 010h, 012h, 0C2h, 093h, 0D1h
          db        014h, 012h, 0CAh, 011h, 06Bh, 03Dh, 011h, 0FCh
          db        016h, 059h, 023h, 007h, 040h, 0D6h, 013h, 008h
          db        040h
          ; #245 Alle Rechte vorbehalten.  Vertrieb unter Lizenz von JP Software.
          db        245, 05Dh, 000h, 001h, 044h, 042h, 030h, 025h, 02Eh
          db        08Ch, 023h, 006h, 071h, 019h, 013h, 028h, 054h
          db        0C2h, 061h, 0A3h, 030h, 065h, 029h, 0C9h, 0A2h
          db        013h, 031h, 026h, 0C2h, 093h, 00Ch, 04Ah, 00Ah
          db        072h, 060h, 0A7h, 030h, 067h, 011h, 063h, 00Ah
          db        040h, 005h, 03Dh, 011h, 0FCh, 016h, 059h, 021h
          db        0A0h
          ifndef    SECURE
          ; #248 *** BETA TEST VERSION.  CONFIDENTIAL, NOT FOR DISTRIBUTION. ***
          db        248, 07Eh, 000h, 019h, 019h, 019h, 030h, 024h, 017h
          db        014h, 001h, 043h, 014h, 017h, 0D1h, 043h, 006h
          db        051h, 070h, 025h, 0D1h, 081h, 0D1h, 0C1h, 0A3h
          db        030h, 034h, 01Dh, 01Ch, 01Fh, 018h, 01Eh, 017h
          db        01Ch, 014h, 018h, 001h, 040h, 0C4h, 00Ch, 023h
          db        01Ch, 01Dh, 014h, 031h, 0F1h, 0D0h, 025h, 031h
          db        0E1h, 08Dh, 014h, 002h, 051h, 080h, 024h, 005h
          db        051h, 041h, 081h, 0D1h, 0C1h, 0A3h, 019h, 019h
          db        019h
          endif
          ;
          db        0FFh, 0             ;end of table
          ;
ServBad   db        BErrLen, "Illegaler Ladefehler"
BErrLen   equ       $ - ServBad - 1
          ;
@curseg   ends
          ;
          end

