#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <dirent.h>
#include <fcntl.h>
#include <unistd.h>
#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

struct special_keys {
  int ctrl;
  int alt;
  int capslock;
  int meta;
};

struct libevdev* get_input_device_by_name(char *input_device_name) {
  DIR *dir = opendir("/dev/input");
  struct dirent *de;
  
  while((de = readdir(dir)) != NULL) {
    if(de->d_type == DT_CHR) {
      char *dev_path = malloc(strlen("/dev/input/") + strlen(de->d_name) + 1);
      strcpy(dev_path, "/dev/input/");
      strcat(dev_path, de->d_name);
      
      int fd = open(dev_path, O_RDONLY);
      if(fd < 0) {
        fprintf(stderr, "Couldn't open %s\n", dev_path);
        free(dev_path);
        continue;
      }
      
      struct libevdev *input_dev;
      int err = libevdev_new_from_fd(fd, &input_dev);
      if(err) {
        fprintf(stderr, "Libevdev input device error %s for %s\n", strerror(err), dev_path);
        free(dev_path);
        continue;
      }
      
      const char *dev_name = libevdev_get_name(input_dev);
      const char *dev_phys = libevdev_get_phys(input_dev);
      
      printf("%s %s %s\n", dev_path, dev_name, dev_phys);
      
      free(dev_path);
      
      // check phys ends with 0 as duplicate device names possible
      if(strcmp(dev_name, input_device_name) == 0 && dev_phys[strlen(dev_phys) - 1] == '0') {
        closedir(dir);
        return input_dev;
      }
      
      libevdev_free(input_dev);
      close(fd);
    }  
  }
  
  closedir(dir);
}

