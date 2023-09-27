; FileZilla Server installation script
; Written by Fabio Alemagna <mailto:fabio@businessfollows.com>
;
; Requires NSIS >= 3.0b3

;--------------------------------
; Build environment
;--------------------------------

	Unicode true

	!define top_srcdir    ../..
	!define srcdir        .

	!if "x86_64" == "x86_64"
	!define BITS "64"
	!define INSTALL_DIR "$PROGRAMFILES64"
	!else
	!define BITS "32"
	!define INSTALL_DIR "$PROGRAMFILES"
	!endif

	!define PACKAGE_NAME  "FileZilla Server"
	!define PACKAGE_STRING "FileZilla Server 1.7.3"

	!define VERSION       1.7.3
	!define VERSION_MAJOR 1
	!define VERSION_MINOR 7
	!define VERSION_MICRO 3
	!define VERSION_FULL  1.7.3.0

	!define PRODUCT_NAME           "FileZilla Server"
	!define REGISTRY_KEY           "Software\${PRODUCT_NAME}"
	!ifndef INSTALLER_NAME
		!error "You must define INSTALLER_NAME with "
	!endif

	!define PUBLISHER      "Tim Kosse <tim.kosse@filezilla-project.org>"
	!define WEBSITE_URL    "https://filezilla-project.org/"

	!define EULA "COPYING"

;--------------------------------
;Installer's VersionInfo

	VIProductVersion                   "${VERSION_FULL}"
	VIAddVersionKey "CompanyName"      "${PUBLISHER}"
	VIAddVersionKey "ProductName"      "${PACKAGE_NAME}"
	VIAddVersionKey "ProductVersion"   "${VERSION}"
	VIAddVersionKey "FileDescription"  "${PACKAGE_NAME}"
	VIAddVersionKey "FileVersion"      "${VERSION}"
	VIAddVersionKey "LegalCopyright"   "${PUBLISHER}"
	VIAddVersionKey "OriginalFilename" "${INSTALLER_NAME}"

;-------------------------
; Build dirs

	!addplugindir          "${srcdir}"
	!addincludedir         "${srcdir}"
	!addincludedir         "."

;--------------------------------
; General
;--------------------------------

	; Name and file
	Name "${PACKAGE_STRING}"
	OutFile "${INSTALLER_NAME}"

	SetCompressor /SOLID LZMA

	RequestExecutionLevel admin
	ShowInstDetails show

;--------------------------------
; Include Modern UI and functions
;--------------------------------

	!include "Sections.nsh"
	!include "MUI2.nsh"
	!include "x64.nsh"
	!include "WinVer.nsh"
	!include "StrFunc.nsh"

	; Activate the following functions
	${StrRep}
	${StrStr}
	${UnStrStr}

	!include "macros.nsh"
	!include "servicesettings.nsdinc"
	!include "serversettings.nsdinc"
	!include "adminsettings.nsdinc"
	!include "Memento.nsh"

	!define MUI_ICON "${top_srcdir}/res/filezilla-server.ico"
	!define MUI_UNICON "${top_srcdir}/res/uninstall.ico"
	!define MUI_ABORTWARNING

	;Show all languages, despite user's codepage
	;!define MUI_LANGDLL_ALLLANGUAGES

;--------------------------------
; Memento config

	!define MEMENTO_REGISTRY_ROOT "SHCTX"
	!define MEMENTO_REGISTRY_KEY "${REG_UNINSTALL}\${PRODUCT_NAME}"

;--------------------------------
; Pages

	;------------------------
	; Install pages

	!insertmacro MUI_PAGE_LICENSE    "${top_srcdir}/${EULA}"
	!insertmacro MUI_PAGE_COMPONENTS
	!insertmacro MUI_PAGE_DIRECTORY

		!define MUI_STARTMENUPAGE_NODISABLE
		!define MUI_STARTMENUPAGE_REGISTRY_ROOT "SHCTX"
		!define MUI_STARTMENUPAGE_REGISTRY_KEY "${REGISTRY_KEY}"
		!define MUI_STARTMENUPAGE_REGISTRY_VALUENAME "Startmenu"
		!define MUI_STARTMENUPAGE_DEFAULTFOLDER "${PRODUCT_NAME}"
		!define MUI_PAGE_CUSTOMFUNCTION_PRE fnc_MUI_PAGE_STARTMENU_Pre
		Var V_StartMenu_Folder
	!insertmacro MUI_PAGE_STARTMENU Application $V_StartMenu_Folder

	Page custom fnc_ServiceSettings_Show fnc_ServiceSettings_Leave
	Page custom fnc_ServerSettings_Show fnc_ServerSettings_Leave
	Page custom fnc_AdminSettings_Show fnc_AdminSettings_Leave

	!insertmacro MUI_PAGE_INSTFILES

	;-----------------
	; Uninstall pages

	!insertmacro MUI_UNPAGE_CONFIRM
	!insertmacro MUI_UNPAGE_INSTFILES

