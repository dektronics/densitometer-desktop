;
;  Copyright (c) 2021 Alex Spataru <alex-spataru.com>
;
;  This program is free software; you can redistribute it and/or modify
;  it under the terms of the GNU General Public License as published by
;  the Free Software Foundation; either version 3 of the License, or
;  (at your option) any later version.
;
;  This program is distributed in the hope that it will be useful,
;  but WITHOUT ANY WARRANTY; without even the implied warranty of
;  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;  GNU General Public License for more details.
;
;  You should have received a copy of the GNU General Public License
;  along with this program; if not, write to the Free Software
;  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02111-1301
;  USA
;

Unicode True

!include "MUI2.nsh"
!include "FileFunc.nsh"
!include "LogicLib.nsh"

!define APPNAME                      "Printalyzer Densitometer Desktop"
!define EXECNAME                     "densitometer"
!define COMPANYNAME                  "Dektronics"
!define COMPANYNAME_FULL             "Dektronics. Inc."
!define DESCRIPTION                  "Companion application for the Printalyzer Densitometer"
!define VERSIONMAJOR                 1
!define VERSIONMINOR                 1
!define VERSIONBUILD                 0
!define MUI_ABORTWARNING
!define INSTALL_DIR                  "$PROGRAMFILES64\${APPNAME}"
!define MUI_FINISHPAGE_RUN
!define MUI_FINISHPAGE_RUN_TEXT      "Run ${APPNAME}"
!define MUI_FINISHPAGE_RUN_FUNCTION  "RunApplication"
!define MUI_FINISHPAGE_LINK          "Visit project website"
!define MUI_FINISHPAGE_LINK_LOCATION "https://github.com/dektronics/densitometer-desktop/"
!define MUI_WELCOMEPAGE_TITLE        "Welcome to the ${APPNAME} installer!"

!insertmacro MUI_PAGE_WELCOME
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH
!insertmacro MUI_UNPAGE_WELCOME
!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES
!insertmacro MUI_LANGUAGE "English"

!macro VerifyUserIsAdmin
UserInfo::GetAccountType
pop $0
${If} $0 != "admin"
        messageBox mb_iconstop "Administrator rights required!"
        setErrorLevel 740
        quit
${EndIf}
!macroend

Name "${APPNAME}"
ManifestDPIAware true
InstallDir "${INSTALL_DIR}"
RequestExecutionLevel admin
OutFile "${EXECNAME}-${VERSIONMAJOR}.${VERSIONMINOR}.${VERSIONBUILD}-Windows.exe"
	
Function .onInit
	setShellVarContext all
	!insertmacro VerifyUserIsAdmin
FunctionEnd

Section "${APPNAME} (required)" SecDummy
  SectionIn RO
  SetOutPath "$INSTDIR"
  File /r "${APPNAME}\*"
  
  ${GetSize} "$INSTDIR" "/S=0K" $0 $1 $2
  IntFmt $0 "0x%08X" $0
  
; DeleteRegKey HKCU "Software\${COMPANYNAME}\${APPNAME}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}"

  WriteUninstaller "$INSTDIR\uninstall.exe"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "DisplayName"      "${APPNAME}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "UninstallString"  "$INSTDIR\uninstall.exe"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "InstallLocation"  "$INSTDIR"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "Publisher"        "${COMPANYNAME_FULL}"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "DisplayIcon"      "$INSTDIR\icon.ico"
  WriteRegStr   HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "DisplayVersion"   ${VERSIONMAJOR}.${VERSIONMINOR}${VERSIONBUILD}
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "VersionMajor"     ${VERSIONMAJOR}
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "VersionMinor"     ${VERSIONMINOR}
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "NoModify"         1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "NoRepair"         1
  WriteRegDWORD HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}" "EstimatedSize"    "$0"
SectionEnd

Section "Start Menu Shortcuts"
  CreateShortCut  "$SMPROGRAMS\${APPNAME}.lnk" "$INSTDIR\bin\${EXECNAME}.exe" "" "$INSTDIR\bin\${EXECNAME}.exe" 0
SectionEnd

Section "Install Visual C++ Redistributable"
  ExecWait "$INSTDIR\bin\vc_redist.x64.exe /quiet /norestart"
  Delete "$INSTDIR\bin\vc_redist.x64.exe"
SectionEnd

Function RunApplication
  ExecShell "" "$INSTDIR\bin\${EXECNAME}.exe"
FunctionEnd

Function un.onInit
	SetShellVarContext all
	MessageBox MB_OKCANCEL|MB_ICONQUESTION "Are you sure that you want to uninstall ${APPNAME}?" IDOK next
		Abort
	next:
	!insertmacro VerifyUserIsAdmin
FunctionEnd

Section "Uninstall"
  RMDir /r "$INSTDIR"
  RMDir /r "$SMPROGRAMS\${APPNAME}.lnk"
; DeleteRegKey HKCU "Software\${COMPANYNAME}\${APPNAME}"
  DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${COMPANYNAME} ${APPNAME}"
SectionEnd
