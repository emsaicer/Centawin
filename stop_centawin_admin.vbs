If Not WScript.Arguments.Named.Exists("elevate") Then
	CreateObject("Shell.Application").ShellExecute WScript.FullName, """" & WScript.ScriptFullName & """ /elevate", "", "runas", 1
	WScript.Quit
End If

Set WshShell = CreateObject("WScript.Shell")
WshShell.Run "taskkill /F /IM centawin.exe", 0, True