;--------------------------------
; Languages

	!include "lang_en.nsh"
	!include "lang_it.nsh"

;--------------------------------
; Installation types

	InstType $(S_IT_Full)
	InstType $(S_IT_ServerOnly)
	InstType $(S_IT_AdminOnly)

;--------------------------------
; File operation options

	AllowSkipFiles Off
	FileErrorText "$(S_FILE_CannotWrite)" "$(S_FILE_CannotWrite)"

;--------------------------------
; Sections
Var V_PRE_32_INSTDIR
Var V_PRE_64_INSTDIR
Var V_ADMIN_TLS_FILEPATH

Section "-PreInstall"
	!macro PreUninstall old
		${If} "${old}" != ""
		${AndIf} ${FileExists} "${old}\Uninstall.exe"
			DetailPrint $(S_PRE_Uninstalling)
			CopyFiles /SILENT "${old}\Uninstall.exe" $PLUGINSDIR
			${If} ${FileExists} "${old}\FileZilla Server.xml"
			${AndIfNot} ${AsBool} $V_ServiceSettings_MustKeepOldConfig
				CopyFiles /SILENT "${old}\FileZilla Server.xml" $PLUGINSDIR ; Save the old config file, if it exists at all.
			${EndIf}
			ClearErrors
			Push $0

			ExecWait '"$PLUGINSDIR\Uninstall.exe" /quiet /keepservice $V_ServiceSettings_MustKeepOldConfig _?=${old}' $0

			${If} ${Errors}
				DetailPrint 'Failed to run "$PLUGINSDIR\Uninstall.exe", with exit code $0'
			${EndIf}
			Pop $0
		${EndIf}
	!macroend
	!define PreUninstall `!insertmacro PreUninstall`

	${If} ${ServiceExists} filezilla-server
		${StopService} filezilla-server
		${WaitForServiceState} filezilla-server STOPPED
	${EndIf}

	Call CheckIfGuiIsRunning

	${PreUninstall} "$V_PRE_32_INSTDIR"
	${PreUninstall} "$V_PRE_64_INSTDIR"

	SetOutPath $INSTDIR
	WriteUninstaller "$INSTDIR\Uninstall.exe"
	Call RegisterSelf
SectionEnd

Section "-Common Files"
	SetOutPath $INSTDIR

	!include "common_install.nsh"

	File "${top_srcdir}\${EULA}"
	File "${top_srcdir}\NEWS"
SectionEnd

${MementoSection} "Server" SEC_Server
	SectionIn 1 2

	SetOutPath $INSTDIR

	!include "server_install.nsh"
${MementoSectionEnd}

${MementoSection} $(S_SEC_Admin) SEC_Admin
	SectionIn 1 3

	SetOutPath $INSTDIR

	!include "gui_install.nsh"
${MementoSectionEnd}

!macro CreateShortcuts where uninstall
	!insertmacro MUI_STARTMENU_WRITE_BEGIN Application
		CreateDirectory "${where}"

		!if "${uninstall}" == "uninstall"
			CreateShortcut "${where}\$(S_LINK_Uninstall).lnk" "$INSTDIR\Uninstall.exe"
			${WriteMenuReg} Application Str "Uninstall_link" "$(S_LINK_Uninstall).lnk"
		!endif

		${If} ${SectionIsSelected} ${SEC_Admin}
			CreateShortcut "${where}\$(S_LINK_Admin).lnk" "$INSTDIR\filezilla-server-gui.exe"
			${WriteMenuReg} Application Str "Admin_link" "$(S_LINK_Admin).lnk"
		${EndIf}

		${If} ${SectionIsSelected} ${SEC_Server}
			CreateShortcut "${where}\$(S_LINK_StartServer).lnk" "net" "start filezilla-server" "$INSTDIR\filezilla-server.exe"
			${ShellLinkSetRunAs} "${where}\$(S_LINK_StartServer).lnk" ""

			CreateShortcut "${where}\$(S_LINK_StopServer).lnk" "net" "stop filezilla-server" "$INSTDIR\filezilla-server.exe"
			${ShellLinkSetRunAs} "${where}\$(S_LINK_StopServer).lnk" ""

			${WriteMenuReg} Application Str "StartServer_link" "$(S_LINK_StartServer).lnk"
			${WriteMenuReg} Application Str "StopServer_link" "$(S_LINK_StopServer).lnk"
		${EndIf}
	!insertmacro MUI_STARTMENU_WRITE_END
