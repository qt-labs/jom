!include $(NOT_DEFINED)include1.mk
include include2.mk # old style include directive

INCLUDE = $(INCLUDE);subdir
!	include <include3.mk>
!include "subdir\include4.mk"
!include <"include9.mk">

includeFoo: # this is not an include directive!

