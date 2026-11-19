create_clock -name clk50 -period 20.000 [get_ports {CLK}]
derive_pll_clocks
derive_clock_uncertainty

# The physical reset button is asynchronous.
set_false_path -from [get_ports {RST_N}]
# The LEDs have no external sampling clock.
set_false_path -to [all_outputs]
