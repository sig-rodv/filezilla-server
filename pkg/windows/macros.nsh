!ifndef FZ_MACROS_NSH
!define FZ_MACROS_NSH

!include "FileFunc.nsh"
!include "LogicLib.nsh"
!include "TextFunc.nsh"
!include "process_running.nsh"

!macro MakeVar __MakeVar.name initial_value
	!ifndef __Var.${__MakeVar.name}.defined
		!define __Var.${__MakeVar.name}.defined ${__MakeVar.name}
		Var /Global ${__MakeVar.name}
	!endif
	StrCpy $${__MakeVar.name} `${initial_value}`
!macroend
!define MakeVar `!insertmacro MakeVar`

!macro Store __Store.parent __Store.name
	!insertmacro Makevar ${__Store.parent}.${__Store.name} `${${__Store.name}}`
!macroend
!define Store `!insertmacro Store`

; Assorted utility macros
!macro WriteMenuReg ID Type Name Value
	!if "${MUI_STARTMENUPAGE_${ID}_REGISTRY_ROOT}" == ""
		!error "You must define MUI_STARTMENUPAGE_REGISTRY_ROOT"
	!endif
	!if "${MUI_STARTMENUPAGE_${ID}_REGISTRY_KEY}" == ""
		!error "You must define MUI_STARTMENUPAGE_REGISTRY_KEY"
	!endif
	!if "${MUI_STARTMENUPAGE_${ID}_REGISTRY_VALUENAME}" == ""
		!error "You must define MUI_STARTMENUPAGE_REGISTRY_VALUENAME"
	!endif

	WriteReg${Type} "${MUI_STARTMENUPAGE_${ID}_REGISTRY_ROOT}" "${MUI_STARTMENUPAGE_${ID}_REGISTRY_KEY}\${MUI_STARTMENUPAGE_${ID}_REGISTRY_VALUENAME}" "${Name}" "${Value}"
!macroend
!define WriteMenuReg `!insertmacro WriteMenuReg`

Var ReadMenuReg.Tmp
!macro ReadMenuReg ID Type Name Var DefValue
	!if "${MUI_STARTMENUPAGE_${ID}_REGISTRY_ROOT}" == ""
		!error "You must define MUI_STARTMENUPAGE_REGISTRY_ROOT"
	!endif
	!if "${MUI_STARTMENUPAGE_${ID}_REGISTRY_KEY}" == ""
		!error "You must define MUI_STARTMENUPAGE_REGISTRY_KEY"
	!endif
	!if "${MUI_STARTMENUPAGE_${ID}_REGISTRY_VALUENAME}" == ""
		!error "You must define MUI_STARTMENUPAGE_REGISTRY_VALUENAME"
	!endif

	ReadReg${Type} $ReadMenuReg.Tmp "${MUI_STARTMENUPAGE_${ID}_REGISTRY_ROOT}" "${MUI_STARTMENUPAGE_${ID}_REGISTRY_KEY}\${MUI_STARTMENUPAGE_${ID}_REGISTRY_VALUENAME}" "${Name}"
	${If} "$ReadMenuReg.Tmp" == ""
		StrCpy $ReadMenuReg.Tmp ${DefValue}
	${Endif}
	StrCpy ${Var} $ReadMenuReg.Tmp
!macroend
!define ReadMenuReg `!insertmacro ReadMenuReg`

!define REG_CURRENT_VERSION "Software\Microsoft\Windows\CurrentVersion"
!define REG_UNINSTALL "${REG_CURRENT_VERSION}\Uninstall"

!macro WriteUninstallReg Type Name Value
	!if "${PRODUCT_NAME}" == ""
		!error "You must define PRODUCT_NAME first"
	!endif
	WriteReg${Type} SHCTX "${REG_UNINSTALL}\${PRODUCT_NAME}" "${Name}" `${Value}`
!macroend
!define WriteUninstallReg `!insertmacro WriteUninstallReg`

!macro ReadUninstallReg Type Name Var
	!if "${PRODUCT_NAME}" == ""
		!error "You must define PRODUCT_NAME first"
	!endif
	ReadReg${Type} ${Var} SHCTX "${REG_UNINSTALL}\${PRODUCT_NAME}" "${Name}"
!macroend
!define ReadUninstallReg `!insertmacro ReadUninstallReg`

!macro DeleteUninstallReg
	!if "${PRODUCT_NAME}" == ""
		!error "You must define PRODUCT_NAME first"
	!endif
	DeleteRegKey SHCTX "${REG_UNINSTALL}\${PRODUCT_NAME}"
