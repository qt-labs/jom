!include include1.mk
include include2.mk # old style include directive

INCLUDE = subdir
!	include <include3.mk>
!include "subdir\include4.mk"

includeFoo: # this is not an include directive!

