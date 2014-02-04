set BUILD=0
for /F "usebackq" %%a in (`type U:\DynamicTranslator\yact\inc\buildno`) do set BUILD=%%a
set /a BUILD=%BUILD%+1
echo %BUILD% >U:\DynamicTranslator\yact\inc\buildno 
copy U:\DynamicTranslator\yact\inc\emversion-template.h  U:\DynamicTranslator\yact\inc\emversion.h 
echo #define EM_BUILD %BUILD% >>U:\DynamicTranslator\yact\inc\emversion.h 
