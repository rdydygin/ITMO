IVERILOG ?= iverilog
VVP ?= vvp
TYPST ?= typst

.PHONY: test test-register test-boe figures report
test: test-register test-boe

build:
	mkdir -p build

test-register: | build
	$(IVERILOG) -g2012 -Wall -s tb_result_register -o build/register rtl/result_register.v tb/tb_result_register.v
	$(VVP) build/register

test-boe: | build
	$(IVERILOG) -g2012 -Wall -s tb_boe -o build/boe rtl/boe_assign.v rtl/boe_always.v rtl/boe_param.v rtl/boe_registered.v rtl/result_register.v tb/tb_boe.v
	$(VVP) build/boe

PYTHON ?= python3
WAVE = $(PYTHON) report/vcd_wave.py build/boe.vcd

figures: test-boe
	$(WAVE) report/figures/wave_boe.svg 0 40 \
		tb_boe.d tb_boe.sel tb_boe.y0_assign tb_boe.y1_assign tb_boe.y0_always tb_boe.y1_always
	$(WAVE) report/figures/wave_param.svg 44 124 \
		tb_boe.nw_d=d[7:0] tb_boe.nw_sel=sel tb_boe.nw_y0=y0[7:0] tb_boe.nw_y1=y1[7:0]
	$(WAVE) report/figures/wave_registered.svg 636 706 \
		tb_boe.clk=clk tb_boe.rst=rst tb_boe.r_d=d[3:0] tb_boe.r_sel=sel \
		tb_boe.u_reg.y0=y0[3:0] tb_boe.u_reg.y1=y1[3:0] tb_boe.r_result=result[7:0]

report:
	$(TYPST) compile --root . report/main.typ report/report.pdf