!macroend
!define CreateShortcuts `!insertmacro CreateShortcuts`

${MementoSection} $(S_SEC_StartMenu) SEC_StartMenu
	SectionIn 1 2 3

	${CreateShortcuts} "$SMPROGRAMS\$V_StartMenu_Folder" uninstall
${MementoSectionEnd}

${MementoSection} $(S_SEC_DeskIcons) SEC_DeskIcons
	SectionIn 1 2 3

	${CreateShortcuts} "$desktop" ""
${MementoSectionEnd}

Section "-PostInstallServer"
	${If} ${SectionIsSelected} ${SEC_Server}
		${If} ${AsBool} $V_ServiceSettings_MustKeepOldConfig
			; Keeping the old service and config, if required
			${IfNot} ${ServiceExists} filezilla-server
				${MessageBox} MB_ICONEXCLAMATION|MB_OK "$(S_Post_Server_ServiceNotExisting)" IDNO ''
				Abort
			${EndIf}
			${StopService} filezilla-server
			${WaitForServiceState} filezilla-server STOPPED
		${Else}
			${If} ${ServiceExists} filezilla-server
				StrCpy $R0 config
			${Else}
				StrCpy $R0 create
			${EndIf}

			; Creating the new service, if required
			${If} "$V_ServiceSettings_HowToInstallAndStart" == "$(S_ServiceSettings_AsServiceAutoStart)"
				${DoService} $R0 filezilla-server "$INSTDIR\filezilla-server.exe" "" auto "$V_ServiceSettings_ServiceUser" "$V_ServiceSettings_ServicePassword"
			${ElseIf} "$V_ServiceSettings_HowToInstallAndStart" == "$(S_ServiceSettings_AsServiceManualStart)"
				${DoService} $R0 filezilla-server "$INSTDIR\filezilla-server.exe" "" demand "$V_ServiceSettings_ServiceUser" "$V_ServiceSettings_ServicePassword"
			${EndIf}

			${If} ${FileExists} "$PLUGINSDIR\FileZilla Server.xml"
				; Try to convert the old config into the new one
				; We need to use a service for this, because the user has to be the same as the server's service one.
				${GrantAccess} "$PLUGINSDIR\FileZilla Server.xml" "$V_ServiceSettings_ServiceUser" "GenericRead"
				${CreateServiceEx} filezilla-server-config-converter "$INSTDIR\filezilla-server-config-converter.exe" "" demand "$V_ServiceSettings_ServiceUser" "$V_ServiceSettings_ServicePassword"
				${StartService} filezilla-server-config-converter '"$PLUGINSDIR\FileZilla Server.xml" filezilla-server'
				${AbortOnServiceError} filezilla-server-config-converter
				${DeleteService} filezilla-server-config-converter
			${EndIf}
		${EndIf}

		;------------------------------------
		; Check existing config file versions

		${CheckConfigVersion} "error" "$R0" "$R1"
		${If} "$R0" == "error"
			${MessageBox} MB_ICONEXCLAMATION|MB_YESNO|MB_DEFBUTTON2 "$(S_CONFIG_VersionError)" IDNO 'IDYES doconfigbackup'
			${CheckConfigVersion} "ignore" "$R0" "$R1"
			goto doconfigcheckresult

		doconfigbackup:
			${CheckConfigVersion} "backup" "$R0" "$R1"
	
		doconfigcheckresult:
			${If} "$R0" == "backup"
				${MessageBox} MB_ICONINFORMATION|MB_OK "$(S_CONFIG_BackupOk)$\n$\n$R1	" IDOK ''
			${ElseIf} "$R0" != "ok"
				${MessageBox} MB_ICONSTOP|MB_OK "$(S_CONFIG_BackupKo)" IDOK ''
				Abort
			${EndIf}
		${EndIf}

		;---------------------------------------------------------------
		; Instruct the server to write its own config file, if required.

		${IfNot} ${AsBool} $V_ServiceSettings_MustKeepOldConfig
			${If} "$V_ServerSettings_Admin_Password" == ""
				StrCpy $R0 "--admin.password@index=0" ; No password
			${Else}
				Var /Global CanaryValue
				${GenRandomString} $CanaryValue
				Push $CanaryValue

				ExecDos::exec /TOSTACK '"$INSTDIR\filezilla-server-crypt.exe" admin.password' "$V_ServerSettings_Admin_Password$\n"
				Pop $R0

				${If} "$R0" != "0"
					${MessageBox} MB_ICONSTOP "Error while generating password hash: exit code $R0. Aborting." IDOK ""
					Abort
				${EndIf}

				StrCpy $R0 ""
				${PopStrings} $R0 $R1 "$CanaryValue"
				${If} ${Errors}
				${OrIf} $R1 != 1
					${MessageBox} MB_ICONSTOP|MB_OK "Error while generating password hash: unrecognizable output. Aborting." IDOK ""
					Abort
				${EndIf}

				DetailPrint "Crypt output: [$R0]"

				${If} "$R0" == ""
					${MessageBox} MB_ICONSTOP "Error while generating password hash: empty output. Aborting." IDOK ""
					Abort
				${EndIf}
			${EndIf}

			CreateDirectory "$INSTDIR\Logs"
			${StartService} filezilla-server '--admin.local_port $V_ServerSettings_Admin_PortNumber $R0 --logger.name:="$INSTDIR\Logs\filezilla-server.log" --logger.enabled_types:=15 --logger.max_amount_of_rotated_files:=5 --write-config --write-admin-tls-fingerprints-to-file "$V_ADMIN_TLS_FILEPATH"'
			${AbortOnServiceError} filezilla-server
		${Else}
			${StartService} filezilla-server '--write-config --write-admin-tls-fingerprints-to-file "$V_ADMIN_TLS_FILEPATH"'
			${AbortOnServiceError} filezilla-server
		${EndIf}

		DetailPrint "====================================="
		DetailPrint $(S_Post_Server_AdminTlsFingerprints)
		${ReadAnsiFile} "$V_ADMIN_TLS_FILEPATH" '${ReadAnsiFile.AppendToVarAndDetailPrint} "$R0"'
		DetailPrint "====================================="

		${If} $R0 != ""
			${MessageBox} MB_ICONINFORMATION "$(S_Post_Server_AdminTlsFingerprints)$\n$\n$R0" IDOK ""
		${EndIf}
	${EndIf}
