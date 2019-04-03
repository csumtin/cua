import sys
import time
from evdev import ecodes, InputEvent, InputDevice, UInput, categorize

def special_keys_mapping(event):
    if event.code == ecodes.KEY_LEFTALT:
        return InputEvent(event.sec, event.usec, event.type, ecodes.KEY_LEFTCTRL, event.value)
    elif event.code == ecodes.KEY_LEFTCTRL:
        return InputEvent(event.sec, event.usec, event.type, ecodes.KEY_LEFTALT, event.value)
    else:
        return event

special_keys_pressed = {
    ecodes.KEY_LEFTCTRL: False,
    ecodes.KEY_LEFTALT: False,
    ecodes.KEY_LEFTMETA: False,
    ecodes.KEY_CAPSLOCK: False,
}

def check_special_keys(event):
    for special_key in special_keys_pressed:
        if event.code == special_key and (event.value == 1 or event.value == 2):
            special_keys_pressed[special_key] = True
        elif event.code == special_key and event.value == 0:
            special_keys_pressed[special_key] = False
    return False

toggle_keys_set = {
    ecodes.KEY_CAPSLOCK: False,
}

previous_key_down = 0

def check_toggle_keys(event):
    global previous_key_down
    if event.value == 0 and event.code == previous_key_down:
        for toggle_key in toggle_keys_set:
            if toggle_key == event.code:
                toggle_keys_set[toggle_key] = not toggle_keys_set[toggle_key]
                special_keys_pressed[toggle_key] = toggle_keys_set[toggle_key]
    else:
        previous_key_down = event.code
        
    return False

def kill_sequence(special_key, kill_key):
    def take_event(event):
        if special_keys_pressed[special_key] and event.code == kill_key:
            sys.exit()
        
        return False

    return take_event

def press(key):
    return lambda: virtual_keyboard.write(ecodes.EV_KEY, key, 1)

def unpress(key):
    return lambda: virtual_keyboard.write(ecodes.EV_KEY, key, 0)

def rule(event, source_key, output_events):
    if event.code == source_key:
        if event.value == 0 or event.value == 2:
            for output_event in output_events:
                output_event()

        return True
    else:
        return False

def special_key_rule(special_key, source_key, output_events, special_keys_not_pressed):
    def take_event(event):
        if special_keys_pressed[special_key] and not any([special_keys_pressed[x] for x in special_keys_not_pressed]):
            return rule(event, source_key, output_events)
        else:
            return False
    return take_event

def block_caps_lock(event):
    return event.code == ecodes.KEY_CAPSLOCK

def write_default_event(event):
    virtual_keyboard.write_event(event)
    return True

