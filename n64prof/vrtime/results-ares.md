# vrtime results: ares

The VR4300 timing tests (`vrtime.c`) run in ares (the build from `releases/fish-oot-vrtime1-hw.bps`). The console's numbers are the truth; these are what ares models, kept to compare. Values that look like ares simplifications rather than the hardware: uncached loads at ~5 cycles (RDRAM is much slower), `add.s` slower than `add.d`, no overlap between independent FPU multiplies.

```
59 tests, best of 5 runs; cycles per operation (the loop's 3.0 cycles a pass taken off)
test              cycles  spread  note
nop                 1.00    0.0%  the pipeline's pace: 1 if nothing stalls
alu.dep             1.00    0.0%  integer ALU result to the next instruction
alu.ind             1.00    0.0%  
sll.dep             1.00    0.0%  
daddu.dep           1.00    0.0%  
dsll32.dep          1.00    0.0%  
lui+ori             2.00    0.0%  
lw.use0             3.00    0.0%  load then use at once (less lw.ind and alu.ind: the load-use stall)
lw.use1             4.00    0.4%  
lw.ind              2.00    0.0%  
lw.chain            2.00    0.0%  a load whose address is the last load's result (cache hit latency)
ld.ind              4.00    0.0%  
lbu.use0            3.00    0.0%  
sw                  2.00    0.6%  
sd                  4.00    0.5%  
sw+lw               4.00    0.0%  
sw+lw.use           5.00    0.0%  store, load the same word, use it
sw+sw               4.00    0.3%  
mult.lat            6.00    0.0%  mult then mflo: the multiply's latency
multu.lat           6.00    0.2%  
mult.thr            5.00    0.0%  
mult+mfhi           6.00    0.0%  
dmult.lat           9.00    0.0%  
dmultu.lat          9.00    0.1%  
div.lat            38.01    0.0%  div then mflo
divu.lat           38.00    0.0%  
ddiv.lat           70.00    0.0%  
ddivu.lat          70.00    0.0%  
mult+3alu           9.00    0.2%  
add.s.lat           5.00    0.0%  FPU add, each needing the last
add.s.thr           5.00    0.0%  FPU adds that don't depend on each other
mul.s.lat           5.00    0.1%  
mul.s.thr           5.00    0.0%  
div.s.lat          29.00    0.0%  
sqrt.s.lat         29.00    0.0%  
add.d.lat           3.00    0.2%  
mul.d.lat           8.00    0.0%  
mul.d.thr           8.00    0.1%  
div.d.lat          58.00    0.0%  
sqrt.d.lat         58.00    0.0%  
mul+add.s          10.00    0.1%  
mul.s+3alu          8.00    0.3%  
cvt.s.w             5.00    0.0%  
trunc.w.s           5.00    0.0%  
cvt.d.s             1.00    0.0%  
mfc1+mtc1           2.00    0.3%  
lwc1.use            7.00    0.0%  
c.lt+bc1t           5.00    0.4%  
b.taken             2.00    0.0%  a taken branch (with its delay slot)
b.not               2.00    0.0%  
bl.taken            2.00    0.0%  
bl.not              1.00    0.0%  a branch-likely not taken (its delay slot cancelled)
bal                 2.00    0.0%  
dc.chase           44.76    6.5%  a data-cache miss each load (64 KB, shuffled)
dc.seq.read        45.63    6.4%  loads streaming through new lines (16 bytes apart)
dc.seq.write       83.74    2.2%  stores streaming through new lines (write-back of the dirty lines they evict)
uncached.chase      4.75    0.0%  uncached loads (KSEG1), each needing the last
ic.miss             7.01    0.0%  straight code through 64 KB (per instruction; every 8th starts a new line)

derived (cycles):
  load-use stall               0.0
  D-cache miss (load)         42.8
  uncached load                4.8
  I-cache line miss           48.0
  dirty line write-back       38.1
```
