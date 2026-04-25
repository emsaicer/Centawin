Set shell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")

shortcutFile = shell.SpecialFolders("Startup") & "\Centawin.lnk"

If fso.FileExists(shortcutFile) Then
	fso.DeleteFile(shortcutFile)
	MsgBox "Centawin has been removed from startup.", 64, "Centawin"
Else
	MsgBox "Centawin is already missing from your startup.", 48, "Centawin"
End If