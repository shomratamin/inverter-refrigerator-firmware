################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Each subdirectory must supply rules for building sources it contributes
main.obj: ../main.c $(GEN_OPTS) | $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"E:/INSTALLED/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.1.LTS/bin/cl430" -vmspx --abi=eabi -O2 --opt_for_speed=1 --include_path="E:/INSTALLED/ti/ccsv7/ccs_base/msp430/include" --include_path="E:/INSTALLED/ti/ccsv7/tools/compiler/ti-cgt-msp430_16.9.1.LTS/include" --advice:power="all" --define=__MSP430F5132__ --diag_warning=225 --display_error_number --diag_wrap=off --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="main.d" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


