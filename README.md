# Класс кода Рида-Соломона (108,120) для ESP32 
Для добавления в проект в папке компонентов из командной строки запустить:    

    git submodule add https://github.com/rbliznets/rscode rscode 


Тест:
```
I (197) cpu_start: cpu freq: 240000000 Hz

(+1441usec) encode time
(+3285usec) decode time
```
Тест CONFIG_RS_IN_RAM=y:
```
I (197) cpu_start: cpu freq: 240000000 Hz

(+80usec) encode time
(+190usec) decode time
```