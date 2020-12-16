cd Builds/LinuxMakefile

if [ $1 == r ]
then
	make CONFIG=Release -j4
	./build/LilKS
elif [ $1 == d ]
then
	make CONFIG=Debug -j4
	gdb ./build/LilKS
elif [ $1 == c ]
then
	make clean
fi

cd ../../
