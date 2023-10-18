![vasetopia](vasetopia.png)

A simple package for generating "generalized" solids of revolution. 
I define these by rotating a shape that you draw in 2D around an axis which you also draw. 

# Building:
Clone the repo and get submodules, then build with cmake:
```
git clone --recursive https://github.com/meeree/Vasetopia.git 
cd Vasetopia
mkdir build
cd build
cmake ..
make
```

# Running:
Run 
```
./custom
```

# Controls: 
First, draw a region to be rotated in 2D with the mouse cursor and right mouse button. 
Then, press ```k``` and draw an axis to rotate around. 
Then, press ```r``` to do the rotation.

To view the shape, go into 3D view by pressing ```p```. 

To move around, use WASD and up/down in 3D view. 
