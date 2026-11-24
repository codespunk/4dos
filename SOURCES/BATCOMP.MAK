!IFNDEF LANGUAGE
LANGUAGE=ENGLISH
!ENDIF

!IF "$(LANGUAGE)" == "ENGLISH"
OBJ=DOS
!ENDIF

!IF "$(LANGUAGE)" == "GERMAN"
OBJ=DOS_D
!ENDIF

CFLAGS=/c /nologo /D__4DOS /D$(LANGUAGE) /Fo$(OBJ)\ /AM /J /Ocs /Gf /Gs /W3 /Zp1

LINKCMD=link /B /NOE /ON:N batcomp+strmenc;

ALL : $(OBJ)\batcomp.exe

$(OBJ)\batcomp.obj : batcomp.c batcomp.h
	cl $(CFLAGS) batcomp.c

$(OBJ)\strmenc.obj : strmenc.c
	cl $(CFLAGS) /NTSHARE_TEXT strmenc.c

$(OBJ)\batcomp.exe : \
	$(OBJ)\batcomp.obj $(OBJ)\strmenc.obj
!INCLUDE DOLINK.MAK        