SectionEnd

Section "-PreserveInstallLog"
	; Preserve the installation log
	${DumpLog} "$INSTDIR\install.log"
SectionEnd

Section "un.Install"
	SetDetailsView show

	; Stop the old service
	${StopService} filezilla-server

	; And maybe remove it
	${IfNot} ${KeepService}
		${DeleteService} filezilla-server
	${EndIf}

	; Force-remove the config converter service, just in case for some reasons it was not removed during installation itself.
	${StopService} filezilla-server-config-converter
	${DeleteService} filezilla-server-config-converter

	Var /GLOBAL Delete.errors

	${Do}
		${If} $Delete.errors == true
			${MessageBox} MB_ICONEXCLAMATION|MB_ABORTRETRYIGNORE "$(S_FILE_CannotDelete)" IDIGNORE "IDIGNORE uninstall_continue IDRETRY uninstall_retry"
			Abort
		uninstall_retry:
		${EndIf}

		StrCpy $Delete.errors false

		; Remove the exes and libs
		!include "common_uninstall.nsh"
		!include "server_uninstall.nsh"
		!include "gui_uninstall.nsh"
	${LoopWhile} $Delete.errors == true
	uninstall_continue:

	Delete "$INSTDIR\${EULA}"
	Delete "$INSTDIR\NEWS"

	; Remove Start menu stuff
	!insertmacro MUI_STARTMENU_GETFOLDER Application $V_StartMenu_Folder
	${ReadMenuReg} Application Str "Uninstall_link" $R0 "$(S_LINK_Uninstall).lnk"
	Delete "$SMPROGRAMS\$V_StartMenu_Folder\$R0"
	Delete "$desktop\$R0"

	${ReadMenuReg} Application Str "Admin_link" $R0 "$(S_LINK_Admin).lnk"
	Delete "$SMPROGRAMS\$V_StartMenu_Folder\$R0"
	Delete "$desktop\$R0"

	${ReadMenuReg} Application Str "StartServer_link" $R0 "$(S_LINK_StartServer).lnk"
	Delete "$SMPROGRAMS\$V_StartMenu_Folder\$R0"
	Delete "$desktop\$R0"

	${ReadMenuReg} Application Str "StopServer_link" $R0 "$(S_LINK_StopServer).lnk"
	Delete "$SMPROGRAMS\$V_StartMenu_Folder\$R0"
	Delete "$desktop\$R0"

	RMDir "$SMPROGRAMS\$V_StartMenu_Folder"
	DeleteRegKey SHCTX "${REGISTRY_KEY}"

	Call un.RegisterSelf

	; Remove the installer itself
	Delete "$INSTDIR\Uninstall.exe"

	; Remove the install log
	Delete "$INSTDIR\install.log"

	; Remove the Logs dir, if empty
	RMDir "$INSTDIR\Logs"

	; And then the installation directory, if emtpy
	RMDir "$INSTDIR"
