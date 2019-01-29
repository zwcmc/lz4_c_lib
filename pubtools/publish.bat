xcopy "../src/%1" "../pub/%1" /E /R /Y /I
@python encrypt_game.py %~dp0\..\pub\%1
