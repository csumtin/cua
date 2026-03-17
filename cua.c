#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <errno.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

struct special_keys {
  int ctrl;
  int alt;
  int capslock;
  int meta;
};

struct input_device {
  int fd;
  struct libevdev *dev;
};

struct input_device get_input_device_by_name(char *input_device_name) {
  DIR *dir = opendir("/dev/input");
  struct dirent *de;
  
  while((de = readdir(dir)) != NULL) {
    if(de->d_type != DT_CHR) {
      continue;
    }
  	
    char dev_path[256];
    snprintf(dev_path, sizeof(dev_path), "/dev/input/%s", de->d_name);
    
    int fd = open(dev_path, O_RDONLY);
    if(fd < 0) {
      fprintf(stderr, "Couldn't open %s\n", dev_path);
      continue;
    }
    
    struct libevdev *dev;
    int err = libevdev_new_from_fd(fd, &dev);
    if(err) {
      close(fd);
      fprintf(stderr, "Libevdev input device error %s for %s\n", strerror(err), dev_path);
      continue;
    }
    
    const char *dev_name = libevdev_get_name(dev);
    const char *dev_phys = libevdev_get_phys(dev);
    
    if(!input_device_name) {
      printf("%s %s %s\n", dev_path, dev_name, dev_phys);
      
      // check phys ends with 0 as duplicate device names possible
    } else if(dev_name && dev_phys && strcmp(dev_name, input_device_name) == 0 && dev_phys[strlen(dev_phys) - 1] == '0') {
      closedir(dir);
      return (struct input_device){
        .fd = fd,
        .dev = dev
      };
    }
    
    libevdev_free(dev);
    close(fd);
  }
  
  closedir(dir);
  return (struct input_device){
    .fd = -1,
    .dev = NULL
  };
}

void press(struct libevdev_uinput *output_dev, int code, int value) {
  libevdev_uinput_write_event(output_dev, EV_KEY, code, value);
}

void press_mod1(struct libevdev_uinput *output_dev, int mod, int code, int value) {
  if(value == 1) {
    press(output_dev, mod, 1);
    press(output_dev, code, 1);
  } else if(value == 2) {
    // don't repeat mod key as may get stuck
    press(output_dev, code, 2);
  } else if(value == 0) {
    press(output_dev, code, 0);
    press(output_dev, mod, 0);
  }
}

void press_mod2(struct libevdev_uinput *output_dev, int mod1, int mod2, int code, int value) {
  if(value == 1) {
    press(output_dev, mod1, 1);
    press(output_dev, mod2, 1);
    press(output_dev, code, 1);
  } else if(value == 2) {
    // don't repeat mod key as may get stuck
    press(output_dev, code, 2);
  } else if(value == 0) {
    press(output_dev, code, 0);
    press(output_dev, mod1, 0);
    press(output_dev, mod2, 0);
  }
}

