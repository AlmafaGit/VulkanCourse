#pragma once
//       position       |   texture
//   x      y       z       u      v
//külső csúcsok
    0.0f, -0.4f,   0.0f,   0.0f, 0.0f, //A 0
    0.4f,  0.0f,   0.0f,   0.0f, 0.0f, //B 1
    0.0f,  0.4f,   0.0f,   0.0f, 0.0f, //C 2
   -0.4f,  0.0f,   0.0f,   0.0f, 0.0f, //D 3
//belső 4 csúcs
   -0.1f, -0.1f,   0.0f,   1.0f, 0.0f, //E 4
    0.1f, -0.1f,   0.0f,   0.0f, 1.0f, //F 5
    0.1f,  0.1f,   0.0f,   1.0f, 0.0f, //G 6
   -0.1f,  0.1f,   0.0f,   0.0f, 1.0f, //H 7
//kozepső 2 csúcs
    0.0f,  0.0f,  -0.1f,   1.0f, 1.0f, //I 8 ELÜLSŐ? lesz az
    0.0f,  0.0f,   0.1f,   1.0f, 1.0f, //J 9