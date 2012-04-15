################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
light.obj: ../light.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430/bin/cl430" -vmspx --abi=coffabi -O0 -g --include_path="C:/ti/ccsv5/tools/compiler/msp430/include" --include_path="C:/Documents and Settings/Administrator/workspace_v5_1/CC3000 Spi/css" --include_path="C:/Documents and Settings/Administrator/workspace_v5_1/CC3000HostDriver/css" --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/msp430/MSP430ware_1_10_01_18/driverlib/5xx_6xx" --define=__MSP430FR5739__ --diag_warning=225 --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="light.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

main.obj: ../main.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430/bin/cl430" -vmspx --abi=coffabi -O0 -g --include_path="C:/ti/ccsv5/tools/compiler/msp430/include" --include_path="C:/Documents and Settings/Administrator/workspace_v5_1/CC3000 Spi/css" --include_path="C:/Documents and Settings/Administrator/workspace_v5_1/CC3000HostDriver/css" --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/msp430/MSP430ware_1_10_01_18/driverlib/5xx_6xx" --define=__MSP430FR5739__ --diag_warning=225 --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="main.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

thermostat.obj: ../thermostat.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430/bin/cl430" -vmspx --abi=coffabi -O0 -g --include_path="C:/ti/ccsv5/tools/compiler/msp430/include" --include_path="C:/Documents and Settings/Administrator/workspace_v5_1/CC3000 Spi/css" --include_path="C:/Documents and Settings/Administrator/workspace_v5_1/CC3000HostDriver/css" --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/msp430/MSP430ware_1_10_01_18/driverlib/5xx_6xx" --define=__MSP430FR5739__ --diag_warning=225 --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="thermostat.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

wlan.obj: ../wlan.c $(GEN_OPTS) $(GEN_SRCS)
	@echo 'Building file: $<'
	@echo 'Invoking: MSP430 Compiler'
	"C:/ti/ccsv5/tools/compiler/msp430/bin/cl430" -vmspx --abi=coffabi -O0 -g --include_path="C:/ti/ccsv5/tools/compiler/msp430/include" --include_path="C:/Documents and Settings/Administrator/workspace_v5_1/CC3000 Spi/css" --include_path="C:/Documents and Settings/Administrator/workspace_v5_1/CC3000HostDriver/css" --include_path="C:/ti/ccsv5/ccs_base/msp430/include" --include_path="C:/ti/msp430/MSP430ware_1_10_01_18/driverlib/5xx_6xx" --define=__MSP430FR5739__ --diag_warning=225 --display_error_number --silicon_errata=CPU21 --silicon_errata=CPU22 --silicon_errata=CPU40 --printf_support=minimal --preproc_with_compile --preproc_dependency="wlan.pp" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