!macroend
!define DeleteUninstallReg `!insertmacro DeleteUninstallReg`

!macro WriteRunReg Type Name Value
	!if "${PRODUCT_NAME}" == ""
		!error "You must define PRODUCT_NAME first"
	!endif
	WriteReg${Type} SHCTX "${REG_CURRENT_VERSION}\Run" "${Name}" `${Value}`
!macroend
!define WriteRunReg `!insertmacro WriteRunReg`

!macro DeleteRunReg
	!if "${PRODUCT_NAME}" == ""
		!error "You must define PRODUCT_NAME first"
	!endif
	DeleteRegValue SHCTX "${REG_CURRENT_VERSION}\Run" "${PRODUCT_NAME}"
!macroend
!define DeleteRunReg `!insertmacro DeleteRunReg`

Var GetInstalledSize.Tmp
!macro GetInstalledSize Result
	Push $0
	Push $1
	StrCpy $GetInstalledSize.Tmp 0

	${ForEach} $1 0 256 + 1
		${if} ${SectionIsSelected} $1
			SectionGetSize $1 $0
			IntOp $GetInstalledSize.Tmp $GetInstalledSize.Tmp + $0
		${Endif}
	${Next}

	Pop $1
	Pop $0

	IntFmt $GetInstalledSize.Tmp "0x%08X" $GetInstalledSize.Tmp
	StrCpy ${Result} $GetInstalledSize.Tmp
!macroend
!define GetInstalledSize `!insertmacro GetInstalledSize`

Var V_AsUser_Open_What
Var V_AsUser_Open_Args
!macro AsUser_Open what args
	; We need to copy the arguments into variables, otherwise on windows they won't get properly expanded if they contain variables themselves
	; We are forced to use registers, because those are the only variables UAC knows about and can pass along.
	; Also - and this is damn important - variables need too be passed to UAC_AsUser_ExecShell *QUOTED*
	StrCpy $V_AsUser_Open_What `${what}`
	StrCpy $V_AsUser_Open_Args `${args}`

	SetOutPath `$INSTDIR`
	ShellExecAsUser::ShellExecAsUser `open` `$V_AsUser_Open_What` `$V_AsUser_Open_Args`
!macroend
!define AsUser_Open `!insertmacro AsUser_Open`

; Begins with, case insensitively
!macro _^== _a _b _t _f
	!insertmacro _LOGICLIB_TEMP

	Push $0

	StrLen $0 `${_b}`
	StrCpy $_LOGICLIB_TEMP `${_a}` $0

	Pop $0

	StrCmp $_LOGICLIB_TEMP `${_b}` `${_t}` `${_f}`
!macroend

; Begins with, case sensitively
!macro _S^== _a _b _t _f
	!insertmacro _LOGICLIB_TEMP

	Push $0

	StrLen $0 `${_b}`
	StrCpy $_LOGICLIB_TEMP `${_a}` $0

	Pop $0

	StrCmpS $_LOGICLIB_TEMP `${_b}` `${_t}` `${_f}`
!macroend

; Does not begin with, case insensitively
!macro _^!= _a _b _t _f
	!insertmacro _^== `${_a}` `${_b}` `${_f}` `${_t}`
!macroend

; Does not begin with, case sensitively
!macro _S^!= _a _b _t _f
	!insertmacro _S^== `${_a}` `${_b}` `${_f}` `${_t}`
!macroend

!macro _FindSubstr S result str substr
	!insertmacro _LOGICLIB_TEMP

	${MakeVar} _FindSubstr.len 0
	${MakeVar} _FindSubstr.position -1

	StrLen $_FindSubstr.len `${substr}`

	!define _FindSubstr_start _FindSubstr_label__${LOGICLIB_COUNTER}
	!insertmacro _IncreaseCounter

	!define _FindSubstr_end _FindSubstr_label_${LOGICLIB_COUNTER}
	!insertmacro _IncreaseCounter

${_FindSubstr_start}:
	IntOp $_FindSubstr.position $_FindSubstr.position + 1
	StrCpy $_LOGICLIB_TEMP `${str}` $_FindSubstr.len $_FindSubstr.position

	StrCmp${S} $_LOGICLIB_TEMP `${substr}` `${_FindSubstr_end}` 0
	StrCmp $_LOGICLIB_TEMP '' 0 `${_FindSubstr_start}`
	StrCpy $_FindSubstr.position -1

${_FindSubstr_end}:
	StrCpy ${result} $_FindSubstr.position

	!undef _FindSubstr_start
	!undef _FindSubstr_end
!macroend
!define FindSubstr `!insertmacro _FindSubstr ''`
!define FindSubstrS `!insertmacro _FindSubstr S`

