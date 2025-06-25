# 3d Terrain Generator which is explorable without a mouse!

## Introduction
This project was initially as a part of the C Group Project at Imperial. It was coded as an extension exercise to coding an assembler and emulator in C. 
As an extension task, we developed a 3D terrain generator and implemented camera movement con
trolled by tracking facial data from a webcam. This approach was partly motivated by accessibility concerns, traditional mouse input can be difficult for individuals with physical disabilities. By using face tracking to control the camera, we explored an alternative approach of interaction, aiming to make 3D environments accessible to all

The code was written in 1 week... Planning to modify bits here and there. May add in physics and multiplayer in the future!!
## Tech stack used
1. GLFW      -> handle window and inputs
2. glad      -> enables us to use OpenGL. 
3. OpenGL    -> shaders and drawing to the screen.
4. MediaPipe -> to provide us with face mesh coordinates

## How to run
### Linux/MacOS
*STEP 1* 
Need to install GLFW 
run the command `sudo apt install libglfw3-dev`

*STEP 2*
Run `make`

*STEP 3*
Run `./main`

### Windows 
*HAVE FUN !! lol*


## Things learnt 

Look at notes.txt