SectionEnd

${MementoSectionDone}

;--------------------------------
;Descriptions

!insertmacro MUI_FUNCTION_DESCRIPTION_BEGIN
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_Server}      $(S_SEC_DESC_Server)
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_Admin}       $(S_SEC_DESC_Admin)
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_StartMenu}   $(S_SEC_DESC_StartMenu)
	!insertmacro MUI_DESCRIPTION_TEXT ${SEC_DeskIcons}   $(S_SEC_DESC_DeskIcons)
!insertmacro MUI_FUNCTION_DESCRIPTION_END

;--------------------------------
;Callbacks

Var V_PRE_INSTDIR

Function .onInit
	${Unless} ${AtLeastWin7}
		${MessageBox} MB_ICONSTOP "$(S_INIT_UnsupportedOS)" IDOK ""
		Abort
	${EndUnless}

	${Unless} ${RunningX64}
		${MessageBox} MB_YESNO|MB_ICONSTOP "$(S_INIT_Unsupported32)" IDNO 'IDYES install64on32'
		Abort
	install64on32:
	${EndUnless}

	${ParseOptions} install

	SetShellVarContext all
	InitPluginsDir

	SetRegView 32
	${ReadUninstallReg} Str "InstallLocation" $V_PRE_32_INSTDIR
	StrCpy $V_PRE_INSTDIR $V_PRE_32_INSTDIR

	${If} ${RunningX64}
		SetRegView 64
		${ReadUninstallReg} Str   "InstallLocation" $V_PRE_64_INSTDIR
		${ReadUninstallReg} DWORD "VersionMajor"    $V_ServiceSettings_OldServiceVersionMajor
		${ReadUninstallReg} DWORD "VersionMinor"    $V_ServiceSettings_OldServiceVersionMinor
		${ReadUninstallReg} DWORD "VersionMicro"    $V_ServiceSettings_OldServiceVersionMicro
		StrCpy $V_PRE_INSTDIR $V_PRE_64_INSTDIR
	${EndIf}

	${MementoSectionRestore}

	Call fnc_ServiceSettings_Init
	Call fnc_ServerSettings_Init
	Call fnc_AdminSettings_Init

	${If} $INSTDIR == ""
		StrCpy $INSTDIR $V_PRE_INSTDIR

		${If} $INSTDIR == ""
			; Default installation folder
			StrCpy $INSTDIR "${INSTALL_DIR}\${PRODUCT_NAME}"
		${EndIf}
	${EndIf}

	StrCpy $V_ADMIN_TLS_FILEPATH "$ParseOptions.AdminTlsFingerprintsFile"

	;!insertmacro MUI_LANGDLL_DISPLAY
FunctionEnd

Function un.onInit
	${ParseOptions} uninstall

	SetShellVarContext all

	${If} ${RunningX64}
		SetRegView 64
	${EndIf}
FunctionEnd

Function .onInstSuccess
	${MementoSectionSave}

	${If} $V_ServiceSettings_StartCheck == ${BST_CHECKED}
	${AndIf} ${SectionIsSelected} ${SEC_Server}
		${StartService} filezilla-server ""
	${EndIf}

	${If} ${SectionIsSelected} ${SEC_Admin}
		${If} $V_AdminSettings_StartCheck == ${BST_CHECKED}
		${AndIfNot} ${Silent}
			${AsUser_Open} "filezilla-server-gui.exe" "--server.name Local --server.host 127.0.0.1 --server.port $V_ServerSettings_Admin_PortNumber"
		${Else}
			${AsUser_Open} "filezilla-server-gui.exe" "--server.name Local --server.host 127.0.0.1 --server.port $V_ServerSettings_Admin_PortNumber --write-config"
		${EndIf}
	${EndIf}
FunctionEnd