!macro _LeftAndRightOfSubstr S left right str substr
	${MakeVar} _LeftAndRightOfSubstr.position -1

	StrCpy ${left} ""
	StrCpy ${right} ""

	!insertmacro _FindSubstr `${S}` $_LeftAndRightOfSubstr.position `${str}` `${substr}`

	!define _LeftAndRightOfSubstr_end _LeftAndRightOfSubstr_label__${LOGICLIB_COUNTER}
	!insertmacro _IncreaseCounter

	StrCmp $_LeftAndRightOfSubstr.position -1 ${_LeftAndRightOfSubstr_end}

	# Found
	!if `${left}` != ``
		StrCpy ${left} ${str} $_LeftAndRightOfSubstr.position 0
	!endif

	!if `${right}` != ``
		${MakeVar} _LeftAndRightOfSubstr.sublen 0
		StrLen $_LeftAndRightOfSubstr.sublen `${substr}`

		${MakeVar} _LeftAndRightOfSubstr.len 0
		StrLen $_LeftAndRightOfSubstr.len `${str}`

		IntOp $_LeftAndRightOfSubstr.position $_LeftAndRightOfSubstr.position + $_LeftAndRightOfSubstr.sublen
		StrCpy ${right} ${str} $_LeftAndRightOfSubstr.len $_LeftAndRightOfSubstr.position
	!endif

${_LeftAndRightOfSubstr_end}:
	!undef _LeftAndRightOfSubstr_end
!macroend
!define LeftAndRightOfSubstr `!insertmacro _LeftAndRightOfSubstr ''`
!define LeftAndRightOfSubstrS `!insertmacro _LeftAndRightOfSubstr S`

; a Contains substring b, case insensitively
!macro _~= _a _b _t _f
	${MakeVar} _Contains.position -1

	${FindSubstr} $_Contains.position `${_a}` `${_b}`
	StrCmp $_Contains.position -1 `${_f}` `${_t}`
!macroend

; a Contains substring b, case sensitively
!macro _S~= _a _b _t _f
	${MakeVar} _Contains.position -1

	${FindSubstrS} $_Contains.position `${_a}` `${_b}`
	StrCmp $_Contains.position -1 `${_f}` `${_v}`
!macroend

; a does not contain b, case insensitively
!macro _~!= _a _b _t _f
	!insertmacro _~= `${_a}` `${_b}` `${_f}` `${_t}`
!macroend

; a does not contain b, case sensitively
!macro _S~!= _a _b _t _f
	!insertmacro _S~= `${_a}` `${_b}` `${_f}` `${_t}`
!macroend

!macro _AsBool _a _b _t _f
	${Store} _AsBool _b

	!define _AsBool_end _AsBool_label__${LOGICLIB_COUNTER}
	!insertmacro _IncreaseCounter

	StrCmp `${_b}` "true" +1 ${_AsBool_end}
	StrCpy $_AsBool._b "1"

${_AsBool_end}:
	StrCmp $_AsBool._b "1" `${_t}` `${_f}`
	!undef _AsBool_end
!macroend
!define AsBool `"" AsBool`
!define AsBoolImpl `!insertmacro _AsBool ""`

!macro _Quiet _a _b _t _f
	${AsBoolImpl} `$ParseOptions.IsQuiet` `${_t}` `${_f}`
!macroend
!define Quiet `"" Quiet ""`

!macro _KeepService _a _b _t _f
	${AsBoolImpl} `$ParseOptions.KeepService` `${_t}` `${_f}`
!macroend
!define KeepService `"" KeepService ""`

!macro _ProcessRunning _a name _t _f
	!insertmacro _LOGICLIB_TEMP

	Push "${name}"
	call IsProcessRunning
	Pop $_LOGICLIB_TEMP

	StrCmp $_LOGICLIB_TEMP `` `${_f}` `${_t}`
!macroend
!define ProcessRunning `"" ProcessRunning`

!define /ifndef	SERVICE_ERROR            0
!define /ifndef	SERVICE_STOPPED          1
!define /ifndef SERVICE_START_PENDING    2
!define /ifndef SERVICE_STOP_PENDING     3
!define /ifndef SERVICE_RUNNING          4
!define /ifndef SERVICE_CONTINUE_PENDING 5
!define /ifndef SERVICE_PAUSE_PENDING    6
!define /ifndef SERVICE_PAUSED           7

