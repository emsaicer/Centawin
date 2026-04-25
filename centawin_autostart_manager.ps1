param (
	[Parameter(Mandatory = $true)]
	[ValidateSet("enable", "disable")]
	$Action
)

$currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
if (-not $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)) {
	Start-Process powershell.exe -ArgumentList "-ExecutionPolicy Bypass -File `"$PSCommandPath`" -Action $Action" -Verb RunAs
	exit
}

Write-Host "Working on it..."

if ($Action -eq "enable") {
	$taskName = "Centawin Autostart"
	$exePath = "$PSScriptRoot\centawin.exe"
	$STAction = New-ScheduledTaskAction -Execute $exePath
	$trigger = New-ScheduledTaskTrigger -AtLogOn
	$settings = New-ScheduledTaskSettingsSet -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries -ExecutionTimeLimit 0
	if (-not (Test-Path -Path $exePath -PathType Leaf)) {
		Write-Host "Error: Can't find $exePath" -ForegroundColor Red
		Read-Host -Prompt "Press Enter to Exit"
		exit
	}
	Register-ScheduledTask -TaskName $taskName -Action $STAction -Trigger $trigger -Settings $settings -RunLevel Highest -Force
	Write-Host "Centawin has been added to startup with administrator privileges." -ForegroundColor Green
}
else {
	Unregister-ScheduledTask -TaskName "Centawin Autostart" -Confirm:$false -ErrorAction SilentlyContinue
	Write-Host "Centawin has been removed from startup with administrator privileges." -ForegroundColor Cyan
}
Read-Host -Prompt "Press Enter to Exit"