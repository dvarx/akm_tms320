################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Each subdirectory must supply rules for building sources it contributes
%.obj: ../%.c $(GEN_OPTS) | $(GEN_FILES) $(GEN_MISC_FILES)
	@echo 'Building file: "$<"'
	@echo 'Invoking: C2000 Compiler'
	"/home/dvarx/ti/ccs1120/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/bin/cl2000" -v28 -ml -mt --cla_support=cla1 --float_support=fpu32 --tmu_support=tmu0 --vcu_support=vcu2 -Ooff --include_path="/home/dvarx/workspace_v11/tms320_akm" --include_path="/home/dvarx/workspace_v11/tms320_akm/device" --include_path="/home/dvarx/ti/C2000Ware_3_04_00_00_Software/driverlib/f2837xd/driverlib" --include_path="/home/dvarx/ti/ccs1120/ccs/tools/compiler/ti-cgt-c2000_21.6.0.LTS/include" --define=CPU1 --define=_FLASH --diag_suppress=10063 --diag_warning=225 --diag_wrap=off --display_error_number --abi=eabi --preproc_with_compile --preproc_dependency="$(basename $(<F)).d_raw" --include_path="/home/dvarx/workspace_v11/tms320_akm/CPU1_FLASH/syscfg" $(GEN_OPTS__FLAG) "$(shell echo $<)"
	@echo 'Finished building: "$<"'
	@echo ' '