; State in Pop 0
; Win32 Exit Code in Pop 1
; Service Specific Exit Code in Pop 2
!macro QueryServiceStatus name
	!define /ifndef SERVICE_QUERY_STATUS 4

	Push $1
	Push $0
	Push $2
	Push $3

	System::Call 'ADVAPI32::OpenSCManager(p 0, p 0, i 1) p.r1'
	${If} $1 P<> 0
		System::Call 'ADVAPI32::OpenService(p r1, t "${name}", i ${SERVICE_QUERY_STATUS}) p . r2'
		System::Call 'ADVAPI32::CloseServiceHandle(p r1)'
		${If} $2 P<> 0
			System::Call 'ADVAPI32::QueryServiceStatus(p r2, @ r3) i .r0'
			System::Call 'ADVAPI32::CloseServiceHandle(p r2)'
			${If} $0 <> 0
				System::Call '*$3(i . r3, i . r0, i, i . r1, i . r2)'
				;DetailPrint "Type=$3 CurrentState=$0 Win32ExitCode=$1 ServiceSpecificExitCode=$2"
			${Else}
				StrCpy $0 ${SERVICE_ERROR}
			${EndIf}
		${Else}
			StrCpy $0 ${SERVICE_ERROR}
		${EndIf}
	${Else}
		StrCpy $0 ${SERVICE_ERROR}
	${EndIf}

	Pop $3
	Exch $2
	Exch 2
	Exch $1
	Exch 1
	Exch $0
!macroend
!define QueryServiceStatus `!insertmacro QueryServiceStatus`

!macro QueryServiceUser out_user service_name
	!define /ifndef SERVICE_QUERY_CONFIG 1

	StrCpy ${out_user} ""

	Push $0
	Push $1
	Push $2
	Push $3

	System::Call 'ADVAPI32::OpenSCManager(p 0, p 0, i 1) p.r1'
	${If} $1 P<> 0
		System::Call 'ADVAPI32::OpenService(p r1, t "${service_name}", i ${SERVICE_QUERY_CONFIG}) p . r2'
		System::Call 'ADVAPI32::CloseServiceHandle(p r1)'
		${If} $2 P<> 0
			System::Call 'ADVAPI32::QueryServiceConfigW(p r2, @ r3, i ${NSIS_MAX_STRLEN}, *i .n) i .r0 ?e'
			Pop $1 #LastError
			System::Call 'ADVAPI32::CloseServiceHandle(p r2)'
			${If} $0 <> 0
				System::Call '*$3(i, i, i, w, w, i, w, w . r0, w)'
				StrCpy ${out_user} $0
			${Else}
				DetailPrint "FAILED QueryServiceConfigW: LastError: [$1]"
			${EndIf}
		${EndIf}
	${Else}
		DetailPrint "FAILED OpenSCManager"
	${EndIf}

	Pop $3
	Pop $2
	Pop $1
	Pop $0
!macroend
!define QueryServiceUser `!insertmacro QueryServiceUser`

Var WaitForServiceState.Tmp
!macro WaitForServiceStateEx name state win32Exit serviceExit
	Push $R0 ; Exec result
	Push $R1 ; Counter

	StrCpy $R1 10 ; Maximum number of tries

	${Do}
		IntOp $R1 $R1 - 1

		${QueryServiceStatus} "${name}"
		Pop $R0
		Pop "${win32Exit}"
		Pop "${serviceExit}"

		${If} $R0 == ${SERVICE_ERROR}
			StrCpy "${win32Exit}" "SERVICE_ERROR"
			${Break}
		${EndIf}

		${If} $R0 == ${SERVICE_${state}}
		${OrIf} $R1 == 0
			${Break}
		${Else}
			Sleep 500
		${Endif}
	${Loop}

	Pop $R1
	Pop $R0
!macroend
!define WaitForServiceStateEx `!insertmacro WaitForServiceStateEx`

!macro WaitForServiceState name state
	${WaitForServiceStateEx} "${name}" "${state}" "$WaitForServiceState.Tmp" "$WaitForServiceState.Tmp"
!macroend
!define WaitForServiceState `!insertmacro WaitForServiceState`

Var AbortOnServiceError.win32Exit
Var AbortOnServiceError.serviceExit
!macro AbortOnServiceError name
	${WaitForServiceStateEx} "${name}" STOPPED "$AbortOnServiceError.win32Exit" "$AbortOnServiceError.serviceExit"
	${If} "$AbortOnServiceError.win32Exit" != "0"
		${MessageBox} MB_ICONSTOP "There was an error while executing service ${name}.$\n$\nWin32 code: $AbortOnServiceError.win32Exit$\nService code: $AbortOnServiceError.serviceExit" IDOK ""
		Abort
	${Endif}