int main(int argc, char **argv) {
  if(argc < 2) {
    get_input_device_by_name(NULL);
    fprintf(stderr, "Missing device name\n");
    return 1;
  }

  char *input_device_name = argv[1];
  
  struct input_device input = { .fd = -1, .dev = NULL };
  struct libevdev_uinput *output_dev = NULL;
  
  for(;;) {
    if(!input.dev) {
      input = get_input_device_by_name(input_device_name);
      if(!input.dev) {
        sleep(3);
        continue;
      }
    }

    int uifd = open("/dev/uinput", O_RDWR);
    if(uifd < 0) {
      fprintf(stderr, "Couldn't open /dev/uinput\n");
      return 1;
    }

    int err = libevdev_uinput_create_from_device(input.dev, uifd, &output_dev);
    if(err) {
      fprintf(stderr, "Libevdev output device error %s for %s\n", strerror(err), input_device_name);
      return 1;
    }

    err = libevdev_grab(input.dev, LIBEVDEV_GRAB);
    if(err) {
      fprintf(stderr, "Libevdev grab error %s for %s\n", strerror(err), input_device_name);
      return 1;
    }

    struct input_event event;
    struct special_keys spk = {
      .ctrl = 0,
      .alt = 0,
      .capslock = 0,
      .meta = 0,
    };

    int previous_key = 0;
    int capslock_toggle = 0;

    for(;;) {

    int rc = libevdev_next_event(input.dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &event);
    if(rc == -ENODEV) {
      libevdev_uinput_destroy(output_dev);
      output_dev = NULL;
      libevdev_free(input.dev);
      close(input.fd);
      close(uifd);
      input = (struct input_device){ .fd = -1, .dev = NULL };
      sleep(1);
      break;
    }

    if(event.type != EV_KEY) {
      continue;
    }    

    const char *event_name = libevdev_event_code_get_name(event.type, event.code);
    // printf("Event: %s %d\n", event_name, event.value);

    // switch ctrl and alt
    if(event.code == KEY_LEFTALT) {
      spk.alt = event.value;
      press(output_dev, KEY_LEFTCTRL, event.value);
    } else if(event.code == KEY_LEFTCTRL) {
      spk.ctrl = event.value;

    } else if(event.code == KEY_CAPSLOCK) {
      // use capslock and meta as modifier keys
      if(event.value == 1) {
        spk.capslock = 1;
      } else if(event.value == 0) {
        if(previous_key == KEY_CAPSLOCK) {
          // capslock is sticky
          if(capslock_toggle) {
            capslock_toggle = 0;
            spk.capslock = 0;
          } else {
            capslock_toggle = 1;
            spk.capslock = 1;
          }
        } else {
          spk.capslock = 0;
          capslock_toggle = 0;
        }
      } 
    } else if(event.code == KEY_LEFTMETA) {
      spk.meta = event.value;
      if(previous_key == KEY_LEFTMETA && event.value == 0) {
        press(output_dev, KEY_LEFTMETA, 1);
        press(output_dev, KEY_LEFTMETA, 0);
      }

    } else if(spk.capslock && !spk.ctrl && !spk.alt && !spk.meta) {
      switch(event.code) {
        // basic navigation
        case KEY_I: press(output_dev, KEY_UP, event.value); break;
        case KEY_J: press(output_dev, KEY_LEFT, event.value); break;
        case KEY_K: press(output_dev, KEY_DOWN, event.value); break;
        case KEY_L: press(output_dev, KEY_RIGHT, event.value); break;
        // pgUp/pgDown
        case KEY_U: press(output_dev, KEY_PAGEUP, event.value); break;
        case KEY_N: press(output_dev, KEY_PAGEDOWN, event.value); break;
        // start and end of line
        case KEY_A: press(output_dev, KEY_HOME, event.value); break;
        case KEY_D: press(output_dev, KEY_END, event.value); break;
        // start and end of file
        case KEY_W: press_mod1(output_dev, KEY_LEFTCTRL, KEY_HOME, event.value); break;
        case KEY_S: press_mod1(output_dev, KEY_LEFTCTRL, KEY_END, event.value); break;
        // start and end of word
        case KEY_H: press_mod1(output_dev, KEY_LEFTCTRL, KEY_LEFT, event.value); break;
        case KEY_F: press_mod1(output_dev, KEY_LEFTCTRL, KEY_RIGHT, event.value); break;
        // autophrase
        case KEY_3: press(output_dev, KEY_A, event.value); break;

        default: press(output_dev, event.code, event.value);
      }
    } else if(spk.ctrl && spk.meta && event.code == KEY_L) {
      // lock screen
      press_mod1(output_dev, KEY_LEFTMETA, KEY_L, event.value);
    } else if(spk.meta) {
      switch(event.code) {
        // tab left and right
        case KEY_J: press_mod2(output_dev, KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_TAB, event.value); break;
        case KEY_LEFT: press_mod2(output_dev, KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_TAB, event.value); break;
        case KEY_L: press_mod1(output_dev, KEY_LEFTCTRL, KEY_TAB, event.value); break;
        case KEY_RIGHT: press_mod1(output_dev, KEY_LEFTCTRL, KEY_TAB, event.value); break;
        // workspace up and down
        case KEY_I: press_mod1(output_dev, KEY_LEFTMETA, KEY_UP, event.value); break;
        case KEY_UP: press_mod1(output_dev, KEY_LEFTMETA, KEY_UP, event.value); break;
        case KEY_K: press_mod1(output_dev, KEY_LEFTMETA, KEY_DOWN, event.value); break;
        case KEY_DOWN: press_mod1(output_dev, KEY_LEFTMETA, KEY_DOWN, event.value); break;
        // fullscreen
        case KEY_F: press_mod1(output_dev, KEY_LEFTMETA, KEY_F, event.value); break;

        default: press(output_dev, event.code, event.value);
      }
    } else if(spk.ctrl) {
      switch(event.code) {
        // ctrl-c, ctrl-l, ctrl-d, ctrl-x, ctrl-k, ctrl-u, ctrl-z
        case KEY_C: press_mod2(output_dev, KEY_LEFTCTRL, KEY_LEFTSHIFT, KEY_C, event.value); break;
        case KEY_L: press_mod1(output_dev, KEY_LEFTCTRL, KEY_L, event.value); break;
        case KEY_D: press_mod1(output_dev, KEY_LEFTCTRL, KEY_D, event.value); break;
        case KEY_X: press_mod1(output_dev, KEY_LEFTCTRL, KEY_X, event.value); break;
        case KEY_K: press_mod1(output_dev, KEY_LEFTCTRL, KEY_K, event.value); break;
        case KEY_U: press_mod1(output_dev, KEY_LEFTCTRL, KEY_U, event.value); break;
        case KEY_Z: press_mod1(output_dev, KEY_LEFTCTRL, KEY_Z, event.value); break;

        default: press(output_dev, event.code, event.value);
      }
    } else {
      // we didn't match any rules, pass along event
      press(output_dev, event.code, event.value);
    }

    previous_key = event.code;

    libevdev_uinput_write_event(output_dev, EV_SYN, SYN_REPORT, 0);
    }
  }
}
