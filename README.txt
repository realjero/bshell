Das Programm ist eine **Shell-Implementierung** namens "basicsh" und "advancedsh" in der Programmiersprache C. Die Shell ist in der Lage, einfache und komplexe Kommandos verschiedener Typen zu interpretieren und auszuführen. Dazu gehören einfache Kommandos, Kommandosequenzen, bedingte Ausführung von Kommandos und Pipelines.

In der _ersten Teilaufgabe_, "basicsh", wird eine grundlegende Shell erstellt, die die genannten Kommandotypen verarbeiten kann.

In der _zweiten Teilaufgabe_, "advancedsh", wird die "basicsh" um eine Prozessliste erweitert. Die erweiterte Shell kann verschiedene Informationen zu ihren Kindprozessen in einer Liste speichern. Mit dem Kommando "_status_" können diese Informationen angezeigt werden. Zusätzlich wird ein Signalhandler für SIGCHLD aktiviert, der den Terminierungsstatus von Subprozessen in die Prozessliste einträgt, wenn sie beendet werden.

Insgesamt ermöglichen die beiden Teilaufgaben die Implementierung einer Shell mit erweiterten Funktionen zur Prozessverwaltung und -steuerung.

1. Building normal executable

1.1 Build Process:

mkdir build
cd build
cmake ..
make

2. Building without libreadline

mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Nolibreadline ..
make

3. Execution

./shell
