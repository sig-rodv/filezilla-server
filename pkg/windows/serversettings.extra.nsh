Var V_ServerSettings_Admin_PortNumber
Var V_ServerSettings_Admin_Password
Var V_ServerSettings_Admin_Password2

Function fnc_ServerSettings_Init
	StrCpy $V_ServerSettings_Admin_PortNumber "14148"
FunctionEnd

Function fnc_ServerSettings_OnCreate
	${NSD_OnBack} fnc_ServerSettings_Remember

	${NSD_SetText} $hCtl_ServerSettings_Admin_Password1 $V_ServerSettings_Admin_Password
	${NSD_SetText} $hCtl_ServerSettings_Admin_Password2 $V_ServerSettings_Admin_Password2
	${NSD_SetText} $hCtl_ServerSettings_Admin_PortNumber $V_ServerSettings_Admin_PortNumber
FunctionEnd

Function fnc_ServerSettings_Remember
	${NSD_GetText} $hCtl_ServerSettings_Admin_Password1 $V_ServerSettings_Admin_Password
	${NSD_GetText} $hCtl_ServerSettings_Admin_Password2 $V_ServerSettings_Admin_Password2
	${NSD_GetText} $hCtl_ServerSettings_Admin_PortNumber $V_ServerSettings_Admin_PortNumber
FunctionEnd

Function fnc_ServerSettings_Leave
	Push $R0
	Push $R1

	${NSD_GetText} $hCtl_ServerSettings_Admin_PortNumber $R0
	${If} $R0 < 1025
	${OrIf} $R0 > 65535
		${MessageBox} MB_ICONEXCLAMATION $(S_ServerSettings_Admin_PortNumberOutOfRange) IDOK ""
		Abort
	${EndIf}

	${NSD_GetText} $hCtl_ServerSettings_Admin_Password1 $R0
	${NSD_GetText} $hCtl_ServerSettings_Admin_Password2 $R1

	${If} $R0 != $R1
		${MessageBox} MB_ICONEXCLAMATION "$(S_ServerSettings_Admin_PasswordsMustMatch)" IDOK ""
		Abort
	${EndIf}

	${If} $R0 == ""
		${MessageBox} MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2 "$(S_ServerSettings_Admin_PasswordsIsEmpty)" IDYES "IDYES accept_empty_password"
		Abort
	accept_empty_password:
	${EndIf}

	Pop $R1
	Pop $R0

	Call fnc_ServerSettings_Remember
FunctionEnd
