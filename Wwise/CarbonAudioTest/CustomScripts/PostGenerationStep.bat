set WwiseBinPath=%1
set SoundBankPath=%2
set ProjectFilePath="%~dp0..\CarbonAudioTest.wproj"
set SPMetadataPath="%SoundBankPath%\SoundPrioritizationMetadata.json"
set VirtualEnvFolderPath=%TMP%\.venv

if not exist "%VirtualEnvFolderPath%" (
    call py -3.9 -m venv %VirtualEnvFolderPath% 
    call python -m pip install --upgrade pip
)

call %VirtualEnvFolderPath%\Scripts\activate.bat
call pip install -i https://pypi.evetech.net/pypi/ audioscripts-standalone==9.1.0

call audioscripts generate-sp-metadata %WwiseBinPath% %ProjectFilePath% %SoundBankPath% -o %SoundBankPath%
call audioscripts assemble-essential-media %SPMetadataPath% %SoundBankPath%  