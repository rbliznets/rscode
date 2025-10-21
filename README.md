# Reed-Solomon (120,136) Code Class for ESP32
To add to a project in the components folder from the command line, run:    

    git submodule add https://github.com/rbliznets/rscode rscode 


Test:
```
I (197) cpu_start: cpu freq: 240000000 Hz

(+1441usec) encode time
(+3285usec) decode time
```
Test CONFIG_RS_IN_RAM=y:
```
I (197) cpu_start: cpu freq: 240000000 Hz

(+80usec) encode time
(+190usec) decode time
```