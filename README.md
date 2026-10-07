# Reed-Solomon (120,136) Code Class for ESP32
To add to a project in the components folder from the command line, run:    

    git submodule add https://github.com/rbliznets/rscode rscode 


Test (ESP32-S3 240 MHz, 120 bytes, 3 errors, average of 100 runs, `CONFIG_RS_IN_RAM=y`):
```
                     encode    decode
CONFIG_RS_PIE=n      54 usec   133 usec
CONFIG_RS_PIE=y     6.3 usec    84 usec
```
`CONFIG_RS_PIE` moves the inner loop of `poly_remainder` (the division by the generator
polynomial) to the 128-bit PIE instructions of the ESP32-S3: the 16-byte remainder is one Q
register, so a step costs one vector load of `remTable[x]` and one vector XOR instead of 16
byte loads, XORs and stores - 8 instructions per data byte instead of about 104. The encoder
is almost entirely that loop; in the decoder it is over a third of the time, the rest (the
syndromes, Berlekamp-Massey, the Chien search and Forney) are data-dependent table lookups,
which PIE has no instruction for. The encoded and decoded data is the same either way, which
the test "RSEncode16 vectors" checks against reference ECC bytes.

Multiplication in GF(256) uses a logarithm table and a power table, 1.5 KB together: `glog`
(256 x 16 bit) and `galfa` (1025 bytes), `a * b = galfa[glog[a] + glog[b]]`. The logarithm of
zero is 512, and `galfa` is zero from index 509 up, so a zero operand needs no branch. The
syndromes and the Chien search keep their running powers as logarithms and need no lookups
for them. `CONFIG_RS_IN_RAM` places the code in IRAM and all the tables in DRAM.

Earlier versions used a 256x256 multiplication table of 65536 bytes. Decoding time of 120
bytes by the number of corrupted bytes (test "RSEncode16 decode cache", average of 100 random
blocks, `CONFIG_RS_PIE=y`, `CONFIG_RS_IN_RAM=y`, flash DIO 80 MHz, 64 KB data cache); for the
64 KB table left in flash the two numbers are a warm data cache and a cache evicted before
every block:
```
errors   logarithms   64 KB table in RAM   64 KB table in flash
0          25 usec         20 usec           20 ..  666 usec
1          47 usec         36 usec           36 ..  953 usec
3          76 usec         56 usec           57 .. 1429 usec
8         145 usec        104 usec          104 .. 2437 usec
```
With the tables in RAM the time does not depend on the state of the cache.