!macroend
!define AbortOnServiceError `!insertmacro AbortOnServiceError`

!define /IfNDef Service.Debug true

!macro DoService op name exe args startmode user password
	${Store} DoService op
	${Store} DoService name
	${Store} DoService exe
	${Store} DoService args
	${Store} DoService startmode
	${Store} DoService user
	${Store} DoService password

	${If} $DoService.user == ""
		StrCpy $DoService.user "LocalSystem"
	${ElseIf} $DoService.user != "LocalSystem"
		${If} $DoService.user ~!= "@"
		${AndIf} $DoService.user ~!= "\"
			StrCpy $DoService.user ".\$DoService.user"
		${EndIf}
	${EndIf}

	DetailPrint "$DoService.op service $DoService.name: $DoService.exe $DoService.args"

	Push $R0
	Push $R1

	nsExec::ExecToStack /OEM '$SYSDIR\sc "$DoService.op" "$DoService.name" binpath= "\"$DoService.exe\" $DoService.args" start= $DoService.startmode displayname= "$DoService.name" obj= "$DoService.user" password= "$DoService.password"'
	Pop $R0
	Pop $R1

	${If} ${AsBool} ${Service.Debug}
		${If} $R0 <> 0
			DetailPrint $R1
		${EndIf}
	${EndIf}

	Pop $R1
	Pop $R0
!macroend
!define DoService `!insertmacro DoService`

!macro CreateServiceEx name exe args startmode user password
	${DoService} create "${name}" "${exe}" "${args}" "${startmode}" "${user}" "${password}"
!macroend
!define CreateServiceEx `!insertmacro CreateServiceEx`

!macro CreateService name exe args startmode
	${DoService} create "${name}" "${exe}" "${args}" "${startmode}" "LocalSystem" ""
!macroend
!define CreateService `!insertmacro CreateService`

!macro StartService name args
	${Store} StartService name
	${Store} StartService args

	Push $R0
	Push $R1

	nsExec::ExecToStack /OEM '$SYSDIR\sc start "$StartService.name" $StartService.args'
	Pop $R0
	Pop $R1

	${If} ${AsBool} ${Service.Debug}
		${If} $R0 <> 0
			DetailPrint $R1
		${EndIf}
	${EndIf}

	Pop $R1
	Pop $R0
!macroend
!define StartService `!insertmacro StartService`

!macro StopService name
	${Store} StopService name

	Push $R0
	Push $R1

	nsExec::ExecToStack /OEM '$SYSDIR\sc stop "$StopService.name"'
	Pop $R0
	Pop $R1

	${If} ${AsBool} ${Service.Debug}
		${If} $R0 <> 0
			DetailPrint $R1
		${EndIf}
	${EndIf}

	${WaitForServiceState} "$StopService.name" STOPPED

	Pop $R1
	Pop $R0
!macroend
!define StopService `!insertmacro StopService`

!macro DeleteService name
	${Store} DeleteService name

	Push $R0
	Push $R1

	nsExec::ExecToStack /OEM '$SYSDIR\sc delete "$DeleteService.name"'
	Pop $R0
	Pop $R1

	${If} ${AsBool} ${Service.Debug}
		${If} $R0 <> 0
			DetailPrint $R1
		${EndIf}
	${EndIf}

	Pop $R1
	Pop $R0
!macroend
!define DeleteService `!insertmacro DeleteService`

!macro _ServiceExists a name _t _f
	${Store} _ServiceExists name
	${MakeVar} _ServiceExists.result 0

	Push $R0
	Push $R1

	nsExec::ExecToStack /OEM '$SYSDIR\sc query "$_ServiceExists.name"'
	Pop $R0
	Pop $R1

	StrCpy $_ServiceExists.result $R0

	;IntCmp $R0 0 +2 0 0
	;DetailPrint "$R1"

	Pop $R1
	Pop $R0

	IntCmp $_ServiceExists.result 1060 `${_f}` `${_t}` `${_t}`
!macroend
!define ServiceExists `"" ServiceExists`

!macro GrantAccess entry user properties
	${MakeVar} GrantAccess.user_for_grant ""
	${MakeVar} GrantAccess.domain ""

	${If} "${user}" != ""
		${LeftAndRightOfSubstr} $GrantAccess.domain $GrantAccess.user_for_grant `${user}` "\"
		${If} $GrantAccess.domain != "."
			StrCpy $GrantAccess.user_for_grant `${user}`
		${EndIf}

		AccessControl::GrantOnFile "$PLUGINSDIR" "$GrantAccess.user_for_grant" `${properties}`
	${EndIf}
