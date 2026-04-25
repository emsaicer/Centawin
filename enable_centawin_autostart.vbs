Set shell = CreateObject("WScript.Shell")
Set fso = CreateObject("Scripting.FileSystemObject")

currentDir = fso.GetParentFolderName(WScript.ScriptFullName)
shortcutPath = shell.SpecialFolders("Startup") & "\Centawin.lnk"

Set shortcut = shell.CreateShortcut(shortcutPath)
shortcut.TargetPath = currentDir & "\centawin.exe"
shortcut.WorkingDirectory = currentDir
shortcut.Save

MsgBox "Centawin has been added to the startup.", 64, "Centawin"