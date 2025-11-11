#pragma once
//       position       |   texture TODO  |  normals TODO
//   x      y       z       u      v      x       y      z
//külső csúcsok
    0.0f, -0.4f,   0.0f,   0.0f, 0.0f,   0.0f,  0.0f, 0.0f, //A 0
    0.4f,  0.0f,   0.0f,   0.0f, 0.0f,   0.0f,  0.0f, 0.0f, //B 1
    0.0f,  0.4f,   0.0f,   0.0f, 0.0f,   0.0f,  0.0f, 0.0f, //C 2
   -0.4f,  0.0f,   0.0f,   0.0f, 0.0f,   0.0f,  0.0f, 0.0f, //D 3
//belső 4 csúcs
   -0.1f, -0.1f,   0.0f,   0.0f, 0.0f,   0.0f,  0.0f,  0.0f, //E 4
    0.1f, -0.1f,   0.0f,   0.0f, 0.0f,   0.0f,  0.0f,  0.0f, //F 5
    0.1f,  0.1f,   0.0f,   0.0f, 0.0f,   0.0f,  0.0f,  0.0f, //G 6
   -0.1f,  0.1f,   0.0f,   0.0f, 0.0f,   0.0f,  0.0f,  0.0f, //H 7
//kozepső 2 csúcs
    0.0f,  0.0f,  -0.1f,   0.0f, 0.0f,   0.0f,  0.0f,  0.0f, //I 8 ELÜLSŐ? lesz az
    0.0f,  0.0f,   0.1f,   0.0f, 0.0f,   0.0f,  0.0f,  0.0f, //J 9