!macroend
!define GrantAccess `!insertmacro GrantAccess`

Var ReadAnsiFile.Handle
Var ReadAnsiFile.Line
Var ReadAnsiFile.Num
!macro ReadAnsiFile file callback
	ClearErrors
	StrCpy $ReadAnsiFile.Num 0

	FileOpen $ReadAnsiFile.Handle "${file}" r
	${IfNot} ${Errors}
		${Do}
			FileRead $ReadAnsiFile.Handle $ReadAnsiFile.Line

			${If} "$ReadAnsiFile.Line" != ""
				${callback} $ReadAnsiFile.Num $ReadAnsiFile.Line
				IntOp $ReadAnsiFile.Num $ReadAnsiFile.Num + 1
			${EndIf}
		${LoopUntil} ${Errors}
		ClearErrors
	${EndIf}
	FileClose $ReadAnsiFile.Handle
!macroend
!define ReadAnsiFile `!insertmacro ReadAnsiFile`

!macro ReadAnsiFile.AppendToVar var num line
	${If} "${num}" == "0"
		StrCpy "${var}" ""
	${EndIf}
	StrCpy "${var}" "${var}${line}"
!macroend
!define ReadAnsiFile.AppendToVar `!insertmacro ReadAnsiFile.AppendToVar`

!macro ReadAnsiFile.AppendToVarAndDetailPrint var num line
	DetailPrint ${line}
	${ReadAnsiFile.AppendToVar} "${var}" "${num}" "${line}"
!macroend
!define ReadAnsiFile.AppendToVarAndDetailPrint `!insertmacro ReadAnsiFile.AppendToVarAndDetailPrint`

!macro CheckConfigVersion.callback first_line rest num line
	${If} "${num}" == "0"
		DetailPrint 'CheckConfigVersion: got [${line}]'
		StrCpy "${first_line}" "${line}"
		${TrimNewLines} "${first_line}" "${first_line}"
	${Else}
		${If} "${num}" == "1"
			DetailPrint "Follows list of backed up files:"
		${EndIf}
		DetailPrint '${line}'
		StrCpy "${rest}" "${rest}${line}"
	${EndIf}
!macroend
!define CheckConfigVersion.callback `!insertmacro CheckConfigVersion.callback`

!macro CheckConfigVersion mode first_line rest
	${Store} CheckConfigVersion mode

	StrCpy "${first_line}" ""
	StrCpy "${rest}" ""

	Delete "$PLUGINSDIR\config-version-check-result.txt"
	ClearErrors

	${If} "$CheckConfigVersion.mode" == "ignore"
		StrCpy $CheckConfigVersion.mode "ignore --write-config"
	${EndIf}

	${MakeVar} CheckConfigVersion.user ""
	${QueryServiceUser} $CheckConfigVersion.user "filezilla-server"
	${GrantAccess} "$PLUGINSDIR" $CheckConfigVersion.user "AddFile"

	${StartService} filezilla-server '--config-version-check $CheckConfigVersion.mode --config-version-check-result-file "$PLUGINSDIR\config-version-check-result.txt"'
	${WaitForServiceState} filezilla-server STOPPED
	${ReadAnsiFile} "$PLUGINSDIR\config-version-check-result.txt" '${CheckConfigVersion.callback} "${first_line}" "${rest}"'

	${If} ${Errors}
	${OrIf} "${first_line}" == ""
		${MessageBox} MB_ICONSTOP|MB_OK "$(S_CONFIG_CannotReadResult)" IDOK ""
		Abort
	${EndIf}

	${If} "${first_line}" != "ok"
	${AndIf} "${first_line}" != "error"
	${AndIf} "${first_line}" != "backup"
		${MessageBox} MB_ICONSTOP|MB_OK "$(S_CONFIG_UnknownResult)" IDOK ""
		Abort
	${EndIf}
!macroend
!define CheckConfigVersion `!insertmacro CheckConfigVersion`

!macro GenRandomString result
	GetTempFileName "${result}"
	Delete "${result}"
!macroend
!define GenRandomString `!insertmacro GenRandomString`

Var PopStrings.Tmp
!macro PopStrings resultStr resultNum untilCanary
	ClearErrors

	StrCpy "${resultNum}" 0
	StrCpy "${resultStr}" ""

	${Do}
		Pop "$PopStrings.Tmp"

		${If} ${Errors}
		${OrIf} "$PopStrings.Tmp" == "${untilCanary}"
			${Break}
		${EndIf}

		${If} "${resultStr}" != ""
			StrCpy "${resultStr}" "${resultStr}$\n"
		${EndIf}
		StrCpy "${resultStr}" "${resultStr}$PopStrings.Tmp"

		IntOp "${resultNum}" "${resultNum}" + 1
	${Loop}
