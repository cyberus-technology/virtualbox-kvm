;-------------------------------------------------------------------------------
; GetServicePack
; Author: Alessio Garbi (e-Project srl) <agarbi@e-project.it>
;
; input:
;			none
; output:
;    	top of stack: last service pack major version
;    	top of stack-1: last service pack minor version
; note:
;			If no service pack installed returns major ver "0" and minor ver "0"
;			Function tested with Win 95, 98SE, NT4, 2000, XP, 2003 (lang ITA and ENG)

!macro GetServicePack un
Function ${un}GetServicePack
	Push $R0
	Push $R1

	ReadRegDWORD $R0 HKLM "System\CurrentControlSet\Control\Windows" "CSDVersion"
	IntOp $R1 $R0 % 256			;get minor version
	IntOp $R0 $R0 / 256			;get major version

	Exch $R1
	Exch
	Exch $R0
FunctionEnd
!macroend
!insertmacro GetServicePack ""
;!insertmacro GetServicePack "un." - unused
