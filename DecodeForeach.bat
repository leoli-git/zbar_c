@echo off

for %%i in (BarcodeData\*.bmp) do (
	rem echo %%i
	ZBarQRDecoder.exe %%i
	echo.
)

pause