!macroend
!define PopStrings `!insertmacro PopStrings`

!macro SplitUserAndDomain res_user res_domain user
	${LeftAndRightOfSubstr} ${res_domain} ${res_user} ${user} "\"

	${If} "${res_user}" == ""
		StrCpy ${res_user} ${user}
		${If} "${res_user}" ~!= "@"
			StrCpy ${res_domain} "."
		${EndIf}
	${EndIf}
!macroend
!define SplitUserAndDomain `!insertmacro SplitUserAndDomain`

!define /IfNDef LOGON32_LOGON_NETWORK 3
!define /IfNDef LOGON32_LOGON_SERVICE 5
!macro CheckIfUserCanLogin name password type
	${Store} CheckIfUserCanLogin name
	${Store} CheckIfUserCanLogin password

	Push $0
	Push $1
	Push $2
	Push $3
	Push $4

	${SplitUserAndDomain} $0 $1 $CheckIfUserCanLogin.name

	StrCpy $2 $CheckIfUserCanLogin.password
	StrCpy $3 "${${type}}"

	ClearErrors
	System::Call "advapi32::LogonUserW(w r0, w r1, w r2, i r3, i 0, *p .r4) i .r0 ?e"
	Pop $2
	${If} $0 == 0
		SetErrors
		DetailPrint "LogonUserW failed with error: [$2]"
	${Else}
		System::Call "kernel32.dll::CloseHandle(p r4) i"
	${Endif}

	Pop $4
	Pop $3
	Pop $2
	Pop $1
	Pop $0
!macroend
!define CheckIfUserCanLogin `!insertmacro CheckIfUserCanLogin`

;-------
; From https://nsis.sourceforge.io/Dump_log_to_file
;-------
!define /IfNDef LVM_GETITEMCOUNT 0x1004
!define /IfNDef LVM_GETITEMTEXTA 0x102D
!define /IfNDef LVM_GETITEMTEXTW 0x1073
!if "${NSIS_CHAR_SIZE}" > 1
!define /IfNDef LVM_GETITEMTEXT ${LVM_GETITEMTEXTW}
!else
!define /IfNDef LVM_GETITEMTEXT ${LVM_GETITEMTEXTA}
!endif

Function DumpLog
  Exch $5
  Push $0
  Push $1
  Push $2
  Push $3
  Push $4
  Push $6
  FindWindow $0 "#32770" "" $HWNDPARENT
  GetDlgItem $0 $0 1016
  StrCmp $0 0 exit
  FileOpen $5 $5 "w"
  StrCmp $5 "" exit
	SendMessage $0 ${LVM_GETITEMCOUNT} 0 0 $6
	System::Call '*(&t${NSIS_MAX_STRLEN})p.r3'
	StrCpy $2 0
	System::Call "*(i, i, i, i, i, p, i, i, i) i  (0, 0, 0, 0, 0, r3, ${NSIS_MAX_STRLEN}) .r1"
	loop: StrCmp $2 $6 done
	  System::Call "User32::SendMessage(p, i, p, p) i ($0, ${LVM_GETITEMTEXT}, $2, r1)"
	  System::Call "*$3(&t${NSIS_MAX_STRLEN} .r4)"
	  !ifdef DumpLog_As_UTF16LE
	  FileWriteUTF16LE ${DumpLog_As_UTF16LE} $5 "$4$\r$\n"
	  !else
	  FileWrite $5 "$4$\r$\n" ; Unicode will be translated to ANSI!
	  !endif
	  IntOp $2 $2 + 1
	  Goto loop
	done:
	  FileClose $5
	  System::Free $1
	  System::Free $3
  exit:
	Pop $6
	Pop $4
	Pop $3
	Pop $2
	Pop $1
	Pop $0
	Pop $5
FunctionEnd
!macro DumpLog file
	Push "${file}"
	Call DumpLog
!macroend
!define DumpLog `!insertmacro DumpLog`

; From https://nsis.sourceforge.io/IShellLink_Set_RunAs_flag
!ifndef IPersistFile
	!define IPersistFile {0000010b-0000-0000-c000-000000000046}
!endif
!ifndef CLSID_ShellLink
	!define CLSID_ShellLink {00021401-0000-0000-C000-000000000046}
	!define IID_IShellLinkA {000214EE-0000-0000-C000-000000000046}
	!define IID_IShellLinkW {000214F9-0000-0000-C000-000000000046}
	!define IShellLinkDataList {45e2b4ae-b1c3-11d0-b92f-00a0c90312e1}
	!ifdef NSIS_UNICODE
		!define IID_IShellLink ${IID_IShellLinkW}
	!else
		!define IID_IShellLink ${IID_IShellLinkA}
	!endif
