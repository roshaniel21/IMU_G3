################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
fatfs/ff.obj: ../fatfs/ff.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O2 --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/include" --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --diag_wrap=off --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="fatfs/ff.pp" --obj_directory="fatfs" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

fatfs/mmc-ek-tm4c1294xl.obj: ../fatfs/mmc-ek-tm4c1294xl.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me -O2 --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/include" --gcc --define=ccs="ccs" --define=PART_TM4C1294NCPDT --diag_wrap=off --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="fatfs/mmc-ek-tm4c1294xl.pp" --obj_directory="fatfs" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


