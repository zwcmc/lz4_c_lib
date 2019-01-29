@rmdir /s /q "..\pub"

xcopy "..\main.lua" "..\pub\" /Y
xcopy "..\config.lua" "..\pub\" /Y
xcopy "..\config.json" "..\pub\" /Y
call xxtea encrypt god key_file=key "..\pub\main.lua" "..\pub\main.luac"
call xxtea encrypt god key_file=key "..\pub\config.lua" "..\pub\config.luac"
call xxtea encrypt god key_file=key "..\pub\config.json" "..\pub\config.json"
del "..\pub\main.lua"
del "..\pub\config.lua"

call publish common
call publish hall
call publish hxlib
call publish language_packs

echo "publish hall over"
@pause
