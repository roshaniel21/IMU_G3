################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
fatfs/ff.obj: ../fatfs/ff.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me --include_path="C:/ti/TivaWare_C_Series-2.1.0.12573" --include_path="C:/Users/Daniel Greenheck/Documents/CCSWorkspace/IMU_G3/utils" --include_path="C:/Users/Daniel Greenheck/Documents/CCSWorkspace/IMU_G3/fatfs" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/include" -g --gcc --define=ccs="ccs" --define=UART_BUFFERED --define=TARGET_IS_TM4C129_RA0 --define=PART_TM4C1294NCPDT --diag_wrap=off --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="fatfs/ff.pp" --obj_directory="fatfs" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '

fatfs/mmc-ek-tm4c1294xl.obj: ../fatfs/mmc-ek-tm4c1294xl.c $(GEN_OPTS) $(GEN_HDRS)
	@echo 'Building file: $<'
	@echo 'Invoking: ARM Compiler'
	"C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/bin/armcl" -mv7M4 --code_state=16 --float_support=FPv4SPD16 --abi=eabi -me --include_path="C:/ti/TivaWare_C_Series-2.1.0.12573" --include_path="C:/Users/Daniel Greenheck/Documents/CCSWorkspace/IMU_G3/utils" --include_path="C:/Users/Daniel Greenheck/Documents/CCSWorkspace/IMU_G3/fatfs" --include_path="C:/ti/ccsv6/tools/compiler/ti-cgt-arm_5.2.5/include" -g --gcc --define=ccs="ccs" --define=UART_BUFFERED --define=TARGET_IS_TM4C129_RA0 --define=PART_TM4C1294NCPDT --diag_wrap=off --diag_warning=225 --display_error_number --preproc_with_compile --preproc_dependency="fatfs/mmc-ek-tm4c1294xl.pp" --obj_directory="fatfs" $(GEN_OPTS__FLAG) "$<"
	@echo 'Finished building: $<'
	@echo ' '


