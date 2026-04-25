Set WshShell = CreateObject("WScript.Shell")
WshShell.Run "powershell.exe -NoProfile -ExecutionPolicy Bypass -File "".\centawin_autostart_manager.ps1"" -Action disable", 0, True