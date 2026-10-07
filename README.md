# Reed-Solomon (120,136) Code Class for ESP32
To add to a project in the components folder from the command line, run:    

    git submodule add https://github.com/rbliznets/rscode rscode 


Test (ESP32-S3 240 MHz, 120 bytes, average of 100 runs, `CONFIG_RS_IN_RAM=y`):
```
                     encode    decode
CONFIG_RS_PIE=n      54 usec   110 usec
CONFIG_RS_PIE=y     6.3 usec    62 usec
```
`CONFIG_RS_PIE` moves the inner loop of `poly_remainder` (the division by the generator
polynomial) to the 128-bit PIE instructions of the ESP32-S3: the 16-byte remainder is one Q
register, so a step costs one vector load of `remTable[x]` and one vector XOR instead of 16
byte loads, XORs and stores - 8 instructions per data byte instead of about 104. The encoder
is almost entirely that loop; in the decoder it is about half of the time, the rest (the
syndromes, Berlekamp-Massey, the Chien search and Forney) are data-dependent table lookups,
which PIE has no instruction for. The encoded and decoded data is the same either way, which
the test "RSEncode16 vectors" checks against reference ECC bytes.

`CONFIG_RS_IN_RAM` places the code in IRAM and the tables in DRAM. The 256x256 multiplication
table `gmul` (65536 bytes) has its own option, `CONFIG_RS_GMUL_IN_RAM` (on by default):
switching it off leaves `gmul` in flash and frees 64 KB of internal RAM, the rest stays in RAM.
Only `decode()` reads `gmul`, `encode()` works with `remTable`.

With `gmul` in flash the decoding time depends on what the data cache holds. Test "RSEncode16
decode cache" (ESP32-S3 240 MHz, flash DIO 80 MHz, 64 KB data cache, 120 bytes, average of 100
random blocks, `CONFIG_RS_PIE=y`); "cold" means the whole data cache is evicted before every
block, the real time is between the two columns:
```
                     gmul in RAM            gmul in flash
errors in the block  warm       cold        warm       cold
0                    20 usec    20 usec     20 usec    666 usec
1                    36 usec    36 usec     36 usec    953 usec
3                    56 usec    56 usec     57 usec   1429 usec
8                   104 usec   104 usec    104 usec   2437 usec
```
