git clone https://gitlab.tuwien.ac.at/e193-02-gcg-course/data.git
xcopy /E /I /Y data\assets\* ..\assets
xcopy /E /I /Y data\include\* ..\include
xcopy /E /I /Y data\lib\* ..\lib
rmdir /S /Q data\