int main(int argc, char **argv) {

  char *input_device_name = "AT Translated Set 2 keyboard";
  struct libevdev *input_dev = get_input_device_by_name(input_device_name);
  if(input_dev == NULL) {
    fprintf(stderr, "Couldn't open %s\n", input_device_name);
    return 1;
  }
  
  int uifd = open("/dev/uinput", O_RDWR);
  if (uifd < 0) {
      fprintf(stderr, "Couldn't open /dev/uinput\n");
      return 1;
  }
  
  struct libevdev_uinput *output_dev;
  int err = libevdev_uinput_create_from_device(input_dev, uifd, &output_dev);
  if (err) {
      fprintf(stderr, "Libevdev output device error %s for %s\n", strerror(err), input_device_name);
      return 1;
  }

  err = libevdev_grab(input_dev, LIBEVDEV_GRAB);
  if (err) {
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
  
  int previous_key;
  
  int capslock_toggle = 0;
  
  for(;;) {

    err = libevdev_next_event(input_dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING, &event);
    if (err) {
        fprintf(stderr, "Libevdev next event error %s for %s\n", strerror(err), input_device_name);
        exit(1);
    }
    
    if(event.type == EV_KEY) {
        const char *event_name = libevdev_event_code_get_name(event.type, event.code);
        // printf("Event: %s %d\n", event_name, event.value);
        
        // switch ctrl and alt
        if(event.code == KEY_LEFTALT) {
          spk.alt = event.value;
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
        } else if(event.code == KEY_LEFTCTRL) {
          spk.ctrl = event.value;
                  
        } else if(event.code == KEY_CAPSLOCK) {
          // use capslock and meta as modifier keys
          if(event.value == 1 || event.value == 2) {
            spk.capslock = 1;
          } else {
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
            libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTMETA, 1);
            libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTMETA, 0);
          }
          
        } else if(spk.capslock && event.code == KEY_I && !spk.ctrl && !spk.alt && !spk.meta) {
          // basic navigation
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_UP, event.value);
        } else if(spk.capslock && event.code == KEY_J && !spk.ctrl && !spk.alt && !spk.meta) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFT, event.value);
        } else if(spk.capslock && event.code == KEY_K && !spk.ctrl && !spk.alt && !spk.meta) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_DOWN, event.value);
        } else if(spk.capslock && event.code == KEY_L && !spk.ctrl && !spk.alt && !spk.meta) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_RIGHT, event.value);
         
        } else if(spk.capslock && event.code == KEY_U && !spk.ctrl && !spk.alt && !spk.meta) {
          // pgUp/pgDn
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_PAGEUP, event.value);
        } else if(spk.capslock && event.code == KEY_N && !spk.ctrl && !spk.alt && !spk.meta) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_PAGEDOWN, event.value);
          
        } else if(spk.capslock && event.code == KEY_A && !spk.ctrl && !spk.alt && !spk.meta) {
          // start and enf of line
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_HOME, event.value);
        } else if(spk.capslock && event.code == KEY_D && !spk.ctrl && !spk.alt && !spk.meta) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_END, event.value);
          
        } else if(spk.capslock && event.code == KEY_W && !spk.ctrl && !spk.alt && !spk.meta) {
          // start and enf of file
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_HOME, event.value);
        } else if(spk.capslock && event.code == KEY_S && !spk.ctrl && !spk.alt && !spk.meta) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_END, event.value);
          
        } else if(spk.capslock && event.code == KEY_H && !spk.ctrl && !spk.alt && !spk.meta) {
          // start and end of word
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFT, event.value);
        } else if(spk.capslock && event.code == KEY_F && !spk.ctrl && !spk.alt && !spk.meta) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_RIGHT, event.value);
          
        } else if(spk.capslock && event.code == KEY_SEMICOLON && !spk.ctrl && !spk.alt && !spk.meta) {
          // enter
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_ENTER, event.value);
          
        } else if(spk.capslock && event.code == KEY_P && !spk.ctrl && !spk.alt && !spk.meta) {
          // backspace
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_BACKSPACE, event.value);
          
        } else if(spk.capslock && event.code == KEY_LEFTBRACE && !spk.ctrl && !spk.alt && !spk.meta) {
          // delete
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_DELETE, event.value);
          
        } else if(spk.meta && event.code == KEY_J) {
          // tab left and right
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTSHIFT, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_TAB, event.value);
        } else if(spk.meta && event.code == KEY_L && !spk.ctrl) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_TAB, event.value);
          
        } else if(spk.meta && event.code == KEY_I) {
          // workspace up and down
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTMETA, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_UP, event.value);
        } else if(spk.meta && event.code == KEY_K) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTMETA, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_DOWN, event.value);
          
        } else if(spk.meta && event.code == KEY_D) {
          // browser forward and back
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTMETA, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_DOWN, event.value);
        } else if(spk.meta && event.code == KEY_A) {
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTMETA, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_UP, event.value);
          
        } else if(spk.ctrl && spk.meta && event.code == KEY_L) {
          // lock screen
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTMETA, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_L, event.value);
          
        } else if(spk.ctrl && event.code == KEY_C) {
          // ctrl-c
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTSHIFT, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_C, event.value);
          
        } else if(spk.ctrl && event.code == KEY_L) {
          // ctrl-l
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_L, event.value);
          
        } else if(spk.ctrl && event.code == KEY_D) { 
          // ctrl-d
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_D, event.value);
  
        } else if(spk.ctrl && event.code == KEY_X) { 
          // ctrl-x
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_X, event.value);
          
        } else if(spk.ctrl && event.code == KEY_K) { 
          // ctrl-k
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_K, event.value);
          
        } else if(spk.ctrl && event.code == KEY_U) { 
          // ctrl-u
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_U, event.value);
          
        } else if(spk.ctrl && event.code == KEY_Z) { 
          // ctrl-z
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_LEFTCTRL, event.value);
          libevdev_uinput_write_event(output_dev, EV_KEY, KEY_Z, event.value);
         
        } else if(spk.capslock && event.code == KEY_3) {
          // auto phrase
          if(event.value == 1) {
       

          }
        } else {
          // we didn't match any rules, pass along event
          libevdev_uinput_write_event(output_dev, EV_KEY, event.code, event.value);
        }
        
        previous_key = event.code;
        
        libevdev_uinput_write_event(output_dev, EV_SYN, SYN_REPORT, 0);
    }
  }
}
