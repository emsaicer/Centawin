# Centawin

`Centawin` is a super simple, lightweight, and portable Windows utility that does only one thing: **centers the focused window** via the hotkey. It works great with **multi-monitor configurations**. The program has **no graphical interface** and doesn't create a tray icon.

## How to Use

1. Run `centawin.exe`.
2. Focus a window.
3. Press `Win` + `Shift` + `Q` keybinding.

## How to Center a Window with Administrative Privileges

You need to run `Centawin` with **administrative privileges**.

## How to Autostart

Run `enable_centawin_autostart.vbs`.

OR

If you want to do it **manually**, create a **shortcut** to `centawin.exe` and put it in the `%appdata%\Microsoft\Windows\Start Menu\Programs\Startup` folder.

> [!NOTE]
> If you want to autostart the program with **administrative privileges**, run `enable_centawin_autostart_admin.vbs`.
>
> OR
>
> If you want to do it **manually**:
>
> 1. Open `Task Scheduler`.
> 2. Click `Create Task...` in the **Actions** panel on the right.
> 3. In the `Name:` field, write *Centawin Autostart*.
> 4. Check `Run with highest privileges`.
> 5. `Triggers` tab > `New...` > `Begin the task:` **At log on** > `OK`.
> 6. `Actions` tab > `New...` > `Browse...` > pick `centawin.exe` > `OK`.
> 7. [**If you have a laptop**] `Conditions` tab > uncheck `Start the task only if the computer is on AC power`.
> 8. `Settings` tab > uncheck `Stop the task if it runs longer than`.
> 9. Click `OK` to create the task.

## How to Stop

Run `stop_centawin.vbs`.

> [!NOTE]
> If you ran `centawin.exe` with **administrative privileges**, you need to run `stop_centawin_admin.vbs`.

OR

Open `Task Manager` > `Details` tab > Right-click on `centawin.exe` > `End task` > `End process`.

OR

Execute `taskkill /F /IM centawin.exe` in **Terminal**.

> [!NOTE]
> If you ran `centawin.exe` with **administrative privileges**, you also need to run **Terminal** with **administrative privileges**.

## How to Disable Autostart

Run `disable_centawin_autostart.vbs`.

OR

If you want to do it **manually**, delete a shortcut to `centawin.exe` from the `%appdata%\Microsoft\Windows\Start Menu\Programs\Startup` folder.

> [!NOTE]
> If you want to disable autostart of the program with **administrative privileges**, run `disable_centawin_autostart_admin.vbs`.
>
> OR
>
> If you want to do it **manually**, `Task Scheduler` > `Centawin Autostart` > `Delete`.

## How to Build

```
gcc main.c -o centawin.exe -mwindows -s
```