!endif

Function ShellLinkSetRunAs
	System::Store S
	pop $9
	System::Call "ole32::CoCreateInstance(g'${CLSID_ShellLink}',i0,i1,g'${IID_IShellLink}',*i.r1)i.r0"
	${If} $0 = 0
		System::Call "$1->0(g'${IPersistFile}',*i.r2)i.r0" ;QI
		${If} $0 = 0
			System::Call "$2->5(w '$9',i 0)i.r0" ;Load
			${If} $0 = 0
				System::Call "$1->0(g'${IShellLinkDataList}',*i.r3)i.r0" ;QI
				${If} $0 = 0
					System::Call "$3->6(*i.r4)i.r0" ;GetFlags
					${If} $0 = 0
						System::Call "$3->7(i $4|0x2000)i.r0" ;SetFlags ;SLDF_RUNAS_USER
						${If} $0 = 0
							System::Call "$2->6(w '$9',i1)i.r0" ;Save
						${EndIf}
					${EndIf}
					System::Call "$3->2()" ;Release
				${EndIf}
			System::Call "$2->2()" ;Release
			${EndIf}
		${EndIf}
		System::Call "$1->2()" ;Release
	${EndIf}
	push $0
	System::Store L
FunctionEnd

Var ShellLinkSetRunAs.result
!macro ShellLinkSetRunAs path resvar
	Push `${path}`
	Call ShellLinkSetRunAs
	Pop $ShellLinkSetRunAs.result
	!if "${resvar}" != ""
		StrCpy "${resvar}" $ShellLinkSetRunAs.result
	!endif
!macroend
!define ShellLinkSetRunAs `!insertmacro ShellLinkSetRunAs`

!macro MessageBoxImpl options text sdreturn checks
  ${If} ${Quiet}
	MessageBox ${options} `${text}` ${checks}
  ${Else}
	MessageBox ${options} `${text}` /SD ${sdreturn} ${checks}
  ${Endif}
!MacroEnd
!define MessageBox `!insertmacro MessageBoxImpl`

!define STRFUNC_STALL_install ``
!define STRFUNC_STALL_uninstall `Un`
!define MyStrStr `${${STRFUNC_STALL_${stall}}StrStr}`

!macro GetToggleOption str opt var
	${GetOptions} `${str}` `${opt}` `${var}`
	${If} ${Errors}
		StrCpy ${var} false
	${ElseIf} `${var}` == ""
		StrCpy ${var} true
	${Else}
		${If} `${var}` != false
		${AndIf} `${var}` != true
		${AndIf} `${var}` != 0
		${AndIf} `${var}` != 1
			${MessageBox} MB_ICONSTOP "5 Wrong value for option ${opt}. It's '${var}' but must be one of: true, false, 0, 1. Aborting." IDOK ""
			Abort
		${EndIf}
	${EndIf}
!macroend
!define GetToggleOption `!insertmacro GetToggleOption`

; Call this at the top of .onInit and un.onInit.
Var ParseOptions.IsQuiet
Var ParseOptions.KeepService
Var ParseOptions.AdminTlsFingerprintsFile
!macro ParseOptions stall
	Push $R0
	Push $R1

	${GetParameters} $R0

	${GetToggleOption} $R0 "/quiet" $ParseOptions.IsQuiet
	${If} ${Quiet}
		SetSilent silent
	${Endif}

	${GetToggleOption} $R0 "/keepservice" $ParseOptions.KeepService

	${GetOptions} $R0 "/AdminTlsFingerprintsFile" $R1
	${If} ${Errors}
		StrCpy $ParseOptions.AdminTlsFingerprintsFile "$PLUGINSDIR\admin-tls-fingerprints.txt"
	${Else}
		StrCpy $ParseOptions.AdminTlsFingerprintsFile "$R1"
	${EndIf}

	;;;;;

	${MyStrStr} $0 "SYSDIR" " "
	${MyStrStr} $1 "SYSDIR" "$\t"
	${If} "$0" != ""
	${OrIf} "$1" != ""
		${MessageBox} MB_ICONSTOP|MB_OK "$$SYSDIR contains spaces, this is not supported.$\n$\nAborting installation." IDOK ""
		Abort
	${EndIf}

	Pop $R1
	Pop $R0
!macroend
!define ParseOptions `!insertmacro ParseOptions`

!endif

