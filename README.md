# CLEAVE
This repository provides the tool for the paper "Compositional Verification of Cryptographic Circuits against Fault Injection Attacks".



Table of Contents
=================
- [CLEAVE](#cleave)
- [Table of Contents](#table-of-contents)
  - [Requirements](#requirements)
  - [Structure](#structure)
  - [Getting Started Instructions](#getting-started-instructions)
    - [Build the tool](#build-the-tool)
      - [**0. Install dependencies**](#0-install-dependencies)
      - [**1. Run the decompose tool**](#1-run-the-decompose-tool)
      - [**2. Run the .nl parser**](#2-run-the-nl-parser)
      - [**3. Run FIRMER extend**](#3-run-firmer-extend)
        - [**3.1. Run FIRMER extend**](#31-run-firmer-extend)
        - [**3.2. Run the encode file by solver**](#32-run-the-encode-file-by-solver)
      - [**4. Run FIVER extend**](#4-run-fiver-extend)


## Requirements
* Z3 library release>=4.11.2.
* SMT solver bitwuzla with Version 0.1-202011.
* SAT solver glucose>=4.0.


## Structure

* test: The benchmarks of our work.

* FIRMER_extend.cpp: The extend of FIRMER.

* change_to_nl.cpp: Translate the netlist verilog file into .nl file.
* parser.h: The main code of the translator, which is provided by Amir Moradi (for the paper: https://eprint.iacr.org/2021/936.pdf )
* util.h: Some tools for use.
* decompose.py: Tools for splitting verilog.


## Getting Started Instructions

### Build the tool

#### **0. Install dependencies**

Download the [Z3 library release](https://github.com/Z3Prover/z3/releases).

Write the path of your Z3 library release into `CMakeLists.txt`:
```bash
INCLUDE_DIRECTORIES(you_path_of_z3_include)
LINK_DIRECTORIES(your_path_of_z3_bin)
```
Build the tool as follows;

```bashaw
mkdir build
cd build
cmake ..
make
```

#### **1. Run the decompose tool**

Before using this tool, you may get some verilog files. Since CLEAVE can only receive `.nl` files, you need to translate it by parser provided by Amir Moradi. The Library we use is NANG45.

First, you should use the decompose tool to get your new vhd files, you can run the tool as follows:
```bash
python3 decompose.py <source_vhd_file> <mux_module_name>
```
'<source_vhd_file>' is the path of the vhdl file.

'<mux_module_name>' is the name of the mux module.

For example, if we want to decompose the file named 'aes.vhd', and the mux module include 'MixColumnMUX' and 'Red_MixColumnMUX', we should input like:

```bash
python3 decompose.py ../aes.vhd MixColumnMUX Red_MixColumnMUX
```

<!--
Your mux file needs to contain information about all the mux functions in the verilog code, and it needs to be written in the file strictly in the following format, note that everything in an element needs to end with a comma (the last element needs to end with a semicolon) and is separated by spaces：
```bash
VARIABLE: n(1,32)

muxModuleName1: n(1,9)
muxModuleName2: n(1,10)
```
-->

<!-- #### **2. Run the design compiler** -->

After obtaining the decomposed VHDL file, you can proceed to execute the Design Compiler for logical synthesis to obtain the Verilog gate-level list file.

#### **2. Run the .nl parser**

After getting the decomposed verilog circuit, you can run parser as follow:
```bash
g++ change_to_nl.cpp -o change_to_nl
./parser your_library_path your_library_name your_verilog_path your_circuit_modelu your_output_path
```

<!--
For example, if we want to translate verilog circuit `led64-3round-1bit` into `.nl` files, we can run follow command:
```bash
python3 decompose.py exampleMuxFile.txt test/led64-1round/test1round1bit.v
g++ change_to_nl.cpp -o change_to_nl
./parser cell/Library.txt NANG45 test/led64-1round/test1round1bit.v LED64Enc test/led64-1round/test1round1bit.nl
```
-->

<!--
And the exampleMuxFile.txt may be like:
```bash
VARIABLE: n(1,32);

MixColumnMUX:
    CONDITION: n<4;
    INPUT: MCOutput, ShiftRowOutput;
    OUTPUT: InputFeedback;
```
-->

#### **3. Run FIRMER extend**

##### **3.1. Run FIRMER extend**

For the fault-resistance verification task, run as follows:
```bash
./CLEAVE 3 your_circuit_path_with_countermeasure your_blacklist_path output_path fault_number minmum_fault clock_cycle_number type_of_countermeasure fault_type optimize optimize_method encode_clock solver_select composition

```

`your_circuit_path_with_countermeasure` is the circuit with protection, e.g. CRAFT 1round with 1bit protection. Those circuits are in `test`.

`your_blacklist_path` is the blacklist for the circuit. Those blacklists are in the subdirectory of each protected circuit, e.g.`test\aes128`.

`output_path` is the your output path of the encoding file.

`fault_number` is the number of fault that injecting to a clock cycle at once.

`minmum_fault` is the miumum number of fault that injecting to a clock cycle at once. No less than 1.

`clock_cycle_number` is the number of clock cycle that you want to inject fault at once.

`type_of_countermeasure` is the type of the countermeasure, including `correction` and `detection`.

`fault_type` is the type of the fault that you want to inject, including `bit-flip`, `set-reset` and `all`. `all` is the set of fault which contains `bit-flip`, `set` and `reset`.

`optimize` indicates whether you want to use the optimizition, including `opt` (use the optimization) and `nopt` (do not use optimization).

`optimize_method` is the method of optimization. There is only one optimization method now. The other methods will coming soon. If you want to use the optimization, please turn it to `1`, otherwise doesn't matter.

`encode_clock` indicates whether you encode all clock cycle into one file or into mutil files. `true` is to encode the clock cycle into one file and `false` is to encode the clock cycle into mutil files. Note that `false` is only allowed when `clock_cycle_number` is `1`. 

`solver_select` is to select the solver, including `SMT` and `SAT`. If you choose `SMT`, the encoding file is `.smt2`. If you choose `SAT`, the encoding file is `.cnf`.

`composition` is to select whether use the compositional verification, including `composition` and `noncomposition`.

For example, if we want to inject faults into circuit `craft 3round 3bit protection` with 3 bit-flip faults in 3 clock cycles, using the optimization and encoding the clock cycle into one file, then using SAT solver to solve it, we can using following command:
```bash
./CLEAVE 3 ../test/craft-3round-mul/craft_3round_3bit.nl ../test/craft-3round-mul/blacklist.txt ../craft_3round_3bit_trivarite 3 3 detection bit-flip opt 1 true SAT
```

##### **3.2. Run the encode file by solver**

After geting the encoding file, next we will solve it using solver.

For SAT solver, we use glucose>=4.0. [Download release here](https://www.labri.fr/perso/lsimon/research/glucose/)

The command is as follows:
```bash
./glucose-syrup -maxnbthreads=number_of_maxthreads your_path_of_cnf_file
```

For SMT solver, we use bitwuzla with Version 0.1-202011. [Download release here](https://bitwuzla.github.io/download.html)

The command is as follows:
```bash
./bitwuzla your_path_of_smt2_file
```

If you want to use other solver, please download it separately.

#### **4. Run FIVER extend**
If you want ot use FIVER_extend, please read the `READMD.md` file in `FIVER_extend`.

<!--#### **5. Quick start
```bash
sh run_quick_start.sh
```bash

#### **5. Get the results in paper**
If you want to get the results in our paper, you can run `run.py`. Note you should ensure the dependent librarys have prepared and run the finish building the code.-->


