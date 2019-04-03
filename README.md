# Common User Actions aka Shortcuts

## Use evdev for keybindings
* `apt install python-pip` then `pip install evdev`
* you will need to get your keyboard device, eg `/dev/input/event0`
* run as root:
```
import evdev
print("\n".join(map(lambda x: str((x.path, x.name, x.phys)), [evdev.InputDevice(path) for path in evdev.list_devices()])))
```
* run as root `python cua.py` 
* add `cua.service` to run on startup `/etc/systemd/system/cua.service`
* run on boot `systemctl enable cua`

## Standard shortcuts
* Switch Ctrl and Alt for better ergonomics
* Ctrl-c = copy
* Ctrl-v = paste
* Ctrl-x = cut, cuts line if no selection

* Ctrl-z = undo
* Ctrl-Shift-z = redo

* Ctrl-a = select all

* Ctrl-f = search with regex
* Ctrl-g = next occurence
* Ctrl-Shift-g = previous occurence
* Ctrl-r = replace with regex

* Ctrl-o = open
* Ctrl-n = new window
* Ctrl-q = close window

* Ctrl-t = new tab
* Ctrl-w = close tab
* Ctrl-s = save
* Ctrl-Shift-s = save as

## Navigation
* Use CapsLock as a modifier key
* Make CapsLock a toggle modifier

* Up or CapsLock-i = up
* Down or CapsLock-k = down
* Left or CapsLock-j = left
* Right or CapsLock-l = right

* PageUp or CapsLock-u = page up
* PageDown or CapsLock-n = page down

* Home or CapsLock-a = start of line
* End or CapsLock-d = end of line

* Ctrl-home or CapsLock-w = start of file
* Ctrl-end or CapsLock-s = end of file

* Ctrl-left or CapsLock-h = start of word
* Ctrl-right or CapsLock-f = end of word

* Shift-<above> = turn on selection

## Window management
* Meta = launch

* Meta-j = switch to left tab
* Meta-l = switch to right tab

* Meta-up or Meta-i = move workspace up
* Meta-down or Meta-k = move workspace down
* Meta-Shift-up or Meta-Shift-i = send window to workspace up
* Meta-Shit-down or Meta-Shift-k = send window to workspace down

* Meta-a = back
* Meta-d = forward

* Meta-f = fullscreen

## Terminal
* copy and paste are same Ctrl-c and Ctrl-v
* keep common terminal keys using actual Ctrl(now Alt); ctrl-c, ctrl-l, ctrl-d, ctrl-x, ctrl-k, ctrl-u and ctrl-z

## Browser
* Ctrl-r = refresh
* Ctrl-l = select address bar

## Code
* Ctrl-i = show usage/find references
* Ctrl-k = go to definition/declaration
* Ctrl-j = show docs
* Ctrl-l = parameter information
* Ctrl-/ = comment out/in
* Ctrl-9 = format

* Alt-r = rename
* Alt-t test