try:
    # when program is run from terminal, give terminal time to see the new line release
    time.sleep(1)

    keyboard = InputDevice('/dev/input/event0')
    keyboard.grab()

    virtual_keyboard = UInput()

    mappings = [
        check_special_keys,
        check_toggle_keys,
        # kill_sequence(ecodes.KEY_CAPSLOCK, ecodes.KEY_EQUAL),

        ## Inside text editor
        # text navigation
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_I, [press(ecodes.KEY_UP), unpress(ecodes.KEY_UP)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_J, [press(ecodes.KEY_LEFT), unpress(ecodes.KEY_LEFT)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_K, [press(ecodes.KEY_DOWN), unpress(ecodes.KEY_DOWN)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_L, [press(ecodes.KEY_RIGHT), unpress(ecodes.KEY_RIGHT)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),

        # pgUp/Down
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_U, [press(ecodes.KEY_PAGEUP), unpress(ecodes.KEY_PAGEUP)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_N, [press(ecodes.KEY_PAGEDOWN), unpress(ecodes.KEY_PAGEDOWN)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),

        # start/end line
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_A, [press(ecodes.KEY_HOME), unpress(ecodes.KEY_HOME)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_D, [press(ecodes.KEY_END), unpress(ecodes.KEY_END)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),

        # start/end file
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_W, [press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_HOME), unpress(ecodes.KEY_HOME), unpress(ecodes.KEY_LEFTCTRL)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_S, [press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_END), unpress(ecodes.KEY_END), unpress(ecodes.KEY_LEFTCTRL)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),

        # word navigation
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_H, [press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_LEFT), unpress(ecodes.KEY_LEFT), unpress(ecodes.KEY_LEFTCTRL)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_F, [press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_RIGHT), unpress(ecodes.KEY_RIGHT), unpress(ecodes.KEY_LEFTCTRL)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),

        ## Window navigation
        # tab navigation
        special_key_rule(ecodes.KEY_LEFTMETA, ecodes.KEY_J, [press(ecodes.KEY_LEFTCTRL), unpress(ecodes.KEY_LEFTMETA), press(ecodes.KEY_LEFTSHIFT), press(ecodes.KEY_TAB), unpress(ecodes.KEY_TAB), unpress(ecodes.KEY_LEFTSHIFT), unpress(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_LEFTMETA), press(ecodes.KEY_LEFTALT), unpress(ecodes.KEY_LEFTALT)], []),
        special_key_rule(ecodes.KEY_LEFTMETA, ecodes.KEY_L, [press(ecodes.KEY_LEFTCTRL), unpress(ecodes.KEY_LEFTMETA), press(ecodes.KEY_TAB), unpress(ecodes.KEY_TAB), unpress(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_LEFTMETA), press(ecodes.KEY_LEFTALT), unpress(ecodes.KEY_LEFTALT)], []),

        # workspace up/down
        special_key_rule(ecodes.KEY_LEFTMETA, ecodes.KEY_I, [press(ecodes.KEY_UP), unpress(ecodes.KEY_UP)], []),
        special_key_rule(ecodes.KEY_LEFTMETA, ecodes.KEY_K, [press(ecodes.KEY_DOWN), unpress(ecodes.KEY_DOWN)], []),

        # back/forward
        special_key_rule(ecodes.KEY_LEFTMETA, ecodes.KEY_A, [press(ecodes.KEY_LEFTALT), unpress(ecodes.KEY_LEFTMETA), press(ecodes.KEY_LEFT), unpress(ecodes.KEY_LEFT), unpress(ecodes.KEY_LEFTALT), press(ecodes.KEY_LEFTMETA), press(ecodes.KEY_LEFTALT), unpress(ecodes.KEY_LEFTALT)], []),
        special_key_rule(ecodes.KEY_LEFTMETA, ecodes.KEY_D, [press(ecodes.KEY_LEFTALT), unpress(ecodes.KEY_LEFTMETA), press(ecodes.KEY_RIGHT), unpress(ecodes.KEY_RIGHT), unpress(ecodes.KEY_LEFTALT), press(ecodes.KEY_LEFTMETA), press(ecodes.KEY_LEFTALT), unpress(ecodes.KEY_LEFTALT)], []),

        # map CapsLock-o to Alt-o(used as shortcut)
        special_key_rule(ecodes.KEY_CAPSLOCK, ecodes.KEY_O, [press(ecodes.KEY_LEFTALT), press(ecodes.KEY_O), unpress(ecodes.KEY_O), unpress(ecodes.KEY_LEFTALT)], [ecodes.KEY_LEFTCTRL, ecodes.KEY_LEFTALT, ecodes.KEY_LEFTMETA]),

        ## Terminal
        # keep common terminal ctrl-c, ctrl-l, ctrl-d, ctrl-x, ctrl-k, ctrl-u and ctrl-z
        special_key_rule(ecodes.KEY_LEFTALT, ecodes.KEY_C, [unpress(ecodes.KEY_LEFTALT), press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_LEFTSHIFT), press(ecodes.KEY_C), unpress(ecodes.KEY_C), unpress(ecodes.KEY_LEFTSHIFT), unpress(ecodes.KEY_LEFTCTRL)], []),
        special_key_rule(ecodes.KEY_LEFTALT, ecodes.KEY_L, [unpress(ecodes.KEY_LEFTALT), press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_L), unpress(ecodes.KEY_L), unpress(ecodes.KEY_LEFTCTRL)], []),
        special_key_rule(ecodes.KEY_LEFTALT, ecodes.KEY_D, [unpress(ecodes.KEY_LEFTALT), press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_D), unpress(ecodes.KEY_D), unpress(ecodes.KEY_LEFTCTRL)], []),
        special_key_rule(ecodes.KEY_LEFTALT, ecodes.KEY_X, [unpress(ecodes.KEY_LEFTALT), press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_X), unpress(ecodes.KEY_X), unpress(ecodes.KEY_LEFTCTRL)], []),
        special_key_rule(ecodes.KEY_LEFTALT, ecodes.KEY_K, [unpress(ecodes.KEY_LEFTALT), press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_K), unpress(ecodes.KEY_K), unpress(ecodes.KEY_LEFTCTRL)], []),
        special_key_rule(ecodes.KEY_LEFTALT, ecodes.KEY_U, [unpress(ecodes.KEY_LEFTALT), press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_U), unpress(ecodes.KEY_U), unpress(ecodes.KEY_LEFTCTRL)], []),
        special_key_rule(ecodes.KEY_LEFTALT, ecodes.KEY_Z, [unpress(ecodes.KEY_LEFTALT), press(ecodes.KEY_LEFTCTRL), press(ecodes.KEY_Z), unpress(ecodes.KEY_Z), unpress(ecodes.KEY_LEFTCTRL)], []),

        block_caps_lock,
        write_default_event
    ]

    for raw_event in keyboard.read_loop():
        if raw_event.type == ecodes.EV_KEY:
            event = special_keys_mapping(raw_event)
            for func in mappings:
                should_we_stop = func(event)
                if(should_we_stop):
                    break
            virtual_keyboard.syn()
finally:
    virtual_keyboard.close()
    keyboard.ungrab()
