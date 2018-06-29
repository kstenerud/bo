
output:
- type: binary, float, decimal, int (hex, decimal, octal, boolean), characters?
- width
- prefix
- suffix
- endian?
- skip bytes, read length


nonspace hex, oct, bool data - inferred boundaries


input
- su xob fd


Commands
--------

input:

i8-i128: integer. always unsigned, but "-" causes negation.

f16-f128: next values are floating point 16-128 bits
d32-d128: next values are decimal 32-128 bits
"xyz": a string value
ascii control chars
(): Group
r[number]: repeat previous item (number) times
?: print help
[hdob](number)

operators? +-*/%~^|&

string escape: \\, \r, \n \t \" \hh \uhhhh

control chars:
nul, soh, stx, etx, eot, enq, ack, bel, bs, ht, lf, vt, ff, cr, so, si
dle, dc1, dc2, dc3, dc4, nak, syn, etb, can, em, sub, esc, fs, gs, rs, us, del

functions [a-zA-Z_]+(...)

output:

O <format>: Output format
- bl / bb binary (little, big endian)
- hl / hb[group size in digits] hex format
- cl / cb[group size in digits] c format 0x00, 0x01, etc

output format:
- first time format
- next time format
- last time format
- \ escape syntax
- % placeholder? @ placeholder? {} placeholder?