Function .onSelChange
	Var /GLOBAL .onSelChange.prevSel

	SectionGetFlags ${SEC_Server} $R0
	SectionGetFlags ${SEC_Admin} $R1

	IntOp $R2 $R0 & 1
	IntOp $R2 $R2 | $.onSelChange.prevSel
	IntCmp $R2 1 +2
	IntOp $R1 $R1 | 1

	IntOp $R2 $R1 & 1
	IntCmp $R2 1 +2
	IntOp $R0 $R0 | 1

	SectionSetFlags ${SEC_Server} $R0
	SectionSetFlags ${SEC_Admin} $R1

	IntOp $.onSelChange.prevSel $R1 & 1
FunctionEnd

Function fnc_MUI_PAGE_STARTMENU_Pre
	${IfNot} ${SectionIsSelected} ${SEC_StartMenu}
		Abort
	${Endif}
FunctionEnd

Function fnc_ServiceSettings_Show
	${If} ${SectionIsSelected} ${SEC_Server}
		Call fnc_ServiceSettings_Create
		nsDialogs::Show
	${EndIf}
FunctionEnd

Function fnc_ServerSettings_Show
	${If} ${SectionIsSelected} ${SEC_Server}
		${IfNot} ${AsBool} $V_ServiceSettings_MustKeepOldConfig
			Call fnc_ServerSettings_Create
			nsDialogs::Show
		${EndIf}
	${EndIf}
FunctionEnd

Function fnc_AdminSettings_Show
	${If} ${SectionIsSelected} ${SEC_Admin}
		Call fnc_AdminSettings_Create
		nsDialogs::Show
	${EndIf}
FunctionEnd

Function RegisterSelf
	Push $R0

	${StrRep} $R0 "$INSTDIR\Uninstall.exe" '"' '""'

	${WriteUninstallReg} ExpandStr "UninstallString" '"$R0"'
	${WriteUninstallReg} ExpandStr "InstallLocation" "$INSTDIR"
	${WriteUninstallReg} Str       "DisplayName"     "${PACKAGE_STRING}"
	${WriteUninstallReg} Str       "DisplayIcon"     "$INSTDIR\filezilla-server.exe"
	${WriteUninstallReg} Str       "DisplayVersion"  "${VERSION}"
	${WriteUninstallReg} Str       "URLInfoAbout"    "${WEBSITE_URL}"
	${WriteUninstallReg} Str       "URLUpdateInfo"   "${WEBSITE_URL}"
	${WriteUninstallReg} Str       "HelpLink"        "${WEBSITE_URL}"
	${WriteUninstallReg} Str       "Publisher"       "${PUBLISHER}"
	${WriteUninstallReg} DWORD     "VersionMajor"    "${VERSION_MAJOR}"
	${WriteUninstallReg} DWORD     "VersionMinor"    "${VERSION_MINOR}"
	${WriteUninstallReg} DWORD     "VersionMicro"    "${VERSION_MICRO}"
	${WriteUninstallReg} DWORD     "NoModify"        "1"
	${WriteUninstallReg} DWORD     "NoRepair"        "1"

	${GetInstalledSize} $R0
	${WriteUninstallReg} DWORD     "EstimatedSize"   "$R0"

	${If} "$V_AdminSettings_HowToStart" != "$(S_AdminSettings_StartManually)"
		${If} "$V_AdminSettings_HowToStart" == "$(S_AdminSettings_StartLogonForUser)"
			; All is default
			SetShellVarContext current
		${EndIf}

		${StrRep} $R0 "$INSTDIR\filezilla-server-gui.exe" '"' '""'
		${WriteRunReg} ExpandStr "${PRODUCT_NAME}" '"$R0"'
	${EndIf}

	; All is default
	SetShellVarContext all

	Pop $R0
FunctionEnd

Function un.RegisterSelf
	${DeleteUninstallReg}
	SetShellVarContext current
	${DeleteRunReg}
	SetShellVarContext all
	${DeleteRunReg}
FunctionEnd

Function CheckIfGuiIsRunning
	${If} ${SectionIsSelected} ${SEC_Admin}
		${While} ${ProcessRunning} "$INSTDIR\filezilla-server-gui.exe"
			${MessageBox} MB_ICONEXCLAMATION|MB_ABORTRETRYIGNORE "$(S_PRE_CloseGuiToContinue)" IDRETRY "IDIGNORE check_continue IDABORT check_abort"
		${EndWhile}
	${EndIf}

	Goto check_continue

check_abort:
	Quit

check_continue:
FunctionEnd

