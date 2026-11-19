IVERILOG ?= iverilog
VVP ?= vvp
PYTHON ?= python3
TYPST ?= typst
QUARTUS_SH ?= quartus_sh

ARITHMETIC = rtl/mul_multicycle.v rtl/sqrt_multicycle.v rtl/add_multicycle.v
CORE = rtl/function_multicycle.v $(ARITHMETIC)
BOARD = rtl/lab2_board.v
WAVE = $(PYTHON) ../Labwork1/report/vcd_wave.py build/function.vcd

.PHONY: test test-arithmetic test-function test-board figures report fpga
test: test-arithmetic test-function test-board

build:
	mkdir -p build

test-arithmetic: | build
	$(IVERILOG) -g2012 -Wall -s tb_arithmetic -o build/arithmetic $(ARITHMETIC) tb/tb_arithmetic.v
	$(VVP) build/arithmetic

test-function: | build
	$(IVERILOG) -g2012 -Wall -s tb_function_multicycle -o build/function $(CORE) tb/tb_function_multicycle.v
	$(VVP) build/function

test-board: | build
	$(IVERILOG) -g2012 -Wall -s tb_lab2_board -o build/board $(CORE) $(BOARD) tb/tb_lab2_board.v
	$(VVP) build/board

figures: test-function
	$(WAVE) report/figures/wave_transaction.svg 0 409 \
		tb_function_multicycle.start tb_function_multicycle.a_low=a[7:0] \
		tb_function_multicycle.b_low=b[7:0] tb_function_multicycle.busy \
		tb_function_multicycle.dut.root_done=root_done tb_function_multicycle.dut.mul_done=mul_done \
		tb_function_multicycle.dut.add_done=add_done tb_function_multicycle.done \
		tb_function_multicycle.y_low=y[7:0]
	$(WAVE) report/figures/wave_completion.svg 345 409 \
		tb_function_multicycle.clk tb_function_multicycle.dut.state=state tb_function_multicycle.dut.add_done=add_done \
		tb_function_multicycle.busy tb_function_multicycle.done tb_function_multicycle.y

report:
	$(TYPST) compile --root . report/main.typ report/ЛР2_Тимошкин_РВ.pdf

fpga:
	cd quartus && $(QUARTUS_SH) --flow compile lab2
